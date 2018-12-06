#ifndef INITIAL_CONDITIONS_FWD_HPP_INCLUDED
#define INITIAL_CONDITIONS_FWD_HPP_INCLUDED

#include <memory>
#include <string>
#include <vector>

// forward declarations for initial condition classes
namespace init_cond
{
	class InitialConditionGenerator;
    class InitialConditionConfiguration;
	class InitialCondition;

    using InitCondGenPtr = std::shared_ptr<InitialConditionGenerator>;

	// generator function
    InitCondGenPtr createInitialConditionGenerator( std::size_t dim, const std::vector<std::string>& arguments );
}

// bring relevant classes into global namespace
using init_cond::InitialCondition;
using init_cond::InitCondGenPtr;
using init_cond::InitialConditionConfiguration;
using init_cond::createInitialConditionGenerator;



#endif // INITIAL_CONDITIONS_FWD_HPP_INCLUDED
