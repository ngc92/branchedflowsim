#ifndef DYNAMIC_GRID_HPP_INCLUDED
#define DYNAMIC_GRID_HPP_INCLUDED

#include <vector>
#include <functional>
#include <boost/lexical_cast.hpp>
#include "grid_storage.hpp"
#include "dynamic_grid_base.hpp"
#include "util.hpp"

/*! \class DynamicGrid
    \brief Class that manages data that is organized in a multidimensional grid.
    \details This class provides an interface for multidimensional grids of arbitrary data types.
            The type genericy means that this class is written template based.
            The memory management, however, is managed by a GridStorage object
            which abstracts all memory access. Furthermore, this allows different
            DynamicGrid objects to point to the same memory, and having different
            access modes.

            The type independent interface is defined in DynamicGridBase. This class mostly
            provides thin wrappers that ensure that the correct types are used.
    \ingroup common
*/
template<class _V>
class DynamicGrid final: public DynamicGridBase
{
    /// no copies
    DynamicGrid& operator=(const DynamicGrid&) = delete;
public:
    // ----------------------
    //       typedefs
    // ----------------------
    typedef DynamicGrid<_V> this_type;                            //!< type that refers to the concrete type of the current template instantiation.
    typedef _V value_type;                                        //!< type of the data that is organized on this grid.
    typedef value_type* pointer_type;                             //!< type of a pointer to data
    typedef const value_type* const_pointer_type;                 //!< type of a const pointer

    // ----------------------
    //      constructors
    // ----------------------
    /// default move
    DynamicGrid(DynamicGrid&&) noexcept = default;
    DynamicGrid& operator=(DynamicGrid&&) noexcept = default;
    
    /// \brief creates a new square grid with a defined number of dimensions.
    /// \details creates a grid with \p dimension dimensions, each extent set to \p size.
    DynamicGrid(size_t dimension, size_t size, TransformationType indexing = TransformationType::IDENTITY) :
        DynamicGrid(std::vector<std::size_t>(dimension, size), indexing)
    {

    }
    
    /*! default constructor, creates grids of size 0. */
    explicit DynamicGrid() : DynamicGrid(1, 0) { }

    /// \brief creates a new gird with extents given in \p sizes.
    explicit DynamicGrid(const extents_type& sizes, TransformationType indexing = TransformationType::IDENTITY) try :
        DynamicGridBase( sizes, GridStorage::create<_V>( safe_product(sizes) ), indexing )
    {

    } catch(std::bad_alloc& ba) {
        std::string shape;
        for(auto size : sizes) {
            shape += boost::lexical_cast<std::string>(size) + ", ";
        }
        THROW_EXCEPTION(std::runtime_error, "Could not create dynamic grid of shape (%1%)", shape);
    }

    // ----------------------
    //        cloning
    // ----------------------

    /// \brief creates a copy of this grid with the same size, data and indexing. The data is
    ///            saved inside a new buffer (i.e. deep copy).
    ///    \sa shallow_copy()
    DynamicGrid clone() const
    {
        DynamicGrid<_V> newgrid( mExtents, mTrafoType );
        // copy data
        std::copy( begin(), end(), newgrid.begin() );
        return newgrid;
    }

    /// \brief creates a shallow copy of this grid with same size, data and indexing.
    /// \details The data pointer of the copy still
    ///             points to the same data as the original grid, i.e. no new memory is
    ///             allocated and changing one grid will change the other (shallow copy).
    /// \sa clone()
    DynamicGrid shallow_copy() const
    {
        return DynamicGrid( *this );
    }

    // ----------------------
    //     element access
    // ----------------------

    /// gets the value pointed to by index
    template<class I>
    value_type& operator()( const I& index )
    {
        return (*this)[ getOffsetOf(index) ];
    }

    /// gets the value pointed to by index
    template<class I>
    const value_type& operator()( const I& index ) const
    {
        return (*this)[ getOffsetOf(index) ];
    }

    /// gets the data from a single, linear index
    value_type& operator[]( std::size_t index ) { return mData.at<value_type>(index); }

    /// gets the data from a single, linear index
    const value_type& operator[]( std::size_t index ) const { return mData.at<value_type>(index); }

    // ----------------------
    //  iteration interface
    //-----------------------
    /// iterator to first element.
    pointer_type begin() { return mData.begin<value_type>(); }
    /// iterator behind last element.
    pointer_type end() { return mData.end<value_type>(); }

    /// const iterator to first element.
    const_pointer_type begin() const { return mData.begin<value_type>(); }
    /// const iterator to last element.
    const_pointer_type end() const { return mData.end<value_type>(); }

    // ----------------------
    //  file io interface
    //-----------------------
    /// loads a dynamic grid from an input stream, typically a binary file.
    static DynamicGrid load( std::istream& in )
    {
        // create new grid
        DynamicGrid grid( load_info(in), TransformationType::IDENTITY );
        // load data into container
        grid.mData.load( in );
        // return
        return std::move(grid);
    }
private:
    /// this c'tor is used for a shallow copy, and therefore only privately available
    DynamicGrid(const DynamicGrid&) = default;
};

/// \addtogroup common
/// \{

/// default grid type: grid of doubles
typedef DynamicGrid<double> default_grid;
/// grid type for FFTs: grid of complex numbers
typedef DynamicGrid<complex_t> complex_grid;

//! \}

#endif // DYNAMIC_GRID_HPP_INCLUDED
