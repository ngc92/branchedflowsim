#ifndef MONODROMY_HPP_INCLUDED
#define MONODROMY_HPP_INCLUDED

#include "vector.hpp"

inline std::array<double, 9> getMonodromyCoeff( std::size_t dimension, const default_grid sources[], const double* position )
{
	std::array<double, 9> coeffs;
	for(unsigned x = 0; x < dimension; ++x)
		for(unsigned y = 0; y <= x; ++y)
		{
			// calc symmetric entries only once
			double value = linearInterpolate(sources[x * dimension + y], position);
			coeffs[x*dimension + y] = -value;
			coeffs[y*dimension + x] = -value;
		}

	return coeffs;
}


// square matrix multiplication
inline void monodromy_matrix_multiply( std::size_t dimension, double* out, const std::array<double, 9>& coeffs, const double* in)
{
	std::size_t rowsize = 2 * dimension;
	std::size_t half_offset = rowsize * dimension;
	/* 	(0			1)	(M11	M12)	=	(M21			M22)
		(d2V		0)	(M21	M22)	=	(d2V M11	d2V M12)
	*/
	for(unsigned i = 0; i < half_offset; ++i)
	{
		// directly copy lower rows from in to out [factor 1]
		out[i] = in[half_offset + i];
	}

	// multiply submatrices
	for(unsigned i = 0; i < dimension; ++i)
	{
		for(unsigned j = 0; j < dimension; ++j)
		{
			double sum_M11 = 0;
			double sum_M12 = 0;
			for(unsigned k = 0; k < dimension; ++k)
			{
				// multiply upper rows by corresponding coefficient and copy
				sum_M11 += coeffs[i*dimension+k] * in[k * rowsize + j];
				sum_M12 += coeffs[i*dimension+k] * in[k * rowsize + j + dimension];
			}
			out[half_offset + i * rowsize + j] = sum_M11;
			out[half_offset + i * rowsize + j + dimension] = sum_M12;
		}
	}
}

#endif // MONODROMY_HPP_INCLUDED
