#include "interpolation.hpp"
#include "dynamic_grid.hpp"
#include "multiindex.hpp"
#include "test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

// reference implementation for 2d bilinear interpolation
template<class Grid>
double bilinearInterpolate( const Grid& grid, double x, double y )
{
    typedef std::vector<int> vc;
    int x0 = std::floor(x);
    int x1 = std::ceil(x);
    int y0 = std::floor(y);
    int y1 = std::ceil(y);
    double f00 = grid( vc{x0, y0});
    double f10 = grid( vc{x1, y0});
    double f01 = grid( vc{x0, y1});
    double f11 = grid( vc{x1, y1});

    return f00 + (f10-f00) * (x - x0) + (f01 - f00) * (y - y0) + (f00+f11-f01-f10) * (x-x0) * (y - y0);
}

// helper function for generating vector indices
gen_vect vec2( double a, double b )
{
    gen_vect v(2);
    v[0] = a;
    v[1] = b;
    return v;
}

gen_vect vec3( double a, double b, double c )
{
    gen_vect v(3);
    v[0] = a;
    v[1] = b;
    v[2] = c;
    return v;
}

BOOST_AUTO_TEST_SUITE(linear_interpolation_test)

BOOST_AUTO_TEST_CASE( bilinearInterpolate_test )
{
    default_grid vg( 2, 2, TransformationType::PERIODIC );
    vg[0] = 0;
    vg[1] = 1;
    vg[2] = 1;
    vg[3] = std::sqrt(2);
    BOOST_CHECK_EQUAL( (bilinearInterpolate(vg, 0.5, 0.0)), 0.5 );
    BOOST_CHECK_EQUAL( (bilinearInterpolate(vg, 0.0, 0.5)), 0.5 );
    BOOST_CHECK_EQUAL( (bilinearInterpolate(vg, 1.0, 1.0)), std::sqrt(2) );
    BOOST_CHECK_EQUAL( (bilinearInterpolate(vg, 0.5, 0.5)), (2+std::sqrt(2))*0.25 );
}

BOOST_AUTO_TEST_CASE( test_nd_method )
{
    // compare results of general n-d method with hand-coded 2d
    // position in 0-1
    // use periodic indexing for safety
    default_grid vg(2, 2, TransformationType::PERIODIC);
    vg[0] = rand_value();
    vg[1] = rand_value();
    vg[2] = rand_value();
    vg[3] = rand_value();

    // check the corners
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec2(0.0,0.0)), vg[0] );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec2(0.0,1.0)), vg[1] );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec2(1.0,0.0)), vg[2] );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec2(1.0,1.0)), vg[3] );

    // check edges
    BOOST_CHECK_CLOSE( linearInterpolate(vg, vec2(0.0,0.5)), 0.5*(vg[0]+vg[1]), 1e-12 );
    BOOST_CHECK_CLOSE( linearInterpolate(vg, vec2(0.5,0.0)), 0.5*(vg[0]+vg[2]), 1e-12 );
    BOOST_CHECK_CLOSE( linearInterpolate(vg, vec2(1.0,0.5)), 0.5*(vg[2]+vg[3]), 1e-12 );
    BOOST_CHECK_CLOSE( linearInterpolate(vg, vec2(0.5,1.0)), 0.5*(vg[1]+vg[3]), 1e-12 );

    for(int i = 0; i < 10; ++i)
    {
        double x = rand01();
        double y = rand01();
        BOOST_CHECK_CLOSE(bilinearInterpolate(vg, x, y), linearInterpolate(vg, vec2(x, y)), 1e-12);
    }
}


BOOST_AUTO_TEST_CASE( test_nd_method_3d )
{
    typedef std::vector<int> id;
    // compare results of general n-d method with hand-coded 2d
    // position in 0-1
    default_grid vg(3, 3, TransformationType::PERIODIC);
    for(int i = 0; i < 27; ++i)
        vg[i] = rand_value();

    // check the corners
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,0.0,0.0)), vg(id{0,0,0}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,0.0,1.0)), vg(id{0,0,1}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,1.0,0.0)), vg(id{0,1,0}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,1.0,1.0)), vg(id{0,1,1}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,0.0,0.0)), vg(id{1,0,0}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,0.0,1.0)), vg(id{1,0,1}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,1.0,0.0)), vg(id{1,1,0}) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,1.0,1.0)), vg(id{1,1,1}) );

    // check edges
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,0.0,0.5)), 0.5*(vg(id{0,0,0})+vg(id{0,0,1})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,0.5,0.0)), 0.5*(vg(id{0,0,0})+vg(id{0,1,0})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,1.0,0.5)), 0.5*(vg(id{0,1,0})+vg(id{0,1,1})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.0,0.5,1.0)), 0.5*(vg(id{0,0,1})+vg(id{0,1,1})) );

    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,0.0,0.5)), 0.5*(vg(id{1,0,0})+vg(id{1,0,1})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,0.5,0.0)), 0.5*(vg(id{1,0,0})+vg(id{1,1,0})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,1.0,0.5)), 0.5*(vg(id{1,1,0})+vg(id{1,1,1})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(1.0,0.5,1.0)), 0.5*(vg(id{1,0,1})+vg(id{1,1,1})) );

    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.5,0.0,0.0)), 0.5*(vg(id{0,0,0})+vg(id{1,0,0})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.5,0.0,1.0)), 0.5*(vg(id{0,0,1})+vg(id{1,0,1})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.5,1.0,0.0)), 0.5*(vg(id{0,1,0})+vg(id{1,1,0})) );
    BOOST_CHECK_EQUAL( linearInterpolate(vg, vec3(0.5,1.0,1.0)), 0.5*(vg(id{0,1,1})+vg(id{1,1,1})) );
}

// test generic linear function
BOOST_AUTO_TEST_CASE( test_method_3d_fn )
{
    auto f = [](const gen_vect& p){ return 3*p[0] + 7*p[1] - 2*p[2]; };

    default_grid vg(3, 3, TransformationType::PERIODIC);
    auto index = vg.getIndex();
    for(; index.valid(); ++index)
    {
        vg(index) = f( vec3((double)index[0], (double)index[1], (double)index[2]) );
    }

    for(int i = 0; i < 100; ++i)
    {
        gen_vect pos(3);
        pos[0] = rand01();
        pos[1] = rand01();
        pos[2] = rand01();
        BOOST_CHECK_CLOSE( f(pos), linearInterpolate(vg, pos), 1e-4 );
    }

}

BOOST_AUTO_TEST_SUITE_END()

// interpolated drawing
BOOST_AUTO_TEST_SUITE( linear_interpolate_draw_test )

BOOST_AUTO_TEST_CASE( distrib_1d )
{
    DynamicGrid<float> vg(1, 3, TransformationType::PERIODIC);
    /// \todo maybe, a fill method would be nice.
    for(auto& val : vg )
        val = 0;

    gen_vect pos(1);
    pos[0] = 1;

    float w = rand_value();

    std::array<int, 1> index = {1};

    // draw dot exactly at a grid point, so all goes there
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w, 1e-7 );
    vg(index) = 0; // reset

    // now draw at 1/3 point
    w = rand_value();
    pos[0] = 1/3.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/3, 1e-7 );
}

BOOST_AUTO_TEST_CASE( distrib_2d )
{
    DynamicGrid<float> vg(2, 3, TransformationType::PERIODIC);
    /// \todo maybe, a fill method would be nice.
    for(auto& val : vg ) val = 0;

    gen_vect pos(2);
    pos[0] = 1;
    pos[1] = 1;

    float w = rand_value();

    std::array<int, 2> index = {1, 1};
    // draw at 1/3 point
    w = 1 + rand_value();
    pos[0] = 1/3.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/3, 1e-6 );

    for(auto& val : vg ) val = 0;
    pos[1] = 1/3.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/9, 1e-4 );

    float sum = 0;
    for(auto& val : vg) sum += val;
    BOOST_CHECK_CLOSE( sum, w, 1e-6 );
}

BOOST_AUTO_TEST_CASE( distrib_3d )
{
    DynamicGrid<float> vg(3, 3, TransformationType::PERIODIC);
    /// \todo maybe, a fill method would be nice.
    for(auto& val : vg ) val = 0;

    gen_vect pos(3);
    pos[0] = 1;
    pos[1] = 1;
    pos[2] = 1;

    float w = rand_value();

    std::array<int, 3> index = {1, 1, 1};
    // draw at 1/3 point
    w = 1 + rand_value();
    pos[0] = 1/3.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/3, 1e-6 );

    for(auto& val : vg ) val = 0;
    pos[1] = 1/3.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/9, 1e-4 );

    double sum = 0;
    for(auto& val : vg) sum += val;
    BOOST_CHECK_CLOSE( sum, w, 1e-6 );

    for(auto& val : vg ) val = 0;
    pos[2] = 1/4.f;
    drawInterpolatedDot(vg, pos, w);
    BOOST_CHECK_CLOSE( vg(index), w/9/4, 1e-4 );

    sum = 0;
    for(auto& val : vg) sum += val;
    BOOST_CHECK_CLOSE( sum, w, 1e-6 );
}

BOOST_AUTO_TEST_SUITE_END()
