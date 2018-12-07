#include "potgen.hpp"
#include "global.hpp"
#include "profiling.hpp"
#include "dynamic_grid.hpp"
#include "fft.hpp"
#include "multiindex.hpp"
#include "discretize.hpp"
#include "randomize.hpp"

#include <array>
#include <cmath>
#include <cassert>
#include <future>
#include <iostream>
#include <boost/throw_exception.hpp>
#include <boost/numeric/ublas/vector.hpp>

// defined here so can be inlined
inline double pow_small(double base, int exponent)
{
    switch(exponent)
    {
        case 0:
            return 1;
        case 1:
            return base;
        case 2:
            return base*base;
        case 3:
            return base*base*base;
        default:
            return std::pow(base, exponent);
    }
}

// -------------------------------------------------------------------------------------------------------------
// calculate the derivative in position space if the fourier transform is given
// this assumes that the original function was defined inside [-1, 1]^N
/// takes order_per_dir by value to make calling from multiple threads easier.
/// threads safe, if \p f_k is not changed when function is run, otherwise undefined behaviour
default_grid calculateDerivative( std::vector<int> order_per_dir, const complex_grid& f_k )
{
    std::size_t dimension = f_k.getDimension();
    // argument checks
    if(order_per_dir.size() != f_k.getDimension())
    THROW_EXCEPTION( std::invalid_argument, "derivation index %1% count does not match data dimension %2%", order_per_dir.size(), f_k.getDimension() );

    /// \todo maybe just enable fft access and disable after the function...
    if( f_k.getAccessMode() != TransformationType::FFT_INDEX )
    THROW_EXCEPTION( std::invalid_argument, "grid is not in fft index mode" );

    /// \todo this might throw. Catch and generate more detailed error message
    auto der_grid = f_k.clone();

    std::size_t total_order = std::accumulate( std::begin(order_per_dir), std::end(order_per_dir), (size_t)0 );
    for( unsigned i = 0; i < dimension; ++i)
    {
        if(order_per_dir[i] < 0)
        THROW_EXCEPTION( std::invalid_argument, "negative order of derivative %1% supplied", i );
    }

    // the actual calculation starts here
    {
        MultiIndex index( dimension );
        for(unsigned i = 0; i < dimension; ++i)
        {
            // get extents returns unsigned ints, so we need to explicitly convert so singed to apply the unary minus.
            index.setLowerBoundAt( i, -(int)(f_k.getExtents()[i]/2) );
            index.setUpperBoundAt( i, (int)f_k.getExtents()[i]/2 );
        }


        PROFILE_BLOCK("derivative calculation");
        complex_t i_factor = std::pow( complex_t(0, pi), total_order );

        for(index.init() ;index.valid(); ++index)
        {
            /// \todo add comment how/why this works
            // f'(k) = i k f(k)
            double r_factor = 1;
            // this is pushed into another function
            for(unsigned dir = 0; dir < dimension; ++dir)
            {
                // if no derivative in that direction, factor is 1 so no computation needed
                if(order_per_dir[dir] == 0)
                    continue;

                r_factor *= pow_small(2*index[dir], order_per_dir[dir]);
            }

            der_grid(index) *= r_factor * i_factor;
        }
    }

    ifft(der_grid);

    default_grid result( f_k.getExtents(), TransformationType::FFT_INDEX );
    std::transform(der_grid.begin(), der_grid.end(), result.begin(), (double(*)(const complex_t&))&std::real);
    return std::move(result);
}


// -------------------------------------------------------------------------------------------------------------
//  first step of potential generation: generate the new potential in k - space
// -------------------------------------------------------------------------------------------------------------
complex_grid generatePotentialInKSpace( std::vector<std::size_t> sizes, std::vector<double> support, correlation_fn cor_fun, const PGOptions& opt )
{
    // create discrete correlation function and fourier transform -> power spectrum
    // load discretized function data into correlation array
    auto grid = discretizeFunctionForFFT(sizes, support, cor_fun);

    /// \todo logging

    // calculate fft of correlation
    fft(grid);

    // calculate randomize potential in momentum space
    // potential is square root of power spectrum
    {
        PROFILE_BLOCK("power spectrum")

        for(auto& v : grid )
        {
            double real = std::real(v);
            /// \todo actually measure the error here and return it in PGResult
            if( real < -1e-5 || std::abs(std::imag(v)) > 1e-5)
            {
                THROW_EXCEPTION( std::runtime_error, "power spectrum contains negative or imaginary components, check correlation function!" );
            }
            // it is faster (and better?) to use v as a nonnegative real here for sqrt calculation
            v = real < 0 ? 0 : std::sqrt(real);
        }
    }

    // randomize phases
    if( opt.randomize )
    {
        randomizePhases(grid, opt.randomSeed);
    }

    return std::move(grid);
}


// -------------------------------------------------------------------------------------------------------------
// takes a potential in k-space and calculates all requested derivatives, stores inside a Potential datatype
/// \todo write tests for this function

void calculateAllDerivatives(Potential &potential, const complex_grid &potential_k, unsigned int max_order)
{
    PROFILE_BLOCK("calculate all derivatives");

    typedef std::future<default_grid> f_type;
    std::vector<f_type> started_tasks;
    std::vector<MultiIndex> task_orders;

    // calculation function
    auto calc_deriv = [&potential_k](MultiIndex order)
    {
        auto deriv = calculateDerivative(order.getAsVector(), potential_k);

        // use same scale factor as for potential
        std::size_t vec_element_count = potential_k.getElementCount();
        double factor = std::sqrt(vec_element_count);
        scaleVectorBy( deriv, factor );
        return std::move(deriv);
    };

    for(MultiIndex order( potential.getDimension(), 0, max_order + 1 ); order.valid(); ++order)
    {
        // check total order of derivative
        std::size_t total_order = order.getAccumulated();
        if(total_order <= max_order && total_order > 0)
        {
            started_tasks.push_back( std::async(std::launch::async, calc_deriv, order) );
            task_orders.push_back( order );
        }
    }

    for(unsigned i = 0; i < task_orders.size(); ++i)
    {
        try
        {
            potential.setDerivative( task_orders[i], std::move(started_tasks[i].get()) );
        } catch (const std::bad_alloc& e)
        {
            std::cerr << "bad alloc called in multi threaded derivative calculation. probably ran out of memory. "
                    "retry as sequential calculation to reduce memory footprint.";

            potential.setDerivative( task_orders[i], std::move( calc_deriv(task_orders[i]) ) );
        }
    }
}


// -------------------------------------------------------------------------------------------------------------

Potential generatePotential( std::vector<std::size_t> sizes, std::vector<double> support, const PGOptions& opt )
{
    Potential res(sizes, std::vector<double>(sizes.size(), 1.0));
    res.setCreationInfo(opt.randomSeed, 3, opt.corrlength);

    // setup threads
    setFFTThreads(opt.numThreads);

    // calculate the potential in k-space
    auto potential_k = generatePotentialInKSpace(sizes, support, opt.cor_fun, opt);

    // calculate derivatives in k-space
    calculateAllDerivatives( res, potential_k, opt.maxDerivativeOrder );

    std::size_t vec_element_count = potential_k.getElementCount();

    // calculate potential in position space
    auto& cpotential_x = potential_k;
    ifft(cpotential_x);

    // this requires additional memory again
    /// \todo actually measure the error here and return it in PGResult
    double averageComplexPart = 0;
    double average = 0;

    default_grid potential_x(sizes, TransformationType::IDENTITY);

    auto it = potential_x.begin();
    for( auto& value : cpotential_x)
    {
        *it = std::real(value);
        ++it;

        average += std::real( value );
        averageComplexPart += std::imag( value );
    }

    average /= vec_element_count;
    averageComplexPart /= vec_element_count;

    // calculate average and shift
    double variance = 0.0;
    for( auto& d : potential_x )
    {
        d -= average;
        variance += d*d;
    }

    if(opt.verbose)
        std::cout << "original quality: " << average << " " << variance << "\n";
    res.scalePotential( std::sqrt(1.0/variance) );

    scaleVectorBy( potential_x, std::sqrt(vec_element_count / variance) );
    // take scaling into account
    if( opt.verbose )
        std::cout << "the average imaginary component in the result was " << averageComplexPart * std::sqrt(vec_element_count / variance)<< "\n";

    res.setPotential( std::move(potential_x) );

    res.setSupport( support );
    return std::move(res);
}

