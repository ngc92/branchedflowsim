#ifndef ANGULAR_HISTOGRAM_OBS_HPP_INCLUDED
#define ANGULAR_HISTOGRAM_OBS_HPP_INCLUDED

#include "observer.hpp"
#include <vector>

class AngularHistogramObserver final: public ThreadLocalObserver
{
public:
    /// create an observer and specify the time interval for saving the particle's position
    AngularHistogramObserver( std::vector<double> timeIntervals, double binsize,
                              std::string file_name="angle_histograms.dat" );

    /// d'tor
    ~AngularHistogramObserver() = default;

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save( std::ostream& target ) override;

private:
    // thread local specific functions
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    void record( std::vector<std::size_t>& histogram, const gen_vect& velocity );

    // config
    double mAngularBinSize;
    std::size_t mBinCount;
    std::vector<double> mTimeIntervals;

    // data
    std::vector<std::vector<std::size_t>> mBinCounts;
    std::vector<double> mSumAngle;
    std::vector<double> mSumSquared;
    unsigned mLastObservedTime = 0;
    gen_vect mOldVelocity;
    double mOldTime;

};

#endif // ANGULAR_HISTOGRAM_OBS_HPP_INCLUDED
