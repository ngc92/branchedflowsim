#ifndef RAY_DYNAMICS_HPP_INCLUDED
#define RAY_DYNAMICS_HPP_INCLUDED

#include <memory>

class GState;
class State;

class RayDynamics
{
public:
	virtual ~RayDynamics() = default;

	/// this function does the dynamics calculation.
	/// \param state current state of the system.
	/// \param deriv derivatives of state at current time.
	/// \param t current time.
	virtual void stateUpdate( const GState& state, GState& deriv, double t) const = 0;
	
	/// returns whether the dynamics include a monodromy matrix.
	virtual bool hasMonodromy() const = 0;
	
	/// returns whether the dynamics want the tracing to be performed with periodic boundary conditions.
	virtual bool hasPeriodicBoundary() const = 0;
	
	/// changes the state so that its total energy is \p energy.
	virtual void normalizeEnergy(State& state, double energy) const = 0;
	
	/// gets the states total energy.
	virtual double getEnergy( const State& state ) const = 0;
};


std::unique_ptr<RayDynamics> createRayDynamics( const std::string& system );

#endif // RAY_DYNAMICS_HPP_INCLUDED
