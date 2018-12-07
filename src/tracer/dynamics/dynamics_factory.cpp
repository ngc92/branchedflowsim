#include "dynamics_factory.hpp"
#include "potential.hpp"
#include "ParticleInPotentialDynamics.hpp"
#include "sound.hpp"
#include "ParticleInScaledPotential.hpp"
#include "factory/builder_base.hpp"

using args::ArgumentSpec;

namespace
{
    class PartInPotBuilder final : public DynamicsBuilder
    {
    public:
        PartInPotBuilder() : BuilderBase("particle_potential")
        {
            BuilderBaseType::args().description("Dynamics of a massive particle in a potential.");
        }

    private:
        std::unique_ptr<RayDynamics> create(const Potential& potential, bool p, bool m) override
        {
            return make_unique<ParticleInPotentialDynamics>(potential, p, m);
        }
    };

    class PartInScaledPotBuilder final : public DynamicsBuilder
    {
    public:
        PartInScaledPotBuilder() : BuilderBase("particle_scaled_potential")
        {
            BuilderBaseType::args().description("Dynamics of a massive particle in a potential where the dynamics are scaled in x direction.");
            BuilderBaseType::args() << ArgumentSpec("scale").positional().store(mScale)
                    .description("Scaling factor.");
        }

    private:
        std::unique_ptr<RayDynamics> create(const Potential& potential, bool p, bool m) override
        {
            return make_unique<ParticleInScaledPotentialDynamics>(potential, p, m, mScale);
        }

        double mScale;
    };

    class SoundBuilder final : public DynamicsBuilder
    {
    public:
        SoundBuilder() : BuilderBase("sound")
        {
            BuilderBaseType::args().description("Dynamics of a massive particle in a potential, where the dynamics are scaled in x direction.");
            BuilderBaseType::args() << ArgumentSpec("sound_speed").optional().positional().store(mSoundSpeed)
                    .description("Sound speed.");
        }

    private:
        std::unique_ptr<RayDynamics> create(const Potential& potential, bool p, bool m) override
        {
            return make_unique<Sound>(potential, p, m, mSoundSpeed);
        }

        double mSoundSpeed = 1.0;
    };

}

DynamicsFactory& getDynamicsFactory() {
    static DynamicsFactory factory;
    static bool init = false;
    if(!init) {
        factory.add_builder<PartInPotBuilder>();
        factory.add_builder<PartInScaledPotBuilder>();
        factory.add_builder<SoundBuilder>();
        init = true;
    }
    return factory;
}