//
// Created by erik on 12/7/17.
//
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <deque>
#include <vector.hpp>
#include "args/error.hpp"
#include "args/spec.hpp"
#include "args/argument_set.hpp"

class check_what
{
public:
    explicit check_what(std::string w) : what(std::move(w))
    {}

    template<class T>
    bool operator()(const T& exception)
    {
        BOOST_CHECK_EQUAL(what, exception.what());
        return what == exception.what();
    }

private:
    std::string what;
};

BOOST_AUTO_TEST_SUITE(argument_set_test)

    BOOST_AUTO_TEST_CASE(arg_set_build)
    {
        args::ArgumentSet argset;
        argset << args::ArgumentSpec("optional").positional().optional();
        argset << args::ArgumentSpec("arg").positional();
        argset << args::ArgumentSpec("named_arg");
        argset << args::ArgumentSpec("arg2").positional();

        struct Visitor
        {
            void operator()(const args::ArgumentSpec& arg)
            {
                std::string EXPECTED[] = {"arg", "arg2", "optional", "named_arg"};
                BOOST_CHECK_EQUAL(arg.name(), EXPECTED[counter]);
                ++counter;
            }

            int counter = 0;
        };
        argset.visit_args(Visitor());
    }

    BOOST_AUTO_TEST_CASE(arg_set_name_validation)
    {
        args::ArgumentSet argset;
        argset << args::ArgumentSpec("same");
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("same"), std::logic_error,
                              check_what("An argument with name 'same' already exists"));

        argset << args::ArgumentSpec("other").positional().optional();
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("other"), std::logic_error,
                              check_what("An argument with name 'other' already exists"));

        argset << args::ArgumentSpec("positional").positional();
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("positional"), std::logic_error,
                              check_what("An argument with name 'positional' already exists"));
    }

    BOOST_AUTO_TEST_CASE(arg_set_alias_validation)
    {
        args::ArgumentSet argset;
        argset << args::ArgumentSpec("same").alias("forbidden");
        BOOST_CHECK_EXCEPTION( argset << args::ArgumentSpec("forbidden"), std::logic_error,
                               check_what("Cannot add argument 'forbidden' because alias 'forbidden' is already in use."));

        BOOST_CHECK_EXCEPTION( argset << args::ArgumentSpec("different").alias("same"), std::logic_error,
                               check_what("Cannot add argument 'different' because alias 'same' is already in use."));
    }

    BOOST_AUTO_TEST_CASE(arg_set_positional_after_multival)
    {
        args::ArgumentSet argset;
        std::vector<int>  t;
        argset << args::ArgumentSpec("same").store_many(t).positional();
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("other").positional(), std::logic_error,
                              check_what("Cannot add positional argument 'other' after multi value argument 'same'"));
    }

    BOOST_AUTO_TEST_CASE(arg_set_invalid_constant)
    {
        args::ArgumentSet argset;
        int t;
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("constant").store_constant(t, 5).required(), std::logic_error,
                              check_what("Constant value argument 'constant' cannot be required."));
        BOOST_CHECK_EXCEPTION(argset << args::ArgumentSpec("constant").store_constant(t, 5).positional(),
                              std::logic_error,
                              check_what("Constant value argument 'constant' cannot be positional."));
    }

    BOOST_AUTO_TEST_CASE(arg_set_simple_positional)
    {
        args::ArgumentSet argset;
        int               a = 0;
        int               b = 0;
        argset << args::ArgumentSpec("a").positional().store(a);
        argset << args::ArgumentSpec("b").positional().store(b).optional();

        argset.parse(std::vector<std::string>({"5", "8"}));
        BOOST_CHECK_EQUAL(a, 5);
        BOOST_CHECK_EQUAL(b, 8);

        /// \todo do we want default values?
        argset.parse(std::vector<std::string>({"12"}));
        BOOST_CHECK_EQUAL(a, 12);
        BOOST_CHECK_EQUAL(b, 8);
    }


    BOOST_AUTO_TEST_CASE(arg_set_simple_named)
    {
        args::ArgumentSet argset;
        int               a = 0;
        int               b = 0;
        argset << args::ArgumentSpec("a").store(a);
        argset << args::ArgumentSpec("b").store(b).optional();

        argset.parse(std::vector<std::string>({"a", "5", "b", "8"}));
        BOOST_CHECK_EQUAL(a, 5);
        BOOST_CHECK_EQUAL(b, 8);

        /// \todo do we want default values?
        argset.parse(std::vector<std::string>({"a", "12"}));
        BOOST_CHECK_EQUAL(a, 12);
        BOOST_CHECK_EQUAL(b, 8);
    }

    BOOST_AUTO_TEST_CASE(arg_set_single_sequence)
    {
        args::ArgumentSet argset;
        std::vector<int>  a;
        argset << args::ArgumentSpec("a").positional().store_many(a);

        argset.parse(std::vector<std::string>({"5", "8", "12"}));
        BOOST_REQUIRE_EQUAL(a.size(), 3);
        BOOST_CHECK_EQUAL(a.at(0), 5);
        BOOST_CHECK_EQUAL(a.at(1), 8);
        BOOST_CHECK_EQUAL(a.at(2), 12);
    }

    BOOST_AUTO_TEST_CASE(arg_set_single_sequence_with_named)
    {
        args::ArgumentSet       argset;
        std::deque<std::string> a;
        int                     next;
        argset << args::ArgumentSpec("a").positional().store_many(a);
        argset << args::ArgumentSpec("next").store(next);

        argset.parse(std::vector<std::string>({"a", "b", "c", "next", "5"}));
        BOOST_REQUIRE_EQUAL(a.size(), 3);
        BOOST_CHECK_EQUAL(a.at(0), "a");
        BOOST_CHECK_EQUAL(a.at(1), "b");
        BOOST_CHECK_EQUAL(a.at(2), "c");

        BOOST_CHECK_EQUAL(next, 5);

        a.clear();

        argset.parse(std::vector<std::string>({"next", "8"}));
        BOOST_CHECK(a.empty());
        BOOST_CHECK_EQUAL(next, 8);
    }

    BOOST_AUTO_TEST_CASE(arg_set_ublas_vector)
    {
        args::ArgumentSet       argset;
        gen_vect data;

        argset << args::ArgumentSpec("vec").store_many(data);

        argset.parse(std::vector<std::string>({"vec", "8", "5"}));

        BOOST_REQUIRE_EQUAL(data.size(), 2);
        BOOST_REQUIRE_EQUAL(data[0], 8);
        BOOST_REQUIRE_EQUAL(data[1], 5);
    }

    BOOST_AUTO_TEST_CASE(arg_set_visit)
    {
        args::ArgumentSet argset;
        argset << args::ArgumentSpec("a").positional();
        argset << args::ArgumentSpec("b").optional();
        argset << args::ArgumentSpec("c");

        struct Visitor {
            void operator()(const args::ArgumentSpec& arg) {
                names.insert(arg.name());
            }
            std::set<std::string> names;
        };

        Visitor v;
        argset.visit_args(v);

        BOOST_REQUIRE_EQUAL(v.names.size(), 3);
        BOOST_REQUIRE_EQUAL(v.names.count("a"), 1);
        BOOST_REQUIRE_EQUAL(v.names.count("b"), 1);
        BOOST_REQUIRE_EQUAL(v.names.count("c"), 1);
    }


    BOOST_AUTO_TEST_CASE(missing_required_positional)
    {
        args::ArgumentSet argset;
        double target;
        argset << args::ArgumentSpec("a").positional().store(target);

        std::vector<std::string> args{};
        BOOST_CHECK_EXCEPTION(argset.parse(args), args::missing_argument_error, check_what("Missing required positional argument 'a'."));
    }

    BOOST_AUTO_TEST_CASE(missing_required_named)
    {
        args::ArgumentSet argset;
        double target;
        argset << args::ArgumentSpec("a").store(target);

        std::vector<std::string> args{};
        BOOST_CHECK_EXCEPTION(argset.parse(args), args::missing_argument_error, check_what("Missing required argument 'a'."));
    }

    BOOST_AUTO_TEST_CASE(missing_value)
    {
        args::ArgumentSet argset;
        double target;
        argset << args::ArgumentSpec("a").store(target);

        std::vector<std::string> args{"a"};
        BOOST_CHECK_EXCEPTION(argset.parse(args), args::missing_value_error, check_what("Missing value for argument 'a'."));
    }

    BOOST_AUTO_TEST_CASE(additional_argument)
    {
        args::ArgumentSet argset;

        std::vector<std::string> args{"b"};
        BOOST_CHECK_EXCEPTION(argset.parse(args), std::runtime_error, check_what("Received unexpected argument 'b'"));
    }

    BOOST_AUTO_TEST_CASE(duplicate_argument)
    {
        args::ArgumentSet argset;
        double target;
        argset << args::ArgumentSpec("a").store(target).alias("c");

        // easy
        std::vector<std::string> args{"a", "5", "a", "10"};
        BOOST_CHECK_EXCEPTION(argset.parse(args), args::duplicate_argument_error, check_what("Received argument 'a' multiple times."));

        // with alias
        std::vector<std::string> args2{"a", "5", "c", "10"};
        BOOST_CHECK_EXCEPTION(argset.parse(args2), args::duplicate_argument_error, check_what("Received argument 'a' multiple times."));
    }
BOOST_AUTO_TEST_SUITE_END()
