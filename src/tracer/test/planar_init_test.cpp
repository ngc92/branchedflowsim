//
// Created by erik on 9/7/17.
//

#include <boost/test/unit_test.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "test_helpers.hpp"
#include "initial_conditions/planar_wave.hpp"
#include "utils.hpp"

using zero_vec = boost::numeric::ublas::zero_vector<double>;

using namespace init_cond;

BOOST_AUTO_TEST_SUITE(planar_init)

    struct Planar2DFixture
    {
        Planar2DFixture() : wave(2, 1)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(100).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0})
                    .setOffset(zero_vec(2));

            wave.init(config);
        }

        PlanarWave wave;
    };

    /*
     * This tests that the PlanarWave IC correctly checks its arguments and refuses mismatching dimensions.
     */

    BOOST_FIXTURE_TEST_CASE(parameter_check, Planar2DFixture)
    {
        BOOST_CHECK_EXCEPTION(PlanarWave(2, 3), std::logic_error,
                              check_what("Manifold dimension 3 exceeds world dimension 2 for PlanarWave."));

        BOOST_CHECK_EXCEPTION(wave.setOrigin(gen_vect(3)), std::invalid_argument,
                              check_what("3 dimensional origin supplied for 2 dimensional world"));
        BOOST_CHECK_EXCEPTION(wave.setInitialVelocity(gen_vect(1)), std::invalid_argument,
                              check_what("1 dimensional initial velocity supplied for 2 dimensional world"));

        // check spanning vector.
        gen_vect wrong(3);
        wrong[0] = 1.0;

        gen_vect right(2);
        right[0] = 1.0;

        BOOST_CHECK_THROW(wave.setSpanningVector(0, wrong), std::invalid_argument);
        BOOST_REQUIRE_NO_THROW(wave.setSpanningVector(0, right));
        BOOST_CHECK_THROW(wave.setSpanningVector(1, right), std::range_error);

        right[0] = 0.0;
        BOOST_CHECK_THROW(wave.setSpanningVector(0, right), std::domain_error);
    }

    /*
     * This checks that PlanarWave produces correct ICs in the 2d case.
     */
    BOOST_FIXTURE_TEST_CASE(deterministic_2d, Planar2DFixture)
    {
        gen_vect init_vel(2);
        init_vel[0] = 1.0;
        init_vel[1] = 2.0;
        wave.setInitialVelocity(init_vel);

        gen_vect origin(2);
        origin[0] = 0.5;
        origin[1] = 0.5;
        wave.setOrigin(origin);

        gen_vect spanning(2);
        spanning[0] = 1.0;
        spanning[1] = 0.0;
        wave.setSpanningVector(0, spanning);

        //! \todo we call init before having supplied all data. That should probably not be allowed!

        std::vector<int> counter(100);
        for(auto ic = wave.next(); ic; ++ic)
        {
            auto& state = ic.getState();
            BOOST_REQUIRE_CLOSE(state.getPosition()[1], 0.5, 1e-6);
            int pos_int = std::floor((state.getPosition()[0] - 0.5) * 100);
            counter[pos_int] += 1;

            // velocity check
            BOOST_REQUIRE_CLOSE(state.getVelocity()[0], 1.0, 1e-6);
            BOOST_REQUIRE_CLOSE(state.getVelocity()[1], 2.0, 1e-6);

            auto& delta = ic.getDelta(0);
            BOOST_CHECK_CLOSE(delta.getPosition()[0], 1.0, 1e-6);
            BOOST_CHECK_CLOSE(delta.getPosition()[1], 0.0, 1e-6);

            BOOST_CHECK_CLOSE(delta.getVelocity()[0], 0.0, 1e-6);
            BOOST_CHECK_CLOSE(delta.getVelocity()[1], 0.0, 1e-6);
        }

        for(auto& count : counter) {
            BOOST_CHECK_EQUAL(count, 1);
        }
    }

    struct RandomPlanar2DFixture
    {
        RandomPlanar2DFixture() : wave(2),
                                  check_phi(false, "phi"),
                                  check_x(false, "x"),
                                  check_y(false, "y")
        {
            InitialConditionConfiguration config;
            config.setParticleCount(10000).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0})
                    .setOffset(zero_vec(2));

            wave.init(config);
        }

        void checkDeltaConsistency(const InitialCondition& ic) {
            auto& delta = ic.getDelta(0);
            // check that delta is perpendicular to velocity
            BOOST_REQUIRE_SMALL(dotProduct(delta.getPosition(), ic.getState().getVelocity()), 1e-6);
            // and zero in velocity
            BOOST_REQUIRE_SMALL(delta.getVelocity()[0], 1e-6);
            BOOST_REQUIRE_SMALL(delta.getVelocity()[1], 1e-6);
        }

        void accumulateStatistics(const State& state) {
            double phi = std::atan2(state.getVelocity()[1], state.getVelocity()[0]);
            double phi_deg = phi * 180 / pi + 180;
            check_phi.put_in_bin(phi_deg / 360);

            check_x.put_in_bin(state.getPosition()[0]);
            check_y.put_in_bin(state.getPosition()[1]);
        }

        RandomPlanar wave;

        UniformityCheck check_phi;
        UniformityCheck check_x;
        UniformityCheck check_y;
    };

    /*
     * This tests that the RandomPlanarWave IC correctly checks its arguments and refuses mismatching dimensions.
     */

    BOOST_FIXTURE_TEST_CASE(parameter_check_random, RandomPlanar2DFixture)
    {
        BOOST_CHECK_THROW(wave.setFixedPosition(gen_vect(3)), std::invalid_argument);
        BOOST_CHECK_THROW(wave.setFixedVelocity(gen_vect(1)), std::invalid_argument);
    }

    /*
     * This checks that the starting positions are distributed uniformly on the plane, and the
     * velocities uniformly on the sphere. Further, we verify that the deltas are correct.
     * We do check uniformity of phi, x, and y separately, even though we actually want these
     * to be jointly uniform, and independent from one another. We also test that the magnitude of
     * the velocity is one.
     */
    BOOST_FIXTURE_TEST_CASE(random_velocity_random_position_2d, RandomPlanar2DFixture)
    {
        check_x.enable();
        check_y.enable();
        check_phi.enable();

        for(auto ic = wave.next(); ic; ++ic)
        {
            checkDeltaConsistency(ic);
            accumulateStatistics(ic.getState());

            // check that ||v|| == 1
            BOOST_CHECK_CLOSE(dotProduct(ic.getState().getVelocity(), ic.getState().getVelocity()), 1.0, 1e-6);
        }
    }

    /*
     * Like above but with fixed position.
     */
    BOOST_FIXTURE_TEST_CASE(random_velocity_fixed_position_2d, RandomPlanar2DFixture)
    {
        gen_vect pos(2);
        pos[0] = 0.3;
        pos[1] = 0.5;
        wave.setFixedPosition(pos);

        check_phi.enable();

        for(auto ic = wave.next(); ic; ++ic)
        {
            checkDeltaConsistency(ic);
            accumulateStatistics(ic.getState());

            BOOST_REQUIRE_CLOSE(ic.getState().getPosition()[0], 0.3, 1e-6);
            BOOST_REQUIRE_CLOSE(ic.getState().getPosition()[1], 0.5, 1e-6);
        }
    }

    /*
     * Like above but with fixed velocity.
     */
    BOOST_FIXTURE_TEST_CASE(fixed_velocity_random_position_2d, RandomPlanar2DFixture)
    {
        gen_vect vel(2);
        vel[0] = 0.3;
        vel[1] = 0.5;
        wave.setFixedVelocity(vel);

        check_x.enable();
        check_y.enable();

        for(auto ic = wave.next(); ic; ++ic)
        {
            checkDeltaConsistency(ic);
            accumulateStatistics(ic.getState());

            BOOST_REQUIRE_CLOSE(ic.getState().getVelocity()[0], 0.3, 1e-6);
            BOOST_REQUIRE_CLOSE(ic.getState().getVelocity()[1], 0.5, 1e-6);
        }
    }

    // 3D Testing
    /*
     * This checks that the starting positions are uniformly distributed in the plane, and that
     * the velocities and deltas are as expected.
     */
    BOOST_AUTO_TEST_CASE(uniformity_deterministic_3d)
    {
        PlanarWave wave(3, 2);
        InitialConditionConfiguration config;
        config.setParticleCount(40000).setEnergyNormalization(false)
                .setSupport(std::vector<double>{1.0, 1.0, 1.0})
                .setOffset(zero_vec(3));

        wave.init(config);

        UniformityCheck check_y(true, "y");
        UniformityCheck check_z(true, "z");

        for(auto ic = wave.next(); ic; ++ic)
        {
            check_y.put_in_bin(ic.getState().getPosition()[1]);
            check_z.put_in_bin(ic.getState().getPosition()[2]);
            BOOST_CHECK_SMALL(ic.getState().getPosition()[0], 1e-6);

            // check velocity
            BOOST_CHECK_SMALL(ic.getState().getVelocity()[1], 1e-6);
            BOOST_CHECK_SMALL(ic.getState().getVelocity()[2], 1e-6);
            BOOST_CHECK_CLOSE(ic.getState().getVelocity()[0], 1.0, 1e-6);

            // check delta X _|_ v
            BOOST_CHECK_SMALL(dotProduct(ic.getState().getVelocity(), ic.getDelta(0).getPosition()), 1e-6);
            BOOST_CHECK_SMALL(dotProduct(ic.getState().getVelocity(), ic.getDelta(1).getPosition()), 1e-6);

            BOOST_CHECK_SMALL(dotProduct(ic.getDelta(0).getVelocity(), ic.getDelta(0).getVelocity()), 1e-6);
            BOOST_CHECK_SMALL(dotProduct(ic.getDelta(1).getVelocity(), ic.getDelta(1).getVelocity()), 1e-6);
        }
    }

    /*
     * This checks that the random planar wave distributes the starting positions uniformly in space, and the starting
     * velocities uniformly in velocity space. For simplicity we check only the marginal distributions.
     */
    BOOST_AUTO_TEST_CASE(uniformity_random_3d)
    {
        RandomPlanar wave(3);
        InitialConditionConfiguration config;
        config.setParticleCount(40000).setEnergyNormalization(false)
                .setSupport(std::vector<double>{1.0, 1.0, 1.0})
                .setOffset(zero_vec(3));

        wave.init(config);

        UniformityCheck check_x(true, "x");
        UniformityCheck check_y(true, "y");
        UniformityCheck check_z(true, "z");

        UniformityCheck check_vx(true, "vx");
        UniformityCheck check_vy(true, "vy");
        UniformityCheck check_vz(true, "vz");

        for(auto ic = wave.next(); ic; ++ic)
        {
            auto& vel = ic.getState().getVelocity();

            check_x.put_in_bin(ic.getState().getPosition()[0]);
            check_y.put_in_bin(ic.getState().getPosition()[1]);
            check_z.put_in_bin(ic.getState().getPosition()[2]);

            // check velocity
            check_vx.put_in_bin((vel[0] + 1) / 2);
            check_vy.put_in_bin((vel[1] + 1) / 2);
            check_vz.put_in_bin((vel[2] + 1) / 2);

            BOOST_REQUIRE_CLOSE(dotProduct(vel, vel), 1.0, 1e-6);

            // check delta X _|_ v
            BOOST_CHECK_SMALL(dotProduct(ic.getState().getVelocity(), ic.getDelta(0).getPosition()), 1e-6);
            BOOST_CHECK_SMALL(dotProduct(ic.getState().getVelocity(), ic.getDelta(1).getPosition()), 1e-6);

            BOOST_CHECK_SMALL(dotProduct(ic.getDelta(0).getVelocity(), ic.getDelta(0).getVelocity()), 1e-6);
            BOOST_CHECK_SMALL(dotProduct(ic.getDelta(1).getVelocity(), ic.getDelta(1).getVelocity()), 1e-6);
        }

        check_vx.save("vx_test_det.txt");
    }

BOOST_AUTO_TEST_SUITE_END()
