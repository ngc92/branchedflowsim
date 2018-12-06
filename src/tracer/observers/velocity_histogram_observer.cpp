//
// Created by eriks on 7/24/17.
//

#include "velocity_histogram_observer.hpp"
#include "interpolation.hpp"
#include "fileIO.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include <boost/algorithm/clamp.hpp>


VelocityHistogram::VelocityHistogram(std::size_t dimension, std::size_t bin_count, double range) :
    mData(dimension-1, bin_count), mRange(range), mBinCount(bin_count)
{
}

void VelocityHistogram::record(const gen_vect& velocity)
{
    // remember data dimension is simulation dimension minus one.
    if(mData.getDimension() == 1)
    {
        double vel = (boost::algorithm::clamp(velocity[1] / mRange, -1.0, 1.0) + 1) / 2;
        std::array<int, 1> index_v;
        index_v[0] = static_cast<int>(std::round(vel * (mBinCount - 1)));
        mData(index_v) += 1;
    } else if(mData.getDimension() == 2)
    {
        double vel1 = (boost::algorithm::clamp(velocity[1] / mRange, -1.0, 1.0) + 1) / 2;
        double vel2 = (boost::algorithm::clamp(velocity[2] / mRange, -1.0, 1.0) + 1) / 2;
        std::array<int, 2> index_v;
        index_v[0] = static_cast<int>(std::round(vel1 * (mBinCount - 1)));
        index_v[1] = static_cast<int>(std::round(vel2 * (mBinCount - 1)));
        mData(index_v) += 1;
    }
}


VelocityHistogramObserver::VelocityHistogramObserver(std::size_t dimension, std::vector<double> timeIntervals,
                                                     std::size_t bin_count, std::string file_name) :
        ThreadLocalObserver( std::move(file_name) ),
        mBinCount(bin_count), mDimension(dimension), mTimeIntervals(std::move(timeIntervals))
{
    for(unsigned i = 0; i < mTimeIntervals.size(); ++i)
    {
        mBinCounts.push_back( VelocityHistogram(dimension, bin_count, 1.5)  );
    }
}

void VelocityHistogramObserver::combine(ThreadLocalObserver& other)
{
    auto& data = dynamic_cast<VelocityHistogramObserver&>( other );
    for(unsigned i = 0; i < mBinCounts.size(); ++i)
    {
        auto& count = mBinCounts[i].getData();
        auto& o_count = data.mBinCounts[i].data();
        // add the counts
        std::transform(count.begin(), count.end(), o_count.begin(), count.begin(), std::plus<std::size_t>());
    }
}

void VelocityHistogramObserver::startTrajectory( const InitialCondition& start, std::size_t )
{
    mOldVelocity = start.getState().getVelocity();
    mLastObservedTime = 0;
    mOldTime = 0;
}


bool VelocityHistogramObserver::watch( const State& state, double t )
{
    // finished after last time
    if( mLastObservedTime >= mTimeIntervals.size() )
        return false;

    // otherwise, check for passing time boundary
    while( t > mTimeIntervals[mLastObservedTime] )
    {
        // linearly interpolated velocity between last and current state
        // this is important, if e have multiple observation times that
        // are closely spaced in an area where the adaptive time step
        // gets big.
        double rtime = (mTimeIntervals[mLastObservedTime] - mOldTime) / (t - mOldTime);
        auto interpol = interpolate_linear_1d(mOldVelocity,
                                              state.getVelocity(),
                                              rtime);
        mBinCounts[mLastObservedTime].record(interpol);
        ++mLastObservedTime;
        // finished after last time
        if( mLastObservedTime >= mTimeIntervals.size() )
            return false;
    }

    // update old position.
    // since we only need this if we continue tracing this trajectory,
    // it is OK to only update when we return true.
    mOldVelocity = state.getVelocity();
    mOldTime = t;
    return true;
}


void VelocityHistogramObserver::save(std::ostream& target)
{
    /*! Angular Velocity histogram save file format.
        Header: velh001\\n
        Data type | Count | Meaning
        --------- | ----- | -------
        Int [H]   | 1     | Number of histograms
        Int [B]   | 1     | Number of bins per histogram
        Int [D]   | 1     | Number of dimensions
        Double    | H     | Histogram times
        Double    | B     | bin center velocities
        Int       | H * B ^ (D-1)| Angle counts
    */

    // file header
    target << "velh001\n";
    writeInteger(target, mBinCounts.size());
    writeInteger(target, mBinCount);
    writeInteger(target, mDimension);
    // write times
    writeFloats(target, mTimeIntervals);

    // write velocities
    for(unsigned j = 0; j < mBinCount; ++j) {
        double bin_center = double(j) / (mBinCount - 1) * 2 - 1;
        writeFloat(target, bin_center * 1.5);
    }

    for( auto& bins : mBinCounts )
    {
        bins.data().dump(target);
    }
}


std::shared_ptr<ThreadLocalObserver> VelocityHistogramObserver::clone() const
{
    return std::make_shared<VelocityHistogramObserver>( mDimension, mTimeIntervals, mBinCount, filename() );
}
