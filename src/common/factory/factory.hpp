//
// Created by eriks on 2/8/18.
//

#ifndef BRANCHEDFLOWSIM_BUILDER_HPP
#define BRANCHEDFLOWSIM_BUILDER_HPP

#include "global.hpp"
#include <utility>
#include <functional>
#include <memory>
#include <unordered_map>
#include <boost/any.hpp>
#include "builder_base.hpp"

namespace factory
{
    namespace detail
    {
        class FactoryBase {
        public:
            virtual args::ArgumentSet get_arguments(const std::string& type) const = 0;

            std::string getHelp(const std::string& type) const;
            void printHelp( ) const;

            /// get a vector that contains the names of all registered types.
            std::vector<std::string> getTypes() const;

        protected:
            /// do not use polymorphism based on FactoryBase!
            ~FactoryBase() = default;

            boost::any get_builder_any(const std::string& type) const;
            void add_builder_any(std::string type, boost::any builder);
        private:
            std::unordered_map<std::string, boost::any> mBuilders;
        };
    }

    /*! \tparam BuilderBase Base class for all builders managed by this factory.
     *  \brief Class that manages a set of object builders.
     *  \details This class allows to register different builders that offer a common construction interface.
     *
     */
    template<class BuilderBase>
    class Factory : public detail::FactoryBase
    {
    public:
        // typedefs
        using built_t = typename BuilderBase::built_t;
        using builder_t = std::unique_ptr<BuilderBase>;
        using make_builder_fn = std::function<builder_t()>;

        /*! \brief create object of type `type` with command line arguments `tokens` and additional
            arguments `args`.
            \details This function looks up the corresponding builder factory, creates a builder and applies it
                to `tokens` and `args`.
        */
        template<class... Args>
        built_t create( const std::string& type, const std::vector<std::string>& tokens, Args&&... args ) const
        {
            return (*get_builder(type))(tokens, std::forward<Args>(args)...);
        }

        /*! \brief Creates a builder for `type`.
         */
        builder_t get_builder(const std::string& type) const {
            auto factory = boost::any_cast<make_builder_fn>(get_builder_any(type));
            return factory();
        };

        /// Gets the ArgumentSet of builder for `type`.
        args::ArgumentSet get_arguments(const std::string& type) const final
        {
            return get_builder(type)->args();
        }

        /// Adds a new builder type. The type to be added is specified as the template parameter.
        template<class BuilderType>
        void add_builder() {
            auto maker = [](){ return make_unique<BuilderType>(); };
            add_builder_any(BuilderType().name(), static_cast<make_builder_fn>(maker));
        }
    };
}

#endif //BRANCHEDFLOWSIM_BUILDER_HPP
