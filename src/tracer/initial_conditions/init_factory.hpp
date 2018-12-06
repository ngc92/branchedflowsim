#ifndef INIT_FACTORY_HPP_INCLUDED
#define INIT_FACTORY_HPP_INCLUDED

#include "factory/factory.hpp"

namespace init_cond
{
    class InitialConditionGenerator;

    using InitBuilder = factory::BuilderBase<std::unique_ptr<InitialConditionGenerator>, unsigned>;
    using InitFactory = factory::Factory<InitBuilder>;

    InitFactory& getInitialConditionFactory();
}

#endif // INIT_FACTORY_HPP_INCLUDED
