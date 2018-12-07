#include "global.hpp"
#include "util.hpp"
#include "test_helpers.hpp"

#define BOOST_TEST_MODULE common_test
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(global_test)

BOOST_AUTO_TEST_CASE(pi_test)
{
    BOOST_CHECK_SMALL( std::sin(pi), 1e-15);
    BOOST_CHECK_EQUAL( std::sin(pi / 2), 1.0 );
    BOOST_CHECK_EQUAL( std::cos(pi), -1.0 );
}

BOOST_AUTO_TEST_CASE(pow_int_test)
{
    BOOST_CHECK_EQUAL( pow_int(10, 5), 100000 );
    BOOST_CHECK_EQUAL( pow_int(2u, 30), 1u<<30 );

    /// test overflow protection
    BOOST_CHECK_THROW( pow_int(-1u, 3), std::overflow_error);
}

BOOST_AUTO_TEST_CASE( scale_vector_test )
{
    std::vector<double> test= {1, 4, 3, 6, 3, 6};
    scaleVectorBy(test, 2.0);

    BOOST_CHECK_EQUAL(test[0], 2);
    BOOST_CHECK_EQUAL(test[1], 4*2);
    BOOST_CHECK_EQUAL(test[5], 6*2);
}


BOOST_AUTO_TEST_SUITE_END()
