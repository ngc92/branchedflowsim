//
// Created by eriks on 7/24/17.
//

#ifndef BRANCHEDFLOWSIM_VELOCITYTRANSITIONOBSERVER_H
#define BRANCHEDFLOWSIM_VELOCITYTRANSITIONOBSERVER_H

#include "dynamic_grid.hpp"
#include "vector.hpp"
#include "observer.hpp"

class VelocityTransitionData
{
    using histogram_t = DynamicGrid<std::uint32_t>;
public:
    VelocityTransitionData(std::size_t dimension, std::size_t bin_count, double range,
                       std::vector<bool> in, std::vector<bool> out, bool increments);

    void record(const gen_vect& old_velocity, const gen_vect& velocity);
    const histogram_t& data() const { return mData; }
    const std::vector<double>& bin_centers() const { return mBinCenters; };
private:
    std::size_t mDimension;
    histogram_t mData;
    double mRange;
    std::size_t mBinCount;
    bool mIncrementMode;

    std::vector<double> mBinCenters;
};

class VelocityTransitionObserver final: public ThreadSharedObserver
{
public:
    /// create an observer and specify the time interval for saving the particle's position
    VelocityTransitionObserver(std::size_t dimension, double timeInterval, std::size_t bin_count, double start_time,
                               double end_time, std::vector<bool> in, std::vector<bool> out, bool increment_mode,
                               std::string file_name="velocity_transitions.dat");

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save(std::ostream& target) override;

private:
    // config
    std::size_t mBinCount;
    std::size_t mDimension;
    double mTimeInterval;
    double mStartRecordingTime = 0.0;
    double mEndRecordingTime = 1e100;
    double mVelocityRange = 1.5;


    // caching on a single ray
    double mLastStepTime = 0.0;
    gen_vect mLastStepVelocity;

    // data
    VelocityTransitionData mBinCounts;
    double mStartTransitionTime = 0.0;
    gen_vect mOldVelocity;
};


#endif //BRANCHEDFLOWSIM_VELOCITYTRANSITIONOBSERVER_H
