#include <fftw3.h>
#include <cassert>
#include <array>
#include <cmath>
#include <cstring>
#include <mutex>
#include <iostream>
#include "fft.hpp"
#include "global.hpp"
#include "profiling.hpp"
#include "dynamic_grid.hpp"
#include "config.h" // for wisdom filename

using std::size_t;

void setFFTThreads( size_t threads )
{
	// this is a general purpose init method
	/// \todo error handling
	fftw_init_threads();
	fftw_import_wisdom_from_filename(WISDOM_FILENAME);
	fftw_plan_with_nthreads( threads );
}

void saveFFTWisdom()
{
	/// \todo error handling
	fftw_export_wisdom_to_filename(WISDOM_FILENAME);
}

std::mutex fft_mutex;

// helper functions

typedef std::vector<std::complex<double>> CArray;

// plan generation
fftw_plan get_plan(std::size_t dimension, fft_extents sizes, complex_t* array, decltype(FFTW_FORWARD) direction)
{
	// lock the fft mutex: planning cannot be done concurrently
	std::unique_lock<std::mutex> lock(fft_mutex);

	static_assert( sizeof(fftw_complex) == sizeof(complex_t), "fftw complex does not match std::complex" );
	PROFILE_BLOCK("fftw plan");
	// try to generate a plan from wisdom. this does not access array, so it is save
	fftw_plan p = fftw_plan_dft(dimension, &sizes[0], (fftw_complex*)array, (fftw_complex*)array, direction, FFTW_MEASURE | FFTW_WISDOM_ONLY);
	// if no plan exists, we have no choice but to create a temp array to perform measurements
	if( !p )
	{
		std::size_t elements = safe_product( sizes );
		// backup, then create plan
		auto copy = fftw_alloc_complex( elements );
		// check that we were able to allocate sufficient memory
		if(!copy)
			BOOST_THROW_EXCEPTION( std::bad_alloc() );

		// copy data into new array
		memcpy(copy, array, elements * sizeof(complex_t));
		// create fftw plan
		p = fftw_plan_dft(dimension, &sizes[0], (fftw_complex*)array, (fftw_complex*)array, direction, FFTW_MEASURE);
		// restore data and delete temp array.
		std::memcpy(array, copy, elements * sizeof(complex_t));
		fftw_free( copy );
	}

	return p;
}

void destroy_plan( fftw_plan plan )
{
	// destruction cannot be done concurrently
	std::unique_lock<std::mutex> lock(fft_mutex);
	fftw_destroy_plan( plan );
}

void fft(complex_t* begin, complex_t* end, fft_extents sizes)
{
	PROFILE_BLOCK("fft");

	// pow_int also checks for overflow
	std::size_t size = safe_product( sizes );
	std::size_t elcount = std::distance(begin, end);

	if(elcount != size)
		THROW_EXCEPTION( std::invalid_argument, "Supplied vector size %1% does not match ifft domain %2%", elcount, size );

	// generate plan
	fftw_plan p = get_plan(sizes.size(), sizes, begin, FFTW_FORWARD);
	if( !p )
		THROW_EXCEPTION( std::runtime_error, "could not create fftw plan" );

	// input vector
	fftw_execute(p);

	// destruction cannot be done concurrently
	destroy_plan(p);
}

void fft(CArray& x, fft_extents size)
{
	fft(&x[0], &x.back()+1, size);
}


void ifft(complex_t* begin, complex_t* end, fft_extents sizes)
{
	PROFILE_BLOCK("ifft");

	std::size_t size = safe_product(sizes);
	std::size_t elcount = std::distance(begin, end);

	if(elcount != size)
		THROW_EXCEPTION( std::invalid_argument, "Supplied vector size %1% does not match ifft domain %2%", elcount, size );

	// generate plan
	fftw_plan p = get_plan(sizes.size(), sizes, begin, FFTW_BACKWARD);
	if( !p )
		THROW_EXCEPTION( std::runtime_error, "could not create fftw plan" );

	// input vector
	fftw_execute(p);

	std::for_each( begin, end, [elcount](complex_t& v){v /= elcount;} );

	destroy_plan(p);
}

void ifft(CArray& x, fft_extents sizes)
{
	ifft(&x[0], &x.back()+1, sizes);
}

void fft(DynamicGrid<complex_t>& grid)
{
	fft( grid.begin(), grid.end(), std::vector<int>(grid.getExtents().begin(), grid.getExtents().end()) );
}

void ifft(DynamicGrid<complex_t>& grid)
{
	ifft( grid.begin(), grid.end(), std::vector<int>(grid.getExtents().begin(), grid.getExtents().end()) );
}
