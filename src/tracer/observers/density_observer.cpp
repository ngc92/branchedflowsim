#include "density_observer.hpp"
#include "interpolation.hpp"
#include "fileIO.hpp"
#include "density_worker.hpp"
#include "initial_conditions/initial_conditions.hpp"

// ---------------------------------------------------------------------------------------------------------

// initializes as non slave
DensityObserver::DensityObserver( std::vector<std::size_t> size, std::vector<double> support,
                    std::string file_name, bool re_center, std::function<float(const State&)> extractor ) :
        DensityObserver(std::move(size), std::move(support),
        std::move(file_name), re_center,
        std::move(extractor), std::shared_ptr<DensityWorker>())
{
}

DensityObserver::DensityObserver( std::vector<std::size_t> size, std::vector<double> support,
                                std::string file_name, bool re_center,
                                std::function<float(const State&)> extractor,
                                std::shared_ptr<DensityWorker> worker) :
        ThreadLocalObserver( std::move(file_name) ),
        mDimension( size.size() ),
        mDPIFactor( 1 ),
        mScalingFactor( size.size() ),
        mSupport( support ),
        mSize( size ),
        mLastTime(10),
        mLastPosition( size.size() ),
        mWorker( worker ? worker : std::make_shared<DensityWorker>(size)),
        mExtractFunction ( std::move(extractor) ),
        mCenterOnStart( re_center ),
        mStartingPosition( size.size() )
{
    assert( size.size() == mDimension );
    assert( support.size() == mDimension );

    /// \todo why initialise mLastTime to 10?
    // scale by pixel real size
    for(unsigned i = 0; i < mDimension; ++i)
    {
        mScalingFactor[i] = size[i] / support[i];
        mDPIFactor *= mScalingFactor[i];
    }
}

void DensityObserver::endTracing(std::size_t particle_count)
{
    // finalize the worker
    mWorker->reduce();

    // scale numbers to particle count
    scaleVectorBy( mWorker->getDensity(), 1.0 / particle_count );
}

void DensityObserver::startTrajectory(const InitialCondition& incoming, std::size_t)
{
    mDotCache.clear();
    // remember starting point in case we need to re-center.
    mStartingPosition = incoming.getState().getPosition();
}

void DensityObserver::endTrajectory(const State& final_state)
{
    mWorker->push_trajectory( mDotCache );
    assert( mDotCache.size() == 0);    // get a new, empty cache
    mWorker->work();
}

bool DensityObserver::watch( const State& state, double time )
{
    gen_vect new_position = state.getPosition();
    if(mCenterOnStart)
        new_position -= mStartingPosition;
    /// \todo we can skip one multiplication with mImage size if we cache the result in mLastPosition
    /// we overwrite mLastPosition in after this use, so we can skip creating a copy
    gen_vect temp(state.getDimension());
    for(unsigned i = 0; i < mDimension; ++i)
    {
        mLastPosition[i] *= mScalingFactor[i];
        temp[i] = new_position[i] * mScalingFactor[i];

        if(mCenterOnStart)
        {
            mLastPosition[i] += mSupport[i]/2 * mScalingFactor[i];
            temp[i] += mSupport[i]/2 * mScalingFactor[i];
        }

        // if we leave the support, stop this trajectory
        if(temp[i] < 0 || temp[i] >= mSize[i])
            return false;
    }
    // draw scaled line
    float weight = mExtractFunction( state );
    if(time > mLastTime)
        addInterpolatedLine(mLastPosition, temp, (time - mLastTime)*weight );

    // update data
    mLastTime = time;
    mLastPosition = new_position;
    return true;
}

void DensityObserver::save(std::ostream& target)
{
    /*! Density save file format.
        Header: dens001\\n
        Data type   | Count | Meaning
        ---------   | ----- | -------
        Int [D]     | 1     | Number of dimensions
        Double      | D     | support
        Grid[Float] | 1     | grids with density data
    */

    // file header
    target << "dens001\n";
    writeInteger(target, mDimension);
    writeFloats(target, mSupport);
    getDensity().dump(target);
}

const DensityObserver::density_grid_type& DensityObserver::getDensity() const
{
    return mWorker->getDensity();
}

std::shared_ptr<ThreadLocalObserver> DensityObserver::clone() const
{
    return std::make_shared<DensityObserver>( mWorker->getDensity().getExtents(), mSupport, filename(),
                                              mCenterOnStart, mExtractFunction, mWorker );
}

void DensityObserver::combine( ThreadLocalObserver& )
{

}

// -----------------------------------------------------------------------------------------------------
void DensityObserver::addInterpolatedLine( const gen_vect& start, const gen_vect& end, double weight )
{
    double len = 0;
    for( unsigned i = 0; i < start.size(); ++i)
        len += (end[i] - start[i])*(end[i] - start[i]);
    len = std::sqrt(len);

    int pcount = std::max(1, (int)(len * 3));    // 3 subpixels

    // increase weight by 1 / pixel size to normalize density and make it independent of image size
    double dpi = weight / pcount * mDPIFactor;

    /// \todo this is way to slow
    for(int substep = 0; substep < pcount; substep++)
    {
        mDotCache.push_back( IPDot{interpolate_linear_1d(start, end, (substep + 0.5) / pcount), dpi} );
    }
}
