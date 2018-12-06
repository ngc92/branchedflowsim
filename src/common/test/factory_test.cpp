//
// Created by erik on 2/9/18.
//

#include <boost/test/unit_test.hpp>
#include "factory/factory.hpp"
#include "factory/builder_base.hpp"
#include "test_helpers.hpp"

struct TestBuilder : public factory::BuilderBase<int, float>
{
    TestBuilder() : BuilderBase("test") {
        args() << args::ArgumentSpec("arg").store(argument);
        args().description("description");
    }

    int create(float extra)
    {
        return 5 + (int)extra + argument;
    }

    int argument = -1;
};

BOOST_AUTO_TEST_SUITE(factory_tests)

    BOOST_AUTO_TEST_CASE(builder_base) {
        TestBuilder test;

        std::vector<std::string> tokens = {"arg", "6"};

        BOOST_CHECK_EQUAL(test.name(), "test");
        BOOST_CHECK_EQUAL(test(tokens, 3.0f), 14);
    }

    BOOST_AUTO_TEST_CASE(factory_double_add) {
        factory::Factory<TestBuilder> fac;
        fac.add_builder<TestBuilder>();
        BOOST_CHECK_EXCEPTION(fac.add_builder<TestBuilder>(), std::logic_error,
                              check_what("A builder for type 'test' is already registered."));
    }

    BOOST_AUTO_TEST_CASE(factory_test) {
        factory::Factory<TestBuilder> fac;
        BOOST_CHECK(fac.getTypes().empty());

        fac.add_builder<TestBuilder>();
        BOOST_CHECK(fac.getTypes() == std::vector<std::string>{"test"});

        // exception on non-existing builder.
        BOOST_CHECK_EXCEPTION(fac.get_builder("other"), std::runtime_error,
                              check_what("Unknown factory for type 'other' requested!"));
        BOOST_CHECK_EXCEPTION(fac.create("other", {"arg"}, 1.5f), std::runtime_error,
                              check_what("Unknown factory for type 'other' requested!"));
        BOOST_CHECK_EXCEPTION(fac.get_arguments("other"), std::runtime_error,
                              check_what("Unknown factory for type 'other' requested!"));
        BOOST_CHECK_EXCEPTION(fac.getHelp("other"), std::runtime_error,
                              check_what("Unknown factory for type 'other' requested!"));

        BOOST_CHECK_EQUAL(fac.get_arguments("test").name(), "test");

        std::string help_msg =  "test arg INTEGER\n"
                                "description\n"
                                "arg (Integer)\n";
        BOOST_CHECK_EQUAL(fac.getHelp("test"), help_msg);

        auto creation = fac.create("test", {"arg", "6"}, 1.f);
        BOOST_CHECK_EQUAL(creation, 12);
    }

BOOST_AUTO_TEST_SUITE_END()
