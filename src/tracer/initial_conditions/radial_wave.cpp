#include "radial_wave.hpp"
#include <boost/numeric/conversion/cast.hpp>

using namespace init_cond;

// ---------------------------------------------------------------------------------------------------------------------
//                                                  radial wave 2d
// ---------------------------------------------------------------------------------------------------------------------

RadialWave2D::RadialWave2D(std::size_t dim) :
    InitialConditionGenerator(dim, 1, "radial"),
    mStartingPos(dim)
{
    if(dim < 2)
        THROW_EXCEPTION(std::logic_error, "Radial two dimensional initial condition requires at least two dimensional "
                "world, got %1%", dim);

    /// \todo this should depend on the world support.
    for(unsigned i = 0; i < mWorldDimension; ++i)
        mStartingPos[i] = 0.5;
}

void RadialWave2D::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& position) const
{
    double param = position[0];

    // set starting pos
    ray_position = mStartingPos;

    // set starting velocity
    ray_velocity[0] = std::sin(param * 2*pi);
    ray_velocity[1] = std::cos(param * 2*pi);
}

void RadialWave2D::setOrigin(const gen_vect& origin)
{
    if(origin.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional origin supplied for %2% dimensional world",
                         origin.size(), getWorldDimension() );
    mStartingPos = origin;
}

const gen_vect& RadialWave2D::getOrigin() const {
    return mStartingPos;
}


// ---------------------------------------------------------------------------------------------------------------------
//                                                  radial wave 3d
// ---------------------------------------------------------------------------------------------------------------------

RadialWave3D::RadialWave3D(std::size_t dim) :
    InitialConditionGenerator(dim, 2, "radial_3d")
{
    // does not work in 2d
    if(dim != 3)
        THROW_EXCEPTION( std::invalid_argument, "RadialWave3D three dimensions, got %1%", dim );
    
    for(unsigned i = 0; i < mWorldDimension; ++i)
        mStartingPos[i] = 0.5;
}

void RadialWave3D::init_generator(MultiIndex& manifold_index)
{
    // whole surface: 4pi -> "area per particle" 4 pi / N
    // if small -> assume square: edge length: sqrt( 4 pi / N )
    mStepSize = std::sqrt( 4 * pi / getParticleCount() );

    // half circle: pi / s => NUM OF STEPS [pi/2, because -RNG ... RNG]
    auto range = boost::numeric_cast<int>(std::ceil(pi / mStepSize));

    manifold_index.setUpperBoundAt(0, range);
    manifold_index.setUpperBoundAt(1, 1);

    std::cout << "azimuth range: " << range << "\n";
}

/// generate next IC
void RadialWave3D::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& params) const
{
    double theta = (2*params[0] - 1) * pi / 2;
    // azimuth: always full great circle
    double phi = params[1] * 2 * pi;

    // set starting pos
    ray_position = mStartingPos;


    ray_velocity[0] = std::cos( theta ) * std::sin( phi );
    ray_velocity[1] = std::cos( theta ) * std::cos( phi );
    ray_velocity[2] = std::sin( theta );
}

void RadialWave3D::setOrigin(const gen_vect& origin)
{
    if(origin.size() != mWorldDimension)
        THROW_EXCEPTION( std::invalid_argument, "%1% dimensional origin supplied for %2% dimensional world",
                         origin.size(), getWorldDimension() );
    mStartingPos = origin;
}

void RadialWave3D::next_trajectory(const manifold_pos& pos, MultiIndex& index)
{
    double theta = (2 * pos[0] - 1) * pi / 2;
    double circumference = std::cos(theta) * 2 * pi;

    // for the start of each new row, recalculate the upper iteration bound.
    if(index[getManifoldDimension()-1] == 0)
    {
        index.setUpperBoundDynamic(1, boost::numeric_cast<int>(std::ceil(circumference / mStepSize)));
    }
}

const gen_vect& RadialWave3D::getOrigin() const {
    return mStartingPos;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                             random radial wave
// ---------------------------------------------------------------------------------------------------------------------
RandomRadial::RandomRadial(std::size_t dim) :
        InitialConditionGenerator(dim, dim-1, "random_radial"),
        initial_position(dim),
        initial_angle(dim-1),
        fixed_angle(dim-1)
{
    //! \todo Get Value from c'tor
    for(auto& val : fixed_angle)
        val = -1;
}

void RandomRadial::next_trajectory(const manifold_pos& pos, MultiIndex&)
{
    manifold_start = pos;
    // position
    for(unsigned i = 0; i < mWorldDimension; ++i)
    {
        initial_position[i] = distribution( random_engine );
    }

    // angle
    if( mWorldDimension == 2 )
    {
        initial_angle[0] = distribution( random_engine ) * 2 * pi;
    }
    else if (mWorldDimension == 3 )
    {
        // http://mathworld.wolfram.com/SpherePointPicking.html
        double u = distribution( random_engine );
        double v = distribution( random_engine );
        initial_angle[0] = u * 2 * pi;
        initial_angle[1] = std::acos(2*v - 1)    ;
    }

    // set fixed angle
    for(unsigned i = 0; i < fixed_angle.size(); ++i)
        if( fixed_angle[i] >= 0)
            initial_angle[i] = fixed_angle[i];
}

void
RandomRadial::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& manifold_position) const
{
    ray_position = initial_position;
    // angle to direction
    if( mWorldDimension == 3 )
    {
        double phi = initial_angle[0] + (manifold_position[1] - manifold_start[1]) * 2 * pi;
        ray_velocity[0] = std::cos(phi);
        ray_velocity[1] = std::sin(phi);

        double theta = initial_angle[1] + (manifold_position[0] - manifold_start[0]) * pi;
        ray_velocity[0] *= std::sin(theta);
        ray_velocity[1] *= std::sin(theta);
        ray_velocity[2]  = std::cos(theta);
    }
    else if( mWorldDimension == 2 )
    {
        double phi = initial_angle[0] + (manifold_position[0] - manifold_start[0]) * 2 * pi;
        ray_velocity[0] = std::cos(phi);
        ray_velocity[1] = std::sin(phi);
    }
}
