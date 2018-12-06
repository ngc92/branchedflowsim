#include "sound.hpp"
#include "ode_state.hpp"
#include "state.hpp"
#include <boost/numeric/ublas/io.hpp>
#include "potential.hpp"
#include "interpolation.hpp"
#include "monodromy.hpp"

Sound::Sound(const Potential& pot, bool periodic, bool monodromy, double speed_of_sound):
	mDimension( pot.getDimension() ),
	mPeriodicBoundaries( periodic ),
	mTraceMonodromy( monodromy ),
	mScalingFactor( pot.getDimension() ),
	mGridSize( pot.getDimension() ),
	mSpeedOfSound( speed_of_sound ),
	mVelocities( pot.getDimension() ),
	mVelDerivatives( pot.getDimension() * pot.getDimension() )
{
	assert(mTraceMonodromy == false);
	// calculate scale factor
	for(unsigned i = 0; i < mDimension; ++i)
	{
		mScalingFactor[i] = pot.getExtents()[i] / pot.getSupport()[i];
		mGridSize[i] = pot.getExtents()[i];
	}
	
	// set up grids
	for(unsigned i = 0; i < mDimension; ++i)
	{
		mVelocities[i] = pot.getPotential("velocity"+boost::lexical_cast<std::string>(i)).shallow_copy();
	}
	
	// the current setup requires us to set the grid as periodic here, independently of
	// whether we actually do periodic tracing.
	for(auto& deriv : mVelocities)
		deriv.setAccessMode(TransformationType::PERIODIC);
	
	// setup derivatives of v
	for(unsigned i = 0; i < mDimension; ++i)
	{
		std::string base_quantity = "velocity"+boost::lexical_cast<std::string>(i);
		for(unsigned j = 0; j < mDimension; ++j)
		{
			mVelDerivatives[mDimension*i + j] = pot.getDerivative(makeIndexVector(mDimension, {j}), base_quantity).shallow_copy();
		}
	}
}

void Sound::stateUpdate( const GState& state , GState& deriv, double) const 
{
	assert( state.dimension() <= 3 );
	assert( state.dimension() == mDimension );
	std::array<double, 3> p;
	for(std::size_t i = 0; i < mDimension; ++i)
	{
		p[i] = state.position()[i] * mScalingFactor[i];
		// we need a safety margin of 1 here, so that interpolation can never
		// access non-existing grid cells.
		if(!mPeriodicBoundaries && (p[i] < 1 || p[i] > mGridSize[i] - 2))
			throw(1);
	}
	
	const double C = mSpeedOfSound;
	
	// dR/di = B / |B| + v/c , di = c dt
	// dR/dt = cB / |B| + v
	gen_vect B(mDimension);
	state.velocity().assignTo(B);
	double b = 0;
	for(std::size_t i = 0; i < mDimension; ++i)
	{
		b += B[i] * B[i];
	}
	b = std::sqrt(b);
	
	deriv.position() = (C/b) * B;
	for(unsigned i = 0; i < mDimension; ++i)
	{
		deriv.position()[i] += linearInterpolate(mVelocities[i], p.data());
	}
	
	// dB/dt = -grad(B dot v) for c = const
	// dB/dt_i = -d/di (B dot v) = - d/di (B_j v^j) = -B_j dv^j/di
	for(unsigned i = 0; i < mDimension; ++i)
	{
		deriv.velocity()[i] = 0;
		for(unsigned j = 0; j < mDimension; ++j)
		{
			deriv.velocity()[i] -= state.velocity()[j] * linearInterpolate(mVelDerivatives[mDimension*j + i], p.data());
		}
	}
}

bool Sound::hasMonodromy() const
{
	return false;
}

bool Sound::hasPeriodicBoundary() const
{
	return mPeriodicBoundaries;
}

void Sound::normalizeEnergy(State&, double) const
{
	/// \todo figure out what to do with energy
}

double Sound::getEnergy( const State& ) const
{
	return 0;
}
