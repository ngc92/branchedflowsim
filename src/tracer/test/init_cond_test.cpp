//
// Created by eriks on 9/1/17.
//

#include "initial_conditions/initial_conditions.hpp"
#include "dynamics/ParticleInScaledPotential.hpp"
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <future>
#include "test_helpers.hpp"

using namespace init_cond;

using zero_vec = boost::numeric::ublas::zero_vector<double>;

struct DummyDynamics: public RayDynamics
{
    DummyDynamics() {};
    void stateUpdate( const GState&, GState&, double) const override { }
    bool hasMonodromy() const override { return false; }
    bool hasPeriodicBoundary() const override { return false; }
    void normalizeEnergy(State& state, double energy) const override {
        state.editVel()[0] = -energy;
    };
    double getEnergy( const State& ) const override { return 0.0; };
};

enum class DummyICMode
{
    CONSTANT, IDENTITY, DELAY
};

class DummyICGenerator : public InitialConditionGenerator
{
public:
    DummyICGenerator(unsigned wd, unsigned md, DummyICMode mode_ = DummyICMode::CONSTANT ) :
            InitialConditionGenerator(wd, md, "dummy"), mode(mode_)
    {
    }

    void init_generator(MultiIndex& manifold_index) override
    {
        BOOST_CHECK_EQUAL(manifold_index.size(), getManifoldDimension());
        BOOST_CHECK_EQUAL(manifold_index.getLowerBound()[0], 0);
        BOOST_CHECK(!manifold_index.valid());
        init_called = true;

        // now do the default init
        InitialConditionGenerator::init_generator(manifold_index);
    }

    void generate( gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& pos ) const override
    {
        BOOST_REQUIRE_EQUAL(pos.size(), getManifoldDimension());
        BOOST_REQUIRE_EQUAL(ray_position.size(), getWorldDimension());
        BOOST_REQUIRE_EQUAL(ray_velocity.size(), getWorldDimension());
        if(mode == DummyICMode::DELAY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else if(mode == DummyICMode::CONSTANT) {
            for(unsigned i = 0; i < getWorldDimension(); ++i) {
                ray_position[i] = 0.5;
                ray_velocity[i] = 0.5;
            }
        } else {
            for(unsigned i = 0; i < getManifoldDimension(); ++i) {
                ray_position[i] = pos[i];
                ray_velocity[i] = 0.5;
            }
        }
    }

    bool init_called = false;
    DummyICMode mode;
};

BOOST_AUTO_TEST_SUITE(init_condition_tests)

    /*
     * This test case checks that the InitialConditionConfig does
     * what it is supposed to.
     */
    BOOST_AUTO_TEST_CASE(config)
    {
        InitialConditionConfiguration config;

        // check that default constructed object has no support, particle count,
        // cannot access dynamics and uses relative coordinates.
        std::vector<double> support = {2.0, 2.0, 2.0};
        BOOST_CHECK_EQUAL(config.getSupport().size(), 0);
        BOOST_CHECK_EQUAL(config.getParticleCount(), 0);
        BOOST_CHECK_THROW(config.getDynamics(), std::logic_error);
        BOOST_CHECK(config.getUseRelativeCoordinates());

        auto dynamics = std::make_shared<DummyDynamics>();

        config.setParticleCount(1000).setEnergyNormalization(false)
                .setSupport(support)
                .setOffset(zero_vec(3))
                .setUseRelativeCoordinates(false)
                .setDynamics(dynamics);

        BOOST_CHECK(&config.getDynamics() == dynamics.get());
        BOOST_CHECK(config.getSupport() == support);
        //BOOST_CHECK(config.getOffset() == zero_vec(3));
        BOOST_CHECK_EQUAL(config.getParticleCount(), 1000);
        BOOST_CHECK_EQUAL(config.getUseRelativeCoordinates(), false);
        BOOST_CHECK_EQUAL(config.getEnergyNormalization(), false);
    }

    /*
     * This test case checks that the InitialConditionGenerator constructor
     * correctly checks its arguments and sets its parameters.
     */
    BOOST_AUTO_TEST_CASE(ctor)
    {
        DummyICGenerator gen(3, 2);
        BOOST_CHECK_EQUAL(gen.getWorldDimension(), 3);
        BOOST_CHECK_EQUAL(gen.getManifoldDimension(), 2);
        BOOST_CHECK_EQUAL(gen.getGeneratorType(), "dummy");
        BOOST_CHECK_EQUAL(gen.getParticleCount(), 0);
    }

    /*
     * This test case checks that the InitialConditionGenerator constructor
     * correctly checks its arguments.
     */
    BOOST_AUTO_TEST_CASE(ctor_checks)
    {
        // test dimension checks
        BOOST_CHECK_EXCEPTION(DummyICGenerator(1, 0), std::logic_error,
                              check_what("incompatible dimensions: manifold 0; world 1"));
        BOOST_CHECK_EXCEPTION(DummyICGenerator(0, 1), std::logic_error,
                              check_what("incompatible dimensions: manifold 1; world 0"));
        BOOST_CHECK_EXCEPTION(DummyICGenerator(1, 4), std::logic_error,
                              check_what("incompatible dimensions: manifold 4; world 1"));
    }

    /*
     * The following functions test that InitialConfigGenerator.init correctly throws in case of invalid config,
     * and does not change any data.
     */

    struct InitConfigFixture
    {
        DummyICGenerator generator;
        InitialConditionConfiguration config;
        InitConfigFixture() : generator(3, 2)
        {
            config.setSupport(std::vector<double>{1.0, 1.0, 1.0}).setParticleCount(100).setEnergyNormalization(false)
                    .setOffset(zero_vec(3));
        }

        ~InitConfigFixture()
        {
            BOOST_REQUIRE_THROW(generator.init(config), std::logic_error);
            // check that init did not change the state of the object
            BOOST_CHECK(!generator.init_called);
            BOOST_CHECK_EQUAL(generator.getParticleCount(), 0);
        }
    };

    BOOST_FIXTURE_TEST_CASE(init_wo_support, InitConfigFixture)
    {
        config.setSupport(std::vector<double>());
    }

    BOOST_FIXTURE_TEST_CASE(init_wo_dynamics, InitConfigFixture)
    {
        config.setEnergyNormalization(true);
    }

    BOOST_FIXTURE_TEST_CASE(init_wo_rays, InitConfigFixture)
    {
        config.setParticleCount(0);
    }

    BOOST_FIXTURE_TEST_CASE(init_incompatible_support, InitConfigFixture)
    {
        config.setSupport(std::vector<double>{1.0, 1.0});
    }

    BOOST_FIXTURE_TEST_CASE(init_incompatible_offset, InitConfigFixture)
    {
        config.setOffset(zero_vec(2));
    }

    /*
     * This checks that valid initialization correctly sets the expected data.
     */

    BOOST_AUTO_TEST_CASE(init)
    {
        DummyICGenerator dummy(3, 2);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{1.0, 1.0, 1.0})
                .setOffset(zero_vec(3));
        BOOST_CHECK_THROW(dummy.next(), std::logic_error);
        dummy.init(config);
        BOOST_CHECK(dummy.init_called);
        BOOST_CHECK_EQUAL(dummy.getParticleCount(), 1000);
        BOOST_CHECK_NO_THROW(dummy.next());
    }

    /*
     * Check that iteration is not possible before init.
     */

    BOOST_AUTO_TEST_CASE(init_check)
    {
        DummyICGenerator dummy(3, 2);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{1.0, 1.0, 1.0})
                .setOffset(zero_vec(3));
        BOOST_CHECK_THROW(dummy.next(), std::logic_error);
    }

    /*
     * This check verifies that using multiple InitialCondition objects during iteration safely generates each IC
     * exactly once. This also checks that manifold index increments work as expected for 1D.
     */
    BOOST_AUTO_TEST_CASE(multi_iteration)
    {
        DummyICGenerator generator(1, 1, DummyICMode::DELAY);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{1.0})
                .setOffset(zero_vec(1));
        generator.init(config);

        auto counting = [&]() {
            std::vector<unsigned> index_counter(1000);
            for(auto cond = generator.next(); cond; ++cond) {
                index_counter[cond.getManifoldIndex()[0]] += 1;
                // some computation time here
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return index_counter;
        };

        std::vector<std::future<std::vector<unsigned>>> results;
        for(int i = 0; i < 10; ++i) {
            results.emplace_back(std::async(std::launch::async, counting));
        }

        std::vector<unsigned> counter(1000);

        for(auto& res: results) {
            auto result = res.get();
            std::transform(result.begin(), result.end(), counter.begin(), counter.begin(), std::plus<unsigned>());
            // printing; comment in for debugging
            // for(unsigned i : result) { std::cout << i << " "; } std::cout << "\n";
        }

        for(auto i : counter) {
            BOOST_CHECK_EQUAL(i, 1);
        }
    }

    /*
     * This function checks that InitialConditionGenerator correctly applies coordinate scaling when requested.
     * Also verifies that this is only applied to the position part.
     */
    BOOST_AUTO_TEST_CASE(coordinate_scaling)
    {
        DummyICGenerator generator(2, 2);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{2.0, 2.0})
                .setOffset(zero_vec(2));
        generator.init(config);

        auto ic = generator.next();
        for(unsigned i = 0; i < 2u; ++i) {
            // check that position was scaled
            BOOST_CHECK_CLOSE(ic.getState().getPosition()[i], 1.0, 1e-8);
            // but velocity was not.
            BOOST_CHECK_CLOSE(ic.getState().getVelocity()[i], 0.5, 1e-8);
        }

        config.setUseRelativeCoordinates(false);
        DummyICGenerator generator2(2, 2);
        generator2.init(config);
        auto ic2 = generator2.next();
        for(unsigned i = 0; i < 2u; ++i) {
            // check that position and velocity were not scaled
            BOOST_CHECK_CLOSE(ic2.getState().getPosition()[i], 0.5, 1e-8);
        }
    }

    /*
     * This function checks that InitialConditionGenerator correctly applies the coordinate offset, independently of
     * the scaling.
     */
    BOOST_AUTO_TEST_CASE(coordinate_offset)
    {
        DummyICGenerator generator(2, 2);
        gen_vect offset(2);
        offset[0] = 0.5;
        offset[1] = 0.5;
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{2.0, 2.0})
                .setOffset(offset);
        generator.init(config);

        auto ic = generator.next();
        for(unsigned i = 0; i < 2u; ++i) {
            // check that position was scaled
            BOOST_CHECK_CLOSE(ic.getState().getPosition()[i], 1.5, 1e-8);
            // but velocity was not.
            BOOST_CHECK_CLOSE(ic.getState().getVelocity()[i], 0.5, 1e-8);
        }

        config.setUseRelativeCoordinates(false);
        DummyICGenerator generator2(2, 2);
        generator2.init(config);
        auto ic2 = generator2.next();
        for(unsigned i = 0; i < 2u; ++i) {
            // check that position and velocity were not scaled
            BOOST_CHECK_CLOSE(ic2.getState().getPosition()[i], 1.0, 1e-8);
            BOOST_CHECK_CLOSE(ic.getState().getVelocity()[i], 0.5, 1e-8);
        }
    }

    /*
     * The test verifies that when energy normalization is enabled, is is performed. We cannot really check the validity
     * of the normalization here, as this falls outside the purview of the IC generator and has to be ensured by the
     * dynamics.
     */
    BOOST_AUTO_TEST_CASE(energy_normalization)
    {
        DummyICGenerator generator(2, 2);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(true).setParticleCount(1000).setSupport(std::vector<double>{1.0, 1.0})
                .setDynamics(std::make_shared<DummyDynamics>())
                .setOffset(zero_vec(2));

        generator.init(config);

        auto ic = generator.next();
        BOOST_CHECK_CLOSE(ic.getState().getVelocity()[0], -0.5, 1e-8);
    }

    /*
     * This test checks that a 2d manifold is sampled uniformly in manifold coordinate space.
     */
    BOOST_AUTO_TEST_CASE(manifold_sampling_2d)
    {
        DummyICGenerator generator(2, 2, DummyICMode::IDENTITY);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(10000).setSupport(std::vector<double>{1.0, 1.0})
              .setOffset(zero_vec(2));
        generator.init(config);

        std::vector<int> counter(10000);
        for(auto ic = generator.next(); ic; ++ic) {
            double x = std::floor(100*ic.getState().getPosition()[0]);
            double y = std::floor(100*ic.getState().getPosition()[1]);
            BOOST_REQUIRE_GE(x, 0);
            BOOST_REQUIRE_GE(y, 0);
            BOOST_REQUIRE_LT(x, 100);
            BOOST_REQUIRE_LT(y, 100);
            counter.at(static_cast<unsigned>(x) + 100 * static_cast<unsigned >(y)) += 1;
        }

        for(auto& c : counter)
        {
            BOOST_REQUIRE_EQUAL(c, 1);
        }
    }

    /*
     * This test case checks that the numerical calculation of the "delta" terms works as expected.
     */
    BOOST_AUTO_TEST_CASE(delta_calculation)
    {
        DummyICGenerator generator(2, 2, DummyICMode::IDENTITY);
        InitialConditionConfiguration config;
        config.setEnergyNormalization(false).setParticleCount(1000).setSupport(std::vector<double>{1.0, 1.0})
              .setOffset(zero_vec(2));
        generator.init(config);

        for(auto ic = generator.next(); ic; ++ic) {
            BOOST_CHECK_CLOSE(ic.getDelta(0).getPosition()[0], 1.0, 1e-6);
            BOOST_CHECK_SMALL(ic.getDelta(0).getPosition()[1], 1e-6);

            BOOST_CHECK_SMALL(ic.getDelta(1).getPosition()[0], 1e-6);
            BOOST_CHECK_CLOSE(ic.getDelta(1).getPosition()[1], 1.0, 1e-6);

            BOOST_CHECK_SMALL(ic.getDelta(0).getVelocity()[0], 1e-6);
            BOOST_CHECK_SMALL(ic.getDelta(0).getVelocity()[1], 1e-6);

            BOOST_CHECK_SMALL(ic.getDelta(1).getVelocity()[0], 1e-6);
            BOOST_CHECK_SMALL(ic.getDelta(1).getVelocity()[1], 1e-6);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
