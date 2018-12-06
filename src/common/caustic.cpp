#include "caustic.hpp"
#include "fileIO.hpp"
#include "global.hpp"

// constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Caustic::Caustic(uint64_t idx, gen_vect p, gen_vect orig, gen_vect cvel, gen_vect ivel, double t, uint8_t i) :
    mTrajectory( idx ),
    mCausticPosition(p),
    mInitialPosition(orig),
    mCausticVelocity(cvel),
    mInitialVelocity(ivel),
    mTime(t),
    mIndex(i)
{
    #ifndef NDEBUG
    ASSERT_EQUAL( mCausticPosition.size(), mInitialPosition.size(), "initial position and position must have the same dimension");
    ASSERT_EQUAL( mCausticPosition.size(), mInitialVelocity.size(), "initial velocity and position must have the same dimension");
    ASSERT_EQUAL( mCausticPosition.size(), mCausticVelocity.size(), "initial velocity and position must have the same dimension");
    #endif // NDEBUG
    /// \todo check that position and initial position are compatible. At least in debug mode.
}

Caustic::Caustic( int dimension ) :
    mCausticPosition( dimension ),
    mInitialPosition( dimension ),
    mCausticVelocity( dimension ),
    mInitialVelocity( dimension )
{

}

// file io
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Caustic::write(std::ostream& file) const
{
    writeInteger( file, mTrajectory );
    // write position
    writeVec( file, mCausticPosition );
    writeVec( file, mCausticVelocity );
    // write origin
    writeVec( file, mInitialPosition );
    writeVec( file, mInitialVelocity );
    writeFloat( file, mTime );
    // write the caustic index (on trajectory) as a single byte
    file.write(reinterpret_cast<const char*>(&mIndex), sizeof(mIndex));
}

void Caustic::read( std::fstream& file )
{
    readInteger( file, mTrajectory );
    // read
    readVec( file, mCausticPosition );
    readVec( file, mCausticVelocity );
    readVec( file, mInitialPosition );
    readVec( file, mInitialVelocity );
    readFloat( file, mTime );
    // read caustic index (on trajectory) from single byte
    file.read(reinterpret_cast<char*>(&mIndex), sizeof(mIndex));
}
