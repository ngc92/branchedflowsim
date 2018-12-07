#ifndef MASTER_OBSERVER_HPP_INCLUDED
#define MASTER_OBSERVER_HPP_INCLUDED

#include "vector.hpp"
#include "state.hpp"
#include "initial_conditions_fwd.hpp"
#include <cmath>
#include <vector>
#include <atomic>
#include <memory>
#include <boost/noncopyable.hpp>

class Observer;
class ThreadSharedObserver;
class ThreadLocalObserver;
class EnergyErrorObserver;
class GState;
class RayDynamics;

/// \todo add progress tracking
class MasterObserver final
{
    MasterObserver( const MasterObserver& ) = delete;
    MasterObserver& operator=( const MasterObserver& ) = delete;
    // commonly used types
    typedef std::shared_ptr<Observer> watch_type;
public:
    /// default c'tor
    MasterObserver( int dim, std::shared_ptr<const RayDynamics> dynamics );
    MasterObserver( MasterObserver&&) = default;
    ~MasterObserver();

    void setPeriodicBoundaries( bool p );

    /// called when the tracing starts
    void startTracing();

    /// called when tracing is finished
    void finishTracing();

    /// called when a the tracing of a trajectory is started.
    void startTrajectory( const InitialCondition& ic );

    /// called after the trajectory is finished, pushes the cached points into their respective observer
    void finishTrajectory(const InitialCondition& ic);

    // standard observe function
    /// operator(), called each simulation step by boost.
    void operator()( const GState& state, double t );

    // add a generic watch object
    void addObserverObject(watch_type object);

    const std::vector<watch_type>& getObservers() const;

    MasterObserver clone() const;

    /// returns number of particles traced
    std::size_t getTracedParticleCount() const;

    /// returns the current trajectory number
    std::size_t getCurrentTrajectory() const { return mCurrentTrajectoryNum; };
private:
    // count particles
    static std::atomic<std::size_t> mParticleCount;  // incremented for each finished, valid trajectory
    static std::atomic<std::size_t> mParticleNumber; // incremented for each started trajectory

    // watches

    // watchlist
    /// this vector contains pointers to the thread-local watches that can be safely called during the integration.
    std::vector<std::shared_ptr<ThreadLocalObserver>> mLocalWatches;
    /// this vector keeps track on which watches are currently active.
    std::vector<char> mActiveWatches;
    /// this vector contains pointers to shared watches, which must be called in a critical section. Calls to
    /// these watches are cached and performed in bulk for a complete trajectory.
    std::vector<std::shared_ptr<ThreadSharedObserver>> mSharedWatches;

    /// this vector contains all watches
    std::vector<watch_type> mWatches;

    std::size_t mDimension;
    bool mPeriodicBoundaries = false;

    // list of all states in current trajectory
    struct TS
    {
        TS( State state, double time );
        State s;
        double t;
    };
    std::vector<TS> mCurrentTrajectory;
    std::size_t mCurrentTrajectoryNum;
    std::shared_ptr<const RayDynamics> mDynamics;
};

#endif // OBSERVER_HPP_INCLUDED
