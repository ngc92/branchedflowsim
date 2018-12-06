#ifndef PARTICLEINPOTENTIALDYNAMICS_HPP_INCLUDED
#define PARTICLEINPOTENTIALDYNAMICS_HPP_INCLUDED

#include "ray_dynamics.hpp"
#include "vector.hpp"
#include "dynamic_grid.hpp"
#include <vector>

class Potential;

class ParticleInPotentialDynamics : public RayDynamics
{
public:
	ParticleInPotentialDynamics(const Potential& pot, bool periodic, bool monodromy);
	
	void stateUpdate( const GState& state , GState& deriv , double) const override;
	
	bool hasMonodromy() const override;
	bool hasPeriodicBoundary() const override;
	void normalizeEnergy(State& state, double energy) const override;
	double getEnergy( const State& state ) const override;
	
private:
	std::size_t mDimension;
	bool mPeriodicBoundaries;
	bool mTraceMonodromy;
	gen_vect mScalingFactor;
	std::vector<unsigned int> mGridSize;
	
	// grids of the potential.
	/// \todo does it make sense to save it like this?
	default_grid mPotential;
	std::vector<default_grid> mFirstDerGrid;
	std::vector<default_grid> mSecondDerGrid;
};

#endif // PARTICLEINPOTENTIALDYNAMICS_HPP_INCLUDED
