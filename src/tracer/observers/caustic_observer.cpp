#include "caustic_observer.hpp"
#include "interpolation.hpp"
#include "fileIO.hpp"
#include "master_observer.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <boost/lexical_cast.hpp>
#include "potential.hpp"

// forward declarations:
// helper functions that determine the area/volume spanned by the particle state and
// the monodromy deltas
double getSignedArea2D(const State& particle, const InitialCondition& IC);
double getSignedArea3D(const State& particle, const InitialCondition& IC);

CausticObserver::CausticObserver( std::size_t dimension, bool breakOnFirst, std::string file_name ) :
        ThreadLocalObserver( std::move(file_name) ),
        mBreakOnFirst( breakOnFirst ), mDimension(dimension) {
    if(mDimension < 2 || mDimension > 3)
        THROW_EXCEPTION(std::invalid_argument, "Dimension for caustic observer must be 2 or 3 but got %1%", mDimension);
}

CausticObserver::~CausticObserver()
{
}

void CausticObserver::combine(ThreadLocalObserver& other)
{
    auto& data = dynamic_cast<CausticObserver&>( other );
    // move all caustic positions to root observer
    std::move( data.mCausticPositions.begin(),
               data.mCausticPositions.end(),
               std::back_inserter(mCausticPositions)    );

    // calculate total particle number
    mParticleNumber = std::max(mParticleNumber, data.mParticleNumber);
}

const CausticObserver::container_type& CausticObserver::getCausticPositions() const
{
    return mCausticPositions;
}

void CausticObserver::startTrajectory( const InitialCondition& start, std::size_t trajectory )
{
    mOldArea = 0;
    mCachedInitialCondition = &start;
    mCausticCount = 0;
    mParticleNumber = trajectory;
}

bool CausticObserver::watch( const State& state, double t )
{
    // calculate enclosed volume
    double signed_area = 0;
    if( mDimension == 2)
         signed_area = getSignedArea2D( state, *mCachedInitialCondition );
    else if(mDimension == 3)
        signed_area = getSignedArea3D( state, *mCachedInitialCondition );
    else
        assert(0);

    // check for sign change
    // we need to check that we are not in the initial frame, because this would cause
    // problems for spherical ICs (NaN; div by zero)
    if(t > 0 && (signed_area * mOldArea < 0 || signed_area == 0 ) )
    {
        // linear interpolate to find position where signed_area was 0
        const auto& pos = state.getPosition();
        // a = (signed_area - mOldArea)
        // A = a * p + mOldArea = 0 => p = -mOldArea / a
        double p = - mOldArea / (signed_area - mOldArea);
        mCausticCount++;

        // create new caustic object and save in data vector
        // only thread-local, so no protection required
        auto position = interpolate_linear_1d(mOldPosition, pos, p);
        auto velocity = interpolate_linear_1d(mOldVelocity, state.getVelocity(), p);
        mCausticPositions.emplace_back( mParticleNumber,
                                        position,
                                        mCachedInitialCondition->getState().getPosition(),
                                        velocity,
                                        mCachedInitialCondition->getState().getVelocity(),
                                        interpolate_linear_1d( mOldTime, t, p ),
                                        mCausticCount );

        // break if we are only looking for first caustic
        if( mBreakOnFirst )
            return false;
    }

    // update cache variables for old state
    mOldArea = signed_area;
    mOldPosition = state.getPosition();
    mOldVelocity = state.getVelocity();
    mOldTime = t;

    return true;
}

void CausticObserver::save(std::ostream& target)
{
    // file header
    target << "caus001\n";
    writeInteger(target, mParticleNumber );
    writeInteger(target, mDimension );
    writeInteger(target, mCausticPositions.size() );

    for(const auto& c : mCausticPositions)
        c.write(target);
}

//

// helper function template to get area between particle velocity and deltas
double getSignedArea2D(const State& particle, const InitialCondition& IC)
{
    auto& d = particle.getMatrix();
    auto delta = IC.getDelta(0).getPhaseSpaceVector();

    boost::numeric::ublas::c_vector<double, 4> result = prod(d, delta);

    return result[0] * particle.getVelocity()[1] - result[1] * particle.getVelocity()[0];
}

double getSignedArea3D(const State& particle, const InitialCondition& IC)
{
    typedef boost::numeric::ublas::c_vector<double, 6> vec_t;
    /// \todo check for 2d initial condition
    auto& d = particle.getMatrix();
    auto delta = IC.getDelta(0).getPhaseSpaceVector();

    /// \todo use 3D vectors here
    vec_t v1 = prod(d, delta);

    delta = IC.getDelta(1).getPhaseSpaceVector();

    vec_t v2 = prod(d, delta);

    std::array<double, 3> v1xv2;
    /// \todo right now we cannot use cross product here, because v1, v2 are 6 element vectors
    v1xv2[0] =  v1[1]*v2[2] - v1[2]*v2[1];
    v1xv2[1] = -v1[0]*v2[2] + v1[2]*v2[0];
    v1xv2[2] = v1[0]*v2[1] - v1[1]*v2[0];
    //crossProduct(v1xv2, v1, v2);

    return std::inner_product(v1xv2.begin(), v1xv2.end(), particle.getVelocity().begin(), 0.0);
}

std::shared_ptr<ThreadLocalObserver> CausticObserver::clone() const
{
    return std::make_shared<CausticObserver>( mDimension, mBreakOnFirst, filename() );
}
