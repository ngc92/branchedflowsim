#ifndef PLANAR_WAVE_HPP_INCLUDED
#define PLANAR_WAVE_HPP_INCLUDED

#include "initial_conditions.hpp"
#include <random>

namespace init_cond
{

    // -----------------------------------------------------------------------------------------------------------------
    //							                    planar wave
    // -----------------------------------------------------------------------------------------------------------------
    /*! \class PlanarWave
     *  \brief This class represents a simple planar wave, i.e. the rays are all spawned with on a hyperplane, and with
     *          equal velocity.
     *  \details The initial position manifold is formed by linear combinations of the vectors in `mSpanningVectors`,
     *          where the coefficients are simply the manifold coordinates.
     *          The velocity has to be set separately (defaults to "along first axis"), it is not automatically set to
     *          be perpendicular to the spanning plane (nor is that required here).
     *          When the spanning vectors form an orthonomal basis, it is guaranteed that the sampling happens
     *          uniformly on the initial plane.
     *
     *          Tests can be found in `tracer/test/planar_init_test.cpp`.
     */
    class PlanarWave final : public InitialConditionGenerator
    {
    public:
        // ctor / dtor
        ///! Constructor, requires the world dimension and the dimension of the manifold / wave.
        ///! \throw std::logic_error, if \p wave_dim > \p world_dim.
        PlanarWave(std::size_t world_dim, std::size_t wave_dim);
        virtual ~PlanarWave() = default;

        void generate(gen_vect& ray_position, gen_vect& ray_velocity,
                      const manifold_pos& manifold_position) const override;

        // configuration
        void setInitialVelocity( gen_vect vel );
        void setOrigin( gen_vect origin );
        void setSpanningVector(unsigned long index, gen_vect vec );

        const gen_vect& getInitialVelocity() const;
        const gen_vect& getOrigin() const;

    private:
        // spanning vectors
        std::vector<gen_vect> mSpanningVectors;

        // coordinate system origin
        gen_vect mOrigin;

        // initial velocity
        gen_vect mVelocity;
    };


    // -----------------------------------------------------------------------------------------------------------------
    //				                            random planar wave
    // -----------------------------------------------------------------------------------------------------------------
    /*!
     * \class RandomPlanar
     * \brief Planar initial condition with random starting position/velocity.
     * \details For every trajectory, a random direction and starting position are chosen
     *          (unless overridden by setFixedVelocity or setFixedPosition). The "manifold"
     *          from which the ray starts is remembered until the next trajectory is created,
     *          which means that the derivatives can be correctly calculated.
     *
     *          Tests can be found in `tracer/test/planar_init_test.cpp`.
     */
    class RandomPlanar final : public InitialConditionGenerator
    {
    public:
        explicit RandomPlanar(std::size_t dim);

        void setFixedVelocity(gen_vect vel);
        void setFixedPosition(gen_vect pos);

        boost::optional<gen_vect> getFixedPosition() const;
        boost::optional<gen_vect> getFixedVelocity() const;

    private:
        // overridden/implemented behaviour
        void next_trajectory(const manifold_pos& pos, MultiIndex& index) override;
        void generate(gen_vect& ray_position, gen_vect& ray_velocity,
                      const manifold_pos& manifold_position) const override;

        // helper functions
        void new_initial_position();
        void new_initial_velocity();

        // randomization stuff
        std::default_random_engine mRandomEngine;
        std::uniform_real_distribution<double> distribution{0.0, 1.0};

        // configuration to fix position or velocity
        boost::optional<gen_vect> mInitialPosition;
        boost::optional<gen_vect> mInitialVelocity;

        // conditions for each trajectory, cached to enable generation of deltas
        gen_vect mCacheInitPosition;
        gen_vect mCacheInitVelocity;
        manifold_pos mCacheManifoldStart;
        std::vector<gen_vect> mCacheManifoldDirections;
    };
}

#endif // PLANAR_WAVE_HPP_INCLUDED
