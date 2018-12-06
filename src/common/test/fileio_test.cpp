#include "fileIO.hpp"
#include "dynamic_grid.hpp"
#include "test_helpers.hpp"

#include <fstream>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(fileio_test)
/// \todo these test methods are obsolete, because io has been moved to the grid classes.
///            new test for the grid classes are yet to be written
/*
BOOST_AUTO_TEST_CASE( fileIO_binary_dump )
{
    std::vector<double> dump_data = {1,5,3,5,7,4.6,3,-5, .3, 3,9,1445,1e6,-25.7, 45};
    std::fstream target("tmp.tmp", std::fstream::out | std::fstream::binary);
    binaryDumpContainer(target, dump_data);
    target.close();

    BOOST_TEST_CHECKPOINT("vector written");

    std::fstream source("tmp.tmp", std::fstream::in);
    // the current implementation requires a preallocated buffer?
    std::vector<double> cmp(15);
    binaryLoadContainer(source, cmp);

    BOOST_TEST_CHECKPOINT("vector read");

    // check vector round-trip
    for(unsigned int i=0; i < dump_data.size(); ++i)
    {
        BOOST_CHECK_EQUAL( dump_data[i], cmp[i] );
    }
}


BOOST_AUTO_TEST_CASE( fileIO_binary_grid_dump )
{
    default_grid grid2(std::vector<std::size_t>{2, 4}, TransformationType::IDENTITY);
    for( auto& v : grid2)
        v = rand01();

    std::fstream target("tmp.tmp", std::fstream::out | std::fstream::binary);
    binaryDumpGrid(target, grid2);
    target.close();

    BOOST_TEST_CHECKPOINT("grid written");

    std::fstream source("tmp.tmp", std::fstream::in);
    auto cmp = binaryLoadGrid<default_grid>(source);

    BOOST_TEST_CHECKPOINT("grid read");

    BOOST_CHECK_EQUAL( grid2.getExtents()[0], cmp.getExtents()[0] );
    BOOST_CHECK_EQUAL( grid2.getExtents()[1], cmp.getExtents()[1] );
    BOOST_CHECK_EQUAL( grid2.getElementCount(), cmp.getElementCount() );

    for(unsigned int i=0; i < grid2.getElementCount(); ++i)
    {
        BOOST_CHECK_EQUAL( grid2[i], cmp[i] );
    }
}


BOOST_AUTO_TEST_CASE( fileio_error_check )
{
    std::vector<double> dump_data = {1,5,3,5,7,4.6,3,-5, .3, 3,9,1445,1e6,-25.7, 45};
    std::fstream target("tmp.tmp", std::fstream::out | std::fstream::binary);
    binaryDumpContainer(target, dump_data);
    target.close();

    std::fstream source("tmp.tmp", std::fstream::in);

    // loading to an incompatible vector type creates exception
    std::vector<int> incomp;
    source.seekg(0);
    BOOST_CHECK_THROW( binaryLoadContainer( source, incomp ), std::runtime_error );

    /// \todo check grid error handling
    /// \todo error handling in case of writing to read-only file and vice versa
    /// \todo maybe there is a possibility to test whether a file is opened binary
}
*/
/// \todo some more testing of TGA writing methods

BOOST_AUTO_TEST_SUITE_END()
