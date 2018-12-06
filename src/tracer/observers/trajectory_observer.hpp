#ifndef TRAJECTORY_OBSERVER_HPP_INCLUDED
#define TRAJECTORY_OBSERVER_HPP_INCLUDED

#include "observer.hpp"
#include "caustic.hpp"
#include <deque>

// struct to save a single trajectory sample
// using this is not very space efficient, we'll have to see
// whether memory becomes an issue.
/// \todo maybe we should add memory tracking to observers.
struct TrajectorySample
{
    std::uint64_t trajectory;
    gen_vect pos;
    gen_vect vel;
    double time;
    TrajectorySample(std::uint64_t t, const gen_vect& p, const gen_vect& v, double tm):
        trajectory(t), pos(p), vel(v), time(tm)
    {

    }
};

class TrajectoryObserver final: public ThreadLocalObserver
{
    // use deque because we do not iterate over the data very often,
    // and deque has better push_back performance and does not require
    // enormous amounts of consecutive memory.
    typedef std::deque<TrajectorySample> container_type;
public:
    /// create an observer and specify the time interval for saving the particle's position
    TrajectoryObserver( double interval, std::string file_name = "trajectory.dat" );

    /// d'tor
    virtual ~TrajectoryObserver();

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save(std::ostream& target) override;

private:
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    // configuration
    double mInterval = 0.01;

    // cache
    double mLastTime = 0;
    std::size_t mParticleNumber = 0;

    // generated results
    container_type mTrajectorySamples;
};

#endif // TRAJECTORY_OBSERVER_HPP_INCLUDED
