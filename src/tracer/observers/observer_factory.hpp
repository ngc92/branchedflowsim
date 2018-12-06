#ifndef OBSERVER_FACTORY_HPP_INCLUDED
#define OBSERVER_FACTORY_HPP_INCLUDED

/*! \file
    \brief Helper classes and functions to register new observer types.
*/

class Observer;
class Potential;

#include "factory/factory.hpp"


/*!
 * \class ObserverBuilder
 * \brief An observer builder object is responsible for the creation of an observer.
 * \details For each Observer type there should be an accompanying ObserverBuilder type that implements the create
 *          method for that specific type. The builder than handles the passing and processing of command line
 *          arguments (handled by factory::BuilderBase). Furthermore an ObserverBuilder can specify whether it requires
 *          monodromy information to function correctly. In that case the tracer is automatically set up to perform
 *          monodromy tracing.
 * \sa factory::BuilderBase
 */
class ObserverBuilder : public factory::BuilderBase<std::shared_ptr<Observer>, const Potential&> {
public:
    /// gets whether the observer requires monodromy data.
    bool need_monodromy() const {
        return mNeedMonodromy;
    }
protected:
    /*!
     * \brief Constructor.
     * \details Set `need_monodromy` to `true` if your observer requires monodromy information. This will cause the
     *          tracer to automatically enable monodromy tracing if this observer is specified.
     */
    ObserverBuilder(std::string name, bool need_monodromy) :
            BuilderBase(std::move(name)),
            mNeedMonodromy(need_monodromy) {
    }

    bool mNeedMonodromy;
};

using ObserverFactory = factory::Factory<ObserverBuilder>;

ObserverFactory& getObserverFactory();

#endif // OBSERVER_FACTORY_HPP_INCLUDED
