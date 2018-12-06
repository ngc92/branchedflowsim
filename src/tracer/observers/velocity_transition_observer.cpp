//
// Created by eriks on 7/24/17.
//

#include "velocity_transition_observer.hpp"
#include "interpolation.hpp"
#include "fileIO.hpp"
#include <boost/algorithm/clamp.hpp>
#include "initial_conditions/initial_conditions.hpp"
#include <boost/range/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

VelocityTransitionData::VelocityTransitionData(std::size_t dimension, std::size_t bin_count, double range,
                                       std::vector<bool> in, std::vector<bool> out, bool increments) :
    mDimension(dimension), mRange(range), mBinCount(bin_count), mIncrementMode(increments)
{
    if(in.size() != dimension)
    {
        THROW_EXCEPTION(std::logic_error, "Invalid number of entries in 'in'");
    }

    if(out.size() != dimension)
    {
        THROW_EXCEPTION(std::logic_error, "Invalid number of entries in 'out'");
    }


    using namespace boost::adaptors;
    using namespace boost::range;

    auto extends_range = join(in, out) | transformed([this](bool use){ return use ? mBinCount : 1; });

    std::vector<std::size_t> extents(begin(extends_range), end(extends_range));
    mData = histogram_t(extents);

    // pre-calculate the bin centers.
    for(unsigned j = 0; j < mBinCount; ++j) {
        double bin_center = double(j) / (mBinCount - 1) * 2 - 1;
        mBinCenters.push_back(bin_center * mRange);
    }
}

int to_index(double value, std::size_t bin_count)
{
    double v = (boost::algorithm::clamp(value, -1.0, 1.0) + 1) / 2;
    return static_cast<int>(std::round(v * (bin_count - 1)));
}

void VelocityTransitionData::record(const gen_vect& old_velocity, const gen_vect& velocity)
{
    boost::numeric::ublas::c_vector<int, 6> index(old_velocity.size() + velocity.size());
    for(unsigned i = 0; i < old_velocity.size(); ++i)
    {
        index[i] = to_index(old_velocity[i] / mRange, mData.getExtents()[i]);
    }

    if(!mIncrementMode) {
        for (unsigned i = 0; i < velocity.size(); ++i) {
            index[i + old_velocity.size()] = to_index(velocity[i] / mRange,
                                                      mData.getExtents()[i + old_velocity.size()]);
        }
    }
    else
    {
        for (unsigned i = 0; i < velocity.size(); ++i) {
            index[i + old_velocity.size()] = to_index((velocity[i] - old_velocity[i]) / mRange,
                                                      mData.getExtents()[i + old_velocity.size()]);
        }
    }

    mData(index) += 1;
}


VelocityTransitionObserver::VelocityTransitionObserver(std::size_t dimension, double timeInterval, std::size_t bin_count,
                                                       double start_time, double end_time, std::vector<bool> in,
                                                       std::vector<bool> out, bool increment_mode, std::string file_name)
        : ThreadSharedObserver( std::move(file_name) ),
        mBinCount(bin_count), mDimension(dimension), mTimeInterval(timeInterval),
          mStartRecordingTime(start_time), mEndRecordingTime(end_time),
          mLastStepVelocity(dimension), mBinCounts(dimension, bin_count, 1.5, in, out, increment_mode),
          mVelocityRange(1.5)
{
    if(timeInterval <= 0)
    {
        THROW_EXCEPTION(std::logic_error, "Negative time interval passed to VelocityTransitionObserver");
    }
}

void VelocityTransitionObserver::startTrajectory( const InitialCondition& start, std::size_t )
{
    mStartTransitionTime = mEndRecordingTime;
    mLastStepTime = 0;
    mLastStepVelocity = start.getState().getVelocity();
}


bool VelocityTransitionObserver::watch( const State& state, double t )
{
    // allow for recording to start at arbitrary times. In that case, we need to
    // generate the initial velocity by doing an interpolation.
    // need to include mLastStepTime == mStartRecordingTime in case recording starts
    // at time 0.
    if(t >= mStartRecordingTime && mLastStepTime <= mStartRecordingTime) {
        double record_step = mStartRecordingTime - mLastStepTime;
        double time_step = t - mLastStepTime;
        double r = record_step / time_step;

        // in the very first timestep, in case mStartRecordingTime == 0, we can get time_step == 0
        // which causes NaNs.
        if(time_step < 1e-20) {
            r = 0.0;
        }

        auto interpol = interpolate_linear_1d(mLastStepVelocity, state.getVelocity(), r);
        mStartTransitionTime = mStartRecordingTime;
        mOldVelocity = interpol;
    }

    // otherwise, check for passing time boundary
    // we add a very small epsilon 1e-10 to the right side of the boundary, because
    // otherwise it is possible for transitions to be skipped. We expect the times
    // involved here to be significantly bigger than 1e-10.
    while( t >= mStartTransitionTime + mTimeInterval &&
            mStartTransitionTime + mTimeInterval <= mEndRecordingTime + 1e-10)
    {
        // linearly interpolated velocity between last and current state
        // this is important, if e have multiple observation times that
        // are closely spaced in an area where the adaptive time step
        // gets big.
        double record_step = mStartTransitionTime + mTimeInterval - mLastStepTime;
        double time_step = t - mLastStepTime;
        double r = record_step / time_step;

        auto interpol = interpolate_linear_1d(mLastStepVelocity, state.getVelocity(), r);
        mBinCounts.record(mOldVelocity, interpol);
        mStartTransitionTime += mTimeInterval;
        mOldVelocity = interpol;
    }

    mLastStepTime = t;
    mLastStepVelocity = state.getVelocity();

    return t < mEndRecordingTime;
}


void VelocityTransitionObserver::save(std::ostream& target)
{
    /*! Angular Velocity histogram save file format.
        Header: velh001\\n
        Data type   | Count | Meaning
        ---------   | ----- | -------
        Int [B]     | 1     | Number of bins per histogram
        Int [D]     | 1     | Number of dimensions
        Double      | 1     | Time interval
        Double      | B     | bin center velocities
        Grid[UInt32]| B ^ (D-1)| grids with transition data
    */

    // file header
    target << "velt002\n";
    writeInteger(target, mBinCount);
    writeInteger(target, mDimension);
    writeFloat(target, mTimeInterval);
    writeFloats(target, mBinCounts.bin_centers());

    mBinCounts.data().dump(target);
}
