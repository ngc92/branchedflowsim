#include "multiindex.hpp"
#include "global.hpp"
#include <cassert>
#include <numeric>
#include <stdexcept>
#include <algorithm>
#include <boost/throw_exception.hpp>

// constructor
MultiIndex::MultiIndex(std::size_t dims) :
    mDimension(dims),
    mIsValid(false)
{
    if( mDimension > MAX_MULTIINDEX_DIMENSION )
    {
        THROW_EXCEPTION( std::logic_error,
                        "requested multiindex dimension %1% exceeds maximum possible dimension %2%",
                        dims, MAX_MULTIINDEX_DIMENSION );
    }
}

MultiIndex::MultiIndex(std::size_t dimensions, index_type lower, index_type upper):
    mDimension( dimensions ),
    mIsValid( false )
{
    setLowerBound( lower );
    setUpperBound( upper );
    init();
}

// get size
std::size_t MultiIndex::size() const
{
    return mDimension;
}

void MultiIndex::checkNotValid()
{
     if( valid() )
        THROW_EXCEPTION( std::logic_error, "trying to change MultiIndex bounds while index might be in use" );
}

// set lower bound
void MultiIndex::setLowerBound( index_type lb )
{
    checkNotValid();
    mLowerBound.fill( lb );
}

void MultiIndex::setLowerBoundAt( std::size_t dim, index_type lb )
{
    checkNotValid();
    mLowerBound.at( dim ) = lb;
}

// set upper bound
void MultiIndex::setUpperBound( index_type ub )
{
    checkNotValid();
    mUpperBound.fill( ub );
}

void MultiIndex::setUpperBoundAt( std::size_t dim, index_type ub )
{
    checkNotValid();
    mUpperBound.at( dim ) = ub;
}

void MultiIndex::setUpperBoundDynamic(std::size_t dim, MultiIndex::index_type ub)
{
    assert(valid());
    if( mLowerBound.at(dim) >= ub )
    {
        THROW_EXCEPTION(std::logic_error, "multi index lower bound %1% for %2% exceeds upper bound %3%",
                        mLowerBound[dim], dim, mUpperBound[dim]);
    }
    if( ub <= mCurrentPos.at(dim))
    {
        THROW_EXCEPTION(std::logic_error, "cannot set upper bound %1% for %2% as current index value is %3%",
                        mUpperBound[dim], dim, mCurrentPos.at(dim));
    }
    mUpperBound.at(dim) = ub;
}


// get bounds
MultiIndex::storage_type MultiIndex::getLowerBound() const
{
    return mLowerBound;
}

MultiIndex::storage_type MultiIndex::getUpperBound() const
{
    return mUpperBound;
}

// iterator functions, just use std::array iterators
const MultiIndex::index_type* MultiIndex::begin() const
{
    return mCurrentPos.begin();
}

const MultiIndex::index_type* MultiIndex::end() const
{
    auto it = mCurrentPos.begin();
    std::advance(it, mDimension);
    return it;
}

std::size_t MultiIndex::getAccumulated() const
{
    return std::accumulate( begin(), end(), 0u );
}

std::vector<int> MultiIndex::getAsVector() const
{
    return std::vector<index_type>( begin(), end() );
}

std::vector<MultiIndex> MultiIndex::split(std::size_t n) const
{
    if(n == 0)
        THROW_EXCEPTION(std::domain_error, "Cannot split into zero parts.");

    std::vector<MultiIndex> result;
    result.reserve(n);

    int last_boundary = mLowerBound[0];
    int range = mUpperBound[0] - mLowerBound[0];
    for(unsigned i = 0; i < n; ++i)
    {
        result.push_back(*this);
        result.back().mIsValid = false;
        result.back().setLowerBoundAt( 0, last_boundary);
        int next = int(i+1) * range / (int)n + mLowerBound[0];
        result.back().setUpperBoundAt( 0, next);

        // if the range is so small that we did not get an increase, do not produce a new multi index for
        // that.
        if(last_boundary == next)
        {
            // remove again
            result.pop_back();
        } else {
            last_boundary = next;
            result.back().init();
        }
    }

    return std::move(result);
}

// init index
void MultiIndex::init()
{
    checkNotValid();

    // check that lower < upper
    for(unsigned i = 0; i < mDimension; ++i)
    {
        if( mLowerBound[i] >= mUpperBound[i] )
        {
            THROW_EXCEPTION(std::logic_error, "multi index lower bound %1% for %2% exceeds upper bound %3%",
                            mLowerBound[i], i, mUpperBound[i]);
        }
    }

    mCurrentPos = mLowerBound;

    mIsValid = true;
}

std::ostream& operator<<(std::ostream& out, const MultiIndex& index)
{
    out << "MultiIndex(";
    for(auto val : index)
    {
        out << val << ", ";
    }
    return out << ")";
}
