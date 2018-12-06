//
// Created by erik on 12/6/17.
//

#ifndef BRANCHEDFLOWSIM_USAGE_HPP
#define BRANCHEDFLOWSIM_USAGE_HPP

#include <sstream>
#include <iterator>
#include <boost/algorithm/string/case_conv.hpp>
#include "spec.hpp"
#include "argument_set.hpp"

namespace args
{
    // -----------------------------------------------------------------------------------------------------------------
    //                                               Usage String
    // -----------------------------------------------------------------------------------------------------------------

    /*! prints the usage string of `args` to `stream`.
     *
     * The usage string consists of the (optional) program name (`name` of `args`) followed by a short specification
     * of the arguments and their types. Positional arguments are marked by an uppercase version of there name whereas
     * named arguments are described by a sequence of `name DATATYPE`. Optional arguments are put in brackets and multi
     * values are marked by an ellipsis `...`.
     * The purpose of the usage string is to show the syntax of passing arguments.
     */

    void usage_string(std::ostream& stream, const ArgumentSet& args);

    /// convenience function to get a usage_string in a std::string.
    /// \sa void usage_string(std::ostream& stream, const ArgumentSet& args)
    std::string usage_string(const ArgumentSet& args);

    // -----------------------------------------------------------------------------------------------------------------
    //                                               Help Message
    // -----------------------------------------------------------------------------------------------------------------

    /*! prints a help string for the arguments in `args` to `stream`.
     *
     * The help string contains the descriptions of all arguments as well as of the ArgumentSet. It also notes the
     * expected data types.
     */
    void help_string(std::ostream& stream, const ArgumentSet& args);

    /// convenience function to get a help string in a std::string.
    /// \sa void help_string(std::ostream& stream, const ArgumentSet& args)
    std::string help_string(const ArgumentSet& args);

    // -----------------------------------------------------------------------------------------------------------------
    //                                               Python Glue
    // -----------------------------------------------------------------------------------------------------------------

    /// creates a string that is used to auto-generate python glue code.
    /// \todo writing to a more universal format would be good (maybe json)
    void argspec_string(std::ostream& stream, const args::ArgumentSet& arguments);

    /// convenience function to get a argspec string in a std::string.
    /// \sa argspec_string(std::ostream& stream, const args::ArgumentSet& arguments)
    std::string argspec_string(const args::ArgumentSet& arguments);
}

#endif //BRANCHEDFLOWSIM_USAGE_HPP
