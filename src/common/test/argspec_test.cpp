//
// Created by erik on 12/2/17.
//

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <deque>
#include <list>
#include <vector.hpp>
#include "args/spec.hpp"
#include "args/argument_set.hpp"
#include "args/usage.hpp"

class check_what
{
public:
    explicit check_what(std::string w) : what(std::move(w)) { }
    template<class T>
    bool operator()(const T& exception){
        BOOST_CHECK_EQUAL(what, exception.what());
        return what == exception.what();
    }
private:
    std::string what;
};

/*
 * This test suite checks the args::ArgumentSpec class.
 */
BOOST_AUTO_TEST_SUITE(argspec_tests)

    struct ArgSpecFixture
    {
        ArgSpecFixture() : arg("arg") { }

        args::ArgumentSpec arg;
    };

    /*
     * Check that the constructor initializes as expected: A required, named argument.
     */
    BOOST_FIXTURE_TEST_CASE(constructor, ArgSpecFixture) {
        BOOST_CHECK_EQUAL(arg.name(), "arg");
        BOOST_CHECK_EQUAL(arg.description(), "");
        BOOST_CHECK_EQUAL(arg.is_required(), true);
        BOOST_CHECK_EQUAL(arg.is_positional(), false);
        BOOST_CHECK(arg.is_match("arg"));
    }

    /*
     * Checks that the setters and getters work as expected and can be chained.
     */
    BOOST_FIXTURE_TEST_CASE(argument_settings, ArgSpecFixture) {
        BOOST_CHECK(&arg.description("describe the arg") == &arg);
        BOOST_CHECK_EQUAL(arg.description(), "describe the arg");

        BOOST_CHECK(&arg.optional() == &arg);
        BOOST_CHECK_EQUAL(arg.is_required(), false);

        BOOST_CHECK(&arg.positional() == &arg);
        BOOST_CHECK_EQUAL(arg.is_positional(), true);
    }

    /*
     * Check that aliases can be added and are correctly matched.
     */
    BOOST_FIXTURE_TEST_CASE(aliases, ArgSpecFixture) {
        // argument name is first alias
        BOOST_CHECK(arg.is_match("arg"));
        BOOST_CHECK(!arg.is_match("ar"));

        // add another alias and check that one.
        BOOST_CHECK(&arg.alias("other") == &arg);
        BOOST_CHECK(arg.is_match("other"));
        BOOST_CHECK(arg.is_match("arg"));

        BOOST_CHECK_EQUAL(arg.aliases().size(), 2);
    }

    /*
     * Check that ArgumentSpec only accepts names and aliases without whitespaces.
     */
    BOOST_AUTO_TEST_CASE(name_validation) {
        BOOST_CHECK_EXCEPTION(args::ArgumentSpec arg("ar g"), std::logic_error, check_what("Argument name/alias 'ar g' invalid (contains whitespace)."));
        BOOST_CHECK_EXCEPTION(args::ArgumentSpec arg("arg\n"), std::logic_error, check_what("Argument name/alias 'arg\n' invalid (contains whitespace)."));
        args::ArgumentSpec arg("arg");
        BOOST_CHECK_EXCEPTION(arg.alias("o\ther"), std::logic_error, check_what("Argument name/alias 'o\ther' invalid (contains whitespace)."));
    }

    BOOST_FIXTURE_TEST_CASE(store_single, ArgSpecFixture) {
        int target = 0;
        BOOST_CHECK(&arg.store(target) == &arg);

        auto parser = arg.value();
        BOOST_CHECK(arg.value_type() == args::ArgValueType::INTEGER);
        BOOST_CHECK(arg.value_count() == args::ArgValueCount::SINGLE);
    }

    BOOST_FIXTURE_TEST_CASE(store_constant, ArgSpecFixture) {
        int target = 0;
        BOOST_CHECK(&arg.store_constant(target, 12) == &arg);
        BOOST_CHECK(!arg.is_required());

        BOOST_CHECK(arg.value_type() == args::ArgValueType::UNSPECIFIED);
        BOOST_CHECK(arg.value_count() == args::ArgValueCount::NONE);
    }

    BOOST_FIXTURE_TEST_CASE(store_many, ArgSpecFixture) {
        std::list<int> target;
        BOOST_CHECK(&arg.store_many(target) == &arg);

        BOOST_CHECK(arg.value_type() == args::ArgValueType::INTEGER);
        BOOST_CHECK(arg.value_count() == args::ArgValueCount::MULTI);
    }

    BOOST_FIXTURE_TEST_CASE(store_many_gen_vect, ArgSpecFixture) {
        gen_vect target;
        BOOST_CHECK(&arg.store_many(target) == &arg);
        arg.value().getFoundHandler()();
        arg.value().parseValue("1.2")();
        arg.value().parseValue("2.5")();
        arg.value().getFinalizer()();

        BOOST_REQUIRE_EQUAL(target.size(), 2);
        BOOST_CHECK_EQUAL(target[0], 1.2);
        BOOST_CHECK_EQUAL(target[1], 2.5);
    }

    BOOST_FIXTURE_TEST_CASE(store_many_gen_vect_no_value, ArgSpecFixture) {
        gen_vect target;
        BOOST_CHECK(&arg.store_many(target) == &arg);
        arg.value().getFoundHandler()();
        BOOST_CHECK_EXCEPTION(arg.value().getFinalizer()(), std::invalid_argument,
                              check_what("No value supplied for vector 'arg'"));
    }

    BOOST_FIXTURE_TEST_CASE(store_many_gen_vect_too_many, ArgSpecFixture) {
        gen_vect target;
        BOOST_CHECK(&arg.store_many(target) == &arg);
        arg.value().getFoundHandler()();
        for(int i = 0; i < 10; ++i) {
            arg.value().parseValue("1.2")();
        }
        BOOST_CHECK_EXCEPTION(arg.value().getFinalizer()(), std::invalid_argument,
                              check_what("To many values (10) supplied for vector 'arg'"));
    }

BOOST_AUTO_TEST_SUITE_END()