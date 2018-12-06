//
// Created by erik on 1/2/18.
//

#include "value.hpp"
#include "global.hpp"

using namespace args;

ArgumentValue::ArgumentValue(HandlerFunction found, ParserFunction parse,
                             ArgValueCount count, ArgValueType type):
        mCount(count),
        mType(type),
        mFoundHandler(std::move(found)),
        mParseFunction(std::move(parse)),
        mFinalizer([](){})
{
    // check that the supplied functions are valid
    if(!mFoundHandler)
    {
        THROW_EXCEPTION(std::invalid_argument, "FoundHandler needs to be a non-empty std::function.");
    }

    if(!mParseFunction)
    {
        THROW_EXCEPTION(std::invalid_argument, "ParseFunction needs to be a non-empty std::function.");
    }
}

ArgumentValue::ArgumentValue() :
    mCount(args::ArgValueCount::UNSPECIFIED),
    mType(args::ArgValueType::UNSPECIFIED)
{
}

void ArgumentValue::setFinalizer(HandlerFunction finalizer) {
    if(!finalizer)
    {
        THROW_EXCEPTION(std::invalid_argument, "finalizer needs to be a non-empty std::function.")
    }
    mFinalizer = finalizer;
}

HandlerFunction ArgumentValue::parseValue(const std::string &string) const {
    return mParseFunction(string);
}
