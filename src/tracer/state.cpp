#include "state.hpp"
#include "global.hpp"
#include "ode_state.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <iostream>

State::State(std::size_t dim) : mDimension(dim), mPosition(dim), mVelocity(dim), mMatrix(2*dim, 2*dim)
{

}

State::State( const GState& ode_state ) : mDimension( ode_state.dimension() ),
											mPosition(ode_state.dimension()),
											mVelocity(ode_state.dimension()),
											mMatrix(2*ode_state.dimension(), 2*ode_state.dimension())
{
	readState( ode_state );
}

void State::readState(const GState& state)
{
	// first, check that mDimensions match. the THROW_SIMPLE_EXCEPTION macro ensures that the exception path does not
	// significantly increase the size of this function.
	if( state.dimension() != mDimension )
		THROW_EXCEPTION( std::logic_error, "Trying to assign states with different dimensions %1% and %2%", state.dimension(), mDimension );

	// ok, we are good to go. First, copy the vectors
	state.position().assignTo( mPosition );
	state.velocity().assignTo( mVelocity );

	// check if there is matrix data to copy
	if( state.monodromy() )
	{
		// get a reference to the matrix
		auto oldmat = state.matrix();
		// ... and copy element wise
		/// \todo since these are consecutive array elements, wouldn't memcpy be faster?
		for(unsigned i = 0; i < 2*mDimension; ++i)
			for(unsigned j = 0; j < 2*mDimension; ++j)
				mMatrix(i, j) = oldmat[i * 2*mDimension + j];
	}
}

c_vector<6> State::getPhaseSpaceVector() const
{
	c_vector<6> p(2*mDimension);
	for(unsigned i = 0; i < mDimension; ++i)
		p[i] = mPosition[i];
	for(unsigned i = 0; i < mDimension; ++i)
		p[i+mDimension] = mVelocity[i];
	return p;
}

std::ostream& operator<<( std::ostream& stream, const State& state )
{
	stream << state.getPosition() << "\n" << state.getVelocity() << "\n";
	stream << state.getMatrix() << "\n";
	return stream;
}
