#ifndef CAUSTIC_HPP_INCLUDED
#define CAUSTIC_HPP_INCLUDED

/*! \file caustic.hpp
    \ingroup common
    \brief Defines a class for caustic data.
*/

#include "vector.hpp"
#include <iosfwd>

/*! \class Caustic
    \ingroup common
    \brief Class for recording data of a single caustic.
    \details Saves data (i..e position, trajectory info etc) for a caustic and allows
            binary io of.
    \todo add information about initial velocity and velocity at caustic.
*/
class Caustic final
{
public:
    /*! generates a caustic with all recorded info set.
        \param idx: index of the trajectory on which the caustic is found.
        \param pos: position of the caustic.
        \param orig: origin of the trajectory on which this caustic is.
        \param cvel: velocity of the ray at the caustic.
        \param ivel: original velocity of the ray.
        \param t: Time after start when the caustic was reached.
        \param i: Index of caustic along this trajectory.
        \pre <tt> pos.size() == orig.size() </tt>
    */
    Caustic(uint64_t idx, gen_vect pos, gen_vect orig, gen_vect cvel, gen_vect ivel, double t, uint8_t i);

    /// generate an empty caustic container for caustics in \p dimension dimensions.
    explicit Caustic( int dimension );

    // copy, move and ~ are default
    ~Caustic() = default;
    Caustic( const Caustic& ) = default;
    Caustic( Caustic&& ) = default;

    // getters
    /// gets the ID of the trajectory this caustic is on.
    uint64_t getTrajectoryID() const { return mTrajectory; };
    /// gets the position of the caustic.
    const gen_vect& getPosition() const { return mCausticPosition; };
    /// gets the velocity of the ray at the caustic.
    const gen_vect& getVelocityAtCaustic() const { return mCausticVelocity; };
    /// gets the initial position of the ray of this caustic.
    const gen_vect& getOrigin() const { return mInitialPosition; };
    /// gets the initial velocity of the ray of this caustic.
    const gen_vect& getOriginalVelocity() const { return mInitialVelocity; };
    /// gets the time (along the trajectory) needed to reach the caustic.
    double getTime() const { return mTime; };
    /// gets the index of the caustic on the trajectory.
    uint8_t getIndex() const { return mIndex; }

    // I/O
    /// writes this caustic into the file \p file.
    /// \attention Does not include dimension information, since it is expected to
    ///             be used to write whole arrays.
    void write(std::ostream& file) const;

    /// reads caustic data from \p file into this objects.
    /// \attention The caustic has to be set to the correct dimension, i.e. by using Caustic(int) ctor.
    void read( std::fstream& file );

private:
    // use uint64_t to be independent of sizeof(int)
    uint64_t mTrajectory;                   ///< index of trajectory this caustic was found on.
    gen_vect mCausticPosition;              ///< position where the caustic was found.
    gen_vect mInitialPosition;              //< position where the ray for this caustic originated.
    gen_vect mCausticVelocity;              ///< velocity of the ray at the caustic.
    gen_vect mInitialVelocity;              ///< position where the ray for this caustic originated.
    double   mTime;                         ///< time it took the particle to reach the caustic.
    uint8_t  mIndex;                        ///< number of the caustic on the current trajectory.
};


#endif // CAUSTIC_HPP_INCLUDED
