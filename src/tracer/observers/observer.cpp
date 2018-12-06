#include "observer.hpp"
#include "global.hpp"
#include <fstream>
#include <cstring>


Observer::Observer(std::string file_name) : mFileName( std::move(file_name) )
{
}


void Observer::setFileName(std::string path)
{
    mFileName = std::move(path);
}

const std::string Observer::filename() const {
    return mFileName;
}

void Observer::init(std::shared_ptr<const RayDynamics> dynamics)
{
    mDynamics = std::move(dynamics);
    mIsInitialized = true;
}

bool Observer::is_ready() const {
    return mIsInitialized;
}

// ---------------------------------------------------------------------------------------------------
//                                 Thread Local Observer
// ---------------------------------------------------------------------------------------------------

ThreadLocalObserver::ThreadLocalObserver(std::string file_name) :
        Observer(std::move(file_name)),
        mRootObserver( nullptr )
{

}

ThreadLocalObserver::~ThreadLocalObserver()
{
    // check that we merged.
    if(mRootObserver != nullptr)
    {
        std::cerr << "A thread local observer was deleted before being merged back into its root observer.";
        std::terminate();
    }
}

bool ThreadLocalObserver::is_slave() const {
    return mRootObserver != nullptr;
}

std::shared_ptr<Observer> ThreadLocalObserver::makeThreadCopy()
{
    static std::mutex protection_mutex;
    std::lock_guard<std::mutex> lock(protection_mutex);

    // create a cloned observer and set this as its root.
    auto new_observer = clone();
    new_observer->mRootObserver = std::dynamic_pointer_cast<ThreadLocalObserver>(shared_from_this());

    // now this object has become the root of another observer, so we need to set the root mutex
    if( !mRootMutex )
        mRootMutex.emplace();

    return new_observer;
}

void ThreadLocalObserver::reduce(  )
{
    // copy data to main
    if( mRootObserver )
    {
        // lock the root observer
        std::lock_guard<std::mutex> lock(*(mRootObserver->mRootMutex));
        mRootObserver->combine( *this );
        mRootObserver = nullptr;
    }
}

bool ThreadLocalObserver::is_root() const {
    return (bool)mRootMutex;
}


// ----------------------------------------------------------------------------------------------------
//                                Thread Shared Observer
// ----------------------------------------------------------------------------------------------------

std::unique_lock<std::mutex> ThreadSharedObserver::getLock()
{
    return std::unique_lock<std::mutex>(mMutex);
}

std::shared_ptr<Observer> ThreadSharedObserver::makeThreadCopy()
{
    return shared_from_this();
}

ThreadSharedObserver::ThreadSharedObserver(std::string file_name) :
        Observer(std::move(file_name)) {

}
