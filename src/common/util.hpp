//
// Created by eriks on 8/31/17.
//

#ifndef BRANCHEDFLOWSIM_UTIL_H
#define BRANCHEDFLOWSIM_UTIL_H

#include "global.hpp"
#include "vector.hpp"
#include <limits>
#include <random>

/// \addtogroup common
/// \{

// templated integer power function
/// function to safely calculate the power of an integer, including overflow detection.
/// Necessary because the std::pow function only operates on floating point values, which might
/// lead to rounding problems for large values.
/// \ingroup common
template<class int_type>
int_type pow_int(int_type base, std::size_t exponent)
{
    static_assert( std::numeric_limits<int_type>::is_integer, "this pow overload is just for integral types" );
    int_type p = 1;
    for( unsigned d = 0; d < exponent; ++d)
    {
        if( p > std::numeric_limits<int_type>::max() / base  )
            THROW_EXCEPTION( std::overflow_error, "overflow in pow %1%^%2%", base, exponent );
        p *= base;
    }

    return p;
}

/// \brief calculates the product of the contents of a vector
/// \details makes sure the calculation does not overflow. The result
///          is written into a std::size_t type.
/// \pre factors[i] >= 0
template<class container_type>
std::size_t safe_product(const container_type& factors)
{
    static_assert( std::is_integral<typename container_type::value_type>::value, "safe_product only works on containers on integral types");
    std::size_t p = 1;
    auto max_value = std::numeric_limits<decltype(p)>::max();
    for( auto f : factors )
    {
        // check that factor is positive
        if( f < 0 )
        THROW_EXCEPTION( std::overflow_error, "negative factor %1% in safe_product", f );

        // prevent overflow
        if( f != 0 && p >= max_value / f  )
        THROW_EXCEPTION( std::overflow_error, "overflow in safe_product" );

        p *= f;
    }

    return p;
}

// ---------------------------------------------------------------------------------------------------------------------

template<class RNG>
gen_vect randomPointOnSphere(RNG& rng) {
    // http://mathworld.wolfram.com/SpherePointPicking.html
    std::uniform_real_distribution<double> distribution{0.0, 1.0};
    double u = distribution( rng ) * 2 - 1;
    double theta = distribution( rng ) * 2 * pi;

    gen_vect result(3);
    result[0] = std::sqrt(1 - u*u) * std::cos(theta);
    result[1] = std::sqrt(1 - u*u) * std::sin(theta);
    result[2] = u;
    return result;
}

// --------------------------------------------------------------------------------------------------------------
/// \brief scales the contents of a container by a constant factor
template<class U, class V>
void scaleVectorBy( U& vec, V scale_factor )
{
    // use equal_to to disable warning about float point comparisons.
    if(std::equal_to<V>()(scale_factor, 1))
        return;

    for( auto& c : vec )
        c *= scale_factor;
}

// ----------------------------------------------------------------------------------------------------------------
inline std::vector<int> makeIndexVector( int length, std::initializer_list<unsigned> idx )
{
    std::vector<int> v( length, 0 );
    for(int i : idx)
        v[i]++;
    return v;
}

/// \}

#endif //BRANCHEDFLOWSIM_UTIL_H
