#include "multiindex.hpp"

#include <boost/test/unit_test.hpp>
#include <numeric>

BOOST_AUTO_TEST_SUITE(multiindex)

// ------------------------------------------------------------------------------------------------------
//  constructor test
// ------------------
BOOST_AUTO_TEST_CASE( constructor )
{
    // one dimensional index
    MultiIndex index(1);

    BOOST_CHECK( !index.valid() );
    BOOST_CHECK_EQUAL( index.size(), 1);
    // bounds and pos unspecified after ctor

    // three dimensional index
    MultiIndex index3(3);
    BOOST_CHECK( !index3.valid() );
    BOOST_CHECK_EQUAL( index3.size(), 3);

    // 10 dimensional index not possible
    BOOST_CHECK_THROW( [](){ MultiIndex index(10); }(), std::logic_error );
    BOOST_CHECK_THROW( [](){ MultiIndex index(-2); }(), std::logic_error );
}

BOOST_AUTO_TEST_CASE( bounds_constructor )
{
    // one dimensional index
    MultiIndex index(2, -2, 3);

    BOOST_CHECK( index.valid() );
    BOOST_CHECK_EQUAL( index.size(), 2);
    // check bounds and values
    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[1], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[2], -2 );

    BOOST_CHECK_EQUAL( index.getUpperBound()[0], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[2], 3);

    BOOST_CHECK_EQUAL( index[0], -2 );
    BOOST_CHECK_EQUAL( index[1], -2 );
    BOOST_CHECK_EQUAL( index[2], -2 );
}


// ------------------------------------------------------------------------------------------------------
//  bounds tests
// ------------------

BOOST_AUTO_TEST_CASE( set_bounds )
{
    // three dimensional index
    MultiIndex index(3);

    // function for all bounds
    index.setLowerBound( -2 );
    index.setUpperBound( 3 );

    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[1], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[2], -2 );

    BOOST_CHECK_EQUAL( index.getUpperBound()[0], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[2], 3);

    // dimension specific
    index.setLowerBoundAt(1, -5);
    index.setUpperBoundAt(2, 4);

    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[1], -5 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[2], -2 );

    BOOST_CHECK_EQUAL( index.getUpperBound()[0], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[2], 4);
}

BOOST_AUTO_TEST_CASE( set_bounds_at_exception )
{
    // three dimensional index
    MultiIndex index(3);

    // correctly initialize
    index.setLowerBound( -2 );
    index.setUpperBound( 3 );

    // dimension specific
    BOOST_CHECK_THROW(index.setLowerBoundAt(5, -5), std::logic_error);
    BOOST_CHECK_THROW(index.setUpperBoundAt(-1, -5), std::logic_error);

    // check that bounds are unchanged (strong guarantee)
    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[1], -2 );
    BOOST_CHECK_EQUAL( index.getLowerBound()[2], -2 );

    BOOST_CHECK_EQUAL( index.getUpperBound()[0], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 3);
    BOOST_CHECK_EQUAL( index.getUpperBound()[2], 3);
}

// ------------------------------------------------------------------------------------------------------
//  initialization test
// ---------------------
BOOST_AUTO_TEST_CASE( init_index )
{
    // three dimensional index
    MultiIndex index(3);

    // correctly initialize
    index.setLowerBound( -2 );
    index.setUpperBound( 3 );

    index.init();

    BOOST_CHECK( index.valid() );

    // check that bounds are still valid and that position is initialized
    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 3 );
    BOOST_CHECK_EQUAL( index[0], -2 );
    BOOST_CHECK_EQUAL( index[1], -2 );
    BOOST_CHECK_EQUAL( index[2], -2 );
}

BOOST_AUTO_TEST_CASE( init_exceptions )
{
    // three dimensional index
    MultiIndex index(2);

    // correctly initialize
    index.setLowerBound( -2 );
    index.setUpperBound( -3 );

    BOOST_CHECK_THROW(index.init(), std::logic_error);

    BOOST_CHECK( !index.valid() );

    index.setUpperBound( -1);
    index.init();
    // check strong guarantee (i.e. lower bound was not changed)
    BOOST_CHECK_EQUAL( index.getLowerBound()[0], -2 );
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], -1 );

    // check double init
    BOOST_CHECK_THROW( index.init(), std::logic_error );
}

BOOST_AUTO_TEST_CASE( change_after_init_exceptions )
{
    MultiIndex index(2);
    index.setLowerBound(-1);
    index.setUpperBound( 2);
    index.init();

    // check change after init
    BOOST_CHECK_THROW( index.setUpperBound(1), std::logic_error );
    BOOST_CHECK_THROW( index.setLowerBound(1), std::logic_error );
    BOOST_CHECK_THROW( index.setUpperBoundAt(1, 1), std::logic_error );
    BOOST_CHECK_THROW( index.setLowerBoundAt(1, 1), std::logic_error );

    // check strong guarantee
    BOOST_CHECK_EQUAL( index.getLowerBound()[1], -1 );
    BOOST_CHECK_EQUAL( index.getUpperBound()[1], 2 );
    BOOST_CHECK_EQUAL( index[1], -1);
}

// ------------------------------------------------------------------------------------------------------
//  increment test
// ---------------------
BOOST_AUTO_TEST_CASE( increment_test )
{
    MultiIndex index(2);
    index.setLowerBound( -1 );
    index.setUpperBound( 3 );
    int counter = 0;
    for( index.init(); index.valid(); ++index)
    {
        counter++;
        BOOST_CHECK_GE( index[0], -1 );
        BOOST_CHECK_GE( index[1], -1 );
        BOOST_CHECK_LT( index[0], 3 );
        BOOST_CHECK_LT( index[1], 3 );
    }
    BOOST_CHECK_EQUAL( counter, 4*4);

    // re-init should be possible
    index.init();

    // check that last index is incremented first
    ++index;
    BOOST_CHECK_EQUAL( index[0], -1 );
    BOOST_CHECK_EQUAL( index[1], 0 );
}

// increment test for non uniform bounds
BOOST_AUTO_TEST_CASE( increment_varying_bounds )
{
    MultiIndex index(2);
    index.setLowerBoundAt(0, -1);
    index.setLowerBoundAt(1, -2);
    index.setUpperBoundAt(0, 3 );
    index.setUpperBoundAt(1, 4 );

    index.init();
    BOOST_CHECK_EQUAL( index[0], -1 );
    BOOST_CHECK_EQUAL( index[1], -2 );

    int counter = 0;
    for( ; index.valid(); ++index)
    {
        BOOST_CHECK_GE( index[0], -1 );
        BOOST_CHECK_GE( index[1], -2 );
        BOOST_CHECK_LT( index[0], 3 );
        BOOST_CHECK_LT( index[1], 4 );
        counter++;
    }
    BOOST_CHECK_EQUAL( counter, 4*6);
}

// ------------------------------------------------------------------------------------------------------
//  iterator interface tests
// --------------------------
BOOST_AUTO_TEST_CASE( iterator_interface )
{
    MultiIndex index(4, 2, 5);

    BOOST_CHECK_EQUAL( &index[0], index.begin());
    BOOST_CHECK_EQUAL( &index[1], (index.begin()+1));
    BOOST_CHECK_EQUAL( &index[3], index.end()-1);
}

// ------------------------------------------------------------------------------------------------------
//  helper tests
// --------------------------

BOOST_AUTO_TEST_CASE( accumulated_test )
{
    MultiIndex index(3, 2, 5);

    BOOST_CHECK_EQUAL( index.getAccumulated(), 3*2 );
    ++index;
    BOOST_CHECK_EQUAL( index.getAccumulated(), 3*2+1 );
    ++index;
    BOOST_CHECK_EQUAL( index.getAccumulated(), std::accumulate( index.begin(), index.end(), 0 ));
}

BOOST_AUTO_TEST_CASE( as_vector )
{
    MultiIndex index(3, 0, 5);
    // create random index
    for(int inc = rand() % 25 + 10; inc > 0; --inc)    ++index;

    auto vec = index.getAsVector();
    BOOST_CHECK_EQUAL( vec.size(), index.size() );
    for(unsigned i = 0; i < index.size(); ++i)
    {
        BOOST_CHECK_EQUAL( vec[i], index[i] );
    }
}

BOOST_AUTO_TEST_SUITE_END()
