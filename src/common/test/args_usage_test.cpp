//
// Created by erik on 1/8/18.
//

//
// Created by erik on 12/2/17.
//

#include <boost/test/unit_test.hpp>
#include <iostream>
#include "args/spec.hpp"
#include "args/argument_set.hpp"
#include "args/usage.hpp"


/*
 * This test suite checks the autogenerated usage and help strings.
 */
BOOST_AUTO_TEST_SUITE(args_usage_test)
    // usage string

    /*
     * Check that argument types, kinds (positional/named; optional/required) are correctly reflected in usage string.
     */
    BOOST_AUTO_TEST_CASE(usage_strings) {
        std::vector<int> t;
        args::ArgumentSet argset;
        int a;
        float f;
        argset << args::ArgumentSpec("a").positional().store(a);
        argset << args::ArgumentSpec("b").positional().store(a);
        BOOST_CHECK_EQUAL(args::usage_string(argset), "A B");

        argset << args::ArgumentSpec("c").positional().optional().store(a);
        BOOST_CHECK_EQUAL(args::usage_string(argset), "A B [C]");

        argset << args::ArgumentSpec("d").store(f);
        BOOST_CHECK_EQUAL(args::usage_string(argset), "A B [C] d NUMBER");

        argset << args::ArgumentSpec("e").optional().store_many(t);
        BOOST_CHECK_EQUAL(args::usage_string(argset), "A B [C] d NUMBER [e INTEGER ...]");
    }

    /*
     * Verify that the usage string handles flags correctly.
     */
    BOOST_AUTO_TEST_CASE(usage_string_flag) {
        args::ArgumentSet argset;
        int a;
        argset << args::ArgumentSpec("a").store_constant(a, 5);
        BOOST_CHECK_EQUAL(args::usage_string(argset), "[a]");
    }

    /*
     * Check that the ArgumentSet name is prepended the usage string if given.
     */
    BOOST_AUTO_TEST_CASE(usage_strings_named_argset) {
        int a;
        args::ArgumentSet named_argset("named");
        named_argset << args::ArgumentSpec("a").positional().store(a);
        BOOST_CHECK_EQUAL(args::usage_string(named_argset), "named A");
    }

    /*
     * Check that the usage string is correct even if there are no arguments. In particular, this checks that
     * there are no superfluous whitespaces.
     */
    BOOST_AUTO_TEST_CASE(usage_string_no_args) {
        BOOST_CHECK_EQUAL(args::usage_string(args::ArgumentSet("empty")), "empty");
    }

    // -----------------------------------------------------------------------------------------------------------------

    /*
     * Check that the help string generated for a simple set of arguments is what we would expect. We test
     * arguments of different types and also verify that things still work if no description is given for
     * the argument.
     */
    BOOST_AUTO_TEST_CASE(help_strings) {
        args::ArgumentSet argset;
        int i;
        float f;
        std::string str;
        argset << args::ArgumentSpec("a").positional().description("an argument").store(i);
        BOOST_CHECK_EQUAL(args::help_string(argset), "a (Integer): an argument\n");

        argset << args::ArgumentSpec("b").positional().store(str);
        argset << args::ArgumentSpec("c").positional().optional().description("another argument").store(f);

        std::string expected_help = "a (Integer): an argument\n"
                                    "b (String)\n"
                                    "c (Number): another argument\n";

        BOOST_CHECK_EQUAL(args::help_string(argset), expected_help);
    }

    /// \todo specify behaviour and add tests for arguments with aliases.

BOOST_AUTO_TEST_SUITE_END()