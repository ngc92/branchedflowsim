#ifndef DENSITY_WORKER_HPP_INCLUDED
#define DENSITY_WORKER_HPP_INCLUDED

#include "density_observer.hpp" // for IPDot
#include <boost/lockfree/queue.hpp>
#include <thread>
#include <atomic>
#include <deque>

// helper worker class
/*! \class Worker
    \brief Density Observer Worker class
    \details This class performs the actual writing of data into the shared grid.
*/
class DensityWorker final
{

typedef DynamicGrid<float> grid_type;
typedef std::vector<DensityObserver::IPDot> trajectory_type;
public:
    DensityWorker(std::vector<std::size_t> size);
    ~DensityWorker();

    // get the final density
    grid_type& getDensity();

    // this collects all density instances into one
    void reduce();

    void push_trajectory( trajectory_type& trajectory);
    /// this function consumes one trajectory from the queue
    void work();
private:
    /*! adds a new thread local density object and corresponding mutex, if requesting more
        memory is allowed.

        \exception std::bad_alloc, if no new grid could be created
    */
    void addLocalDensity() noexcept(false);

    /// this function checks if the system is allowed to allocate more memory for
    /// an additional memory grid.
    bool checkForDensMem();

    std::size_t mDimension;

    // queue with all the dot arrays
    boost::lockfree::queue<trajectory_type*> mQueue;
    std::atomic<int> mQueueSize;
    std::atomic<int> mMaxQueueSize;    // maximum queue size until a new copy of density data is created
    // pool of trajectories so we do not need to allocate new memory every time we
    boost::lockfree::queue<trajectory_type*> mReusePool;

    // data grids and mutexes

    // save density data array and access grid ...
    // we need data type deque here because we might add new densities in another thread an cannot afford to
    // invalidate old pointers
    std::deque<grid_type> mDensities;
    // ...and corresponding mutexes
    std::deque<std::mutex> mWorkMutexes;
    std::mutex mAddDensityMutex;    //!< this mutex is locked whenever the above containers are modified

    /// since std::deque is not thread safe, we use this variable to keep track of
    ///    mWorkMutexes and mDensities.size in a reliable manner. it is always guaranteed
    ///  to be smaller or equal to the actual size.
    std::atomic<int> mMutexCount;

    /// this counter stores the number of density write objects that are currently not processed by any thread
    std::atomic<int> mFreeGrids;

    // this variable tracks whether new local observers can be added. it is assumed that once the creation of one
    // fails due to a bad_alloc, system memory is used up and density grids can no longer be created
    std::atomic<bool> mCanCreateGrid;
};

#endif // DENSITY_WORKER_HPP_INCLUDED
