#ifndef INTERPOLATION_HPP_INCLUDED
#define INTERPOLATION_HPP_INCLUDED

/*! \file interpolation.hpp
    \brief defines generic methods for linearly interpolating n-dimensional data.
    \ingroup common
    \details This file was completely tested on 06.06.14.
            Changes since:
    \todo some comments / documentation needed

*/

#include "global.hpp"
#include "vector.hpp"

// forward declarations
template<class T>
class DynamicGrid;

typedef DynamicGrid<double> default_grid;

// documentation grouping
//! \addtogroup common
//! \{

// 1D linear interpolation
/*! \brief one dimensional linear interpolation
    \details calculates \f$ (b-a)*pos + a \f$. Should be always inlined.
    \return result of the calculation as newly constructed value.
*/
template<class T>
inline T interpolate_linear_1d(const T& a, const T& b, double pos)
{
    return (b-a)*pos + a;
}

// ------------------------------------------------------------------------------------

/*! \brief (tri/bi)linearly interpolate values on a grid
    \details gets the interpolated value on the grid at position \p v.
    \param grid Grid on which to interpolate
    \param v Interpolation position. has to be in range [0, \p grid->extents ).
*/
double linearInterpolate( const default_grid& grid, const gen_vect& v);

/*! \brief (tri/bi)linearly interpolate values on a grid
    \details gets the interpolated value on the grid at position \p v.
    \param grid Grid on which to interpolate
    \param v Interpolation position. has to be in range [0, \p grid->extents ).
*/
double linearInterpolate( const default_grid& grid, const double* v);

// ----------------------------------------------------------------------------
//                          interpolated drawing
// ----------------------------------------------------------------------------


/// this function draws a point to grid, i.e. it adds a total value of \p weight
/// around the position \p pos. It is distributed to the nearest neighbours of \p pos.
/// This function operates on float grids, because drawing does not require that much of a precision.
void drawInterpolatedDot( DynamicGrid<float>& grid, const gen_vect& pos, double weight );

//! \}

#endif // INTERPOLATION_HPP_INCLUDED
