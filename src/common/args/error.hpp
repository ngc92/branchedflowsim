//
// Created by erik on 1/6/18.
//

#ifndef BRANCHEDFLOWSIM_ARG_ERROR_HPP
#define BRANCHEDFLOWSIM_ARG_ERROR_HPP

#include <stdexcept>
#include "spec.hpp"

namespace args
{
    /// Error thrown when parsing does not find a mandatory argument.
    class missing_argument_error : public std::runtime_error
    {
    public:
        missing_argument_error(ArgumentSpec arg, const std::string& message);

        const ArgumentSpec& arg() const { return mMissingArg; }
    private:
        ArgumentSpec mMissingArg;
    };

    /// Error thrown when parsing does not find a value for a specified argument.
    class missing_value_error : public std::runtime_error
    {
    public:
        missing_value_error(ArgumentSpec arg, const std::string& message);

        const ArgumentSpec& arg() const { return mArgument; }
    private:
        ArgumentSpec mArgument;
    };

    /// Error thrown when the same argument encountered twice during parsing.
    class duplicate_argument_error : public std::runtime_error {
    public:
        duplicate_argument_error(ArgumentSpec arg, const std::string& message);

        const ArgumentSpec& arg() const { return mDuplicateArgument; }

    private:
        ArgumentSpec mDuplicateArgument;
    };
}

#endif //BRANCHEDFLOWSIM_ARG_ERROR_HPP
