#include "trajectory_observer.hpp"
#include "fileIO.hpp"


TrajectoryObserver::TrajectoryObserver( double interval, std::string file_name ) :
        ThreadLocalObserver( std::move(file_name) ),
        mInterval( interval )
{
}

TrajectoryObserver::~TrajectoryObserver() = default;

void TrajectoryObserver::combine(ThreadLocalObserver& other)
{
    auto& data = dynamic_cast<TrajectoryObserver&>( other );
    // move all caustic positions to root observer
    std::move(    data.mTrajectorySamples.begin(),
                data.mTrajectorySamples.end(),
                std::back_inserter(mTrajectorySamples)    );

    // use mParticleNumber to gather total particle count
    mParticleNumber = std::max(mParticleNumber, data.mParticleNumber);
}

void TrajectoryObserver::startTrajectory( const InitialCondition&, std::size_t trajectory )
{
    mLastTime = -1; // set to -1, so t=0 gets recorded
    mParticleNumber = trajectory;
}

bool TrajectoryObserver::watch( const State& state, double t )
{
    if( t > mLastTime + mInterval )
    {
        mTrajectorySamples.emplace_back( mParticleNumber, state.getPosition(),
                                        state.getVelocity(), t);
        mLastTime = t;
    }
    return true;
}

void TrajectoryObserver::save(std::ostream& target)
{
    /// save file format: header, dimension, max number, num samples, data

    // file header
    /// \todo if no caustics found, this throws!
    // this saves the dimension
    target << "traj001\n";
    writeInteger(target, mTrajectorySamples.at(0).pos.size());
    writeInteger(target, mParticleNumber );
    writeInteger(target, mTrajectorySamples.size() );

    std::cout << "SAVING " << mTrajectorySamples.size() << " trajectory points\n";

    for(const auto& c : mTrajectorySamples)
    {
        writeInteger(target, c.trajectory );
        // write position
        writeFloats(target, c.pos);
        writeFloats(target, c.vel);
        writeFloat(target, c.time );
    }
}

std::shared_ptr<ThreadLocalObserver> TrajectoryObserver::clone() const
{
    return std::make_shared<TrajectoryObserver>( mInterval, filename() );
}
