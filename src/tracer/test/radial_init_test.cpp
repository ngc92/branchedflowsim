#include "initial_conditions/radial_wave.hpp"
#include "utils.hpp"
#include <boost/test/unit_test.hpp>

#include <boost/numeric/ublas/io.hpp>

using zero_vec = boost::numeric::ublas::zero_vector<double>;

using namespace init_cond;

BOOST_AUTO_TEST_SUITE(radial_init)

    struct Radial2DFixture
    {
        Radial2DFixture() : wave(2)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(100).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0})
                    .setOffset(zero_vec(2));

            wave.init(config);
        }

        RadialWave2D wave;
    };

    /*
     * This tests that the RadialWave nad RadialWave3D IC correctly checks its arguments and refuses mismatching dimensions.
     */

    BOOST_FIXTURE_TEST_CASE(parameter_check_2d, Radial2DFixture)
    {
        // need at least 2d for 2d IC
        BOOST_CHECK_THROW(RadialWave2D(1), std::logic_error);

        RadialWave2D wave2d(2);

        BOOST_CHECK_THROW(wave.setOrigin(gen_vect(3)), std::invalid_argument);
        BOOST_CHECK_THROW(wave.setOrigin(gen_vect(1)), std::invalid_argument);
    }

    BOOST_AUTO_TEST_CASE(parameter_check_3d)
    {
        // need at least 3d for 3d IC
        BOOST_CHECK_THROW(RadialWave3D(1), std::logic_error);
        BOOST_CHECK_THROW(RadialWave3D(2), std::logic_error);

        RadialWave3D wave3d(3);

        BOOST_CHECK_THROW(wave3d.setOrigin(gen_vect(2)), std::invalid_argument);
    }

    /*
     * This checks that PlanarWave produces correct ICs in the 2d case.
     */
    BOOST_FIXTURE_TEST_CASE(deterministic_2d, Radial2DFixture)
    {
        gen_vect origin(2);
        origin[0] = 0.3;
        origin[1] = 0.6;
        wave.setOrigin(origin);

        //! \todo we call init before having supplied all data. That should probably not be allowed!

        std::vector<int> counter(100);
        for(auto ic = wave.next(); ic; ++ic)
        {
            auto& state = ic.getState();
            BOOST_REQUIRE_CLOSE(state.getPosition()[0], 0.3, 1e-6);
            BOOST_REQUIRE_CLOSE(state.getPosition()[1], 0.6, 1e-6);

            double angle = std::atan2(state.getVelocity()[1], state.getVelocity()[0]) + pi;
            int angle_int = std::floor(angle * 100 / 2 / pi);
            counter[angle_int] += 1;

            // delta:
            auto& delta = ic.getDelta(0);
            // no change in position
            BOOST_REQUIRE_SMALL(delta.getPosition()[0], 1e-6);
            BOOST_REQUIRE_SMALL(delta.getPosition()[1], 1e-6);

            // and dv _|_ v
            BOOST_REQUIRE_SMALL(dotProduct(delta.getVelocity(), state.getVelocity()), 2e-4);
            BOOST_REQUIRE_CLOSE(dotProduct(delta.getVelocity(), delta.getVelocity()), 4*pi*pi, 2e-4);
        }

        for(auto& count : counter) {
            BOOST_CHECK_EQUAL(count, 1);
        }
    }

    /*
     * Utility used as base class for fixtures for 2D radial tests (both random and deterministic). This registers
     * checks for the distribution of velocities that are run automatically at the end of the test case.
     */

    struct Radial2DTest
    {
        Radial2DTest() : check_phi(true, "phi"),
                         check_vx(true, "vx"),
                         check_vy(true, "vy")
        {
        }

        void checkCondition(InitialCondition& cond) {
            auto& vel = cond.getState().getVelocity();
            double x = vel[0];
            double y = vel[1];
            double phi = std::atan2(y, x);


            double phi_deg = phi * 180 / pi + 180;
            check_phi.put_in_bin(phi_deg / 360);

            check_vx.put_in_bin((x + 1)/2);
            check_vy.put_in_bin((y + 1)/2);

            // delta:
            auto& delta_phi = cond.getDelta(0);

            // and dv/dphi _|_ v, rate of 2pi
            BOOST_REQUIRE_SMALL(dotProduct(delta_phi.getVelocity(), vel), 2e-4);
            // check the length of the vector
            BOOST_REQUIRE_CLOSE(dotProduct(delta_phi.getVelocity(), delta_phi.getVelocity()), 4*pi*pi, 2e-4);

            // no change in position
            BOOST_REQUIRE_SMALL(delta_phi.getPosition()[0], 1e-6);
            BOOST_REQUIRE_SMALL(delta_phi.getPosition()[1], 1e-6);
        }


        std::size_t NUM_PARTICLES = 200000;

        UniformityCheck check_phi;
        UniformityCheck check_vx;
        UniformityCheck check_vy;
    };

    struct DeterministicRadial2D : public Radial2DTest
    {
        DeterministicRadial2D() : wave(2)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(NUM_PARTICLES).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0})
                    .setOffset(zero_vec(2));

            wave.init(config);
        }

        RadialWave2D wave;
    };

    struct RandomRadial2D : public Radial2DTest
    {
        RandomRadial2D() : wave(2)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(NUM_PARTICLES).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0})
                    .setOffset(zero_vec(2));

            wave.init(config);
        }

        RandomRadial wave;
    };


    BOOST_FIXTURE_TEST_CASE(deterministic_2d_velocity_distribution, DeterministicRadial2D)
    {
        for(InitialCondition cond = wave.next(); cond; ++cond)
        {
            checkCondition(cond);

            auto& delta = cond.getDelta(0);
        }
    }

    BOOST_FIXTURE_TEST_CASE(random_2d_velocity_distribution, RandomRadial2D)
    {
        UniformityCheck check_x(true, "x");
        UniformityCheck check_y(true, "y");
        for(InitialCondition cond = wave.next(); cond; ++cond)
        {
            checkCondition(cond);
            check_x.put_in_bin(cond.getState().getPosition()[0]);
            check_y.put_in_bin(cond.getState().getPosition()[1]);
        }
    }


    /*
     * Utility used as base class for fixtures for 3D radial tests (both random and deterministic). This registers
     * checks for the distribution of velocities that are run automatically at the end of the test case.
     */

    struct Radial3DTest
    {
        Radial3DTest() : check_phi(true, "phi"),
                         check_vx(true, "vx"),
                         check_vy(true, "vy"),
                         check_vz(true, "vz")
        {
        }

        void checkCondition(InitialCondition& cond) {
            auto& vel = cond.getState().getVelocity();
            double x = vel[0];
            double y = vel[1];
            double z = vel[2];
            double c = std::sqrt(1 - z*z);   // cos(theta)
            double phi = std::atan2(y/c, x/c);


            double phi_deg = phi * 180 / pi + 180;
            check_phi.put_in_bin(phi_deg / 360);

            check_vx.put_in_bin((x + 1)/2);
            check_vy.put_in_bin((y + 1)/2);
            check_vz.put_in_bin((z + 1)/2);

            // delta:
            auto& delta_theta = cond.getDelta(0);
            auto& delta_phi = cond.getDelta(1);

            // and dv _|_ v, rate of 2pi
            BOOST_REQUIRE_SMALL(dotProduct(delta_phi.getVelocity(), vel), 2e-4);
            BOOST_REQUIRE_SMALL(delta_phi.getVelocity()[2], 1e-6);
            BOOST_REQUIRE_CLOSE(dotProduct(delta_phi.getVelocity(), delta_phi.getVelocity()), c*c*4*pi*pi, 2e-4);

            // and dv _|_ v
            BOOST_REQUIRE_SMALL(dotProduct(delta_theta.getVelocity(), vel), 2e-4);
            BOOST_REQUIRE_CLOSE(dotProduct(delta_theta.getVelocity(), delta_theta.getVelocity()), pi*pi, 2e-4);

            // no change in position
            BOOST_REQUIRE_SMALL(delta_phi.getPosition()[0], 1e-6);
            BOOST_REQUIRE_SMALL(delta_phi.getPosition()[1], 1e-6);
            BOOST_REQUIRE_SMALL(delta_phi.getPosition()[2], 1e-6);

            BOOST_REQUIRE_SMALL(delta_theta.getPosition()[0], 1e-6);
            BOOST_REQUIRE_SMALL(delta_theta.getPosition()[1], 1e-6);
            BOOST_REQUIRE_SMALL(delta_theta.getPosition()[2], 1e-6);
        }


        std::size_t NUM_PARTICLES = 200000;

        UniformityCheck check_phi;
        UniformityCheck check_vx;
        UniformityCheck check_vy;
        UniformityCheck check_vz;
    };

    struct DeterministicRadial3D : public Radial3DTest
    {
        DeterministicRadial3D() : wave(3)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(NUM_PARTICLES).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0, 1.0})
                    .setOffset(zero_vec(3));

            wave.init(config);
        }

        RadialWave3D wave;
    };

    struct RandomRadial3D : public Radial3DTest
    {
        RandomRadial3D() : wave(3)
        {
            InitialConditionConfiguration config;
            config.setParticleCount(NUM_PARTICLES).setEnergyNormalization(false)
                    .setSupport(std::vector<double>{1.0, 1.0, 1.0})
                    .setOffset(zero_vec(3));

            wave.init(config);
        }

        ~RandomRadial3D()
        {
            check_phi.save("phi_test_det.txt");
        }


        RandomRadial wave;
    };

    BOOST_FIXTURE_TEST_CASE(deterministic_3d_velocity_distribution, DeterministicRadial3D)
    {
        for(InitialCondition cond = wave.next(); cond; ++cond)
        {
            checkCondition(cond);

            auto& delta = cond.getDelta(0);
        }
    }

    BOOST_FIXTURE_TEST_CASE(random_3d_velocity_distribution, RandomRadial3D)
    {
        UniformityCheck check_z(true, "z");
        UniformityCheck check_y(true, "y");
        UniformityCheck check_x(true, "x");
        for(InitialCondition cond = wave.next(); cond; ++cond)
        {
            checkCondition(cond);
            check_x.put_in_bin(cond.getState().getPosition()[0]);
            check_y.put_in_bin(cond.getState().getPosition()[1]);
            check_z.put_in_bin(cond.getState().getPosition()[2]);
        }
    }


BOOST_AUTO_TEST_SUITE_END()
