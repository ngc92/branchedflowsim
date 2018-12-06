#include "tracer.hpp"
#include <fstream>
#include "fileIO.hpp"
#include "density_observer.hpp"

#include <boost/test/unit_test.hpp>

/*! tracer tests
	createTracer potential
	createTracer function
	create with/without monodromy
	check exception if derivatives are missing
	check thread safety
*/

BOOST_AUTO_TEST_SUITE(tracer_tests)

// helper functions
/// \todo this could be done with boost fixtures
Potential<grid<std::vector<double>, 1>> makeEmpty1DPot(int size)
{
	Potential<grid<std::vector<double>, 1>> pot(1.0, size, 2);
	std::vector<double> v(size);
	grid<std::vector<double>, 1> pg(size, v);
	for(auto& v : pg)
	{
		v = 0.0;
	}
	auto copy = v;
	pot.setPotential( std::move(v) );
	v = copy;
	pot.setDerivative( std::vector<int>{1}, std::move(v) );
	return pot;
}

Potential<grid<std::vector<double>, 2>> makeEmpty2DPot(int size)
{
	Potential<grid<std::vector<double>, 2>> pot(1.0, size, 2);
	std::vector<double> v(size*size);
	grid<std::vector<double>, 1> pg(size, v);
	for(auto& v : pg)
	{
		v = 0.0;
	}
	auto copy = v;
	pot.setPotential( std::move(v) );
	v = copy;
	pot.setDerivative( std::vector<int>{1,0}, std::move(v) );
	v = copy;
	pot.setDerivative( std::vector<int>{0,1}, std::move(v) );
	return pot;
}

BOOST_AUTO_TEST_CASE(tracer_ctor)
{
	auto pot = makeEmpty1DPot(128);

	auto tracer = createTracer( pot, false, false );

	// check energy functor
	auto ef = tracer->getEnergy();
	State tp(1);
	tp.editPos()[0] = 0.5;
	tp.editVel()[0] = 1;
	BOOST_CHECK_EQUAL( ef.getPotentialEnergy(tp), 0.0 );
	BOOST_CHECK_EQUAL( ef.getKineticEnergy(tp), 0.5 );
	BOOST_CHECK_EQUAL( ef.getEnergy(tp), 0.5 );

	BOOST_CHECK_EQUAL( tracer->getMaximumEnergyError(), 0.0 );
	BOOST_CHECK_EQUAL( tracer->getDimension(), 1);
	BOOST_CHECK_EQUAL( tracer->getPotentialSize(), 128);
	// no monodromy
	BOOST_CHECK_EQUAL( tracer->getTraceMonodromy(), false);
	BOOST_CHECK_EQUAL( tracer->getPeriodicBoundaries(), false);
	BOOST_CHECK_EQUAL( *tracer->getSecondOrderDerivatives(), (void*)0);

	/// \todo check first order derivative; thread count?
}


BOOST_AUTO_TEST_CASE(empty_pot_tests)
{
	/// \todo 1d trace test currently not available, because no IC can be generated

	double REQ_ACC = 0.1; // required accuracy, currently 0.1%, should be lower

	auto pot2d = makeEmpty2DPot(64);
	auto tr_2d = createTracer( pot2d, false, false );
	auto density_ob_2d = std::make_shared<DensityObserver<2>>( 64 );
	auto ig2d = createInitialConditionGenerator( 2, "planar" );
	ig2d->setParticleCount(640);
	tr_2d->addObserver( density_ob_2d );
	tr_2d->trace(ig2d);
	auto g= density_ob_2d->getDensity();
	for(int i=5; i < 30; ++i)
	for(int j=5; j < 60; ++j)
	{
		// boost require so test fails after first error -> does not spam error output
		/// \todo for some reaseon, this is 0.5 instead of 1
		BOOST_REQUIRE_CLOSE( g(i, j), 1.0, REQ_ACC );
	}
}

BOOST_AUTO_TEST_CASE( no_derivative_test )
{
	constexpr int size = 10;
	Potential<grid<std::vector<double>, 1>> pot(1.0, size, 2);
	std::vector<double> v(size);
	grid<std::vector<double>, 1> pg(size, v);
	for(auto& v : pg)
	{
		v = 0.0;
	}
	pot.setPotential( std::move(v) );

	BOOST_CHECK_THROW(createTracer( pot, false, false ), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( multithread_test )
{
	auto pot2d = makeEmpty2DPot(64);
	auto tr_single = createTracer( pot2d, false, false );
	auto density_ob_single = std::make_shared<DensityObserver<2>>( 64 );
	auto ig2d_single = createInitialConditionGenerator( 2, "planar" );
	ig2d_single->setParticleCount(64);
	tr_single->setMaxThreads(1);
	tr_single->trace(ig2d_single);

	auto tr_multi = createTracer( pot2d, false, false );
	auto density_ob_multi = std::make_shared<DensityObserver<2>>( 64 );
	auto ig2d_multi = createInitialConditionGenerator( 2, "planar" );
	ig2d_multi->setParticleCount(64);
	tr_multi->trace(ig2d_multi);

	auto g1 = density_ob_single->getDensity();
	auto g2 = density_ob_multi->getDensity();
	for(unsigned i=0; i < g1.getGridSize(); ++i)
	for(unsigned j=0; j < g1.getGridSize(); ++j)
	{
		// boost require so test fails after first error -> does not spam error output
		BOOST_REQUIRE_EQUAL( g1(i, j), g2(i,j));
	}
}

BOOST_AUTO_TEST_SUITE_END()
