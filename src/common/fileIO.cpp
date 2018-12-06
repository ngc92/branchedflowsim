#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "global.hpp"
#include "fileIO.hpp"

using namespace std;

std::uint64_t readInteger( std::istream& file )
{
    uint64_t fixed_size;
    file.read( (char*)&fixed_size, sizeof(fixed_size) );
    return fixed_size;
}

double readFloat( std::istream& file )
{
    double fixed_size;
    file.read( (char*)&fixed_size, sizeof(fixed_size) );
    return fixed_size;
}

void writeVec( std::ostream& file, const gen_vect& value)
{
    for(unsigned i = 0; i < value.size(); ++i)
        writeFloat( file, value[i] );
}

void readVec( std::istream& file, gen_vect& value)
{
    for(unsigned i = 0; i < value.size(); ++i)
        readFloat( file, value[i] );
}
