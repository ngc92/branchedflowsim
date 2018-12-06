//
// Created by erik on 1/2/18.
//

#ifndef BRANCHEDFLOWSIM_ARG_VAL_HPP
#define BRANCHEDFLOWSIM_ARG_VAL_HPP

#include <functional>
#include <boost/lexical_cast.hpp>

namespace args {
    /*!
     * Enum that describes the amount of (command line) values an Argument
     * does expect.
     */
    enum class ArgValueCount {
        UNSPECIFIED, //!< No value set yet!
        NONE,        //!< Value less argument -- a simple flag.
        SINGLE,      //!< Single values argument. Only the next value belongs to this argument.
        MULTI        //!< Multi valued argument. All values until the next named argument belong to this one.
    };

    /*!
     * Enum that describes the input type that an argument expects. The main purpose of specifying
     * the type explicitly like this is the automatic generation of help messages.
     * The helper template arg_type can be used to deduce the ArgValueType based on the
     * c++ type of the target variable.
     */
    enum class ArgValueType {
        UNSPECIFIED, //!< No value set yet!
        BOOLEAN,     //!< A truth value: 0/1 or true, false, yes, no
        STRING,      //!< An arbitrary string value
        NUMBER,      //!< A real number
        INTEGER      //!< An integral number.
    };

    using HandlerFunction = std::function<void()>;
    using ParserFunction = std::function<HandlerFunction(const std::string&)>;

    /*!
     * \brief This class bundles information about the value of a factory argument.
     * \details We save number and type of argument, and functions for processing the
     *          command line input. The handler function will be executed if the argument
     *          is found, the parse function will generate a handler for every argument
     *          string processed. To create an ArgumentValue descriptor you can use
     *          one of the static functions in this class.
     *
     *          The destination for the parsed value is saved as a reference. The caller is responsible
     *          that the target is still valid when parsing is performed.
     *
     * \todo ArgumentValue for enum-like arguments, gen_vects etc.
     */
    class ArgumentValue {
    public:
        /// default constructor, leaves handler and parser empty and sets type and count to unspecified.
        ArgumentValue();

        /// Constructor that sets found handler, parse function, value count and type.
        ArgumentValue(HandlerFunction found, ParserFunction parse, ArgValueCount count, ArgValueType type);

        /*!
         * \brief ArgumentValue for a single valued argument.
         * \details Adds a parser that converts a single string using `parse_value` and returns a handler that
         * sets `target`.
         */
        template<class T>
        static ArgumentValue create_store_single(T& target);

        /*!
         * \brief ArgumentValue for a flag argument.
         * \details The found handler sets `target` to `value`. The parser function should never be called.
         */
        template<class T>
        static ArgumentValue create_store_constant(T& target, T value);

        /*!
         * \brief ArgumentValue for a multi valued argument.
         * \details Adds a parser that converts strings using `parse_value` and returns a handler that
         * adds them to `target` using `emplace_back`. The found handler clears `target`.
         */
        template<class C>
        static ArgumentValue create_store_sequence(C& target);

        /// gets the FoundHandler
        const HandlerFunction& getFoundHandler() const {
            return mFoundHandler;
        }

        /// gets the finalizer
        const HandlerFunction& getFinalizer() const {
            return mFinalizer;
        }

        void setFinalizer(HandlerFunction finalizer);

        /// applies the parse function to `string` and returns the resulting handler.
        HandlerFunction parseValue(const std::string& string) const;

        /// gets the value count.
        ArgValueCount count() const { return mCount; }
        /// gets the value's type.
        ArgValueType type() const { return mType; }
    private:
        ArgValueCount mCount;
        ArgValueType mType;

        HandlerFunction mFoundHandler;
        ParserFunction mParseFunction;
        HandlerFunction mFinalizer;
    };

    /*!
     * \name arg_type
     * \brief Gets the argument type as an ArgValueType based on a target variable.
    */

    inline ArgValueType arg_type(const bool&) { return ArgValueType::BOOLEAN; }
    inline ArgValueType arg_type(const float&) { return ArgValueType::NUMBER; }
    inline ArgValueType arg_type(const double&) { return ArgValueType::NUMBER; }
    inline ArgValueType arg_type(const std::string&) { return ArgValueType::STRING; }

    template<class T>
    ArgValueType arg_type(const T&, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr)
    {
        return ArgValueType::INTEGER;
    }

    namespace detail {
        /*!
         * \brief This class is used to parse a string into a usable value.
         * \details While the behaviour of `boost::lexical_cast` is close to what we want, it allows negative
         *          values to be passed to unsigned integers, resulting in huge numbers instead of an error.
         *          Therefore, this template dispatches depending on whether the target type is unsigned, and
         *          does manual checkin in that case, and a simple lexical_cast else.
         *
         * @tparam T The target type.
         * @tparam IsUnsigned Whether the target type is unsigned.
         */
        template<class T, bool IsUnsigned>
        struct ValueParser;

        template<class T>
        struct ValueParser<T, true> {
            static T parse(const std::string& token) {
                // this assumes we never need unsigned arguments > MAX(std::int64_t), which seems reasonable.
                auto safe_int = boost::lexical_cast<std::int64_t>(token);
                return boost::numeric_cast<T>(safe_int);
            }
        };

        template<class T>
        struct ValueParser<T, false> {
            static T parse(const std::string& token) {
                return boost::lexical_cast<T>(token);
            }
        };

        /// helper function that parses string tokens into usable values.
        /// Dispatches to the correct ValueParser instantiation.
        /// \sa ValueParser
        template<class T>
        T parse_value(const std::string& token) {
            return ValueParser<T, std::is_unsigned<T>::value>::parse(token);
        }
    }

    // implementation of the templated member functions.

    template<class T>
    ArgumentValue ArgumentValue::create_store_single(T& target) {
        T* target_ptr = &target;
        auto parser = [target_ptr](const std::string& str) {
            auto value = detail::parse_value<T>(str);
            return [value, target_ptr]() { (*target_ptr) = value; };
        };
        return ArgumentValue([](){}, parser, ArgValueCount::SINGLE, arg_type(target));
    }

    template<class T>
    ArgumentValue ArgumentValue::create_store_constant(T& target, T value) {
        T* target_ptr = &target;
        return ArgumentValue([value, target_ptr]() { (*target_ptr) = value; },
                             [](const std::string&) -> HandlerFunction
                             {
                                 std::terminate();
                             },
                             ArgValueCount::NONE, ArgValueType::UNSPECIFIED);
    }

    template<class C>
    ArgumentValue ArgumentValue::create_store_sequence(C& target) {
        C* target_ptr = &target;
        using value_t = typename C::value_type;

        auto parser = [target_ptr](const std::string& str) {
            auto value = detail::parse_value<value_t>(str);
            return [value, target_ptr]() { target_ptr->emplace_back(value); };
        };

        return ArgumentValue([&target]() { target.clear(); }, parser,
                             ArgValueCount::MULTI, arg_type(value_t()));
    }
}

#endif //BRANCHEDFLOWSIM_ARG_VAL_HPP
