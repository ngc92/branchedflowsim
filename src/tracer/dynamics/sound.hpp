#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED


#include "ray_dynamics.hpp"
#include "vector.hpp"
#include "dynamic_grid.hpp"
#include <vector>

class Potential;

class Sound : public RayDynamics
{
public:
	Sound(const Potential& pot, bool periodic, bool monodromy, double speed_of_sound = 1);
	
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
	
	double mSpeedOfSound;
	
	// grids of first derivatives
	std::vector<default_grid> mVelocities;
	std::vector<default_grid> mVelDerivatives;
};

#endif // SOUND_HPP_INCLUDED
