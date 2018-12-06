//
// Created by erik on 1/6/18.
//

#include "error.hpp"

using namespace args;

missing_argument_error::missing_argument_error(ArgumentSpec arg, const std::string& message) :
    runtime_error(message), mMissingArg(std::move(arg))
{

}

missing_value_error::missing_value_error(ArgumentSpec arg, const std::string& message) :
        runtime_error(message), mArgument(std::move(arg))
{

}


duplicate_argument_error::duplicate_argument_error(ArgumentSpec arg, const std::string& message) :
        runtime_error(message), mDuplicateArgument(std::move(arg))
{

}