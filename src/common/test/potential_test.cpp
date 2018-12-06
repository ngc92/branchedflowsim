#include "potential.hpp"
#include <fstream>
/*
#include <boost/test/unit_test.hpp>

typedef grid<std::vector<double>, 1> grid1d;
typedef grid<std::vector<double>, 2> grid2d;
typedef grid<std::vector<double>, 3> grid3d;

BOOST_AUTO_TEST_SUITE(potential_tests)

BOOST_AUTO_TEST_CASE(potential_ctor_and_info)
{
    Potential<grid2d> pot(1.0, 256, 2);

    BOOST_CHECK_EQUAL(pot.getSize(), 256);
    BOOST_CHECK_EQUAL(pot.getMaxOrder(), 2);
    BOOST_CHECK_EQUAL(pot.getSupport(), 1.0);

    pot.setCreationInfo( 3, 1, 0.5 );
    BOOST_CHECK_EQUAL( pot.getSeed(), 3 );
    BOOST_CHECK_EQUAL( pot.getPotgenVersion(), 1 );
    BOOST_CHECK_EQUAL( pot.getCorrelationLength(), 0.5 );
}

BOOST_AUTO_TEST_CASE(potential_set_derivatives)
{
    Potential<grid2d> pot(2.0, 4, 2);

    std::vector<double> pv(16);
    for(int i=0; i < 16; ++i)
        pv[i] = i;

    pot.setPotential( std::move(pv) );
    auto pg = pot.getPotential();

    BOOST_CHECK_EQUAL( pg(0,0), 0 );
    BOOST_CHECK_EQUAL( pg(0,3), 3 );
    BOOST_CHECK_EQUAL( pg(1,0), 4 );

    BOOST_TEST_CHECKPOINT( "get/set potential" );

    std::vector<double> dv(16);
    for(int i=0; i < 16; ++i)
        dv[i] = i*i;

    std::vector<int> i {1,0};
    pot.setDerivative( i, std::move(dv) );
    auto dg = pot.getDerivative( i );

    BOOST_CHECK_EQUAL( dg(0,0), 0 );
    BOOST_CHECK_EQUAL( dg(0,3), 9 );
    BOOST_CHECK_EQUAL( dg(1,0), 16 );

    i[0] = 3;
    BOOST_CHECK_THROW( pot.setDerivative(i, std::move(pv)), std::invalid_argument );

    // check that the data survives moves
    Potential<grid2d> copy = std::move( pot );
    i[0] = 1;
    auto dgc = copy.getDerivative( i );

    BOOST_CHECK_EQUAL( dgc(0,0), 0 );
    BOOST_CHECK_EQUAL( dgc(0,3), 9 );
    BOOST_CHECK_EQUAL( dgc(1,0), 16 );
}

BOOST_AUTO_TEST_CASE( check_scaling )
{
    Potential<grid2d> pot(2.0, 4, 2);

    BOOST_CHECK_EQUAL( pot.getSupport(), 2.0);

    std::vector<double> pv(16);
    std::vector<double> dv(16);
    std::vector<double> ddv(16);
    for(int i=0; i < 16; ++i)
    {
        pv[i] = i;
        dv[i] = i*i;
        ddv[i] = i;
    }

    pot.setPotential( std::move(pv) );
    std::vector<int> i {1,0};
    pot.setDerivative( i, std::move(dv) );

    std::vector<int> ii {2,0};
    pot.setDerivative( ii, std::move(ddv) );

    pot.scalePotential( 2.5 );

    auto pg = pot.getPotential();
    auto dg = pot.getDerivative( i );
    auto ddg = pot.getDerivative( ii );

    BOOST_CHECK_EQUAL( pg(0,0), 0 );
    BOOST_CHECK_EQUAL( pg(0,3), 3 * 2.5 );
    BOOST_CHECK_EQUAL( pg(1,0), 4 * 2.5 );

    BOOST_CHECK_EQUAL( dg(0,0), 0 );
    BOOST_CHECK_EQUAL( dg(0,3), 9 * 2.5 );
    BOOST_CHECK_EQUAL( dg(1,0), 16 * 2.5 );

    BOOST_CHECK_EQUAL( ddg(0,0), 0 );
    BOOST_CHECK_EQUAL( ddg(0,3), 3 * 2.5 );
    BOOST_CHECK_EQUAL( ddg(1,0), 4 * 2.5 );

    // check scaling when support changes
    pot.setSupport(1.0);
    // supp / 2 => dx * 2
    BOOST_CHECK_EQUAL( pg(0,3), 3 * 2.5 );
    BOOST_CHECK_EQUAL( dg(0,3), 9 * 5.0 );
    BOOST_CHECK_EQUAL( ddg(0,3), 3 * 10.0 );


}

/// \todo find a way to re-enable that test
/*
BOOST_AUTO_TEST_CASE( check_serialization )
{
    std::function<double(const vec<2>& x)> f = [](const vec<2>& x) { return std::exp(-100*x.getLengthSQ());};
    PGOptions opt;
    opt.maxDerivativeOrder = 2;
    auto potential = generatePotential<2>(256, f, opt);
    potential.setSupport(5.0);

    std::fstream tmp("tmp.tmp", std::ios::out | std::ios::binary);

    potential.writeToFile( tmp );
    tmp.close();
    std::fstream load("tmp.tmp", std::ios::in | std::ios::binary);

    auto pot2 = decltype(potential)::readFromFile( load );

    BOOST_CHECK_EQUAL( potential.getSupport(), pot2.getSupport() );
    BOOST_CHECK_EQUAL( potential.getSeed(), pot2.getSeed() );
    BOOST_CHECK_EQUAL( potential.getSize(), pot2.getSize() );
    BOOST_CHECK_EQUAL( potential.getPotgenVersion(), pot2.getPotgenVersion() );
    BOOST_CHECK_EQUAL( potential.getMaxOrder(), pot2.getMaxOrder() );

    typedef std::vector<int> arr;
    std::array<arr, 6> d{arr{0,0},arr{0,1},arr{1,0},arr{1,1}, arr{2,0}, arr{0,2}};

    for(auto idx : d)
    {
        auto& d1 = potential.getDerivative(idx).getContainer();
        auto& d2 = pot2.getDerivative(idx).getContainer();

        BOOST_CHECK( d1 == d2 );
    }
}

BOOST_AUTO_TEST_SUITE_END()
*/
