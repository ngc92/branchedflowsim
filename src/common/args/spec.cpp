//
// Created by erik on 1/7/18.
//

#include "spec.hpp"

using namespace args;

ArgumentSpec::ArgumentSpec(std::string name) :
        mName( std::move(name) ),
        mArgVal() {
    alias(mName);
}


ArgumentSpec& ArgumentSpec::alias(std::string alias) {
    // verify that alias does not contain whitespaces
    auto is_space = [](char c) { return std::isspace(c); };
    bool contains_space = std::any_of(begin(alias), end(alias), is_space);
    if(contains_space) {
        THROW_EXCEPTION(std::invalid_argument, "Argument name/alias '%1%' invalid (contains whitespace).", alias)
    }
    mAliases.emplace(std::move(alias));
    return *this;
}
