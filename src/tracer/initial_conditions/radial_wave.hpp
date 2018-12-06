#ifndef RADIAL_WAVE_HPP_INCLUDED
#define RADIAL_WAVE_HPP_INCLUDED

#include "initial_conditions.hpp"
#include <random>

namespace init_cond
{

    // -----------------------------------------------------------------------------------------------------------------
    //                                            radial wave 2d
    // -----------------------------------------------------------------------------------------------------------------
    /*! \class RadialWave2D
     * \brief A two dimensional radial wave (all rays lie in a plane).
     * \details An initial condition where all rays start at the same position, and move outwards radially
     *          in the (0, 1) plane.
     *          For a three dimensional variation use RadialWave3D.
     *
     *          Tests can be found in `tracer/test/radial_init_test.cpp`.
     */
    class RadialWave2D final : public InitialConditionGenerator
    {
    public:
        explicit RadialWave2D(std::size_t dim);

        /// generate next IC
        void generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& position) const override;

        void setOrigin(const gen_vect& pos);
        const gen_vect& getOrigin() const;

    private:
        gen_vect mStartingPos;
    };


    // -----------------------------------------------------------------------------------------------------------------
    //                                          radial wave 3d
    // -----------------------------------------------------------------------------------------------------------------
    /*! \class RadialWave3D
     * \brief A three dimensional radial wave (all rays start from the same point)
     * \details Starts rays from a given point (defaults to the center of the potential) and
     *          lets them move radially outward. The rays are chosen such that they are uniformly
     *          distributed among all directions.
     *          For the two dimensional equivalent use RadialWave2D.
     *
     *          Tests can be found in `tracer/test/radial_init_test.cpp`.
     */
    class RadialWave3D final : public InitialConditionGenerator
    {
    public:
        explicit RadialWave3D(std::size_t dim);

        virtual void init_generator(MultiIndex& manifold_index);

        void next_trajectory(const manifold_pos& pos, MultiIndex& index) override;
        /// generate next IC
        void generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& params) const override;

        void setOrigin(const gen_vect& pos);
        const gen_vect& getOrigin() const;

    private:
        double mStepSize = -1;
        gen_vect mStartingPos;
    };

    // ---------------------------------------------------------------------
    //				radial wave random start, random direction
    // ---------------------------------------------------------------------
    /*! \class RandomRadial
     * \brief Random start and velocity variation of the radial initial condition.
     * \details The rays start at random positions and with random directions,
     *          uniformly distributed over the potential and the unit sphere resp.
     *          In first order, this is the same as the RandomPlanar initial condition,
     *          but the `delta` vectors used for caustic detection are generated as if
     *          the rays came from a full point source.
     *
     *          Tests can be found in `tracer/test/radial_init_test.cpp`.
     */
    struct RandomRadial final : public InitialConditionGenerator
    {
        explicit RandomRadial(std::size_t dim);

        void next_trajectory(const manifold_pos& pos, MultiIndex& index) override;
        void generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& manifold_position) const override;

    private:
        std::default_random_engine random_engine;
        std::uniform_real_distribution<double> distribution{0.0, 1.0};

        gen_vect initial_position;
        gen_vect initial_angle;
        manifold_pos manifold_start;

        // if the entries in this variable are not bigger than zero, always use a fixed value for that angle
        gen_vect fixed_angle;
    };


} // namespace init_cond

#endif // RADIAL_WAVE_HPP_INCLUDED
