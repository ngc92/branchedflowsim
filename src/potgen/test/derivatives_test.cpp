#include "vector.hpp"
#include "potential.hpp"
#include "fft.hpp"
#include "potgen.hpp"
#include "correlation.hpp"
#include "interpolation.hpp"
#include "dynamic_grid.hpp"
#include <boost/numeric/odeint.hpp>
#include <fstream>

#include <boost/test/unit_test.hpp>

using std::size_t;

// forward declarations
default_grid calculateDerivative( std::vector<int> order_per_dir, const complex_grid& f_k );
void calculateAllDerivatives(Potential &potential, const complex_grid &potential_k, unsigned int max_order);
complex_grid discretizeFunctionForFFT(std::vector<std::size_t> gridsize, std::vector<double>, correlation_fn F);

static const std::vector<double> vec_1{1.0};
static const std::vector<double> vec_2{1.0, 1.0};
static const std::vector<double> vec_3{1.0, 1.0, 1.0};

BOOST_AUTO_TEST_SUITE(derivatives_test)

// derivative calculation check

BOOST_AUTO_TEST_CASE( derivative_from_k_space_check )
{
	constexpr int data_size = 512;
	// generate target data for function f
	std::function<double(const gen_vect&)> f = [](const gen_vect& x) { return std::exp(-x[0]*x[0]*100); };
	auto df = [](double x) { return -200*x*std::exp(-100*x*x); };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>({data_size}), vec_1, f);

	// transform into k-space
	fft(dgrid);
	auto derivative = calculateDerivative(std::vector<int>{1}, dgrid);

	std::vector<int> index(1);
	for(int i=-data_size/2; i < data_size/2; ++i)
	{
		double x = (double)i / data_size;
		index[0] = i;
		BOOST_REQUIRE_SMALL( std::abs(derivative( index ) - df(x)), 1e-2 );
	}
}

BOOST_AUTO_TEST_CASE( under_over_specified_derivative_check )
{
	complex_grid dgrid(2, 8, TransformationType::FFT_INDEX);

	// throw exception because derivation vector size does not match grid dimension
	BOOST_CHECK_THROW(calculateDerivative(std::vector<int>{1}, dgrid), std::invalid_argument);
	BOOST_CHECK_THROW(calculateDerivative(std::vector<int>{1,0,4}, dgrid), std::invalid_argument);

	// check for fft indexing
	complex_grid dgrid2(2, 8, TransformationType::PERIODIC);
	BOOST_CHECK_THROW(calculateDerivative(std::vector<int>{1,0}, dgrid2), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE( derivative_from_k_space_check_2d )
{
	constexpr int data_size = 128;
	// generate target data for function f
	std::function<double(const gen_vect&)> f = [](const gen_vect& x) { return std::exp(-x[0]*x[0]*100-x[1]*x[1]*100); };
	auto df = [](double x, double y) { return -200*x*std::exp(-100*(x*x+y*y)); };
	//auto f_k = [](double k) { return std::exp(-k*k/4) * std::sqrt(pi); };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{128,128}, vec_2, f);

	// transform into k-space
	fft(dgrid);

	auto der_grid = calculateDerivative(std::vector<int>{1,0}, dgrid);
	std::vector<int> index(2);
	for(int i=-data_size/2; i < data_size/2; ++i)
	for(int j=-data_size/2; j < data_size/2; ++j)
	{
		index[0] = i;
		index[1] = j;
		double x = (double)i / data_size;
		double y = (double)j / data_size;
		BOOST_REQUIRE_SMALL( std::abs(der_grid( index ) - df(x,y)), 1e-8 );
	}
}


BOOST_AUTO_TEST_CASE( second_order_derivative_from_k_space_check_2d )
{
	/// \todo add test for second oder d/dx² derivative
	constexpr int data_size = 128;
	// generate target data for function f
	std::function<double(const gen_vect&)> f = [](const gen_vect& x) { return std::exp(-x[0]*x[0]*100-x[1]*x[1]*100); };
	auto dfdxdy = [](double x, double y) { return 40000*y*x*std::exp(-100*(x*x+y*y)); };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{128,128}, vec_2, f);

	// transform into k-space
	fft(dgrid);
	std::vector<int> der_dir{1,1};

	auto der_grid = calculateDerivative(der_dir, dgrid);

	std::vector<int> index(2);
	for(int i=-data_size/2; i < data_size/2; ++i)
	for(int j=-data_size/2; j < data_size/2; ++j)
	{
		index[0] = i;
		index[1] = j;
		double x = (double)i / data_size;
		double y = (double)j / data_size;
		BOOST_REQUIRE_SMALL( std::abs(der_grid( index ) - dfdxdy(x,y)), 1e-7 );
	}
}

BOOST_AUTO_TEST_CASE( potgen_derivatives_calculation )
{
	constexpr int size = 256;

	std::function<double(const gen_vect&)> f = [](const gen_vect& x) { return std::exp(-x[0]*x[0]*200); };
	std::function<double(const gen_vect&)> dxf = [](const gen_vect& x) {
		return -160000*std::exp(-200*(x[0]*x[0]))*(400*x[0]*x[0]-3)*x[0];  };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{size}, vec_1, f);

	// make a potential object out of g
	Potential pot(1, 1.0, size);
	default_grid grid( 1, size, TransformationType::IDENTITY );
	std::transform(dgrid.begin(), dgrid.end(), grid.begin(), (double(*)(const complex_t&))std::real);
	pot.setPotential( std::move(grid) );

	// check that potential fits
	for(auto i = dgrid.begin(); i != dgrid.end(); ++i)
	{
		BOOST_REQUIRE_SMALL( std::real((*i)) - pot.getPotential()[ std::distance(dgrid.begin(), i) ], 1e-10 );
	}

	// transform g into k-space and calculate derivatives up to third order
	fft(dgrid);
	calculateAllDerivatives( pot, dgrid, 3);

	// hand-calculated derivative
	auto dxg = discretizeFunctionForFFT(std::vector<size_t>{size}, vec_1, dxf);

	// check that these match
	auto dxc = pot.getDerivative( std::vector<int>{3} ).shallow_copy();

	double scale_factor = std::sqrt( size );
	for(auto i = 0; i < size; ++i)
	{
		BOOST_REQUIRE_SMALL( std::abs(dxg[i]*scale_factor - dxc[i]), 1e-6 );
	}
}

BOOST_AUTO_TEST_CASE( derivative_from_k_space_nonsquare_check_2d )
{
	// here we assume a box 1x1, with non-uniform sampling.
	constexpr int size_x = 128;
	constexpr int size_y = 256;
	// generate target data for function f
	auto f = makeGaussianCorrelation(0.1);
	auto df = [](double x, double y) { return -200*x*std::exp(-100*(x*x+y*y)); };
	//auto f_k = [](double k) { return std::exp(-k*k/4) * std::sqrt(pi); };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>({size_x, size_y}), vec_2, f);

	// transform into k-space
	fft(dgrid);

	std::fstream file("debug.txt", std::fstream::out);
	auto der_grid = calculateDerivative(std::vector<int>{1,0}, dgrid);
	std::vector<int> index(2);
	for(int i=-size_x/2; i < size_x/2; ++i)
	for(int j=-size_y/2; j < size_y/2; ++j)
	{
		index[0] = i;
		index[1] = j;
		double x = (double)i / size_x;
		double y = (double)j / size_y;
		if(i == 4)
		{
			file << der_grid( index ) << " " << df(x,y) << "\n";
		}
		BOOST_REQUIRE_SMALL( std::abs(der_grid( index ) - df(x,y)), 1e-8 );
	}
}


BOOST_AUTO_TEST_CASE( change_support )
{
	constexpr int data_size = 512;
	// generate target data for function f
	auto disgrid = discretizeFunctionForFFT(std::vector<size_t>({data_size}), vec_1, makeGaussianCorrelation(0.1));
	default_grid grid(disgrid.getExtents(), TransformationType::IDENTITY);
	for(unsigned i = 0; i < grid.size(); ++i)
	{
		grid[i] = std::real(disgrid[i]);
	}

	// transform into k-space
	fft(disgrid);
	auto derivative = calculateDerivative(std::vector<int>{1}, disgrid);

	//Potential(std::size_t dimension, double support, std::size_t size, std::size_t max_order);
	Potential p(1, 1.0, data_size);
	p.setPotential( grid.shallow_copy() );
	MultiIndex idx(1, 0, 2);
	++idx;
	p.setDerivative( idx, derivative.shallow_copy() );
	constexpr double SCALE_FACTOR = 20.0;
	p.setSupport( std::vector<double>{SCALE_FACTOR} );

	// integration test
	auto pgrid = p.getPotential().shallow_copy();
    pgrid.setAccessMode( TransformationType::PERIODIC );
	auto idvec = makeIndexVector(1, {0});
	auto dgrid = p.getDerivative( idvec ).shallow_copy();
    dgrid.setAccessMode( TransformationType::PERIODIC );

	constexpr double STEP = 0.01;

	double p_int = pgrid(std::vector<int>{2});

	/// \todo use an integrator and check that results are consistent
	int c = 0;

	double max_dev = 0;
	double avg_dev = 0;

	for(double x = 2; x < data_size - 2; x += STEP)
	{
		gen_vect pos(1);
		pos[0] = x;
		double p_here = linearInterpolate( pgrid, pos );

		double dx_here = linearInterpolate( dgrid, pos );
		p_int += dx_here * (SCALE_FACTOR*STEP) / data_size;

		double dev = std::abs( p_int - p_here );
		if ( dev > max_dev )
			max_dev = dev;
		avg_dev += dev;

		c++;
	}

	avg_dev /= c;
	/// \todo 1e-3 and 2e-4 seem quite high here
	BOOST_CHECK_SMALL( max_dev, 1e-3 );
	BOOST_CHECK_SMALL( avg_dev, 2e-4 );

	std::cout << "average deviation: " << avg_dev << "\n";

	// same thing using boost::odeint
	using namespace boost::numeric::odeint;
	typedef std::vector<double> state_type;
	runge_kutta4< state_type > stepper;
	auto sysf = [&dgrid, data_size, SCALE_FACTOR]( const state_type &x , state_type &dxdt , double ) {
						dxdt[0] = x[1];
						gen_vect pos(1);
						pos[0] = x[0] * data_size / SCALE_FACTOR;
						double dx = linearInterpolate( dgrid, pos );
						dxdt[1] = -dx;
					};
	std::vector<double> x{ 2.0 / data_size, 1.0};
	double start_e = pgrid(std::vector<int>{2}) + 0.5;
	integrate_const( stepper , sysf, x , 0.0 , 0.5 , 0.01 );
	gen_vect v(1);
	v[0] = x[0] * data_size / SCALE_FACTOR;
	double end_e = linearInterpolate(pgrid, v) + 0.5 * x[1] * x[1];
	std::cout << "energy change: " << start_e << " -> " << end_e << "\n";
}
/// \todo add test that checks consistency of Potential and its Derivatives

BOOST_AUTO_TEST_SUITE_END()
