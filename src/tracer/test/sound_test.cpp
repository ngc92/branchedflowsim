#include "tracer.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include "dynamics/sound.hpp"
#include "observers/observer.hpp"
#define BOOST_TEST_MODULE sound_test
#include <boost/test/unit_test.hpp>

using zero_vec = boost::numeric::ublas::zero_vector<double>;

// make an observer that checks that analytic and numeric solutins coincide

// x(t) = (1+y0 - sqrt(1+t^2)/2)t + sinh^-1(t)/2
// y(t) = 1 - sqrt(1+t^2) + y0

class CheckSoundObserver final: public ThreadLocalObserver
{
public:
	/// create an observer and specify the time interval for saving the particle's position
	CheckSoundObserver( );

	/// d'tor
	~CheckSoundObserver() = default;

	// standard observer functions
	// for documentation look at observer.hpp
	bool watch( const State& state, double t ) override;
	void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
	void save( std::ostream& ) override {}
	std::shared_ptr<ThreadLocalObserver> clone() const override;
	void combine(ThreadLocalObserver&) override {}
	
	double x0;
	double y0;
};



CheckSoundObserver::CheckSoundObserver() : ThreadLocalObserver("tlo") { }


void CheckSoundObserver::startTrajectory( const InitialCondition& start, std::size_t )
{
    x0 = start.getState().getPosition()[0];
    y0 = start.getState().getPosition()[1];
}

bool CheckSoundObserver::watch( const State& state, double t )
{
    double ax = (1 + y0 - std::sqrt(1+t*t)/2)*t + std::asinh(t)/2 + x0;
    double ay = 1 - std::sqrt( 1 + t*t ) + y0;
    double nx = state.getPosition()[0];
    double ny = state.getPosition()[1];
	BOOST_CHECK_CLOSE( ax, nx, 1e-9 );
	BOOST_CHECK_CLOSE( ay, ny, 1e-9 );
	return true;
}


std::shared_ptr<ThreadLocalObserver> CheckSoundObserver::clone() const
{
	return std::make_shared<CheckSoundObserver>( );
}


BOOST_AUTO_TEST_SUITE(sound_trace_tests)

BOOST_AUTO_TEST_CASE(gradient)
{
    Potential potential(2, 1, 256);
    default_grid g(2, 256);
    g.setAccessMode(TransformationType::PERIODIC);
    for(auto& data : g)
        data = 0;
    potential.setDerivative(std::vector<int>{0,0}, g.clone(), "velocity1");
    potential.setDerivative(std::vector<int>{1,0}, g.clone(), "velocity1");
    potential.setDerivative(std::vector<int>{0,1}, g.clone(), "velocity1");
    potential.setDerivative(std::vector<int>{1,0}, g.clone(), "velocity0");
    for(auto& data : g)
        data = 1;
    potential.setDerivative(std::vector<int>{0,1}, g.clone(), "velocity0");
    for(auto ind = g.getIndex(); ind.valid(); ++ind)
    {
        g(ind) = ind[1] / 256.;
    }
    potential.setDerivative(std::vector<int>{0,0}, g.clone(), "velocity0");
    
	std::unique_ptr<RayDynamics> dynamics(new Sound(potential, false, false));
	auto tracer = std::make_shared<Tracer>( potential, std::move(dynamics));
	tracer->setMaxThreads(1);
	tracer->addObserver( CheckSoundObserver().clone() );
	
	auto generator = createInitialConditionGenerator( 2, std::vector<std::string>{"planar"} );
	init_cond::InitialConditionConfiguration config;
	config.setParticleCount(1000).setEnergyNormalization(true).setSupport(potential.getSupport())
            .setOffset(zero_vec(2));
	tracer->trace( generator, config );
	std::cout << "FINNISHED\n";
}

BOOST_AUTO_TEST_SUITE_END()
