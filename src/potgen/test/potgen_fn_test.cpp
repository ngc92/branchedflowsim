#include "potgen.hpp"
#include "vector.hpp"
#include "interpolation.hpp"
#include "correlation.hpp"

#include <boost/test/unit_test.hpp>

/// \todo check that PGOptions cor_fun is valid

// helper function
static Potential generatePotential( int N,  std::size_t size, const PGOptions& opt )
{
	std::vector<std::size_t> sizes(N, size);
	return generatePotential( sizes, std::vector<double>(N, 1.0), opt);
}

BOOST_AUTO_TEST_SUITE(potgen_fn_test)

BOOST_AUTO_TEST_CASE( check_potgen_seed_determinism )
{
	PGOptions opt;
	opt.randomSeed = rand();
	opt.cor_fun = makeGaussianCorrelation(0.01);
	auto result1 = generatePotential(2, 128, opt);
	auto result2 = generatePotential(2, 128, opt);

	BOOST_CHECK_EQUAL( opt.randomSeed, result1.getSeed() );

	for(unsigned int i = 0; i < result1.getPotential().size(); ++i)
	{
		BOOST_CHECK_EQUAL( result1.getPotential()[i], result2.getPotential()[i] );
	}
}

BOOST_AUTO_TEST_CASE( check_potgen_randomization )
{
	PGOptions opt;
	opt.randomize = false;
	opt.cor_fun = makeGaussianCorrelation(0.01);
	auto result1 = generatePotential(1, 128, opt);
	// check that randomization of phases is disabled. howto?
}

BOOST_AUTO_TEST_CASE( deriv_generation )
{
	PGOptions opt;
	opt.cor_fun = makeGaussianCorrelation(0.01);
	opt.randomize = false;
	opt.maxDerivativeOrder = 2;
	auto p = generatePotential(2, 128, opt);

	// if not present, this generates segfaults
	for(MultiIndex idx(2, 0, 3); idx.valid(); ++idx)
	{
		// only check those of order <= 2!
		if( idx[0] + idx[1] <= 2 )
			BOOST_CHECK( p.hasDerivative( idx ) );
	}
}

/*! \todo check invariants:
		*	check resulting correlation function [approximate check?]
*/
BOOST_AUTO_TEST_SUITE_END()
