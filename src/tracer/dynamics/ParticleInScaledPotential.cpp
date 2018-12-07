#include "ParticleInScaledPotential.hpp"
#include "ode_state.hpp"
#include "state.hpp"
#include <boost/numeric/ublas/io.hpp>
#include "potential.hpp"
#include "interpolation.hpp"
#include "monodromy.hpp"
#include "dynamics_factory.hpp"
#include <boost/lexical_cast.hpp>

ParticleInScaledPotentialDynamics::ParticleInScaledPotentialDynamics(const Potential& pot, bool periodic, bool monodromy, float scale):
	mDimension( pot.getDimension() ),
	mPeriodicBoundaries( periodic ),
	mTraceMonodromy( monodromy ),
	mPotScaleFactor(scale),
	mScalingFactor( pot.getDimension() ),
	mGridSize( pot.getDimension() ),
	mFirstDerGrid( pot.getDimension() )
{
	// calculate scale factor
	for(unsigned i = 0; i < mDimension; ++i)
	{
		mScalingFactor[i] = pot.getExtents()[i] / pot.getSupport()[i];
		mGridSize[i] = pot.getExtents()[i];
	}
	
	mPotential = pot.getPotential().shallow_copy();
	
	// set up grids
	for(unsigned i = 0; i < mDimension; ++i)
	{
		mFirstDerGrid[i] = pot.getDerivative( makeIndexVector(mDimension, {i}) ).shallow_copy();
	}
	
	// the current setup requires us to set the grid as periodic here, independently of
	// whether we actually do periodic tracing.
	for(auto& deriv : mFirstDerGrid)
		deriv.setAccessMode(TransformationType::PERIODIC);
	
	if(mTraceMonodromy)
	{
		if(!pot.hasDerivativesOfOrder( 2 ))
			THROW_EXCEPTION( std::runtime_error, "monodromy integration requires derivatives of second order!" );
		
		mSecondDerGrid.resize(mDimension * mDimension);
		for(unsigned i = 0; i < mDimension; ++i)
		for(unsigned j = 0; j < mDimension; ++j)
			mSecondDerGrid[i*mDimension + j] = pot.getDerivative(makeIndexVector(mDimension, {i, j})).shallow_copy();

		for(auto& deriv : mSecondDerGrid)
			deriv.setAccessMode(TransformationType::PERIODIC);
	}
}

void ParticleInScaledPotentialDynamics::stateUpdate( const GState& state , GState& deriv, double) const 
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

	p[0] *= mPotScaleFactor;
	
	// calculate acceleration
	vector_proxy acceleration = deriv.velocity();
	for(unsigned i = 0; i < mDimension; ++i)
	{
		auto& grid = mFirstDerGrid[i];
		acceleration[i] = -linearInterpolate(grid, p.data());
	}
	
	acceleration[0] *= mPotScaleFactor;

	// change in position = current velocity
	deriv.position() = state.velocity();

	/// \todo fix scaling of monodromy!
	if( mTraceMonodromy )
	{
		assert(0);
		auto monomat = getMonodromyCoeff( mDimension, mSecondDerGrid.data(), p.data() );
		monodromy_matrix_multiply(mDimension, &deriv.matrix()[0], monomat, &state.matrix()[0]);
	}
}

bool ParticleInScaledPotentialDynamics::hasMonodromy() const
{
	return mTraceMonodromy;
}

bool ParticleInScaledPotentialDynamics::hasPeriodicBoundary() const
{
	return mPeriodicBoundaries;
}

void ParticleInScaledPotentialDynamics::normalizeEnergy(State& state, double total_energy) const
{
	std::array<double, 3> p;
	for(std::size_t i = 0; i < mDimension; ++i)
		p[i] = state.getPosition()[i] * mScalingFactor[i];
	
	
	p[0] *= mPotScaleFactor;
	
	// normalize energy
	double epot = linearInterpolate(mPotential, p.data());
	double diff = total_energy - epot;
	if(diff < 0)
		THROW_EXCEPTION(std::runtime_error, "Cannot normalize energy of particle, as potential energy %1% already exceeds total energy %2%.", epot, total_energy);
	/// \todo assert/check that epot < 0.5
	// 1/2 v^2 + epot = E0
	// v^2 = 2E0 - 2epot
	double vi = std::sqrt(2*diff);
	double len = 0;
	for(unsigned i = 0; i < mDimension; ++i)
	{
		len += state.getVelocity()[i] * state.getVelocity()[i];
	}
	len = std::sqrt(len);

	state.editVel() *= vi / len;
}

double ParticleInScaledPotentialDynamics::getEnergy( const State& state ) const
{
	std::array<double, 3> p;
	for(std::size_t i = 0; i < mDimension; ++i)
		p[i] = state.getPosition()[i] * mScalingFactor[i];
	
	p[0] *= mPotScaleFactor;

	// then interpolate linearly
	double epot = linearInterpolate( mPotential, p.data() );
	double ekin = 0.5 * std::inner_product( state.getVelocity().begin(), state.getVelocity().end(), state.getVelocity().begin(), 0.0 );
	return epot + ekin;
}
