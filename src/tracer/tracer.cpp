#include "tracer.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include "ode_state.hpp"
#include "dynamics/ray_dynamics.hpp"
#include <future>
#include <chrono>

#include <boost/numeric/odeint/integrate/integrate_const.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54.hpp>
#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>

#include <boost/range/combine.hpp>
#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/numeric/odeint/stepper/euler.hpp>
#include "observers/energy_error_observer.hpp"
#include <utility>

Tracer::Tracer( const Potential& pot, std::shared_ptr<RayDynamics> dynamics ) :
	mDimension( pot.getDimension() ),
	mSupport( pot.getSupport() ),
	mExtents( pot.getExtents() ),
	mDynamics( std::move(dynamics) ),
    mMasterObserver( pot.getDimension(), mDynamics ),
	mEnergyErrorObs(std::make_shared<EnergyErrorObserver>())
{
	using namespace boost::range;
	using namespace boost::adaptors;

	// we choose initial step according to the potential length
	// scaling = size / support, i.e. dt = support/size

	// cannot use a lambda as older boost looks for member result_type
	struct ratio
	{
		typedef double result_type;
		double operator()(const boost::tuple<double, std::size_t>& t) const { return boost::get<0>(t) / boost::get<1>(t); }
	};
	
	auto ratios = combine(pot.getSupport(), pot.getExtents()) | transformed(ratio());
	mInitialDeltaT = *min_element(ratios);

	mMasterObserver.addObserverObject(mEnergyErrorObs);
}

Tracer::~Tracer() = default;

void Tracer::setErrorBounds( double abs_err, double rel_err )
{
	mAbsErrorBound = abs_err;
	mRelErrorBound = rel_err;
}

void Tracer::addObserver( obs_type observer )
{
	mMasterObserver.addObserverObject(std::move(observer));
}

TraceResult Tracer::trace(InitCondGenPtr& incoming_wave, InitialConditionConfiguration config)
{
	// fix coordinate transformation for initial condition
	auto support = mSupport;
	gen_vect offset(mDimension);
	for(unsigned i = 0; i < mDimension; ++i)
	{
		offset[i] = mSupport[i] / mExtents[i];
		support[i] -= 2*offset[i];
	}
    config.setDynamics( mDynamics ).setSupport( support ).setOffset(offset);
	incoming_wave->init( config );

	// set up master observer
	mMasterObserver.setPeriodicBoundaries( mDynamics->hasPeriodicBoundary() );
	mMasterObserver.startTracing( );

	unsigned int threadcount = std::min(mMaxThreads, (std::size_t)std::thread::hardware_concurrency());
	#ifndef NDEBUG
	std::cout << "distribute computation to " << threadcount << " threads\n";
	#endif

	std::vector<std::future<void>> threads;
	auto tf = [this](InitCondGenPtr w, bool is_printer)
	{
		return this->traceThreadFunction( std::move(w), is_printer );
	};

	for(unsigned i = 0; i < threadcount; ++i)
	{
		// start threads, only thread zero prints progress
		threads.push_back( std::async ( std::launch::async, tf, incoming_wave, i == 0) );
	}

	for( auto& f : threads)
		f.get();

	mMasterObserver.finishTracing();

	return TraceResult{mEnergyErrorObs->getMaximumError(), mEnergyErrorObs->getMeanError(), getTracedParticleCount()};
}

void Tracer::traceThreadFunction( InitCondGenPtr incoming_wave, bool printer )
{
	// check that dynamcis are set
	if(!mDynamics)
		THROW_EXCEPTION( std::runtime_error, "Cannot perform tracing when no dynamics are set!" );

	// setup types for boost odeint solver
	using namespace boost::numeric::odeint;

	if(mIntegrator == Integrator::RUNGE_KUTTA_CASH_KARP_54_ADAPTIVE) {
		typedef runge_kutta_cash_karp54<GState> error_stepper_type;
		typedef controlled_runge_kutta<error_stepper_type> controlled_stepper_type;
		typedef default_error_checker<double, range_algebra, default_operations> error_checker_type;

		// setup the integrator once!
		controlled_stepper_type stepper(error_checker_type(mAbsErrorBound, mRelErrorBound));

		traceThreadFunction_imp(stepper, incoming_wave, printer);
	} else if (mIntegrator == Integrator::EULER_CONST) {
		typedef euler<GState> stepper_type;

		// setup the integrator once!
		stepper_type stepper{};

		traceThreadFunction_imp(stepper, incoming_wave, printer);
	}
}

template<class T>
void Tracer::traceThreadFunction_imp( T&& stepper, InitCondGenPtr incoming_wave, bool printer )
{
	MasterObserver thread_observer( mMasterObserver.clone() );
	InitialCondition incoming = incoming_wave->next();

	GState p(mDimension, mDynamics->hasMonodromy());
	auto last_time = std::chrono::steady_clock::now();

	for(std::size_t i = 0; incoming; ++i, ++incoming)
	{
		
		if(printer && (std::chrono::steady_clock::now() - last_time) > std::chrono::seconds(10))
		{
		    last_time = std::chrono::steady_clock::now();
			std::cout << "integrate " << thread_observer.getTracedParticleCount() <<  " \n";
		}
		
		// generate initial condition and set up state
		p.position() = incoming.getState().getPosition();
		p.velocity() = incoming.getState().getVelocity();
		if( mDynamics->hasMonodromy() )
			p.init_monodromy();

		// notify the observer
		thread_observer.startTrajectory( incoming );

		try
		{
			/// \todo this can return... do we want to do sth with the return value?
			boost::numeric::odeint::integrate_const(
					std::ref(stepper),
					[this](const GState& s, GState& d, double t) { mDynamics->stateUpdate(s, d, t); },
					p,
					0.0, 				// start time
					mEndTime, 			// end time
					mInitialDeltaT, 	// initial time step
					std::ref(thread_observer)
			);
		} catch(int& i) {};

		thread_observer.finishTrajectory( incoming );
	}
}


void Tracer::setTimeStep(double dt)
{
	mInitialDeltaT = dt;
}


std::size_t Tracer::getDimension() const
{
	return mDimension;
}

void Tracer::setMaxThreads( std::size_t threads )
{
	if( threads == 0 )
		threads = 1;
	mMaxThreads = threads;
}

std::size_t Tracer::getMaxThreads( ) const
{
	return mMaxThreads;
}

void Tracer::setIntegrator(Integrator integrator)
{
    mIntegrator = integrator;
}

const std::vector<std::shared_ptr<Observer>>& Tracer::getObservers() const
{
	return mMasterObserver.getObservers();
}

void Tracer::setEndTime( double et )
{
	mEndTime = et;
}

double Tracer::getEndTime() const
{
	return mEndTime;
}

std::size_t Tracer::getTracedParticleCount() const
{
	return mMasterObserver.getTracedParticleCount();
}
