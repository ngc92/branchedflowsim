#include "potential.hpp"

#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include "fileIO.hpp"
#include <boost/lexical_cast.hpp>



// helper typedef
typedef Potential::grid_type grid_type;

// Base

Potential::Potential(std::size_t dimension, double support, std::size_t size, double strength):
            mDimension( dimension ), mExtents(dimension, size), mSupport(dimension, support),
            mStrength(strength), mCorrelationLength( -1 ), mPotgenVersion(3)
{

}

Potential::Potential(std::vector<std::size_t> extents, std::vector<double> support, double strength):
            mDimension( extents.size() ), mExtents( extents ), mSupport( std::move(support) ),
            mStrength(strength), mCorrelationLength( -1 ), mPotgenVersion(3)
{

}

Potential::Potential( Potential&& other ) noexcept :
            mDimension( other.mDimension ),
            mSeed( other.mSeed ),
            mExtents( other.mExtents ),
            mSupport( other.mSupport ),
            mStrength( other.mStrength ),
            mCorrelationLength( other.mCorrelationLength ),
            mPotgenVersion( other.mPotgenVersion )
{
    std::swap(const_cast<std::vector<std::size_t>&>(mExtents), const_cast<std::vector<std::size_t>&>(other.mExtents));
    std::swap( mData, other.mData );
}

// --------------------------------------------------
//             general info
// --------------------------------------------------

void Potential::setCreationInfo( std::size_t seed, std::size_t version, double corrlength)
{
    mSeed = seed;
    mPotgenVersion = version;
    mCorrelationLength = corrlength;
}

const std::vector<double>& Potential::getSupport() const
{
    return mSupport;
}

std::size_t Potential::getSeed() const
{
    return mSeed;
}

std::size_t Potential::getDimension() const
{
    return mDimension;
}

std::size_t Potential::getPotgenVersion() const
{
    return mPotgenVersion;
}

double Potential::getCorrelationLength() const
{
    return mCorrelationLength;
}

std::size_t Potential::getOrder( const MultiIndex& index ) const
{
    return std::accumulate( index.begin(), index.end(), 0 );
}


// --------------------------------------------------
//            read access potential data
// --------------------------------------------------

const grid_type& Potential::getPotential(const std::string& name) const
{
    // potential is just 0'th derivative
    return getDerivative( std::vector<int>(mDimension, 0), name );
}

const grid_type& Potential::getDerivative( std::vector<int> deriv, const std::string& name ) const
{
    if(deriv.size() != mDimension)
        THROW_EXCEPTION(std::runtime_error, "Trying to get derivative with %1% components, but dimension is %2%", deriv.size(), mDimension);
    
    return mData.at( IndexType(name, std::move(deriv)) );
}

bool Potential::hasDerivative( const MultiIndex& index, const std::string& name ) const
{
    return mData.count( IndexType(name, index.getAsVector()) );
}

bool Potential::hasDerivativesOfOrder( std::size_t order, const std::string& name ) const
{
    MultiIndex index(mDimension);
    index.setLowerBound(0);
    index.setUpperBound(order + 1);
    for(index.init(); index.valid(); ++index)
    {
        // check total order of derivative
        std::size_t total_order = getOrder( index );
        if(total_order == order)
        {
            if(!hasDerivative(index, name)) return false;
        }
    }
    // if never returned false, all derivatives are present
    return true;
}


// --------------------------------------------------
//            write access potential data
// --------------------------------------------------
const grid_type&  Potential::setPotential( grid_type data, const std::string& name )
{
    return setDerivative( std::vector<int>(mDimension, 0), std::move(data), name);
}

const grid_type&  Potential::setDerivative( const MultiIndex& index, grid_type data, const std::string& name )
{
    return setDerivative(index.getAsVector(), std::move(data), name);
}

const grid_type& Potential::setDerivative( std::vector<int> deriv, grid_type data, const std::string& name )
{
    if(deriv.size() != mDimension)
        THROW_EXCEPTION(std::runtime_error, "Trying to set derivative with %1% components, but dimension is %2%", deriv.size(), mDimension);
    
    if(data.getDimension() != mDimension)
        THROW_EXCEPTION(std::runtime_error, "Trying to set derivative of dimension %1% components, but dimension is %2%", data.getDimension(), mDimension);
    
    return mData[IndexType(name, deriv)] = std::move(data);
}

// --------------------------------------------------
//             transformation
// --------------------------------------------------

void Potential::scalePotential( double factor, const std::string& name )
{
    PROFILE_BLOCK("scale potential");
    // scale potential and all derivatives
    for(auto& i : mData)
        if(name.empty() || i.first.name == name)
            scaleVectorBy( i.second, factor );
}


void Potential::setSupport( const std::vector<double>& supp, const std::string& name )
{
    assert( supp.size() == mDimension );

    PROFILE_BLOCK("set potential support");

    // early out if nothing changed. For big potentials this can save some time
    if(supp == mSupport)
        return;

    // calculate the rescale factor by detecting the change in support
    std::vector<double> scale;
    for(unsigned i = 0; i < mDimension; ++i)
        scale.push_back( mSupport.at(i) / supp.at(i) );

    // iterate over all derivatives
    for(auto& data : mData)
    {
        double scale_factor = 1;
        /// the scale factor of the derivatives is derived from the change
        /// in \p dx for the derivatives.
        for(unsigned i = 0; i < mDimension; ++i)
            scale_factor *= std::pow(scale[i], data.first.derivations.at(i));
            // factor is inverse, so we need to subtract location_power

        // scale derivative by the calculated scale factor
        scaleVectorBy( data.second, scale_factor );
    }

    // set new support
    mSupport = supp;
}


// -----------------------------------------------------------------------------------------------------
//            serialization
// -----------------------------------------------------------------------------------------------------


// version 4, header contains also dimension
const char header[] = {'b', 'p', 'o', 't', '5'};

void Potential::writeToFile( std::fstream& file ) const
{
    PROFILE_BLOCK("write potential to file");
    // header
    file.write(header, sizeof(header));

    // add a human readable comment in the beginning
    // this makes it possible to examine potential files from the command line via 'head'
    std::stringstream human_info;
    // build human readable string
    human_info << "\npotgen generated potential:\n";
    human_info << " seed    = " << mSeed << "\n";
    human_info << " corlen  = " << mCorrelationLength << "\n";
    human_info << " version = " << mPotgenVersion << "\n";
    human_info << " extents = (" ;
    for( auto& e : mExtents)
        human_info << e << ", ";
    human_info.seekp( human_info.tellp() - (std::streamoff)2 );
    human_info << ")\n";
    human_info << " support = (" ;
    for( auto& e : mSupport)
        human_info << e << ", ";
    human_info.seekp( human_info.tellp() - (std::streamoff)2 );
    human_info << ")\n\n";

    std::string histr = human_info.str();
    // write human string size as character string
    file << " " << histr.size();
    file << histr;    // this does not write a terminating \0, so our counts are correct

    // now write the actual info
    writeInteger( file, mDimension );

    for(unsigned i=0; i < mDimension; ++i)
        writeFloat( file, mSupport[i] );
    for(unsigned i=0; i < mDimension; ++i)
        writeInteger( file, mExtents[i] );
    writeInteger( file, mSeed );
    writeInteger( file, mPotgenVersion );
    writeInteger( file, mData.size() );
    writeFloat( file, mCorrelationLength );
    writeFloat( file, mStrength );

    for(auto& data : mData )
    {
        writeInteger( file, data.first.name.size() );
        file.write(data.first.name.data(), data.first.name.size());
        // write derivative index
        for(unsigned i = 0; i < mDimension; ++i)
            writeInteger(file, data.first.derivations[i]);
        
        data.second.dump(file);
    }
}

Potential Potential::readFromFile( std::fstream& file )
{
    if(!file.is_open()) {
        THROW_EXCEPTION(std::logic_error, "file is not opened!");
    }

    PROFILE_BLOCK("read potential from file");
    char cmp[sizeof(header)+1];
    cmp[sizeof(header)] = 0;
    file.read( cmp, sizeof(header) );
    if(std::memcmp(cmp, header, sizeof(header)) != 0)
    {
        THROW_EXCEPTION( std::runtime_error, "potential file header %1% does not match %2%", cmp, header );
    }

    // skip human string: read
    std::uint64_t hs;
    file >> hs;
    file.seekg( file.tellg() + (std::fstream::pos_type)hs );

    auto dimension = readInteger( file );

    std::vector<double> support(dimension);
    for(unsigned i=0; i < dimension; ++i)
        readFloat( file, support[i] );
    std::vector<std::size_t> extents(dimension);
    for(unsigned i=0; i < dimension; ++i)
        readInteger(file, extents[i]);
    auto seed = readInteger(file);
    auto potgen = readInteger(file);
    auto gridcount = readInteger(file);
    auto corrlength = readFloat( file );
    auto strength = readFloat( file );

    Potential pot( extents, support, strength );
    pot.setCreationInfo(seed, potgen, corrlength);

    /// \todo rewrite reading!
    for(unsigned j = 0; j < gridcount; ++j)
    {
        auto namelen = readInteger(file);
        std::string name;
        for(unsigned i = 0; i < namelen; ++i)
        {
            name += file.get();
        }
        
        std::vector<int> index( dimension );
        for(unsigned i = 0; i < dimension; ++i)
        {
            index[i] = readInteger(file);
        }
        pot.setDerivative(index, default_grid::load(file), name );
    }

    return std::move(pot);
}

double Potential::getStrength() const
{
    return mStrength;
}

void Potential::setStrength(double new_strength)
{
    scalePotential(new_strength / mStrength);
    mStrength = new_strength;
}


