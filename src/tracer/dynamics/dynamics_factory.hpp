#ifndef DYNAMICS_FACTORY_HPP_INCLUDED
#define DYNAMICS_FACTORY_HPP_INCLUDED
#include "factory/factory.hpp"

/*! \file
	\brief Helper classes and functions to register new dynamics types.
	\todo unify with observer factory and look at initial conditions, those might be similar.
*/

class RayDynamics;
class Potential;

using DynamicsBuilder = factory::BuilderBase<std::unique_ptr<RayDynamics>, const Potential&, bool, bool>;
using DynamicsFactory = factory::Factory<DynamicsBuilder>;

DynamicsFactory& getDynamicsFactory();


#endif // DYNAMICS_FACTORY_HPP_INCLUDED
