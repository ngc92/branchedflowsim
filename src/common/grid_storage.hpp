#ifndef GRID_STORAGE_HPP_INCLUDED
#define GRID_STORAGE_HPP_INCLUDED

/*! \file grid_storage.hpp
    \ingroup common
    \brief Data container for grid types.
    \details Contains GridStorage as well as the necessary helper classes that have to
            be implemented as inline templates. This file is mostly for internal use
            in the DynamicGrid classes, i.e. inside the common module.
*/

#include <memory>
#include <vector>
#include <cassert>
#include "profiling.hpp"
#include <typeindex>
#include <boost/align/aligned_allocator.hpp>

/*! \class GridStorage
    \brief class that manages memory for grids.
    \details this class hides the details of memory management of the DynamicGrid class. It is
            coded as a non-template class that provides template access to different type specific methods.
            In DEBUG mode types can be checked, in RELEASE mode the class works as fast as possible.
            Aside from direct access via an at function, it also provides an iterator interface by means of
            templated begin and end functions which return the correct pointers. This works because internally
            the data is saved as a continuous vector.

            The implementation abstracts the allocated type by using a polymorphic internal class. The
            polymorph functions are only used in memory management, so they do not affect the speed of data
            lookups and writes.

    \note    It is currently not possible to change the amount of data allocated, but that should rarely be
            helpful anyway.
    \ingroup common
*/
class GridStorage
{
public:
    /*! \brief Generator function for GridStorage classes for objects of type \p T.
        \details This function is only necessary because we cannot make the constructor
                    a template function where no argument depends in the template
                    parameter.
        \note This justs forwards to the generic constructor, adding a dummy pointer to
                resolve template deduction.
    */
    template<class T, class... Args>
    static GridStorage create(Args... args);

    /// this allows moving grids. Just switches internal pointers, thus very fast.
    GridStorage( GridStorage&& ) = default;

    GridStorage& operator=(GridStorage&&) = default;
    
    // --------------------------------------------------------------------
    //  direct access functions
    // -------------------------

    /*! returns a reference to the object at the offset \p offset
        in the encapsulated array. In DEBUG mode, it also checks that
        the sizes of T and the saved objects match.
        \param offset: offset into the array, in increments of sizeof(T)
    */
    template<class T>
    inline T& at( std::size_t offset );

    /// \brief const version of at(std::size_t)
    template<class T>
    inline const T& at( std::size_t offset ) const;

    // ---------------------------------------------------------------------
    //  iterator interface
    // --------------------

    /// const pointer to the first element
    template<class T>
    const T* begin() const;

    /// const pointer to the last element
    template<class T>
    const T* end() const;

    /// pointer to the first element
    template<class T>
    T* begin();
    /// pointer to the last element
    template<class T>
    T* end();

    // ------------------------------------
    //  info functions
    // ----------------

    /// gets the starting address of the memory where the data is saved. == begin<void>
    void* getStartingAddress() const;

    /// gets the size of the saved objects, i.e. the difference between subsequent data elements
    std::size_t getStride() const;

    /// gets the type saved inside this object
    std::type_index getType() const;

    /// get the size (i.e. the number of elements) of this storage object.
    std::size_t size() const;

    // -------------------------------------------
    //  shallow copy, i.e. points to same memory
    // -------------------------------------------
    /*! \brief Creates a shallow copy of this storage.
        \details Constructs a new GridStorage object
                that refers to the same data. An internal
                reference counter makes sure the data is
                only deleted when all referring GridStorage
                objects are deleted.
    */
    GridStorage shallow_copy() const;

    // -------------------------------------------
    //    file io
    // -------------------------------------------
    /// dumpy the containers contents
    void dump( std::ostream& out ) const;

    /// read a binary dump of container contents. This
    /// only works if the dump contains the same amount of
    /// elements of the same type as this container.
    void load( std::istream& in );

private:
    /// constructor for GridStorage for data of type T
    /// needs the second dummy parameter to deduce the type T, the value of \p type is never used.
    template<class T>
    GridStorage( std::size_t size, T* type );

    /// shallow copy ctor
    /// default implementation works, because we are using a smart ptr for implementation details.
    GridStorage( const GridStorage& /*other*/ ) = default;

    // implementation hiding internal type manage data
    /// \brief helper struct that contains the type specific data management
    struct Impl;
    std::shared_ptr<Impl> mDataRecord;    //!< the implementation details are hidden behind this pointer.

    // data variables
    void* mStartAddress = nullptr;        //!< address to the memory for the data
    std::size_t mStride = 0;              //!< size of a single data element
    std::size_t mSize = 0;                //!< number of elements

    // type info data, might help debugging and needed for file io
    std::type_index mType;                //!< type saved inside this grid-
};

// -------------------------------------------------------------------------------------------------------
//        implementation of template functions
// -------------------------------------------------------------------------------------------------------

template<class T, class... Args>
GridStorage GridStorage::create(Args... args)
{
    // this is necessary because a constructor cannot have a template argument that has no influence on its parameters,
    // so we add a fake nullptr to convey the type information.
    return GridStorage( args..., (T*)nullptr );
}

template<class T>
inline T& GridStorage::at( std::size_t offset )
{
    // precondition checks
    assert( sizeof(T) == mStride );
    assert( offset < mSize );

    // use a reinterpret_cast and an array access to automatically get the right object.
    return reinterpret_cast<T*>(mStartAddress)[offset];
}

template<class T>
inline const T& GridStorage::at( std::size_t offset ) const
{
    // precondition checks
    assert( sizeof(T) == mStride );
    assert( offset < mSize );

    return reinterpret_cast<T*>(mStartAddress)[offset];
}

//  iterator interface
template<class T>
const T* GridStorage::begin() const
{
    return &at<T>(0);
}

template<class T>
const T* GridStorage::end() const
{
    assert( sizeof(T) == mStride );
    return (const T*)mStartAddress + mSize;
}

template<class T>
T* GridStorage::begin()
{
    return &at<T>(0);
}

template<class T>
T* GridStorage::end()
{
    assert( sizeof(T) == mStride );
    return (T*)mStartAddress + mSize;
}

// -------------------------------------------------------------------------------------------------------
//        helper classes
// -------------------------------------------------------------------------------------------------------

/*! \class GridStorage::Impl
    \brief interface of implementation detail class for GridStorage.
    \details This class defines the interface boundary between the templated implementation of the storage StorageImpl
                and the non-template interface GridStorage.
            It provides only three methods, for getting the address of the memory, the size of a single object, and one for
                resizing the memory region.
*/
struct GridStorage::Impl
{
    /// virtual dtor so derived classes can be used polymorphically
    virtual ~Impl() = default;

    /// gets the start address of the reserved memory area
    virtual void* getStartAddress() const = 0;

    /// gets the size of a single object
    virtual std::size_t getStride() const = 0;

    /// changes the size of the internal memory, and returns the new starting address.
    virtual void* resize( std::size_t count ) = 0;
};


namespace detail
{
    /*! \class StorageImpl
        \brief class that manages the memory for the GridStorage object
        \details Implements the GridStorage::Impl interface for a concrete stored type.
                The data is saved inside a vector with 32 byte alignment (so simd possible).
                The class performs memory profiling, i.e. it records how much memory it allocates.
        \todo maybe hide this inside another header, only to be included when the grid storage
                creation is actually needed.
    */
    template<class T>
    struct StorageImpl final : public GridStorage::Impl
    {
        /// alignment specification
        constexpr static int DataAlignment = 64;

        /// d'tor, performs memory logging
        ~StorageImpl() override {
            profile_deallocate( mStorage.capacity()* sizeof(T) );
        }

        /// data vector that actually contains the memory
        std::vector<T, boost::alignment::aligned_allocator<T, DataAlignment>> mStorage;

        /// implementation of getStartAddress, returns address of first vector element
        void* getStartAddress() const override
        {
            return const_cast<T*>(&mStorage.front());
        }

        /// gets size of a saved object, == sizeof(T)
        std::size_t getStride() const override
        {
            return sizeof(T);
        }

        /// resize memory, based on vector<T>::resize
        void* resize( std::size_t new_size ) override
        {
            std::size_t old_cap = mStorage.capacity();
            mStorage.resize( new_size );
            // as capacity can only grow, this is always positive
            profile_allocate( (mStorage.capacity() - old_cap) * sizeof(T) );
            return getStartAddress();
        }
    };
}

// implementation
template<class T>
GridStorage::GridStorage( std::size_t size, T* /*type*/ ) :
    mDataRecord( std::make_shared<detail::StorageImpl<T>>() ),
    mSize( size ),
    mType( typeid( T ) )
{
    assert( mDataRecord );
    mStartAddress = mDataRecord->resize( size );
    mStride = mDataRecord->getStride();
}

#endif // GRID_STORAGE_HPP_INCLUDED
