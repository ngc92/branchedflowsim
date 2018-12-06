//
// Created by erik on 12/5/17.
//

#include "argument_set.hpp"
#include "error.hpp"

using namespace args;

ArgumentSet& ArgumentSet::add_argument(ArgumentSpec arg)
{
    // check that the argument name is unique
    for(const auto& a : mRequiredPositionalArguments) {
        if(a.name() == arg.name())
            THROW_EXCEPTION(std::logic_error, "An argument with name '%1%' already exists", a.name());
    }

    for(const auto& a : mOptionalPositionalArguments) {
        if(a.name() == arg.name())
            THROW_EXCEPTION(std::logic_error, "An argument with name '%1%' already exists", a.name());
    }

    for(const auto& a : mNamedArguments) {
        if(a.name() == arg.name())
            THROW_EXCEPTION(std::logic_error, "An argument with name '%1%' already exists", a.name());
    }

    // check also that there are no clashing aliases
    for(const auto& alias : arg.aliases()) {
        if (is_argument_name(alias)) {
            THROW_EXCEPTION(std::logic_error, "Cannot add argument '%1%' because alias '%2%' is already in use.",
                            arg.name(), alias);
        }
    }

    if(arg.value_count() == ArgValueCount::NONE) {
        if(arg.is_required())
        {
            THROW_EXCEPTION(std::logic_error, "Constant value argument '%1%' cannot be required.", arg.name());
        } else if (arg.is_positional()) {
            THROW_EXCEPTION(std::logic_error, "Constant value argument '%1%' cannot be positional.", arg.name());
        }
    }

    if (arg.is_positional()) {
        if (arg.is_required()) {
            if(!mRequiredPositionalArguments.empty() &&
                    mRequiredPositionalArguments.back().value_count() == ArgValueCount::MULTI) {
                THROW_EXCEPTION(std::logic_error,
                                "Cannot add positional argument '%1%' after multi value argument '%2%'",
                                arg.name(), mRequiredPositionalArguments.back().name());
            }
            mRequiredPositionalArguments.emplace_back(std::move(arg));
        } else {
            mOptionalPositionalArguments.emplace_back(std::move(arg));
        }
    } else {
        mNamedArguments.emplace(std::move(arg));
    }
    return *this;
}

bool ArgumentSet::is_argument_name(const std::string& str) const
{
    auto found = std::find_if(begin(mNamedArguments), end(mNamedArguments),
                              [&str](const ArgumentSpec& arg) { return arg.is_match(str); });
    return found != end(mNamedArguments);
}

ArgumentSet::ArgumentSet(std::string name) : mName( std::move(name) )
{
}

ArgumentSet& ArgumentSet::operator<<(ArgumentSpec arg)
{
    return add_argument(std::move(arg));
}

ArgumentSet& ArgumentSet::description(std::string description)
{
    mDescription = std::move(description);
    return *this;
}

void ArgumentSet::parse_token_vector(const std::vector<std::string> &tokens) const
{
    auto first = tokens.begin();
    auto last = tokens.end();

    std::vector<HandlerFunction> success_actions;
    std::set<std::string> handled_named_args;

    // first process the positionals
    for (const auto& arg : mRequiredPositionalArguments) {
        assert(arg.is_required());
        assert(arg.is_positional());

        parse_argument(arg, first, last, success_actions);
    }

    auto optional_positional = begin(mOptionalPositionalArguments);


    // now use up the remaining args
    while (first != last) {
        // check for matching named arguments
        // the strategy is now as follows. First, we check for a matching named argument.
        // We assume that the argument spec is sensible, i.e. that there can only be one
        // matching named argument.
        auto match = std::find_if(begin(mNamedArguments), end(mNamedArguments),
                                  [&first](const ArgumentSpec& arg) { return arg.is_match(*first); });

        // if there was no matching named argument, it could be an optional positional.
        if (match == end(mNamedArguments)) {
            if (optional_positional == end(mOptionalPositionalArguments)) {
                THROW_EXCEPTION(std::runtime_error, "Received unexpected argument '%1%'", *first);
            }
            parse_argument(*optional_positional, first, last, success_actions);
            ++optional_positional;
        } else {
            // a named argument
            auto result = handled_named_args.insert(match->name());
            if(result.second) {
                parse_argument(*match, ++first, last, success_actions);
            } else {
                // if nothing was inserted, we have already seen this argument.
                THROW_EXCEPTION([match](const std::string& what){ return args::duplicate_argument_error(*match, what); },
                                "Received argument '%1%' multiple times.", match->name());
            }
        }
    }

    // check that all required args have been processed.
    for(auto& arg: mNamedArguments) {
        if(arg.is_required())
        {
            if(handled_named_args.count(arg.name()) == 0) {
                THROW_EXCEPTION([arg](const std::string& what){ return args::missing_argument_error(arg, what); },
                                "Missing required argument '%1%'.", arg.name());
            }
        }
    }

    // process the gathered data.
    for(auto& handler : success_actions) {
        handler();
    }

}

void ArgumentSet::parse_argument(const ArgumentSpec& arg, TokenIterator& first, const TokenIterator& last,
                                 std::vector<HandlerFunction>& handlers) const {
    handlers.emplace_back(arg.value().getFoundHandler());

    if(arg.value_count() == ArgValueCount::SINGLE) {
        if(!parse_single_token_argument(arg.value(), first, last, handlers)) {
            // no value submitted. When trying to read a positional argument, this means that the argument is missing,
            // whereas in case of a named argument this is "only" a missing value.
            if(arg.is_positional()) {
                THROW_EXCEPTION(
                        [arg](std::string message) {
                            return args::missing_argument_error(arg, message);
                        },
                        "Missing required positional argument '%1%'.", arg.name());
            } else {
                THROW_EXCEPTION(
                        [arg](std::string message) {
                            return args::missing_value_error(arg, message);
                        },
                        "Missing value for argument '%1%'.", arg.name());
            }
        }
    } else if(arg.value_count() == ArgValueCount::MULTI) {
        parse_multi_token_argument(arg.value(), first, last, handlers);
    }

    handlers.emplace_back(arg.value().getFinalizer());
}

bool ArgumentSet::parse_single_token_argument(const ArgumentValue& arg, TokenIterator& first, const TokenIterator& last,
                                              std::vector<HandlerFunction>& handlers) const
{
    if(first != last) {
        handlers.emplace_back(arg.parseValue(*first));
        ++first;
        return true;
    } else {
        return false;
    }
}

bool ArgumentSet::parse_multi_token_argument(const ArgumentValue& arg, TokenIterator& first, const TokenIterator& last,
                                             std::vector<HandlerFunction>& handlers) const
{
    while (first != last) {
        if (is_argument_name(*first)) break;
        handlers.emplace_back(arg.parseValue(*first));
        ++first;
    }
    return true;
}

