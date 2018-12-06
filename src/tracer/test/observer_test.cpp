#include "observer.hpp"
#include "density_observer.hpp"

#include <boost/test/unit_test.hpp>

// additional tests:
//	check master observer
//  check observer integration with tracer

BOOST_AUTO_TEST_SUITE(observer_test)

BOOST_AUTO_TEST_CASE(constructor)
{
	DensityObserver<1> obs(125);

	BOOST_CHECK_EQUAL( obs.getDensity().getContainer().size(), 125 );
	BOOST_CHECK_EQUAL( obs.getDensity().getGridSize(), 125);
	BOOST_CHECK_EQUAL( obs.getSize(), 125);

	DensityObserver<2> obs2(20);

	BOOST_CHECK_EQUAL( obs2.getDensity().getContainer().size(), 20*20 );
	BOOST_CHECK_EQUAL( obs2.getDensity().getGridSize(), 20);
	BOOST_CHECK_EQUAL( obs2.getSize(), 20);
}

// backend tests

BOOST_AUTO_TEST_CASE( add_dot )
{
	constexpr int obs_size = 10;
	// observer normalizes density to image size, which is obs_size - 1
	constexpr int img_size = obs_size - 1;
	DensityObserverBackend<1> obs_1d(obs_size);

	obs_1d.addInterpolatedDot( vec<1>(1.0), 1 );

	BOOST_CHECK_EQUAL( obs_1d.getDensity()(0), 0 );
	BOOST_CHECK_EQUAL( obs_1d.getDensity()(1), 1 * img_size );
	BOOST_CHECK_EQUAL( obs_1d.getDensity()(2), 0 );

	obs_1d.addInterpolatedDot( vec<1>(5.5), 2);
	BOOST_CHECK_EQUAL( obs_1d.getDensity()(5), 1 * img_size);
	BOOST_CHECK_EQUAL( obs_1d.getDensity()(6), 1 * img_size);

	BOOST_CHECKPOINT("2d observer dot");

	DensityObserverBackend<2> obs_2d( obs_size );
	obs_2d.addInterpolatedDot( vec<2>(1.0, 1.0), 1 );
	for(int x = 0; x < 10; ++x)
	for(int y = 0; y < 10; ++y)
		if(x != 1 || y != 1)
		BOOST_CHECK_EQUAL( obs_2d.getDensity()(x, y), 0);

	BOOST_CHECK_EQUAL( obs_2d.getDensity()(1, 1), 1 * img_size * img_size );

	obs_2d.addInterpolatedDot( vec<2>(5.5, 5.5), 4);
	BOOST_CHECK_EQUAL( obs_2d.getDensity()(5, 5), 1 * img_size * img_size);
	BOOST_CHECK_EQUAL( obs_2d.getDensity()(5, 6), 1 * img_size * img_size);
	BOOST_CHECK_EQUAL( obs_2d.getDensity()(6, 5), 1 * img_size * img_size);
	BOOST_CHECK_EQUAL( obs_2d.getDensity()(6, 6), 1 * img_size * img_size);

	BOOST_CHECKPOINT("3d observer dot");

	DensityObserverBackend<3> obs_3d( obs_size );
	obs_3d.addInterpolatedDot( vec<3>(1.0, 1.0, 1.0), 1.0 / img_size / img_size / img_size);
	for(int x = 0; x < 10; ++x)
	for(int y = 0; y < 10; ++y)
	for(int z = 0; z < 10; ++z)
		if(x != 1 || y != 1 || z != 1)
		BOOST_CHECK_EQUAL( obs_3d.getDensity()(x, y, z), 0);

	BOOST_CHECK_EQUAL( obs_3d.getDensity()(1, 1, 1), 1 );

	obs_3d.addInterpolatedDot( vec<3>(5.5, 5.5, 5.5), 8.0 / img_size / img_size / img_size);
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(5, 5, 5), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(5, 6, 5), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(6, 5, 5), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(6, 6, 5), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(5, 5, 6), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(5, 6, 6), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(6, 5, 6), 1 );
	BOOST_CHECK_EQUAL( obs_3d.getDensity()(6, 6, 6), 1 );
}

BOOST_AUTO_TEST_CASE( add_line )
{
	// check that infinitesimal line equals point
	DensityObserverBackend<2> obs_2d(10);
	obs_2d.addInterpolatedDot( vec<2>(1.0, 1.0), 1 );
	obs_2d.addInterpolatedLine( vec<2>(1.0, 1.0), vec<2>(1.0, 1.0), -1 );
	BOOST_CHECK_SMALL( obs_2d.getDensity()(1,1), 1e-7f );

	// check center of line
	obs_2d.addInterpolatedLine( vec<2>(1.0, 1.0), vec<2>(3.0, 1.0), 2 );
	BOOST_CHECK_CLOSE( obs_2d.getDensity()(2, 1), 81.0f, 1e-7f );

	// check along line
	DensityObserverBackend<2> big(64);
	// these values are taken because in the old implementation, this causes problems
	big.addInterpolatedLine( vec<2>(9.06, 1.0/3*64), vec<2>(37.7, 1.0/3*64), 1.0 );

	for(int x = 12; x < 30; ++x)
	{
		BOOST_CHECK_CLOSE( big.getDensity()(x, 22), big.getDensity()(x+1, 22), 1e-5 );
	}
}

/// \todo rework this test for new observer structure
/*
BOOST_AUTO_TEST_CASE( trajectory_test )
{
	constexpr int size = 10;
	DensityObserver<State<2>> obs( size );
	DensityObserver<State<2>> ref( size );

	std::array<vec<2>, 2> initial_pos;
	initial_pos[0] = vec<2>{1.0, 1.0} / size;
	initial_pos[1] = vec<2>{2.0, -1.0};
	obs.startTrajectory( initial_pos );
	ref.startTrajectory( initial_pos );

	BOOST_CHECK_EQUAL( initial_pos[0], obs.getLastPosition()[0] );
	BOOST_CHECK_EQUAL( initial_pos[1], obs.getLastPosition()[1] );

	initial_pos[0][0] = 4.0 / size;
	obs.addTrajectoryPoint(initial_pos, 1.0);
	ref(initial_pos, 1.0);
	BOOST_CHECK_EQUAL( initial_pos[0], obs.getLastPosition()[0] );
	BOOST_CHECK_EQUAL( initial_pos[1], obs.getLastPosition()[1] );

	// remove line
	obs.addInterpolatedLine( vec<2>{1.0, 1.0}, vec<2>(4.0, 1.0), -1);
	ref.addInterpolatedLine( vec<2>{1.0, 1.0}, vec<2>(4.0, 1.0), -1);
	for(float v : obs.getDensityData())
	{
		// scale error with size
		BOOST_CHECK_SMALL(v, 1e-7f * size * size);
	}

	initial_pos[0][1] = 4.0 / size;
	obs.addTrajectoryPoint(initial_pos, 1.5);
	ref(initial_pos, 1.5);
	BOOST_CHECK_EQUAL( initial_pos[0], obs.getLastPosition()[0] );
	BOOST_CHECK_EQUAL( initial_pos[1], obs.getLastPosition()[1] );
	BOOST_CHECK_EQUAL( initial_pos[0], ref.getLastPosition()[0] );
	BOOST_CHECK_EQUAL( initial_pos[1], ref.getLastPosition()[1] );

	// remove second line
	obs.addInterpolatedLine( vec<2>{4.0, 1.0}, vec<2>(4.0, 4.0), -0.5);
	ref.addInterpolatedLine( vec<2>{4.0, 1.0}, vec<2>(4.0, 4.0), -0.5);
	for(auto v : obs.getDensityData())
	{
		// scale error with size
		BOOST_CHECK_SMALL(v, 1e-7f * size * size);
	}

	BOOST_CHECK( obs.getDensityData() == ref.getDensityData() );
}
*/

BOOST_AUTO_TEST_SUITE_END()
