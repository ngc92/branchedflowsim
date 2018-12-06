#ifndef MULTIINDEX_HPP_INCLUDED
#define MULTIINDEX_HPP_INCLUDED

/*! \file multiindex.hpp
    \brief defines a multidimensional index type.
    \ingroup common
*/

#include <array>
#include <vector>
#include <cassert>
#include <iosfwd>
#include "global.hpp"

/*! \brief maximum dimension for MultiIndex objects
    \details used to compute how much space to reserve.
    the default value is four, because     1) we need at least 3, but a little room for experimentation can't hurt
                                        2) it might be a good idea to use a POT for alignment help
    By using a static maximum size, we ensure that MultiIndex does not require dynamic allocations, and that all
    memory used by MultiIndex is inside the class, making it cache friendly.

    \ingroup common
*/
constexpr int MAX_MULTIINDEX_DIMENSION = 4;

/*! \class MultiIndex
    \brief Multidimensional index with defined boundaries.
    \details This class is used to iterate an integral index between multidimensional bounds, i.e.
            [x_1, .., x_n] with x_i in [a_i, b_i].
            When counting, the highest index is incremented first, i.e. it is the fastest varying index.

            To use a MultiIndex, first an instance has to be created with the desired dimension,
            then lower and upper bounds have to be set. Afterwards, the \p init() function has
            to be called, which prepares the index to be used.

            The functions that are expected to be used inside hot loops are defined inline to speed things up.
            No dynamically allocated memory is used to make the class more cache friendly. This means that
            there is a compile time imposed limit on the number of dimensions that can be used, which is
            specified in MAX_MULTIINDEX_DIMENSION.

    \todo add more iteration methods +=, -- ?
    \todo test cases
    \ingroup common
*/
class MultiIndex final
{
    typedef int index_type;
    /*! type to save position and bounds. uses a compile time sized array, so all data is on the stack. might
        speed up computations a little because we can be sure memory reads are from cache.
        the slightly bigger class should not be problem.
        \sa MAX_MULTIINDEX_DIMENSION
    */
    typedef std::array<index_type, MAX_MULTIINDEX_DIMENSION> storage_type;
public:
    /// default copy
    MultiIndex(const MultiIndex& ) = default;

    // setup
    /*! \brief constructor
        \details creates a new multi index with the specified dimension, which cannot be changed later.
                initializes upper bound, lower bound and current position to zero. The index is not set
                into valid state until a call to init().
        \param dimensions number of dimensions to index.
    */
    explicit MultiIndex(std::size_t dimensions);

    /*! \brief bounds constructor
        \details creates a multi index and initializes it with the defined bounds. No additional call to init required.
        \param dimensions Number of dimension
        \param lower lower bound
        \param upper upper bound
    */
    MultiIndex(std::size_t dimensions, index_type lower, index_type upper);

    /// default move operation
    MultiIndex(MultiIndex&& o) = default;

    // - - - - - - - - - - - - - - - -
    //    bounds
    // - - - - - - - - - - - - - - - -
    /// sets the lower bound for all positions in this multiindex to the same value \p lb
    /// \exception When called while the index is in use (valid() == true), a std::logic_error is raised
    void setLowerBound( index_type lb );
    /// sets the upper bound for all positions in this multiindex to the same value \p ub
    /// \exception When called while the index is in use (valid() == true), a std::logic_error is raised.
    void setUpperBound( index_type ub );

    /// sets the lower bound for a specific dimension
    /// \exception may throw \p out_of_bounds exception and \p logic_error as in setLowerBound()
    /// \pre 0 <= \p dim < size()
    void setLowerBoundAt( std::size_t dim, index_type lb );

    /// sets the upper bound for a specific dimension
    /// \exception may throw \p out_of_bounds exception and \p logic_error as in setUpperBound(). If you really need
    ///           dynamical upper bounds, use setUpperBoundDynamic().
    /// \pre 0 <= \p dim < size()
    void setUpperBoundAt( std::size_t dim, index_type ub );

    /// sets the upper bound for a specific dimension. This function allows for
    /// changing the iteration bounds at iteration time, making it potentially dangerous.
    /// For a safe alternative during setup time, use setUpperBoundAt.
    void setUpperBoundDynamic( std::size_t dim, index_type ub );

    /// returns a copy of the internal lower bounds array
    storage_type getLowerBound() const;

    /// returns a copy of the internal upper bounds array
    storage_type getUpperBound() const;

    /*! \brief initializes the index for use.
        \details Sets the current position to lower bounds, checks that
                lower bounds < upper bounds and sets the index into valid
                state.
        \exception std::logic_error, if lower bounds > upper bounds.
    */
    void init();

    /// gets the size, i.e. the dimension, of the index.
    std::size_t size() const;

    // - - - - - - - - - - - - - - - - - - - -
    //  fast access functions
    // - - - - - - - - - - - - - - - - - - - -
    /*! gets the \p i'th index of the multiindex. Currently, direct manipulation
        of the index is not allowed, so no non-const, reference returning variant
        is provided.
        \details has to return a const reference (instead of by value), so we can use
                &index[0] to get the address inside this object.
        \pre 0 <= \p i < size()
    */
    inline const index_type& operator[](std::size_t i) const;

    /// tests whether the index is valid, i.e. that is has been init() 'ed and that current pos
    /// does not exceed bounds
    inline bool valid() const;

    // index arithmetic
    /// increment the index by one.
    inline MultiIndex& operator++();
    
    /// increment the index by one. Returns the position if the most significant
    /// index incremented.
    inline std::size_t increment();

    // - - - - - - - - - - - - - - - - - - - -
    //      iteration interface
    // - - - - - - - - - - - - - - - - - - - -
    /// returns pointer to the first index.
    const index_type* begin() const;

    /// returns pointer behind the last index.
    const index_type* end() const;

    // - - - - - - - - - - - - - - - - - - - -
    //            other useful helpers
    // - - - - - - - - - - - - - - - - - - - -

    /// returns the sum of all indices. equals std::accumulate( begin(), end(), 0);
    std::size_t getAccumulated() const;

    /// returns the index as a vector. The vector is newly created, so this
    /// function should not be called often.
    std::vector<index_type> getAsVector() const;

    /// returns a vector of MultiIndices that together cover the same
    /// indices as the original one. Splitting occurs with respect to
    /// the most slowly changing dimension.
    /// \param n Number of sub index objects to split into.
    /// \return A vector with at most \p n entries.
    std::vector<MultiIndex> split(std::size_t n) const;

private:
    // bounds
    storage_type mLowerBound;    //!< saves lower bound
    storage_type mUpperBound;    //!< saves upper bound (\p mUpperBound[i] >= \p mLowerBound[i])

    // position
    storage_type mCurrentPos;    //!< saves current position

    // iterator config
    const std::size_t mDimension;    //!< dimension of the multiindex

    // status
    bool mIsValid;                //!< whether the index is valid \sa valid()

    /// helper function that checks that the index is not in use and otherwise throws an exception
    void checkNotValid();
};

// --------------------------------------------------------------------------------------------------------------
//                     inline implementations
// --------------------------------------------------------------------------------------------------------------

// get i'th index
inline auto MultiIndex::operator[](std::size_t index) const -> const index_type&
{
    // get pos at index
    assert( valid() );
    return mCurrentPos[index];
}


// check if valid
inline bool MultiIndex::valid() const
{
    return mIsValid;
}


// increment
inline MultiIndex& MultiIndex::operator++()
{
    increment();
    return *this;
}

inline std::size_t MultiIndex::increment()
{
    assert( valid() );

    // start with highest dimension and decrease
    // when i would fall below zero, underflow occurs (unsigned) and i > mDimension, so the
    // loop is left.
    for( std::size_t i = mDimension-1; i < mDimension ; --i)
    {
        mCurrentPos[i]++;
        if( mCurrentPos[i] < mUpperBound[i] )
            return i;
        else
            mCurrentPos[i] = mLowerBound[i];
    }

    mIsValid = false;

    return 0;
}

// io
std::ostream& operator<<(std::ostream& out, const MultiIndex& index);

#endif // MULTIINDEX_HPP_INCLUDED
