#ifndef FILEIO_HPP_INCLUDED
#define FILEIO_HPP_INCLUDED

/*! \file fileIO.hpp
    \brief Helper functions for binary file I/O.
    \ingroup common
*/

#include <fstream>
#include <type_traits>
#include <boost/numeric/conversion/cast.hpp>

#include "vector.hpp" // for type gen_vect

// documentation grouping
//! \addtogroup common
//! \{

// ------------------------------------------------------------------------------------------------------
//                                    simple output functions
// ------------------------------------------------------------------------------------------------------

/// writes an integer to the file. always writes as 64 bit.
template<class T>
void writeInteger(std::ostream& file, T value)
{
    static_assert( std::is_integral<T>::value, "Write integer only supports integral types" );
    uint64_t fixed_size = value;
    file.write( (char*)&fixed_size, sizeof(fixed_size) );
}

/// reads an integer from the file and returns it. Always assumes 64 bit
std::uint64_t readInteger( std::istream& file );

/// reads an integer from file an converts it to type \p T. The integer in the file
/// is assumed to be 64 bit.
template<class T>
void readInteger(std::istream& file, T& value)
{
    static_assert( std::is_integral<T>::value, "read integer only supports integral types" );
    value = boost::numeric_cast<T>( readInteger( file ));
}

/// writes a floating point number to the file
template<class T>
void writeFloat( std::ostream& file, T value )
{
    static_assert( std::is_floating_point<T>::value, "write float only supports float point types" );
    double bit64 = boost::numeric_cast<double>(value);
    file.write( (char*)&bit64, sizeof(bit64) );
}

/// writes a container of floating point numbers to file
template<class C>
void writeFloats(std::ostream& file, const C& container)
{
    for(auto f : container)
        writeFloat(file, f);
}

/// reads a floating point number from a file
double readFloat( std::istream& file );

/// reads a floating point number from a file
template<class T>
void readFloat( std::istream& file, T& value )
{
    static_assert( std::is_floating_point<T>::value, "read float only supports float point types" );
    value = boost::numeric_cast<T>( readFloat( file ));
}

/// writes the contents of a gen_vec to a file (i.e. does not write its dimension)
void writeVec( std::ostream& file, const gen_vect& value);
/// reads the contents of a gen_vec from a file (i.e. does not change its dimension)
void readVec( std::istream& file, gen_vect& value);

/// \todo use boost::serialization

//! \}

#endif // FILEIO_HPP_INCLUDED
