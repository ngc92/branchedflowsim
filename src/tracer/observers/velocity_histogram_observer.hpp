//
// Created by eriks on 7/24/17.
//

#ifndef BRANCHEDFLOWSIM_VELOCITYHISTOGRAMOBSERVER_H
#define BRANCHEDFLOWSIM_VELOCITYHISTOGRAMOBSERVER_H

#include "dynamic_grid.hpp"
#include "vector.hpp"
#include "observer.hpp"

class VelocityHistogram
{
    using histogram_t = DynamicGrid<std::size_t>;
public:
    VelocityHistogram(std::size_t dimension, std::size_t bin_count, double range);

    void record(const gen_vect& velocity);
    const histogram_t& data() const { return mData; }
    histogram_t& getData() { return mData; }
private:
    histogram_t mData;
    double mRange;
    std::size_t mBinCount;
};

class VelocityHistogramObserver final: public ThreadLocalObserver
{
public:
    /// create an observer and specify the time interval for saving the particle's position
    VelocityHistogramObserver( std::size_t dimension, std::vector<double> timeIntervals, std::size_t bin_count,
                               std::string file_name="velocity_histograms.dat");

    /// d'tor
    ~VelocityHistogramObserver() = default;

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save(std::ostream& target) override;

private:
    // thread local specific functions
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    // config
    std::size_t mBinCount;
    std::size_t mDimension;
    std::vector<double> mTimeIntervals;

    // data
    /// bin counts for every time step
    std::vector<VelocityHistogram> mBinCounts;
    unsigned mLastObservedTime = 0;
    gen_vect mOldVelocity;
    double mOldTime;
};


#endif //BRANCHEDFLOWSIM_VELOCITYHISTOGRAMOBSERVER_H
