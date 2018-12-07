#include "master_observer.hpp"
#include "observer.hpp"
#include "ode_state.hpp"
#include "dynamics/ray_dynamics.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include <mutex>

std::atomic<std::size_t> MasterObserver::mParticleCount;
std::atomic<std::size_t> MasterObserver::mParticleNumber;

MasterObserver::MasterObserver( int dim, std::shared_ptr<const RayDynamics> dynamics ): 
    mDimension(dim), 
    mDynamics( std::move(dynamics) )
{
}

MasterObserver::~MasterObserver()
{
    for( auto& w : mLocalWatches )
        w->reduce();
}

void MasterObserver::setPeriodicBoundaries( bool p )
{
    mPeriodicBoundaries = p;
}

void MasterObserver::addObserverObject(watch_type object)
{
    // categorize
    auto thread_loc = std::dynamic_pointer_cast<ThreadLocalObserver>(object);
    if( thread_loc )
        mLocalWatches.push_back( thread_loc );

    auto thread_shared = std::dynamic_pointer_cast<ThreadSharedObserver>(object);
    if( thread_shared )
        mSharedWatches.push_back( thread_shared );
    mWatches.push_back( object );
}

void MasterObserver::startTracing()
{
    mParticleCount = 0;     // reset particle index
    mParticleNumber = 0;    // ... and number
    mCurrentTrajectory.reserve(1000);

    mActiveWatches.resize( mLocalWatches.size() );
    /// \todo preallocate?
    for( const auto& f : mWatches )
    {
        f->init(mDynamics);
        f->startTracing();
    }
}

void MasterObserver::finishTracing()
{
    for( const auto& f : mWatches )
        f->endTracing(mParticleCount);
}

MasterObserver::TS::TS( State state, double time ) : s( std::move(state)), t(time)
{
}

void MasterObserver::startTrajectory( const InitialCondition& ic )
{
    // update trajectory number. This loads the value and then increments atomically, so the return values
    // are really unique
    mCurrentTrajectoryNum = ++mParticleNumber;

    // activate all watches
    for(auto& value : mActiveWatches )
        value = true;

    for(auto& watch : mLocalWatches)
        watch->startTrajectory(ic, mCurrentTrajectoryNum);

    // clear trajectory cache (does not cause need to reallocate memory, so
    //    this is not be a performance problem)
    mCurrentTrajectory.clear();
}

void MasterObserver::finishTrajectory( const InitialCondition& ic )
{
    if(!mCurrentTrajectory.empty())
    {
        // process all shared watches
        for( const auto& f : mSharedWatches )
        {
            auto lock = f->getLock();
            f->startTrajectory( ic, mCurrentTrajectoryNum );
            for(auto& point : mCurrentTrajectory)
            {
                if(!f->watch(point.s, point.t))
                break;
            }
            f->endTrajectory( mCurrentTrajectory.back().s );
        }

        // finish all local watches
        for(const auto& f : mLocalWatches)
            f->endTrajectory( mCurrentTrajectory.back().s );

        // only count particles for which points were found
        ++mParticleCount;
    }
}


// -----------------------------------------------------------------------------------------------------
void MasterObserver::operator()( const GState& state, double t )
{
    // if we have shared watches, we need to continue watching till the end
    bool still_watching = !mSharedWatches.empty();


    // add current point to cache
    mCurrentTrajectory.emplace_back( State(state), t );

    // process local watches

    for( unsigned i = 0; i < mLocalWatches.size(); ++i)
        if( mActiveWatches[i] )
        {
            State nstate( mDimension );
            nstate.readState( state );
            bool watching = mLocalWatches[i]->watch(nstate, t);
            if( watching )
                still_watching = true;
            else
            {
                mActiveWatches[i] = false;
            }
        }

    // check if any watches remain
    if( !still_watching )
        throw(1);
}


MasterObserver MasterObserver::clone() const
{
/// \todo this assert may fire when one thread starts integrating before the other is initialized. Should not be a problem,
/// so commented out.
//    assert( mParticleCount == 0 );

    // copy only observer pointers, or create real copy if supported by that observer.
    MasterObserver ob( mDimension, mDynamics );
    for(auto& f : mWatches) {
        ob.addObserverObject( f->makeThreadCopy() );
    }

    // initialize new watches
    for(auto& w : ob.mWatches)
    {
        if(!w->is_ready())
        {
            w->init(mDynamics);
        }
    }

    ob.setPeriodicBoundaries( mPeriodicBoundaries );
    ob.mActiveWatches.resize( mLocalWatches.size() );

    return ob;
}

const std::vector<MasterObserver::watch_type>& MasterObserver::getObservers() const
{
    return mWatches;
}

std::size_t MasterObserver::getTracedParticleCount() const
{
    return mParticleCount;
}
