#ifndef CAUSTIC_OBSERVER_HPP_INCLUDED
#define CAUSTIC_OBSERVER_HPP_INCLUDED

#include "observer.hpp"
#include "caustic.hpp"
#include <deque>

class CausticObserver final: public ThreadLocalObserver
{
    // use deque because we do not iterate over the data very often,
    // and deque has better push_back performance and does not require
    // enormous amounts of consecutive memory.
    typedef std::deque<Caustic> container_type;
public:
    /// create an observer and specify the size of the density track object
    CausticObserver( std::size_t dimension, bool breakOnFirst = false, std::string file_name = "caustics.dat" );

    /// d'tor
    virtual ~CausticObserver();



    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save(std::ostream& target) override;

    const container_type& getCausticPositions() const;

private:
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    // configuration
    bool mBreakOnFirst = false;
    const std::size_t mDimension;

    std::size_t mCausticCount = 0;    // counts the number of caustics on the current trajectory
    std::size_t mParticleNumber = 0;

    // cache data for a single trajectory run
    double mOldArea = 0;
    gen_vect mOldPosition;
    gen_vect mOldVelocity;
    double mOldTime;
    const InitialCondition* mCachedInitialCondition;

    // generated results
    container_type mCausticPositions;
};

#endif // CAUSTIC_OBSERVER_HPP_INCLUDED
