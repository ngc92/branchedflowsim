//
// Created by erik on 12/5/17.
//

#ifndef BRANCHEDFLOWSIM_ARGS_V2_HPP
#define BRANCHEDFLOWSIM_ARGS_V2_HPP


#include <vector>
#include <string>
#include <set>
#include <functional>
#include <algorithm>
#include <cassert>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <typeindex>

#include "global.hpp"
#include "value.hpp"

/*!
 * \namespace args
 * \brief Specification and parsing of command line parameters.
 */


namespace args
{
    /*!
    \namespace args
    \details
    A small argument parser library is defined inside the `args` namespace. The reason for
    creating our own is twofold: For one, we want to process arguments recursively, e.g.
    when specifying observers. Secondly, we would like to autogenerate a python interface
    to built up these commands.

    The main class for handling cmd line arguments is the `ArgumentSet`. It is responsible for
     managing and parsing a group of arguments. An `ArgumentSet` consists of multiple
     `ArgumentSpec` objects, where each `ArgumentSpec` specifies a single argument.
     These two classes are all that is needed for most use cases. To automatically generate
     usage and help strings, the `usage_string()` and `help_string()` functions can be used.

     The translation from command line supplied arguments to c++ values is handled by the
     `ArgumentValue` class. There are functions for single value arguments that handle any
     streamable c++ object, for multiple arguments that can put in stl containers, and
     for constant arguments, that do not need any value and always produce the same output.

     An example: We create two arguments `"a"` and `"b"`. A is a positional argument whereas
     b is named and optional. Valid command lines are e.g. `"5 b 3"` or `"8"`.
     ```
        args::ArgumentSet argset;
        int               a = 0;
        int               b = 0;
        argset << args::ArgumentSpec("a").positional().store(a);
        argset << args::ArgumentSpec("b").store(b).optional();
     ```
     \attention It is your responsibility to ensure that `a` and `b` remain in scope util
     after the command line parsing.
    */


    /*!
     * \class ArgumentSpec
     * \brief Specification for a command line argument.
     * \details This class saves name (and aliases) and description of a command line argument.
     *          It also records whether the Argument is required and positional.
     *
     *          The actual parsing and saving of command line arguments is done by the
     *          corresponding ArgumentValue object. To attach an ArgumentValue call one of the
     *          store, store_many, store_constant functions. Multiple ArgumentSpec can be put together in
     *          an ArgumentSet which is used for parsing command line arguments.
     *
     *          For easy creation of ArgumentSpec objects, all setter methods support chaining,
     *          i.e. they return the object itself.
     */
    class ArgumentSpec
    {
    public:
        /*! Create an ArgumentSpec with a given name.
         * \details By default the new argument will be considered named (non-positional) and required.
         * \param name Name of the argument. Should not contain whitespaces.
         * \throws std::invalid_argument if name contains whitespaces.
         */
        explicit ArgumentSpec(std::string name);

        // getters
        const std::string& name() const { return mName; }
        const std::string& description() const { return mDescription; }
        bool is_required() const { return mIsRequired; }
        bool is_positional() const { return mIsPositional; }

        /// Gets the count of expected tokens for the argument.
        /// \sa ArgValueCount
        ArgValueCount value_count() const { return mArgVal.count(); }

        /// Gets the type of the argument.
        /// \sa ArgValueType
        ArgValueType value_type() const { return mArgVal.type(); }

        /// Gets the corresponding ArgumentValue which is responsible for
        /// parsing and saving arguments.
        const ArgumentValue& value() const { return mArgVal; }

        // setters
        /// Set (or update) the description.
        ArgumentSpec& description(std::string description) {
            mDescription = std::move(description);
            return *this;
        }

        /// Mark the argument as optional.
        /// \sa required()
        ArgumentSpec& optional() {
            mIsRequired = false;
            return *this;
        }

        /// Mark the argument as required.
        /// \sa optional()
        ArgumentSpec& required() {
            mIsRequired = true;
            return *this;
        }

        /// Mark the argument as positional.
        ArgumentSpec& positional() {
            mIsPositional = true;
            return *this;
        }

        /*! Add an alias for the argument.
         * \param alias Alias for the argument. Should not contain whitespaces.
         * \throws std::invalid_argument if name contains whitespaces.
         */
        ArgumentSpec& alias(std::string alias);

        /// gets the set of aliases.
        const std::set<std::string>& aliases() const { return mAliases; }

        /// Adds an ArgumentValue for storing the received argument
        /// in `target`.
        /// \sa ArgumentValue::create_store_single
        template<class T>
        ArgumentSpec& store(T& target) {
            mArgVal = ArgumentValue::create_store_single(target);
            return *this;
        }

        /// Adds an ArgumentValue for storing the `value`
        /// in `target` whenever argument is present.
        /// \sa ArgumentValue::create_store_constant
        template<class T>
        ArgumentSpec& store_constant(T& target, const T value) {
            mArgVal = ArgumentValue::create_store_constant(target, value);
            return this->optional();
        }

        /// Adds an ArgumentValue for storing multiple values in
        /// `target`. This requires that `target` supports `clear` and
        /// `push_back` methods.
        template<class C>
        ArgumentSpec& store_many(C& target, decltype(target.push_back(typename C::value_type()))* = nullptr) {
            mArgVal = ArgumentValue::create_store_sequence(target);
            return *this;
        }

        /// Adds an ArgumentValue for storing multiple values in
        /// `target`, for the special case of a `gen_vect` vector.
        template<class T, std::size_t N>
        ArgumentSpec& store_many(boost::numeric::ublas::c_vector<T, N>& target) {
            auto cache = std::make_shared<std::vector<T>>();
            std::string arg_name = name();
            auto finalizer = [cache, &target, arg_name]() {
                if(cache->empty())
                {
                    THROW_EXCEPTION(std::invalid_argument, "No value supplied for vector '%1%'", arg_name);
                }
                if(cache->size() > N) {
                    THROW_EXCEPTION(std::invalid_argument, "To many values (%1%) supplied for vector '%2%'",
                                    cache->size(), arg_name);
                }
                target.resize(cache->size());
                std::copy(begin(*cache), end(*cache), target.begin());
            };
            mArgVal = ArgumentValue::create_store_sequence(*cache);
            mArgVal.setFinalizer(finalizer);
            return *this;
        }

        /// Returns whether the token `name` matches any of the aliases of this
        /// argument.
        /// \sa alias()
        bool is_match(const std::string& name) const {
            return mAliases.count(name) != 0;
        }

    private:
        std::string mName;
        std::string mDescription = "";

        bool mIsRequired = true;
        bool mIsPositional = false;
        ArgumentValue mArgVal = ArgumentValue();

        std::set<std::string> mAliases = {};
    };
}

#endif //BRANCHEDFLOWSIM_ARGS_V2_HPP
