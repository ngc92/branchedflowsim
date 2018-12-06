#include "dynamic_grid.hpp"
#include "grid_storage.hpp"
#include "multiindex.hpp"

#include <boost/test/unit_test.hpp>

using std::size_t;

// ---------------------------------------------------------------------------------------------
//          GridStorage test cases
// ---------------------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(grid_storage)

BOOST_AUTO_TEST_CASE( grid_storage_ctor )
{
    GridStorage storage = GridStorage::create<double>(10);

    BOOST_CHECK_EQUAL( storage.getStride(), sizeof(double) );
    BOOST_CHECK_EQUAL( storage.getType().name(), typeid(double).name() );
    BOOST_CHECK_EQUAL( storage.size(), 10 );
    BOOST_CHECK_NE( storage.getStartingAddress(), (void*)nullptr);      // cast to void* to disambiguate operator<<
}

BOOST_AUTO_TEST_CASE( grid_storage_access )
{
    GridStorage storage = GridStorage::create<double>(10);

    BOOST_CHECK_EQUAL( std::distance(&storage.at<double>(2), &storage.at<double>(8)), 6 );
    BOOST_CHECK_EQUAL( storage.getStartingAddress(), &storage.at<double>(0));
}

BOOST_AUTO_TEST_SUITE_END()

// --------------------------------------------------------------------------------------------------
//          DynamicGrid test cases
// --------------------------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(dynamic_grid_test)

BOOST_AUTO_TEST_CASE(dynamic_grid_ctors)
{
    size_t dimension = 3;
    size_t size = 32;
    default_grid grid(dimension, size, TransformationType::IDENTITY);

    // check data
    BOOST_CHECK_EQUAL( grid.getDimension(), dimension );
    BOOST_CHECK_EQUAL( grid.getExtents().size(), dimension );
    BOOST_CHECK_EQUAL( grid.getExtents()[0], size );
    BOOST_CHECK_EQUAL( grid.getExtents()[1], size );
    BOOST_CHECK_EQUAL( grid.getExtents()[2], size );

    BOOST_CHECK( grid.getAccessMode() == TransformationType::IDENTITY);

    // check container: size
    BOOST_CHECK_EQUAL( grid.getContainer().size(), pow_int(size, dimension) );
    BOOST_CHECK_EQUAL( grid.getContainer().size(), grid.getElementCount() );


    // non square constructor
    std::vector<std::size_t> extent({32, 64});
    default_grid grid2( extent );

    // check data
    BOOST_CHECK_EQUAL( grid2.getDimension(), 2 );
    BOOST_CHECK_EQUAL( grid2.getExtents().size(), 2);
    BOOST_CHECK_EQUAL( grid2.getExtents()[0], 32);
    BOOST_CHECK_EQUAL( grid2.getExtents()[1], 64);
    BOOST_CHECK( grid2.getAccessMode() == TransformationType::IDENTITY);

    // check container: size
    BOOST_CHECK_EQUAL( grid2.getContainer().size(), 32*64 );
    BOOST_CHECK_EQUAL( grid2.getContainer().size(), grid2.getElementCount() );
}

/// \todo the following two tests are almost identical
BOOST_AUTO_TEST_CASE( edit_roundtrip )
{
    size_t dimension = 3;
    size_t size = 32;
    default_grid grid(dimension, size, TransformationType::IDENTITY);

    std::vector<int> index{0,0,0};
    grid(index) = 5;
    BOOST_CHECK_EQUAL( grid(index), 5);

    index = {3,6,2};
    grid(index) = 3;
    BOOST_CHECK_EQUAL( grid(index), 3);
}

BOOST_AUTO_TEST_CASE( edit_roundtrip_nonsquare )
{
    std::vector<std::size_t> sizes{16, 32, 48};
    default_grid grid(sizes, TransformationType::IDENTITY);

    std::vector<int> index{0,0,0};
    grid(index) = 5;
    BOOST_CHECK_EQUAL( grid(index), 5);

    index = {3,6,2};
    grid(index) = 3;
    BOOST_CHECK_EQUAL( grid(index), 3);
}

BOOST_AUTO_TEST_CASE( offset_of )
{
    size_t dimension = 3;
    size_t size = 32;
    default_grid grid(dimension, size, TransformationType::IDENTITY);

    std::vector<int> index{0,1,0};
    grid(index) = 5;
    std::size_t offset = grid.getOffsetOf( index );
    BOOST_CHECK_EQUAL( grid.getContainer().at<double>(offset), grid(index));
}

BOOST_AUTO_TEST_CASE( check_adress_order )
{
    size_t dimension = 3;
    size_t size = 32;
    default_grid grid(dimension, size, TransformationType::IDENTITY);

    MultiIndex index(3);
    index.setLowerBound(0);
    index.setUpperBound(32);
    index.init();
    std::size_t o1 = grid.getOffsetOf( index );
    double* p1 = &grid(index);
    ++index;
    std::size_t o2 = grid.getOffsetOf( index );
    double* p2 = &grid(index);

    BOOST_CHECK_EQUAL( o1 + 1, o2);
    BOOST_CHECK_EQUAL( p1 + 1, p2);
}

BOOST_AUTO_TEST_CASE( check_nonsquare_grid )
{
    default_grid grid(std::vector<std::size_t>({16, 8}), TransformationType::IDENTITY);

    for(MultiIndex idx = grid.getIndex(); idx.valid(); ++idx)
    {
        grid(idx) = 42;
    }

    for(int i = 0; i < 16*8; ++i)
    {
        BOOST_CHECK_EQUAL( grid[i], 42 );
    }

    std::vector<int> idx(2);
    int i = 0;
    for(int x = 0; x < 16; ++x)
    for(int y = 0; y < 8; ++y)
    {
        idx[0] = x;
        idx[1] = y;
        BOOST_CHECK_EQUAL( grid.getOffsetOf(idx), i++);
        BOOST_CHECK_EQUAL( grid(idx), 42 );
    }
}


BOOST_AUTO_TEST_SUITE_END()
