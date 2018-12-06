#ifndef PROFILING_HPP_INCLUDED
#define PROFILING_HPP_INCLUDED

/*! \file profiling.hpp
    \ingroup common
    \brief This file provides several utilities for profiling code performance and memory consumption.
*/

#include <boost/noncopyable.hpp>
#include <chrono>
#include <string>
#include <atomic>

class ProfileRecord;

/*! \class ScopeTimer
    \ingroup common
    \brief Allows to measure the time spent in a certain scope, i.e. a function.
    \details Counts the milliseconds between creating and destruction of an instance
            and adds to a designated counter. Should not be used inside the innermost
            functions to prevent measuring overhead, but should be a good tool to
            estimate time in sub-algorithms whose calculation time is in the millisecond scale.
*/
class ScopeTimer final : public boost::noncopyable
{
    /// clock type used to measure time. high_resolution_clock to be able to capture small time differences.
    typedef std::chrono::high_resolution_clock clock_t;
public:
    /// create the timer, and add passed time (in milliseconds) to target upon destruction, i.e. scope exit
    ScopeTimer( ProfileRecord& target );
    /// stop measuring time and add to target
    ~ScopeTimer();

    /// gets the time this timer has measured
    unsigned getTiming() const;
private:
    /// this is the counter to which the passed time will be added
    ProfileRecord& mTarget;
    /// save the time point when the ScopeTimer was created as reference point.
    clock_t::time_point mStartTime;
};

/*! \class ProfileRecord
    \ingroup common
    \brief class for storing profile data.
    \details This class is used to store profiling data. This includes total running time and number of calls.
            The variables are atomic, so that profiling can be performed thread safe.
*/
class ProfileRecord final : public boost::noncopyable
{
    friend class ScopeTimer;
public:
    ProfileRecord( std::string&& name );

    /// gets the total time this record counted
    unsigned getTotalTime() const { return mTotalTime; };
    /// gets the number of times the record was invoked
    unsigned getCallCount() const { return mCallCount; };

    /// gets the average time per call
    double getCallTime() const { return (double)mTotalTime / mCallCount; };

    // info

    /// gets the record's name
    const std::string& getName() const { return mName; };

    // manage collection of all profile records
    static void print_profiling_data();

private:
    // counts the time
    inline void record( unsigned ms )
    {
        mTotalTime += ms;
        ++mCallCount;
    }

    // manage collection
    static void addRecord( const ProfileRecord* rec );

    /// total time in ms
    std::atomic<unsigned> mTotalTime = {0u};
    /// total call count
    std::atomic<unsigned> mCallCount = {0u};

    // info
    std::string mName;
};

#define PROFILE_BLOCK_INTERNAL(name, file, line) static ProfileRecord profrec##file##line(name); ScopeTimer timer##file##line(profrec##file##line);
#define PROFILE_BLOCK(name) PROFILE_BLOCK_INTERNAL(name, __FILE__, __LINE__);

// documentation grouping
//! \addtogroup common
//! \{

// memory profile info
/// call this function whenever a big memory allocation is performed to keep approximate track of used memory
void profile_allocate( std::size_t bytes );
/// calls this function when memory which was registered with profile_allocate is freed again.
void profile_deallocate( std::size_t bytes );

/// gets an estimate for the total amount of bytes allocated now
/// since only grid allocations are tracked by profiling right now,
/// only that memory is taken into account, but it should comprise
/// the majority of memory use for big potentials
std::size_t getBytesInUse();

// maximum memory available
/// gets the maximum memory available to the programme, either a physical (i.e. address space for 32 bit)
/// or a user imposed limit.
std::size_t getMaximumMemoryAvailable();

/// set a user imposed limit on how much memory to alloced.
/// \note this is not a strict limit, but rather a guideline. Certain parts of the code won't perform
///       additional, optional allocations (which might speed up computations)
///       if that meant exceeding this limit.
void setMaximumMemoryAvailable( std::size_t mem );

//! \}

#endif // PROFILING_HPP_INCLUDED
