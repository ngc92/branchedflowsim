#include "planar_wave.hpp"
#include "util.hpp"

using namespace init_cond;

// ---------------------------------------------------------------------------------------------------------------------
//							                        planar wave
// ---------------------------------------------------------------------------------------------------------------------
PlanarWave::PlanarWave(std::size_t world_dim, std::size_t wave_dim) :
		InitialConditionGenerator( world_dim, wave_dim, "planar" ),
        mOrigin( world_dim ), mVelocity( world_dim )
{
    if(wave_dim > world_dim)
    {
        THROW_EXCEPTION( std::logic_error, "Manifold dimension %1% exceeds world dimension %2% for PlanarWave.",
                         getManifoldDimension(), getWorldDimension() );
    }

	// set initial velocity vector
	for(unsigned i = 0; i < mWorldDimension; ++i) {
        mVelocity[i] = i == 0 ? 1.0 : 0.0;
        mOrigin[i] = 0.0;
    }

    // set the spanning vectors
    for(unsigned i = 0; i < mManifoldDimension; ++i)
    {
        gen_vect vec(mWorldDimension);
        for(unsigned j = 0; j < mWorldDimension; ++j)
        {
            vec[j] = 0.0;
        }
        vec[mWorldDimension - 1 - i] = 1.0,
        mSpanningVectors.push_back(vec);
    }
}

// configuration functions

void PlanarWave::setInitialVelocity( gen_vect vel )
{
    if(vel.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional initial velocity supplied for %2% dimensional world",
                         vel.size(), getWorldDimension() );
    mVelocity = vel;
}

void PlanarWave::setOrigin( gen_vect origin )
{
    if(origin.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional origin supplied for %2% dimensional world",
                         origin.size(), getWorldDimension() );
    mOrigin = origin;
}

void PlanarWave::setSpanningVector(unsigned long index, gen_vect vec) {
    if(index >= mSpanningVectors.size())
        THROW_EXCEPTION( std::range_error, "Index %1% not valid for spanning vector. Manifold has %2% dimensions.",
                         index, getManifoldDimension());

    if(vec.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional spanning vector supplied for %2% dimensional world",
                         vec.size(), getWorldDimension() );

    if(dotProduct(vec, vec) < 1e-5)
        THROW_EXCEPTION( std::domain_error, "Spanning vector for index %1% has zero length", index);

    mSpanningVectors.at(index) = vec;
}

// generation function

void
PlanarWave::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& manifold_position) const
{
    ray_position = mOrigin;
    for(unsigned i = 0; i < mManifoldDimension; ++i)
    {
        ray_position += mSpanningVectors[i] * manifold_position[i];
    }

	ray_velocity = mVelocity;
}

const gen_vect& PlanarWave::getInitialVelocity() const {
    return mVelocity;
}

const gen_vect& PlanarWave::getOrigin() const {
    return mOrigin;
}

// ---------------------------------------------------------------------
//                        random planar wave
// ---------------------------------------------------------------------
RandomPlanar::RandomPlanar(std::size_t dim) :
        InitialConditionGenerator(dim, dim-1, "random_planar"),
        mCacheInitPosition(dim),
        mCacheInitVelocity(dim),
        mCacheManifoldStart( dim )
{
    for(unsigned i = 0; i < dim-1; ++i)
        mCacheManifoldDirections.emplace_back(dim);
}

void RandomPlanar::new_initial_position()
{
    // fixed?
    if(mInitialPosition) {
        mCacheInitPosition = mInitialPosition.get();
        return;
    }

    //
    for (unsigned i = 0; i < mWorldDimension; ++i) {
        mCacheInitPosition[i] = distribution(mRandomEngine);
    }
}

void RandomPlanar::new_initial_velocity()
{
    // fixed?
    if(mInitialVelocity) {
        mCacheInitVelocity = mInitialVelocity.get();
        return;
    }

    if( mWorldDimension == 2)
    {
        double manifold_angle = distribution( mRandomEngine ) * 2 * pi;
        mCacheInitVelocity[0] = std::sin(manifold_angle);
        mCacheInitVelocity[1] = std::cos(manifold_angle);
    }
    else if (mWorldDimension == 3 ) {
        mCacheInitVelocity = randomPointOnSphere(mRandomEngine);
    }
}

void RandomPlanar::next_trajectory(const manifold_pos& pos, MultiIndex&)
{
    mCacheManifoldStart = pos;

    new_initial_position();
    new_initial_velocity();

    if( mWorldDimension == 2)
    {
        // (x,y,0) x (0,0,1) = (y, -x)
        mCacheManifoldDirections[0][1] = mCacheInitVelocity[0];
        mCacheManifoldDirections[0][0] = -mCacheInitVelocity[1];
    }
    else if (mWorldDimension == 3 )
    {
        int j = 0;
        for(int i = 0; i < 3; ++i) {
            // take a vector from the standard basis
            gen_vect basis(3);
            basis[i] = 1;

            // and use it to form part of the IC direction
            crossProduct(mCacheManifoldDirections[j], basis, mCacheInitVelocity);

            // this is sin(angle(init vel, basis))
            double norm = boost::numeric::ublas::norm_2(mCacheManifoldDirections[j]);
            mCacheManifoldDirections[j] /= norm; // normalize
            // if we have sufficient norm, accept the vector and move to the next,
            // until we have both.
            if (norm > 0.2)
            {
                j += 1;
                if(j == 2) {
                    break;
                }
            }
        }
    }
}

void
RandomPlanar::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& manifold_position) const
{
    // generate random starting position
    for(unsigned i = 0; i < mWorldDimension; ++i)
    {
        double v = mCacheInitPosition[i];
        for(unsigned j = 0; j < manifold_position.size(); ++j)
            v += mCacheManifoldDirections[j][i] * (manifold_position[j] - mCacheManifoldStart[j]);

        ray_position[i] = v;
        ray_velocity[i] = mCacheInitVelocity[i];
    }
}

void RandomPlanar::setFixedVelocity(gen_vect vel)
{
    if(vel.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional velocity supplied for %2% dimensional world",
                         vel.size(), getWorldDimension() );
    mInitialVelocity = vel;
}

void RandomPlanar::setFixedPosition(gen_vect pos)
{
    if(pos.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional origin supplied for %2% dimensional world",
                         pos.size(), getWorldDimension() );
    mInitialPosition = pos;
}

boost::optional<gen_vect> RandomPlanar::getFixedPosition() const {
    return mInitialPosition;
}

boost::optional<gen_vect> RandomPlanar::getFixedVelocity() const {
    return mInitialVelocity;
}
