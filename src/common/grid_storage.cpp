#include "grid_storage.hpp"
#include "fileIO.hpp"
#include "global.hpp"
#include <ostream>
#include <cstring>

void* GridStorage::getStartingAddress() const
{
    return mStartAddress;
}

std::size_t GridStorage::getStride() const
{
    return mStride;
}

std::type_index GridStorage::getType() const
{
    return mType;
}

std::size_t GridStorage::size() const
{
    return mSize;
}

GridStorage GridStorage::shallow_copy() const
{
    return GridStorage(*this);
}

// write to file
void GridStorage::dump( std::ostream& out ) const
{
    PROFILE_BLOCK("grid storage dump");
    // size + 1 to write trailing \0
    out.write( mType.name(), std::strlen(mType.name())+1 );
    writeInteger( out, size() );
    out.write((char*)getStartingAddress(), getStride() * size());
}

// read from file
void GridStorage::load( std::istream& in )
{
    PROFILE_BLOCK("grid storage load");
    std::string type;
    // read count and type id
    std::getline(in, type, (char)0);
    auto count = readInteger( in );

    if(type != mType.name())
        THROW_EXCEPTION(std::runtime_error,
                        "binary reading of incompatible data : expected %1%, got %2%",
                        mType.name(), type);

    if( count != size() )
        THROW_EXCEPTION( std::runtime_error, "number of data elements %1% does not match container size %2%", (long)count, (long)size());

    in.read( (char*)getStartingAddress(), getStride() * size() );
}
