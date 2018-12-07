#include "multiindex.hpp"
#include "potgen.hpp"
#include "fft.hpp"
#include "dynamic_grid.hpp"

#include <boost/test/unit_test.hpp>

complex_grid discretizeFunctionForFFT(std::vector<std::size_t> gridsize, std::vector<double>, correlation_fn F);

static const std::vector<double> vec_1{1.0};
static const std::vector<double> vec_2{1.0, 1.0};
static const std::vector<double> vec_3{1.0, 1.0, 1.0};

BOOST_AUTO_TEST_SUITE(potgen_internals_test)

// test discretize function
BOOST_AUTO_TEST_CASE( discretizeFunctionForFFT_1d )
{
	std::function<double(const gen_vect&)> f = [](const gen_vect& x) { return std::exp(-x[0]*x[0]*10); };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{64}, vec_1, f);

	gen_vect s(1);
	for(int i=0; i < 64; ++i)
	{
		// compare with shifted: this is required to make FT real
		s[0] = ((i+32)%64 - 32) / 64.0;
		BOOST_CHECK_EQUAL(dgrid.getContainer().at<complex_t>(i), f(s));
	}

	// check symmetry
	/// \todo this is not symmetric. why is this correct?
	/*for(int i = 0; i < 32; ++i)
	{
		BOOST_CHECK_EQUAL(dgrid.getContainer().at<double>(i), dgrid.getContainer().at<double>(63-i));
	}*/
}

BOOST_AUTO_TEST_CASE( discretize_1d_identity )
{
	constexpr std::size_t size = 64;
	std::function<double(const gen_vect&)> f = [=](const gen_vect& x) { return x[0] * size; };

	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{size}, vec_1, f);

	gen_vect s(1);
	/// \todo is there any need for the complicated index calculations here?
	for(unsigned i=0; i < size; ++i)
	{
		BOOST_REQUIRE_EQUAL((int)((dgrid.getContainer().at<complex_t>(i)).real()), (i+size/2)%size - size/2);
	}
}

BOOST_AUTO_TEST_CASE( discretizeFunctionForFFT_2d )
{
	// first check target range
	std::function<double(const gen_vect&)> f = [](const gen_vect&) { return 1; };
	auto dgrid = discretizeFunctionForFFT(std::vector<size_t>{8,8}, vec_2, f);

	for(int i=0; i < 64; ++i)
	{
		BOOST_REQUIRE_EQUAL(dgrid.getContainer().at<complex_t>(i), 1.0);
	}

	std::function<double(const gen_vect&)> f2 = [](const gen_vect& g) { return std::exp((-g[0]*g[0]-g[1]*g[1])*100); };
	auto dgrid2 = discretizeFunctionForFFT(std::vector<size_t>{32, 32}, vec_2, f2);

	// check that fft phases make it completely real
	std::vector<complex_t> ct;
	ct.assign(dgrid2.begin(), dgrid2.end());

	fft( ct, std::vector<int>({32, 32}) );

	for(unsigned i = 0; i < ct.size(); ++i)
	{
		BOOST_REQUIRE_SMALL(std::imag(ct[i]), 1e-10);
		BOOST_REQUIRE_GE(std::real(ct[i]), -1e-10);
	}

	/// \todo check general positions for general function
}

BOOST_AUTO_TEST_CASE( discretizeFunctionForFFT_error )
{
	std::function<double(const gen_vect&)> f = [](const gen_vect&) { return 1; };
	// odd_sized grids are unsupported
	BOOST_CHECK_THROW( (discretizeFunctionForFFT(std::vector<size_t>{3}, vec_1, f)), std::invalid_argument);
	// incompatible dimensions between grid size and support
	BOOST_CHECK_THROW( (discretizeFunctionForFFT(std::vector<size_t>{2, 2}, vec_1, f)), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
