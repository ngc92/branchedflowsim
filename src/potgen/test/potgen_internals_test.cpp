#include <fstream>
#include "multiindex.hpp"
#include "potgen.hpp"
#include "dynamic_grid.hpp"
#include "randomize.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(potgen_internals_test)

BOOST_AUTO_TEST_CASE( randomize_error_test )
{
    DynamicGrid<complex_t> dgrid(2, 8, TransformationType::FFT_INDEX);
    auto rg = []() { return 0.5; };
    // uninitialized multi index
    BOOST_CHECK_THROW(randomize_generic( dgrid, rg, MultiIndex(1)), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE( randomize_phases_area_check )
{
	DynamicGrid<complex_t> dgrid(2, 8, TransformationType::FFT_INDEX);
	for( auto& f : dgrid )
		f = 1;

	// multiply with -1, so twice (or never) will raise an error
	auto rg = []() { return pi; };
    randomize_generic( dgrid, rg, fft_indexing(dgrid) );
	for( auto& f : dgrid ) {
        BOOST_REQUIRE_EQUAL(std::real(f), -1);
    }
}

BOOST_AUTO_TEST_CASE( randomize_phases_symmetry_check )
{
	DynamicGrid<complex_t> dgrid(2, 8, TransformationType::FFT_INDEX);
	for( auto& f : dgrid )
		f = 1;

	randomizePhases( dgrid, 0 );

	MultiIndex index = fft_indexing(dgrid);

	/// \todo cannot use multi index here, because [] assignment not possible right now
	std::vector<int> inverted( 2 );

	for( ;index.valid(); ++index)
	{
		// set inverted index
		for(int i = 0; i < 2; ++i)
			inverted[i] = -index[i];

		BOOST_CHECK_EQUAL( std::conj(dgrid(index)), dgrid(inverted));
	}

}

/// \todo randomize precondition exceptions check

/// \todo generate potential in k space test
BOOST_AUTO_TEST_SUITE_END()
