#include "init_factory.hpp"
#include "initial_conditions.hpp"
#include "factory/factory.hpp"
#include "generic_form.hpp"
#include "planar_wave.hpp"
#include "radial_wave.hpp"
#include "factory/builder_base.hpp"
#include <boost/numeric/ublas/io.hpp>

using namespace init_cond;
using args::ArgumentSpec;

namespace
{
    class GenericInitCondBuilder final : public InitBuilder
    {
    public:
        GenericInitCondBuilder() : BuilderBase("generic")
        {
            BuilderBaseType::args().description("Define an initial wavefront in terms of local coordinates u and v in lua syntax, "
                                       "and this generator will generate rays starting from that wavefront.");
            BuilderBaseType::args() << ArgumentSpec("term").positional().store(mTerm);
            BuilderBaseType::args() << ArgumentSpec("boundary").optional().store(mBoundary);
            BuilderBaseType::args() << ArgumentSpec("scale").optional().store(mScale);
        }

    private:
        std::unique_ptr<InitialConditionGenerator> create(unsigned dimension) override
        {
            auto res = make_unique<GenericCaustic2D>( dimension, mBoundary, mScale );
            res->setFunction( mTerm );
            return std::move(res);
        }

        double mBoundary = 0.0;
        double mScale = 1.0;
        std::string mTerm = "";
    };

    class PlanarInitCondBuilder final : public InitBuilder
    {
    public:
        PlanarInitCondBuilder() : BuilderBase("planar")
        {
            BuilderBaseType::args().description("Starts all rays from a plane/line.");
            BuilderBaseType::args() << ArgumentSpec("velocity").alias("vel").store_many(mVelocity).optional()
                    .description("Initial velocity of the starting rays.");
            BuilderBaseType::args() << ArgumentSpec("origin").alias("pos").store_many(mOrigin).optional()
                    .description("Origin position on the starting manifold.");
            /// \todo add spanning vectors, add spanning vectors to tests!
        }

    private:
        std::unique_ptr<InitialConditionGenerator> create(unsigned dimension) override
        {
            auto pw = make_unique<PlanarWave>(dimension, dimension - 1);
            if(mVelocity.size() != 0) {
                pw->setInitialVelocity(mVelocity);
            }

            if(mOrigin.size() != 0) {
                pw->setOrigin(mOrigin);
            }

            return std::move(pw);
        }

        gen_vect mVelocity{0};
        gen_vect mOrigin{0};
    };

    class RandomPlanarInitCondBuilder final : public InitBuilder
    {
    public:
        RandomPlanarInitCondBuilder() : BuilderBase("random_planar")
        {
            BuilderBaseType::args().description("The rays start at random positions, and in random directions. "
                               "The deltas are set up as if neighbouring rays started into the same direction.");
            BuilderBaseType::args() << ArgumentSpec("velocity").alias("vel").optional().store_many(mVelocity)
                    .description("Do not randomize initial velocity; use supplied value.");
            BuilderBaseType::args() << ArgumentSpec("origin").alias("pos").optional().store_many(mOrigin)
                    .description("Do not randomize initial position; use supplied value.");
        }

    private:
        std::unique_ptr<InitialConditionGenerator> create(unsigned dimension) override
        {
            auto pw = make_unique<RandomPlanar>(dimension);
            if(mVelocity.size() != 0) {
                pw->setFixedVelocity(mVelocity);
            }

            if(mOrigin.size() != 0) {
                pw->setFixedPosition(mOrigin);
            }

            return std::move(pw);
        }

        gen_vect mVelocity{0};
        gen_vect mOrigin{0};
    };

    class RadialInitCondBuilder final : public InitBuilder
    {
    public:
        RadialInitCondBuilder() : BuilderBase("radial")
        {
            BuilderBaseType::args().description("All rays start from a single point and are evenly distributed in angle.");
            BuilderBaseType::args() << ArgumentSpec("origin").alias("pos").optional().store_many(mOrigin)
                    .description("Origin position.");
        }

    private:
        std::unique_ptr<InitialConditionGenerator> create(unsigned dimension) override
        {
            if(mOrigin.size() == 0) {
                mOrigin.resize(dimension);
                for (unsigned i = 0; i < dimension; ++i)
                    mOrigin[i] = 0.5;
            }

            if (dimension == 2)
            {
                auto rw = make_unique<RadialWave2D>(dimension);
                rw->setOrigin(mOrigin);
                return std::move(rw);
            } else if (dimension == 3)
            {
                auto rw = make_unique<RadialWave3D>(dimension);
                rw->setOrigin(mOrigin);
                return std::move(rw);
            }

            THROW_EXCEPTION(std::runtime_error, "Invalid dimension %1% for RadialWave initial condition", dimension);
        }

        gen_vect mOrigin{0};
    };

    class RandomRadialInitCondBuilder final : public InitBuilder
    {
    public:
        RandomRadialInitCondBuilder() : BuilderBase("random_radial")
        {
            BuilderBaseType::args().description("The rays start at random positions, and in random directions. "
                               "The deltas are set up as if neighbouring rays started from the same point.");
            /// \todo make things configurable.
        }

    private:
        std::unique_ptr<InitialConditionGenerator> create(unsigned dimension) override
        {
            return make_unique<RandomRadial>(dimension);
        }
    };
}

InitFactory& init_cond::getInitialConditionFactory()
{
    static InitFactory factory;
    static bool init = false;
    if(!init) {
        factory.add_builder<GenericInitCondBuilder>();
        factory.add_builder<PlanarInitCondBuilder>();
        factory.add_builder<RandomPlanarInitCondBuilder>();
        factory.add_builder<RadialInitCondBuilder>();
        factory.add_builder<RandomRadialInitCondBuilder>();
        init = true;
    }
    return factory;
}

