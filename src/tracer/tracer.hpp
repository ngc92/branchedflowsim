#ifndef TRACER_HPP_INCLUDED
#define TRACER_HPP_INCLUDED

#include "initial_conditions_fwd.hpp"
#include "potential.hpp"
#include "observers/master_observer.hpp"
#include <vector>
#include <boost/noncopyable.hpp>
#include "initial_conditions/initial_conditions.hpp"

class RayDynamics;

enum class Integrator : int
{
    RUNGE_KUTTA_CASH_KARP_54_ADAPTIVE,
    EULER_CONST
};

struct TraceResult
{
	double mMaximumEnergyDeviation;
	double mMeanEnergyDeviation;
	std::size_t mParticleCount;
};

/// \todo some documentation, look if we can reduce the number of member variables (trace monodromy, periodic etc)
class Tracer final
{
	// typedef
	using obs_type = std::shared_ptr<Observer>;

public:
	/// \todo currently, uses pot just to get the geometry data
	Tracer( const Potential& pot, std::shared_ptr<RayDynamics> dynamics );
	// c'tor, d'tor
	~Tracer();

	/// set the error bounds for the adaptive runge-kutta algorithm
	void setErrorBounds(double abs_err, double rel_err);
	/// adds an observer to the master observer
	void addObserver( obs_type observer );
	/// set the integrator
	void setIntegrator(Integrator integrator);

	/// common model for a tracing function, specialised behaviour is inserted via virtual functions
	TraceResult trace(InitCondGenPtr& initial_condition, InitialConditionConfiguration config);

	// these simple getters/setters are not defined inline because they are not expected to be called
	// frequently
	std::size_t getDimension() const;

	void setEndTime( double et );
	double getEndTime() const;

	/// Sets the integrator time step.
	void setTimeStep(double dt);

	/// sets the maximum number of threads used for integration. if \p threads == 0, use 1 thread.
	void setMaxThreads( std::size_t threads );
	std::size_t getMaxThreads( ) const;

	/// \todo observer access
	/// returns the actual number of particles traced, which might differ from the amount
	///	requested when setting up the initial conditions.
	std::size_t getTracedParticleCount() const;

	// observer vector
	const std::vector<std::shared_ptr<Observer>>& getObservers() const;

private:
	// tracing config
	// error bounds members
	double mRelErrorBound = 1e-6;
	double mAbsErrorBound = 1e-6;
    Integrator mIntegrator = Integrator::RUNGE_KUTTA_CASH_KARP_54_ADAPTIVE;
	double mInitialDeltaT;
	double mEndTime = 1.0;
	std::size_t mMaxThreads = -1;	// -1 = un

	// potential data
	std::size_t mDimension;
    std::vector<double> mSupport;
    std::vector<std::size_t> mExtents;
	
	// dynamical system spec
	std::shared_ptr<RayDynamics> mDynamics;

	void traceThreadFunction(InitCondGenPtr incoming_wave, bool printer);

	template<class StepperType>
	void traceThreadFunction_imp(StepperType&& stepper, InitCondGenPtr incoming_wave, bool printer);

	MasterObserver mMasterObserver;

	std::shared_ptr<EnergyErrorObserver> mEnergyErrorObs;
};


#endif // TRACER_HPP_INCLUDED
