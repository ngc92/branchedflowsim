#include "profiling.hpp"
#include <atomic>
#include <iostream>
#include <unordered_set>
#include <vector> // for dummy max_size

//                Timing
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using namespace std::chrono;

ScopeTimer::ScopeTimer( ProfileRecord& target ) : mTarget(target), mStartTime( clock_t::now() )
{
}

ScopeTimer::~ScopeTimer()
{
    mTarget.record(getTiming());
}

unsigned ScopeTimer::getTiming() const
{
    auto duration = clock_t::now() - mStartTime;
    auto ms = duration_cast<milliseconds>(duration);
    return ms.count();
}

ProfileRecord::ProfileRecord( std::string&& name ) : mName( name )
{
    addRecord(this);
}

// internal map to save all records
std::unordered_set<const ProfileRecord*> profile_records;

void ProfileRecord::addRecord( const ProfileRecord* rec )
{
    profile_records.insert(rec);
}

void ProfileRecord::print_profiling_data()
{
    for( auto& rec : profile_records )
    {
        std::cout << rec->getName() << ": " << rec->getCallCount() << "x" << rec->getCallTime() << " =\t" << rec->getTotalTime() << "ms\n";
    }
}

//                 memory profiling
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/// current amount of memory in use. atomic, so multiple threads can simultaneously profile
/// allocations and deallocations.
std::atomic<std::size_t> profile_memory_use(0);

/// maximum amount of memory that could be allocated in a single vector [real max might be much lower]
std::size_t max_mem_avail = std::vector<char>().max_size();

void profile_allocate( std::size_t bytes )
{
    profile_memory_use += bytes;
    //std::cout << profile_memory_use / 1024 / 1024  << " MB / ";
    //std::cout << getMaximumMemoryAvailable() / 1024 / 1024  << " MB\n";
}

void profile_deallocate( std::size_t bytes )
{
    profile_memory_use -= bytes;
}

std::size_t getBytesInUse()
{
    return profile_memory_use;
}

std::size_t getMaximumMemoryAvailable()
{
    return max_mem_avail;
}

void setMaximumMemoryAvailable( std::size_t mem )
{
    max_mem_avail = mem;
}
