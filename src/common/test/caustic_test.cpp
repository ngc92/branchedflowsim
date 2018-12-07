#include "caustic.hpp"
#include "test_helpers.hpp"
#include <fstream>
#include <boost/test/unit_test.hpp>

// tests for the caustic class.

BOOST_AUTO_TEST_SUITE(caustic)

BOOST_AUTO_TEST_CASE(constructor)
{
    // init vectors
    auto p = rand_vec(3);
    auto o = rand_vec(3);
    auto v = rand_vec(3);
    auto ov = rand_vec(3);

    const int T_ID = rand() % 20;
    const double TIME = rand01();
    unsigned char INDEX = rand() % 10;

    // data ctor
    Caustic c = Caustic(T_ID, p, o, v, ov, TIME, INDEX);

    // check that all data is set correctly
    BOOST_CHECK_EQUAL( c.getTrajectoryID(), T_ID );
    BOOST_CHECK_EQUAL( c.getPosition(), p );
    BOOST_CHECK_EQUAL( c.getVelocityAtCaustic(), v );
    BOOST_CHECK_EQUAL( c.getOrigin(), o );
    BOOST_CHECK_EQUAL( c.getOriginalVelocity(), ov );
    BOOST_CHECK_EQUAL( c.getTime(), TIME );
    BOOST_CHECK_EQUAL( c.getIndex(), INDEX );

    // dimension ctor
    Caustic c2 = Caustic(2);
    BOOST_CHECK_EQUAL( c2.getPosition().size(), 2);
    BOOST_CHECK_EQUAL( c2.getOrigin().size(), 2);
    BOOST_CHECK_EQUAL( c2.getVelocityAtCaustic().size(), 2);
    BOOST_CHECK_EQUAL( c2.getOriginalVelocity().size(), 2);


    // data integrity
    #ifndef NDEBUG
    o = rand_vec(2);
    BOOST_CHECK_THROW( Caustic(T_ID, p, o, v, ov, TIME, INDEX), std::logic_error);
    BOOST_CHECK_THROW( Caustic(T_ID, p, v, o, ov, TIME, INDEX), std::logic_error);
    BOOST_CHECK_THROW( Caustic(T_ID, p, ov, v, o, TIME, INDEX), std::logic_error);
    #endif
}

BOOST_AUTO_TEST_CASE( copy_move )
{
    // initial value
    Caustic c = Caustic(rand() % 10, rand_vec(3), rand_vec(3), rand_vec(3), rand_vec(3), rand01(), rand() % 10);

    // copy
    Caustic cpy = c;

    // check that all data is set correctly
    BOOST_CHECK_EQUAL( c.getTrajectoryID(), cpy.getTrajectoryID() );
    BOOST_CHECK_EQUAL( c.getPosition(), cpy.getPosition() );
    BOOST_CHECK_EQUAL( c.getVelocityAtCaustic(), cpy.getVelocityAtCaustic() );
    BOOST_CHECK_EQUAL( c.getOrigin(), cpy.getOrigin() );
    BOOST_CHECK_EQUAL( c.getOriginalVelocity(), cpy.getOriginalVelocity() );
    BOOST_CHECK_EQUAL( c.getTime(), cpy.getTime() );
    BOOST_CHECK_EQUAL( c.getIndex(), cpy.getIndex() );

    // move
    Caustic mv(std::move(cpy));

    // check that all data is set correctly
    BOOST_CHECK_EQUAL( c.getTrajectoryID(), mv.getTrajectoryID() );
    BOOST_CHECK_EQUAL( c.getPosition(), mv.getPosition() );
    BOOST_CHECK_EQUAL( c.getVelocityAtCaustic(), mv.getVelocityAtCaustic() );
    BOOST_CHECK_EQUAL( c.getOrigin(), mv.getOrigin() );
    BOOST_CHECK_EQUAL( c.getOriginalVelocity(), mv.getOriginalVelocity() );
    BOOST_CHECK_EQUAL( c.getTime(), mv.getTime() );
    BOOST_CHECK_EQUAL( c.getIndex(), mv.getIndex() );
}

BOOST_AUTO_TEST_CASE(io)
{
    // make sure to integrate control characters to spot problems from text mode
    unsigned char index_data[8] = {'0', '0', '\n', '\r', '\r', '\n','\r', '\n'};
    uint64_t index = *index_data;

    Caustic c = Caustic(index, rand_vec(3), rand_vec(3), rand_vec(3), rand_vec(3), rand01(), '\n');

    std::fstream out("tmp.txt", std::fstream::out | std::fstream::binary);
    c.write(out);
    out.close();

    Caustic ld(3);
    std::fstream in("tmp.txt", std::fstream::in | std::fstream::binary);
    ld.read( in );
    BOOST_CHECK(in.good());

    // check that all data is set correctly
    BOOST_CHECK_EQUAL( c.getTrajectoryID(), ld.getTrajectoryID() );
    BOOST_CHECK_EQUAL( c.getPosition(), ld.getPosition() );
    BOOST_CHECK_EQUAL( c.getOrigin(), ld.getOrigin() );
    BOOST_CHECK_EQUAL( c.getVelocityAtCaustic(), ld.getVelocityAtCaustic() );
    BOOST_CHECK_EQUAL( c.getOriginalVelocity(), ld.getOriginalVelocity() );
    BOOST_CHECK_EQUAL( c.getTime(), ld.getTime() );
    BOOST_CHECK_EQUAL( c.getIndex(), ld.getIndex() );
}

BOOST_AUTO_TEST_SUITE_END()
