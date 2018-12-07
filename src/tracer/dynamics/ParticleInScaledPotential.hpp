#ifndef PARTICLE_IN_SCALED_POTENTIAL_DYNAMICS_HPP_INCLUDED
#define PARTICLE_IN_SCALED_POTENTIAL_DYNAMICS_HPP_INCLUDED

#include "ray_dynamics.hpp"
#include "vector.hpp"
#include "dynamic_grid.hpp"
#include <vector>

class Potential;

class ParticleInScaledPotentialDynamics : public RayDynamics
{
public:
	ParticleInScaledPotentialDynamics(const Potential& pot, bool periodic, bool monodromy, float scale);
	
	void stateUpdate( const GState& state , GState& deriv , double) const override;
	
	bool hasMonodromy() const override;
	bool hasPeriodicBoundary() const override;
	void normalizeEnergy(State& state, double energy) const override;
	double getEnergy( const State& state ) const override;
	
private:
	std::size_t mDimension;
	bool mPeriodicBoundaries;
	bool mTraceMonodromy;
	float mPotScaleFactor;
	gen_vect mScalingFactor;
	std::vector<unsigned int> mGridSize;
	
	// grids of first derivatives
	default_grid mPotential;
	std::vector<default_grid> mFirstDerGrid;
	std::vector<default_grid> mSecondDerGrid;
};

#endif // PARTICLE_IN_SCALED_POTENTIAL_DYNAMICS_HPP_INCLUDED
