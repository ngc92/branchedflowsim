#include "dynamic_grid_base.hpp"
#include "multiindex.hpp"
#include "global.hpp"
#include "fileIO.hpp"

DynamicGridBase::DynamicGridBase(const extents_type& sizes, GridStorage data, TransformationType mode):
    mData( std::move(data) ),
    mDimension( sizes.size() ),
    mExtents( sizes )
{
    setAccessMode( mode );
}

auto DynamicGridBase::getExtents() const -> const extents_type&
{
    return mExtents;
}

std::size_t DynamicGridBase::getElementCount() const
{
    return mData.size();
}

const GridStorage& DynamicGridBase::getContainer() const
{
    return mData;
}

std::size_t DynamicGridBase::size() const
{
    return mData.size();
}

MultiIndex DynamicGridBase::getIndex() const
{
    MultiIndex index( getDimension() );
    index.setLowerBound(0);
    for(unsigned i = 0; i < mExtents.size(); ++i)
    {
        index.setUpperBoundAt(i, mExtents[i]);
    }
    index.init();
    return index;
}

// copy c'tor to implement shallow copy
DynamicGridBase::DynamicGridBase(const DynamicGridBase& dg) :
    mData( dg.mData.shallow_copy() ),
    mDimension( dg.mDimension ),
    mExtents( dg.mExtents ),
    mIndexFunction( dg.mIndexFunction ),
    mTrafoType( dg.mTrafoType)
{

}

// *******************************************************************
//                            grid access handling
// *******************************************************************


namespace
{
    using index_fn = DynamicGridBase::index_fn;

    // --------------------------------------------------------------------------------
    //                 different indexing functions as hard coded templates
    // --------------------------------------------------------------------------------
    template<int D>
    inline std::size_t IdentityIndexFn(const int* index, const std::size_t* extends)
    {
        std::size_t offset = 0;
        for(int i = 0; i < D; ++i)
            offset = offset * extends[i] + index[i];
        return offset;
    }

    template<int D>
    inline std::size_t FFTIndexFn(const int* index, const std::size_t* extends)
    {
        std::size_t offset = 0;
        for(int i = 0; i < D; ++i)
            offset = offset * extends[i] + (index[i] < 0 ? extends[i] + index[i] : index[i]);
        return offset;
    }
    template<int D>
    inline std::size_t PeriodicIndexFn(const int* index, const std::size_t* extends)
    {
        std::size_t offset = 0;
        for(int i = 0; i < D; ++i)
        {
            auto mod = (index[i] % extends[i]);
            offset = offset * extends[i] + (mod < 0 ? mod + extends[i] : mod);
        }
        return offset;
    }

    // --------------------------------------------------------------------------------
    //                 dispatch to generate the correct function pointer
    // --------------------------------------------------------------------------------

    template<int N>
    inline index_fn* getIndexFnTemplate(TransformationType trafo)
    {
        switch( trafo )
        {
        case TransformationType::IDENTITY:
            return IdentityIndexFn<N>;
        case TransformationType::FFT_INDEX:
            return FFTIndexFn<N>;
        case TransformationType::PERIODIC:
            return PeriodicIndexFn<N>;
        }

        THROW_EXCEPTION( std::logic_error, "requested trafo type %1% does not exist.", (int)trafo );
        __builtin_unreachable();
    }

    inline index_fn* getIndexFn(TransformationType trafo, int dimension)
    {
        switch( dimension )
        {
        case 1:
            return getIndexFnTemplate<1>( trafo );
        case 2:
            return getIndexFnTemplate<2>( trafo );
        case 3:
            return getIndexFnTemplate<3>( trafo );
        case 4:
            return getIndexFnTemplate<4>( trafo );
        case 5:
            return getIndexFnTemplate<5>( trafo );
        case 6:
            return getIndexFnTemplate<6>( trafo );
        }

        THROW_EXCEPTION( std::logic_error, "requested dimension %1% not supported", dimension );
        __builtin_unreachable();
    }
}


void DynamicGridBase::setAccessMode( TransformationType type )
{
    mIndexFunction = getIndexFn(type, mDimension);
    mTrafoType = type;
}

TransformationType DynamicGridBase::getAccessMode() const
{
    return mTrafoType;
}

void DynamicGridBase::dump( std::ostream& out ) const
{
    out.write( "g", 1 );
    writeInteger( out, mDimension );
    for( auto e : mExtents )
        writeInteger(out, e);

    mData.dump( out );
}

auto DynamicGridBase::load_info(std::istream& in) -> extents_type
{
    // check for grid marker
    char temp;
    in.read( &temp, 1 );
    if(temp != 'g')
        THROW_EXCEPTION( std::runtime_error, "file does not contain a grid, or grid identifier is missing" );

    // read extents
    auto dimension = readInteger( in );
    std::vector<std::size_t> extents(dimension);
    for( unsigned i = 0 ; i < dimension; ++i)
        readInteger(in, extents[i]);

    return extents;
}

