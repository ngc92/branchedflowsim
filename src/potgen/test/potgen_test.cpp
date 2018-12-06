#include "potgen.hpp"
#include "vector.hpp"
#include "interpolation.hpp"
#include "fileIO.hpp"
#include "correlation.hpp"
#include "config.h"

#include <fstream>

#define BOOST_TEST_MODULE potgen_test
#include <boost/test/unit_test.hpp>

// declaration of compare function
/// \todo define some kind of test utility header
bool compare_files_binary(std::string f1, std::string f2, bool verbose);

// helper function
static Potential generatePotential( int N,  std::size_t size, const PGOptions& opt )
{
	std::vector<std::size_t> sizes(N, size);
	return generatePotential( sizes, std::vector<double>(N, 1.0), opt);
}

BOOST_AUTO_TEST_SUITE(potgen_test)

BOOST_AUTO_TEST_CASE( potential_properties )
{
	PGOptions opt;
	opt.randomSeed = rand();
	opt.maxDerivativeOrder = 2;
	opt.corrlength = 0.1;
	opt.cor_fun = makeGaussianCorrelation(0.01);
	auto result = generatePotential(2, 128, opt);
	BOOST_CHECK_EQUAL( opt.randomSeed, result.getSeed() );
	BOOST_CHECK_EQUAL( result.getExtents()[0], 128);
	BOOST_CHECK_EQUAL( result.getExtents()[1], 128);
	BOOST_CHECK_EQUAL( result.getSupport()[0], 1.0);
	BOOST_CHECK_EQUAL( result.getSupport()[1], 1.0);
	BOOST_CHECK_EQUAL( result.getCorrelationLength(), opt.corrlength);
	BOOST_CHECK( result.hasDerivativesOfOrder( opt.maxDerivativeOrder ) );

}

// check determinism
BOOST_AUTO_TEST_CASE( check_potgen_determinism )
{
	auto f = makeGaussianCorrelation(0.01);
	PGOptions opt;
	opt.randomSeed = rand();
	opt.cor_fun = f;
	auto result1 = generatePotential(2, 128, opt);
	auto result2 = generatePotential(2, 128, opt);

	BOOST_CHECK_EQUAL( opt.randomSeed, result1.getSeed() );

	for(unsigned int i = 0; i < result1.getPotential().getElementCount(); ++i)
	{
		BOOST_CHECK_EQUAL( result1.getPotential()[i], result2.getPotential()[i] );
	}

	// check consistency across multiple revision
	std::cout << TEST_DATA_DIRECTORY"/pot_2d_128_ref" << "\n";
	std::fstream rfile(TEST_DATA_DIRECTORY"/pot_2d_128_ref", std::fstream::in | std::fstream::binary);
	if(!rfile.is_open()) {
		BOOST_FAIL("Could not open reference file " TEST_DATA_DIRECTORY "/pot_2d_128_ref");

	}

	auto ref = Potential::readFromFile(rfile);
	opt.randomSeed = ref.getSeed();
	opt.cor_fun = f;
	opt.corrlength = 0.01;
	auto result3 = generatePotential(2, 128, opt);

	std::fstream generated("pot_2d_128_cmp", std::fstream::out | std::fstream::binary);
	result3.writeToFile(generated);

	BOOST_CHECK(compare_files_binary(TEST_DATA_DIRECTORY"/pot_2d_128_ref", "pot_2d_128_cmp", true));

	for(int x = 0; x < 128*128; ++x)
	{
		BOOST_REQUIRE_EQUAL(ref.getPotential()[x], result3.getPotential()[x]);

	}

	/// \todo error check for too high sizes
	/// \todo this belong into another test case
	opt.cor_fun = makeGaussianCorrelation(1);
	BOOST_CHECK_THROW( generatePotential(3, std::size_t(-1), opt), boost::exception );
}

BOOST_AUTO_TEST_CASE( check_potgen_derivatives )
{
	/// \todo for this check to make sense, we should actually generate a larger potential and scale down by hand, then make comparisons
	constexpr int size = 5120;

	auto f = makeGaussianCorrelation(0.01);
	/// \todo allow the direct use of lambdas
	PGOptions opt;
	opt.randomSeed = rand();
	opt.maxDerivativeOrder = 2;
	opt.cor_fun = f;
	auto result1 = generatePotential(1, size, opt);
	result1.setSupport(std::vector<double>{1.0});

	auto pgrid = result1.getPotential().shallow_copy();
	pgrid.setAccessMode(TransformationType::PERIODIC);
	auto dgrid = result1.getDerivative( makeIndexVector(1, {0}) ).shallow_copy();
	dgrid.setAccessMode(TransformationType::PERIODIC);

	constexpr double STEP = 0.01;

	double p_int = pgrid(std::vector<int>{2});

	/// \todo use an integrator and check that results are consistent
	int c = 0;

	double max_dev = 0;
	double avg_dev = 0;

	for(double x = 2; x < size - 2; x += STEP)
	{
		gen_vect pos(1);
		pos[0] = x;
		double p_here = linearInterpolate( pgrid, pos );

		double dx_here = linearInterpolate( dgrid, pos );
		p_int += dx_here * STEP / size;

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
}

BOOST_AUTO_TEST_SUITE_END()
