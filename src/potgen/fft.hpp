/*! \file fft.hpp
	\brief Fast fourier transform interface
	\details This file was completely documented and tested on 05.06.14.
			Changes since:
*/
#ifndef FFT_HPP_INCLUDED
#define FFT_HPP_INCLUDED

#include <vector>
#include <complex>

typedef std::complex<double> complex_t;
typedef const std::vector<int>& fft_extents;

template<class V>
class DynamicGrid;

// ----------------------------------------------------------
//				config interface
// ----------------------------------------------------------
void setFFTThreads( std::size_t threads );

void saveFFTWisdom();

// ----------------------------------------------------------
//				iterator interface
// ----------------------------------------------------------
// fourier transform
void fft(complex_t* begin, complex_t* end, fft_extents size);

// inverse fourier transform
void ifft(complex_t* begin, complex_t* end, fft_extents size);


// ----------------------------------------------------------
//				vector interface
// ----------------------------------------------------------

// fourier transform
void fft(std::vector<std::complex<double>>& data, fft_extents size);

// inverse fourier transform
void ifft(std::vector<std::complex<double>>& data, fft_extents size);

// ----------------------------------------------------------
//  			grid interface
// ----------------------------------------------------------

void fft(DynamicGrid<complex_t>& grid);
void ifft(DynamicGrid<complex_t>& grid);
#endif // FFT_HPP_INCLUDED
