#include "fft.hpp"

#include "dynamic_grid.hpp"
#include "global.hpp"
#include "test_helpers.hpp"

#include <fftw3.h>
#include <vector>
#include <complex>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(fft_tests)

BOOST_AUTO_TEST_CASE(check_roundtrip)
{

	std::vector<complex_t> data(64);
	for(int i=0; i < 64; ++i)
	{
		data[i] = rand_value();
	}

	BOOST_TEST_CHECKPOINT("checking 1d fft");

	auto copy = data;

	fft(copy, std::vector<int>({64}));
	ifft(copy, std::vector<int>({64}));

	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_SMALL( std::abs(copy[i] - data[i]), 1e3*std::numeric_limits<double>::epsilon() );
	}

	BOOST_TEST_CHECKPOINT("checking 2d fft");

	copy = data;

	fft(copy, std::vector<int>({8,8}));
	ifft(copy, std::vector<int>({8,8}));
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_SMALL( std::abs(copy[i] - data[i]), 1e3*std::numeric_limits<double>::epsilon() );
	}


	BOOST_TEST_CHECKPOINT("checking 3d fft");

	copy = data;

	fft(copy, std::vector<int>({4, 4, 4}));
	ifft(copy, std::vector<int>({4, 4, 4}));
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_SMALL( std::abs(copy[i] - data[i]), 1e3*std::numeric_limits<double>::epsilon() );
	}
}

BOOST_AUTO_TEST_CASE(check_error_handling)
{

    std::vector<complex_t> data(35);
    for(int i=0; i < 35; ++i)
	{
		data[i] = rand_value();
	}

	auto copy = data;

	// throw because copy.size does not match expected size
	BOOST_CHECK_THROW(fft(copy, std::vector<int>({64})), std::invalid_argument);
	BOOST_CHECK_THROW(fft(copy, std::vector<int>({5,5})), std::invalid_argument);
	BOOST_CHECK_THROW(fft(copy, std::vector<int>({5,5,5})), std::invalid_argument);

	// check that copy is left unchanged by the preceding calls
	for(unsigned int i=0; i < data.size(); ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], data[i] );
	}

	// same thing for ifft
	BOOST_CHECK_THROW(ifft(copy, std::vector<int>({64})), std::invalid_argument);
	BOOST_CHECK_THROW(ifft(copy, std::vector<int>({5,5})), std::invalid_argument);
	BOOST_CHECK_THROW(ifft(copy, std::vector<int>({5,5,5})), std::invalid_argument);
	// check that copy is left unchanged
	for(unsigned int i=0; i < data.size(); ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], data[i] );
	}


	// check size argument fitting into int
	BOOST_CHECK_THROW( fft(copy, std::vector<int>({-1})), std::overflow_error );
	BOOST_CHECK_THROW( ifft(copy, std::vector<int>({-1})), std::overflow_error );
}

/// \todo grid_interface_test is obsolete
/// \todo iterator interface and DynamicGrid interface tests missing.
/*
BOOST_AUTO_TEST_CASE(grid_interface_test)
{

    std::vector<std::complex<double>> data(64);
    for(int i=0; i < 64; ++i)
	{
		data[i] = rand_value();
	}

	// 1d

	auto grid_dat = data;
    grid<decltype(data), 1> grid1d(64, grid_dat );

	auto copy = data;

	fft(grid1d);
	fft(1, copy, 64);
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], grid_dat[i] );
	}

	ifft(grid1d);
	ifft(1, copy, 64);
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], grid_dat[i] );
	}

	// 3d
    grid<decltype(data), 3> grid3d(4, grid_dat );

	fft(grid3d);
	fft(3, copy, 4);
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], grid_dat[i] );
	}

	ifft(grid3d);
	ifft(3, copy, 4);
	for(int i=0; i < 64; ++i)
	{
		BOOST_CHECK_EQUAL( copy[i], grid_dat[i] );
	}
}*/

BOOST_AUTO_TEST_CASE( fftw_wisdom_check )
{
	// try fft without wisdom
	fftw_forget_wisdom();
	fftw_cleanup();

	std::vector<std::complex<double>> data(64);
	for( auto& d : data )
		d = rand_value();

	auto copy = data;

	// fft without wisdom
	fft( data, std::vector<int>({64}) );
	// fft with wisdom
	fft( copy, std::vector<int>({64}) );

	for(unsigned i = 0; i < data.size(); ++i)
	{
		BOOST_CHECK_SMALL( std::abs(data[i] - copy[i]), 1e-10 );
	}

	// check inverse
	fftw_cleanup();
	ifft( data, std::vector<int>({64}) );
	ifft( copy, std::vector<int>({64}) );
	for(unsigned i = 0; i < data.size(); ++i)
		BOOST_CHECK_SMALL( std::abs(data[i] - copy[i]), 1e-10 );


}

/// \todo maybe add another check that actually checks values for a know case.

BOOST_AUTO_TEST_SUITE_END()
