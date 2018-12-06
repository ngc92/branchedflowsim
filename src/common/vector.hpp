#ifndef VEC2D_HPP_INCLUDED
#define VEC2D_HPP_INCLUDED

/*! \file vector.hpp
    \ingroup common
    \brief vector data types
    \details this file imports the vector data types from boost.ublas for use under less verbose
            type names.
*/

#include <cassert>
#include <numeric>
#include <boost/numeric/ublas/vector.hpp>

//! \addtogroup common
//! \{

//! vector of doubles with constant maximum length \p N
template<int N>
using c_vector = boost::numeric::ublas::c_vector<double, N>;

/*! normal generic vector type with at most three entries.
    uses c_vector, i.e. preallocated space so it does not
    require any dynamic memory management to create these
    objects.
*/
using gen_vect = c_vector<3>;

/// integer vector of at most three dimension.
using int_vect = boost::numeric::ublas::c_vector<int, 3>;

/// cross product function, very ugly, not generic or easy to use.
/// \todo should be removed or significantly improved.
template<class Result, class Vector>
void crossProduct( Result& target, const Vector& v1, const Vector& v2)
{
    assert( target.size() == 3 );
    assert( v1.size() == 3);
    assert( v2.size() == 3 );
    target[0] =  v1[1]*v2[2] - v1[2]*v2[1];
    target[1] = -v1[0]*v2[2] + v1[2]*v2[0];
    target[2] = v1[0]*v2[1] - v1[1]*v2[0];
}


template<class Vec1, class Vec2>
double dotProduct(Vec1&& v1, Vec2&& v2) {
    using std::begin;
    using std::end;
    return std::inner_product(begin(v1), end(v1), begin(v2), 0.0);
}

//! \}

#endif // VEC2D_HPP_INCLUDED
