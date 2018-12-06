#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED

#include "vector.hpp"
#include <boost/numeric/ublas/matrix.hpp>
#include <iosfwd>

class GState;
typedef boost::numeric::ublas::c_matrix<double, 6, 6> gen_mat;

/*! \class State
	\brief Class describing a generic particle state.
	\details This class describes a generic particle state without the use of templates. This simplifies code a
			lot, at the cost of runtime speed. Therefore, this class is only used in non-speed-critical sections of the code,
			for which measurements have shown no noticeable performance impact.
			These are
				generation of initial conditions
				processing of data in state objects.
			Because old versions of this class performed dynamic memory allocations, copying is currently disabled. Should it be required for a
			new algorithm, it should be tested whether copying still hurts performance.
*/
class State
{
public:
	// constructors
	explicit State(std::size_t dimension);

	// prevent copying
	State( const State& ) = delete;
	State( State&& ) = default;
	explicit State( const GState& ode_state );

	// inline getters
	const gen_vect& getPosition() const { return mPosition; };
	const gen_vect& getVelocity() const { return mVelocity; };
	const gen_mat&  getMatrix() const { return mMatrix; };

	gen_vect& editPos() { return mPosition; };
	gen_vect& editVel() { return mVelocity; };
	gen_mat&  editMat() { return mMatrix; };

	std::size_t getDimension() const { return mDimension; };

	// other functions

	/// read a state from the ode-solver and convert it into this state, which is more convenient for further processing
	void readState(const GState& state);

	/// write whole state into a phase space vector
	c_vector<6> getPhaseSpaceVector() const;
private:
	// configuration
	std::size_t mDimension;

	// trivial data
	gen_vect mPosition;
	gen_vect mVelocity;

	gen_mat  mMatrix;
};

std::ostream& operator<<( std::ostream& stream, const State& state );

#endif // STATE_HPP_INCLUDED
