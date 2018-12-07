#include "angular_histogram_obs.hpp"
#include "fileIO.hpp"
#include "global.hpp" // for pi
#include "initial_conditions/initial_conditions.hpp"
#include "interpolation.hpp"

AngularHistogramObserver::AngularHistogramObserver( std::vector<double> timeIntervals, double binsize,
        std::string file_name) :
        ThreadLocalObserver( std::move(file_name) ),
        mAngularBinSize( binsize ), mTimeIntervals( std::move(timeIntervals) )
{
    mBinCount = 2 * pi / mAngularBinSize;
    for(unsigned i = 0; i < mTimeIntervals.size(); ++i)
    {
        mBinCounts.push_back( std::vector<std::size_t>( mBinCount, 0u ) );
    }
    
    mSumAngle.resize(mTimeIntervals.size(), 0.0);
    mSumSquared.resize(mTimeIntervals.size(), 0.0);
}

void AngularHistogramObserver::combine(ThreadLocalObserver& other)
{
    auto& data = dynamic_cast<AngularHistogramObserver&>( other );
    for(unsigned i = 0; i < mBinCounts.size(); ++i)
    {
        auto& bins = mBinCounts[i];
        for(unsigned j = 0; j < bins.size(); ++j)
        {
            bins[j] += data.mBinCounts[i][j];
        }
        
        mSumAngle[i] += data.mSumAngle[i];
        mSumSquared[i] += data.mSumSquared[i];
    }
}

void AngularHistogramObserver::startTrajectory( const InitialCondition& start, std::size_t )
{
    mOldVelocity = start.getState().getVelocity();
    mLastObservedTime = 0;
    mOldTime = 0;
}

bool AngularHistogramObserver::watch( const State& state, double t )
{
    // finished after last time
    if( mLastObservedTime >= mTimeIntervals.size() )
        return false;
    
    // otherwise, check for passing time boundary
    while( t > mTimeIntervals[mLastObservedTime] )
    {
        // linearly interpolated velocity between last and current state
        // this is important, if e have multiple observation times that
        // are closely spaced in an area where the adaptive timestep 
        // gets big.
        double rtime = (mTimeIntervals[mLastObservedTime] - mOldTime) / (t - mOldTime);
        auto interpol = interpolate_linear_1d(mOldVelocity, 
                                            state.getVelocity(), 
                                            rtime);
        record( mBinCounts[mLastObservedTime], interpol );
        double angle = std::atan2(interpol[1], interpol[0]); // in [-pi, pi]
        mSumAngle[mLastObservedTime] += angle;
        mSumSquared[mLastObservedTime] += angle * angle;
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

void AngularHistogramObserver::record( std::vector<std::size_t>& histogram, const gen_vect& velocity )
{
    /// \todo this only makes sense for 2D?
    double angle = std::atan2(velocity[1], velocity[0]); // in [-pi, pi]
    double pos_angle = angle + pi; // in [0, 2pi] 
    std::size_t index = pos_angle / mAngularBinSize;
    // make sure that we don't get an overflow due to a rounding problem
    if(index == histogram.size()) index = histogram.size() - 1;
    histogram[index] += 1;
}

void AngularHistogramObserver::save( std::ostream& save_file )
{
    /*! Angular Histogram save file format.
        Header: ang001\\n
        Data type     | Count | Meaning
        ---------     | ----- | -------
        Int [\#H]     | 1     | Number of histograms
        Int    [\#B]  | 1     | Number of bins per histogram
        Double        | \#H   | Histogram times
        Double        | \#B   | Bin angles (radians)
        Double        | \#H   | Sum of angle
        Double        | \#H   | Sum of angle squares
        Int           | \#H * \#B | Angle counts
    */

    // file header
    save_file << "angh001\n";
    writeInteger(save_file, mBinCounts.size());
    writeInteger(save_file, mBinCounts.front().size());
    // write times
    writeFloats(save_file, mTimeIntervals);

    // write angles
    for(unsigned j = 0; j < mBinCounts[0].size(); ++j)
        writeFloat(save_file, j * mAngularBinSize - pi);
    
    // write sum and sum square
    writeFloats(save_file, mSumAngle);
    writeFloats(save_file, mSumSquared);

    for( auto& bins : mBinCounts ) 
    {
        for( auto count : bins)    writeInteger(save_file, count);
    }
    
    
}

std::shared_ptr<ThreadLocalObserver> AngularHistogramObserver::clone() const
{
    return std::make_shared<AngularHistogramObserver>( mTimeIntervals, mAngularBinSize, filename() );
}
