#ifndef GLOBAL_HPP_INCLUDED
#define GLOBAL_HPP_INCLUDED

/*! \file global.hpp
    \brief globally useful helpers, types, macros etc.
    \ingroup common
    \todo hide implementation details, better comments, missing tests
    \todo add more common typedefs
*/

#include <complex>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <boost/throw_exception.hpp>
#include <boost/format.hpp>

// optimization attributes
#ifdef NDEBUG
// check cmake compile config and set defines accordingly
#define HOT_FUNCTION __attribute__((hot, optimize("Ofast")))
#define FLATTEN_FN __attribute__((flatten))
#else
#define FLATTEN_FN
#define HOT_FUNCTION
#endif


/*! \def THROW_EXCEPTION
    \brief own exception macro.
    \details Creates an additional, non-inlined code path for creating and exception object,
            formatting an error message using boost.format and throwing that exception.
            This enables the possibility to throw exceptions from hot code paths without
            cluttering the instruction cache and simplifies the creation of error messages.
    \param type type of the exception being thrown.
    \param __VA_ARGS__ first argument is the error message format string, followed by the data for boost.format.
    \ingroup common
*/
#define THROW_EXCEPTION(type, ...)                                                              \
[&]() __attribute__((noinline,cold,noreturn)) {                                                 \
BOOST_THROW_EXCEPTION( boost::enable_error_info(type (::detail::build_error(__VA_ARGS__)) ) );    \
}();

/*! \def ASSERT_EQUAL
    \brief assertion macro for equality. not for performance relevant assertions
    \details This assertion will be performed in all compilation modes, regardless of a
            NDEBUG definitions. It is intended to be used for seldomly checked conditions
            where no performance penalty is incurred. expect and got should not have side effects.
*/
#define ASSERT_EQUAL(got, expect, message)             \
{                                                      \
    auto v1 = expect;                                  \
    auto v2 = got;                                     \
    if(__builtin_expect(v1 != v2, false))              \
    {                                                  \
      THROW_EXCEPTION(std::logic_error, message " [expected %1%, got %2%]", v1, v2);    \
    }                                                  \
}

namespace detail
{
    // exception helper functions
    /// recursion ending
    inline boost::format& format(boost::format& target)
    {
        return target;
    }


    /// \todo shouldn't this whole stuff work with std::forward?
    template<class A, class... Args>
    boost::format& format(boost::format& target, const A& a, const Args&... args)
    {
        return format(target % a, args...);
    }

    /// helper function for building an error message string using boost.format.
    template<class... Args>
    std::string build_error(const char* spec, const Args&... args)
    {
        boost::format fmt(spec);
        format(fmt, args...);
        return fmt.str();
    }
}
//

/// \addtogroup common
/// \{

/// define standard type for complex numbers
typedef std::complex<double> complex_t;

// define some constants
/// constexpr value for double pi
constexpr double pi = std::asin(1)*2;

/// shared_ptr helper function, will be obsolete with c++14 and is the analog to std::make_shared
/// used to construct unique_ptr objects with convenient syntax.
template<class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//! \}

#endif // GLOBAL_HPP_INCLUDED
