#ifndef DYNAMIC_GRID_BASE_HPP_INCLUDED
#define DYNAMIC_GRID_BASE_HPP_INCLUDED

#include <vector>
#include <functional>
#include "grid_storage.hpp"

// documentation grouping
//! \addtogroup common
//! \{

class MultiIndex;

/*! \brief enum for the different types of transformations for grid access
    \details a transformation describes how an index from Z^n is transformed
            into a memory access to get a grid element.
*/
enum class TransformationType
{
    IDENTITY,        //!< directly access grid elements. Causes invalid memory accesses if
                    //!< the index is out of range [0, S]
    FFT_INDEX,        //!< fft mode access, interpret indices from [-S/2, S/2]
    PERIODIC        //!< periodic access, all indices are converted into [0, S] range via modulo
};


/*! \class DynamicGridBase
    \brief Base class for DynamicGrid, contains all functions that are independent of the template parameter.
    \details This includes saving the grid extents and info, access mode as well as index calculations.
*/
class DynamicGridBase
{
    // no copies
    DynamicGridBase& operator=(const DynamicGridBase& o) = delete;
public:
    // ----------------------
    //         typedefs
    // ----------------------
    typedef std::vector<std::size_t> extents_type;                //!< type that contains the extents of this grid in different dimensions
    using index_fn = size_t(const int*, const std::size_t*);    //!< define index function pointer type

    // ----------------------
    //        constructors
    // ----------------------
    DynamicGridBase(DynamicGridBase&&) = default;
    DynamicGridBase& operator=(DynamicGridBase&&) = default;
    
    /// \brief creates a new gird with extents given in \p sizes.
    DynamicGridBase(const extents_type& sizes, GridStorage data, TransformationType access);

    // ----------------------
    //        info access
    // ----------------------
    /// gets the extents of this grid, i.e. the sizes in the different dimensions
    const extents_type& getExtents() const;
    /// gets the number of dimensions in the grid.
    size_t getDimension() const { return mDimension; }
    /// gets number of elements in the grid
    std::size_t getElementCount() const;

    /// gets the current index transformation
    TransformationType getAccessMode() const;

    /// changes the indexing mode
    void setAccessMode( TransformationType type );

    // ----------------------
    //        element access
    // ----------------------

    /// gets the index offset from an iterator range of index values
    template<class I>
    inline std::size_t getOffsetOf( const I& index ) const;

    /// gets the index offset, using the explicitly specified indexing
    /// class provided in \p A instead of the save function pointer.
    /// faster than getOffsetOf, but at a loss of flexibility.
    /// \note Only use when performance is really necessary. The function pointer version
    /// is typically very fast (faster than switch case dispatch).
    template<class A, class I>
    inline std::size_t getOffsetWithMode( const I& index ) const;

    /// this returns the number of elements, i.e. std::distance( end(), begin() ).
    /// to get the edge length for different dimensions, use getExtents
    /// the same as getElementCount, but provides an interface compatible with std containers
    std::size_t size() const;

    /// returns a newly created multiindex with bounds according to the extents
    /// \note does not take into account the addressing scheme, i.e. lower bound is always 0.
    MultiIndex getIndex() const;

    /// gets the internally used GridStorage container. Normally, this method should not
    ///    be used for anything but debugging and testing.
    const GridStorage& getContainer() const;


    // ----------------------
    //        file io
    // ----------------------
    void dump( std::ostream& out ) const;

protected:
    DynamicGridBase(const DynamicGridBase& dg);

    static extents_type load_info( std::istream& in);

    ///! in this variable, we save the grid data.
    GridStorage mData;
    ///! dimension of the grid.
    size_t mDimension;
    ///! size of the grid. uses a container to allow for different sizes in different dimensions.
    extents_type mExtents;

    ///! the grid transform operation.
    index_fn* mIndexFunction;
    ///! get the transform type
    TransformationType mTrafoType;
};

// ----------------------------------------------------------------------------------------------
//        implementation of inline functions
// ----------------------------------------------------------------------------------------------
/// getOffsetOf is called so often that it needs to be inlined
template<class I>
std::size_t DynamicGridBase::getOffsetOf( const I& index ) const
{
    assert( index.size() == mDimension );
    //! \todo add static asserts to guard against wrong I [contiguous]
    return mIndexFunction( &(index[0]), &(mExtents[0]) );
}

template<class A, class I>
inline std::size_t DynamicGridBase::getOffsetWithMode( const I& index ) const
{
    assert( index.size() == mDimension );
    //! \todo add static asserts to guard against wrong I [contiguous]
    return A::index_of( &index[0], &mExtents[0] );
}

//! \}

#endif // DYNAMIC_GRID_BASE_HPP_INCLUDED
