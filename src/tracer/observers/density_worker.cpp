#include "density_worker.hpp"
#include "interpolation.hpp"

// Configuration constants
const int INITIAL_TRAJECTORY_RESERVE = 1020;
const int INITIAL_QUEUE_RESERVE = 1000;

DensityWorker::DensityWorker(std::vector<std::size_t> size) :
    mDimension( size.size() ),
    mQueue( INITIAL_QUEUE_RESERVE ),
    mQueueSize(0),
    mMaxQueueSize(500),
    mReusePool(0),
    mMutexCount(1),
    mFreeGrids(1),
    mCanCreateGrid( true )
{
    assert( mQueue.is_lock_free() );

    // create first grid and mutex
    mDensities.push_back( grid_type(size, TransformationType::PERIODIC) );
    mWorkMutexes.emplace_back( );
}

DensityWorker::~DensityWorker()
{
    assert( mQueue.empty() );
    assert( mQueueSize == 0 );

    // free memory of reuse pool
    trajectory_type* pointer = nullptr;
    while(mReusePool.pop(pointer))
        delete pointer;

}

// transfer data from Density Observer to DensityWorker
void DensityWorker::push_trajectory(trajectory_type& trajectory)
{
    // new trajectory
    trajectory_type* new_container = nullptr;
    // try to reuse old one, otherwise, we need to allocate a new one
    if( !mReusePool.pop(new_container) )
    {
        new_container = new trajectory_type;
        new_container->reserve( INITIAL_TRAJECTORY_RESERVE );
    }

    assert( new_container->size() == 0);

    // swap content, no memory allocation here, everything fine
    std::swap( *new_container, trajectory );
    // insert into queue, might sometimes allocate but normally lock free
    mQueue.push( new_container );
    ++mQueueSize;

    // check if queue exceeds maximum
    if( mQueueSize > mMaxQueueSize )
    {
        /// \todo is this sufficient to prevent mass spawnings?
        if( checkForDensMem() )
        {
            // try to add another density instance. this might throw in case memory could not be allocated
            try
            {
                // and adjust max queue size so we get some sort of cooldown.
                // maybe this could be solved more elegantly
                mMaxQueueSize += 500;
                // add the new density only after the queue size was adapted to reduce the window for multiple threads to come here with
                // the same code. this means we have to undo the max queue size change in case of an exception
                addLocalDensity();
                //#ifndef NDEBUG
                std::cout << "add density " << mQueueSize << " / " << mMaxQueueSize << " " << mMutexCount <<  " \n";
                //#endif
            } catch (std::bad_alloc& ex)
            {
                // if adding a new observer does not work, we need to make sure that constant trying to add
                mCanCreateGrid = false;
                mMaxQueueSize -= 500;    // do no change queue size in case of exception
            }

        } else
        {
            // let this thread rest until the writers catch up
            while( mQueueSize >= mMaxQueueSize )
                std::this_thread::sleep_for( std::chrono::milliseconds(100) );
        }
    }
}

void drawTrajectory(DynamicGrid<float>& grid, const std::vector<DensityObserver::IPDot>& trajectory)
{
    for( const auto & point : trajectory)
    {
        drawInterpolatedDot(grid, point.pos, point.weight);
    }
}

void DensityWorker::work()
{
    // if no grids are free, don't bother to search
    if(mFreeGrids == 0)
        return;

    for(int free_index = 0; free_index < mMutexCount; ++free_index)
    {
        // try to lock the mutex, and return if another thread is currently consuming trajectories
        std::unique_lock<std::mutex> lock(mWorkMutexes[ free_index ], std::try_to_lock);
        if(! lock.owns_lock() )
            continue;

        --mFreeGrids;    // if we got an exception in the following code, --mFreeGrids would "leak"!
        assert( mFreeGrids >= 0 );
        trajectory_type* trajectory = nullptr;
        grid_type& grid = mDensities[ free_index ];
        while( mQueue.pop(trajectory) )
        {
            --mQueueSize;
            drawTrajectory( grid, *trajectory );

            // allow reuse of that trajectory
            trajectory->clear();
            mReusePool.push(trajectory);
        }

        ++mFreeGrids;

        return;
    }
}

DensityWorker::grid_type& DensityWorker::getDensity()
{
    // check that grids are already combined
    assert( mMutexCount == 1);
    return mDensities[0];
}

void DensityWorker::reduce()
{
    auto& main_grid = mDensities.front();

    // iterate over all grids but the first one
    for(unsigned i = 1; i < mDensities.size(); ++i)
    {
        // add the grids together
        auto& local_grid = mDensities[i];
        for(unsigned j = 0; j < main_grid.size(); ++j)
        {
            main_grid[j] += local_grid[j];
        }
    }

    // remove excess grids and mutexes
    while(mDensities.size() > 1)
        mDensities.pop_back();
    mWorkMutexes.resize(1);
    mMutexCount = 1;
}

void DensityWorker::addLocalDensity()
{
    int old_count = mMutexCount;
    std::lock_guard<std::mutex> lock(mAddDensityMutex);

    // check if we are still allowed to allocate, and that no other process has added a density object yet
    if( !checkForDensMem() || mMutexCount != old_count )
        return;    // silent fail for now
        /// \todo if it fails, the maxQueueCount size should be left unaffected!


    // this might allocate a lot of memory, so chances are that it could fire a bad_alloc exception
    // since push_back is exception save, the mutex will be unlocked and the function will exit with that
    // exception, as desired, so no exception handling needed here
    mDensities.push_back( grid_type( mDensities.front().getExtents(), TransformationType::PERIODIC) );
    mWorkMutexes.emplace_back( );
    ++mMutexCount;    // it is important that this happens after we created the new mutex
    ++mFreeGrids;
}

bool DensityWorker::checkForDensMem()
{
    // early out
    if(!mCanCreateGrid)
        return false;

    // estimate usage
    std::size_t memuse = getBytesInUse();
    // estimate additional size of a new grid
    // safe_product not really necessary, but convenient for code readability
    memuse += safe_product( mDensities[0].getExtents() ) * sizeof(float);
    // check that that maximum memory is not exceeded and that allocation is expected to succeed.
    if( memuse > getMaximumMemoryAvailable() )
    {
        // if we request more, return false and take notice for later
        mCanCreateGrid = false;
        return false;
    }

    // everything OK from a memory standpoint
    return true;
}
