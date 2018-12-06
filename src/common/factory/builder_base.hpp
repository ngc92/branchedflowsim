//
// Created by eriks on 2/8/18.
//

#ifndef BRANCHEDFLOWSIM_FACTORY_BASE_HPP
#define BRANCHEDFLOWSIM_FACTORY_BASE_HPP

#include <vector>
#include <args/argument_set.hpp>

namespace factory
{
    namespace detail
    {
        /*! \class BuilderBaseType
         *  \brief Base type for all builders.
         *  \details This is the common builder interface that is independent of the built type and extra arguments.
         *           It keeps track of the args::ArgumentSet that parses the command line configuration.
         *
         *           Do not use this class directly! Define your own factory::BuilderBase and derive from that.
         *
         */
        class BuilderBaseType
        {
        public:
            explicit BuilderBaseType(std::string name) :
                    mArguments( std::move(name) ) {

            }

            /// get the name of the object type.
            std::string name() const {
                return args().name();
            }

            /// get the set of arguments that are expected/accepted.
            args::ArgumentSet& args() {
                return mArguments;
            }

            /// get the set of arguments that are expected/accepted.
            const args::ArgumentSet& args() const {
                return mArguments;
            }
        private:
            args::ArgumentSet mArguments;
        };
    }

    /*! \class BuilderBase
     * @tparam Produced Type of the object that is produced.
     * @tparam ExtraArgs Types of additional arguments that should be supplied to the create function. Types will be
     *                   used exactly as given -- specify as const reference if you do want to avoid copying.
     * \brief Base class template for all user defined builders.
     * \details A builder is an object that takes in some command line config (a sequence of strings) and optionally
     *        some additional arguments, and produces an object of type `Produced`.
     *
     *        The `BuilderBase` template defines a base class to be used for builders templated over the produced type
     *        and the types of the extra arguments. A typical use case is to typedef a builder base class for your given
     *        types and create a factory based on that builder. Then you can create a builder class that is derived from
     *        that for every type that your factory should be able to create.
     *
     *        The derived classes need to implement the pure virtual `create` method. Since they need the exact same
     *        signature, set the `ExtraArgs` template parameter exactly as you would like the parameters to be passed
     *        to `create`.
     *
     *        Note that the create function is not const. A `BuilderBase` object is expected to be used only once.
     *
     */
    template<class Produced, class... ExtraArgs>
    class BuilderBase : public detail::BuilderBaseType
    {
    public:
        using built_t = Produced;

        using BuilderBaseType::BuilderBaseType;

        /// create a new object using this factory.
        Produced operator()(const std::vector<std::string>& args, const ExtraArgs&... extra)
        {
            this->args().parse(args);
            return create(extra...);
        }
    private:

        virtual Produced create(ExtraArgs... extra) = 0;
    };
}

#endif //BRANCHEDFLOWSIM_FACTORY_BASE_HPP
