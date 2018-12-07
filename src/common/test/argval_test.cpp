//
// Created by erik on 1/2/18.
//

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <deque>
#include <list>
#include "args/value.hpp"

BOOST_AUTO_TEST_SUITE(argval_test)

    BOOST_AUTO_TEST_CASE(argval_ctor) {
        args::HandlerFunction empty_handler;
        args::HandlerFunction valid_handler = [](){};
        args::ParserFunction empty_parser;
        args::ParserFunction valid_parser = [](const std::string&){ return args::HandlerFunction(); };

        // this should work
        args::ArgumentValue val(valid_handler, valid_parser, args::ArgValueCount::SINGLE, args::ArgValueType::BOOLEAN);

        // invalid functions passed
        BOOST_CHECK_THROW(args::ArgumentValue(empty_handler, valid_parser,
                                  args::ArgValueCount::SINGLE, args::ArgValueType::BOOLEAN),
                          std::invalid_argument);

        BOOST_CHECK_THROW(args::ArgumentValue(valid_handler, empty_parser,
                                              args::ArgValueCount::SINGLE, args::ArgValueType::BOOLEAN),
                          std::invalid_argument);
    }

    BOOST_AUTO_TEST_CASE(arg_type) {
        using args::arg_type;
        using args::ArgValueType;
        bool b;
        float f;
        double d;
        int i;
        short s;
        std::uint64_t u;
        std::string str;

        BOOST_CHECK(arg_type(b) == ArgValueType::BOOLEAN);
        BOOST_CHECK(arg_type(f) == ArgValueType::NUMBER);
        BOOST_CHECK(arg_type(d) == ArgValueType::NUMBER);
        BOOST_CHECK(arg_type(i) == ArgValueType::INTEGER);
        BOOST_CHECK(arg_type(s) == ArgValueType::INTEGER);
        BOOST_CHECK(arg_type(u) == ArgValueType::INTEGER);
        BOOST_CHECK(arg_type(str) == ArgValueType::STRING);
    }

    BOOST_AUTO_TEST_CASE(store_constant) {
        double target = 0.0;
        const double value = 10.5;
        auto sc = args::ArgumentValue::create_store_constant(target, value);

        BOOST_CHECK(sc.count() == args::ArgValueCount::NONE);
        BOOST_CHECK(sc.type() == args::ArgValueType::UNSPECIFIED);
        sc.getFoundHandler()();

        BOOST_CHECK_EQUAL(target, value);
    }

    BOOST_AUTO_TEST_CASE(store_single)
    {
        std::string target = "INITIAL VALUE";
        auto sc = args::ArgumentValue::create_store_single(target);

        BOOST_CHECK(sc.count() == args::ArgValueCount::SINGLE);
        BOOST_CHECK(sc.type() == args::ArgValueType::STRING);
        sc.getFoundHandler()();

        auto handler = sc.parseValue("new value");
        BOOST_CHECK_EQUAL(target, "INITIAL VALUE");

        handler();
        BOOST_CHECK_EQUAL(target, "new value");
    }

    BOOST_AUTO_TEST_CASE(store_single_invalid_float)
    {
        int target = 0;
        auto sc = args::ArgumentValue::create_store_single(target);

        BOOST_CHECK(sc.count() == args::ArgValueCount::SINGLE);
        BOOST_CHECK(sc.type() == args::ArgValueType::INTEGER);
        sc.getFoundHandler()();

        BOOST_CHECK_THROW(sc.parseValue("15.8"), boost::bad_lexical_cast);
    }

    BOOST_AUTO_TEST_CASE(store_single_invalid_overflow)
    {
        short target = 0;
        auto sc = args::ArgumentValue::create_store_single(target);

        BOOST_CHECK(sc.count() == args::ArgValueCount::SINGLE);
        BOOST_CHECK(sc.type() == args::ArgValueType::INTEGER);
        sc.getFoundHandler()();

        BOOST_CHECK_THROW(sc.parseValue("999999"), std::bad_cast);
    }

    BOOST_AUTO_TEST_CASE(store_single_invalid_unsigned)
    {
        unsigned target = 0;
        auto sc = args::ArgumentValue::create_store_single(target);

        BOOST_CHECK(sc.count() == args::ArgValueCount::SINGLE);
        BOOST_CHECK(sc.type() == args::ArgValueType::INTEGER);
        sc.getFoundHandler()();

        BOOST_CHECK_THROW(sc.parseValue("-8"), std::bad_cast);
    }

    BOOST_AUTO_TEST_CASE(store_sequence)
    {
        std::vector<bool> target = {true, false};
        auto sc = args::ArgumentValue::create_store_sequence(target);

        BOOST_CHECK(sc.count() == args::ArgValueCount::MULTI);
        BOOST_CHECK(sc.type() == args::ArgValueType::BOOLEAN);

        const auto& fh = sc.getFoundHandler();
        auto h1 = sc.parseValue("1");
        auto h2 = sc.parseValue("0");
        BOOST_CHECK_EQUAL(target.size(), 2);

        fh(); h1(); h2();

        BOOST_CHECK_EQUAL(target.size(), 2);
        BOOST_CHECK_EQUAL(target.at(0), true);
        BOOST_CHECK_EQUAL(target.at(1), false);
    }

    BOOST_AUTO_TEST_CASE(finalizers)
    {
        auto av = args::ArgumentValue();
        BOOST_CHECK_THROW(av.setFinalizer(std::function<void()>()), std::invalid_argument);

        bool check = false;
        auto fin = [&](){check = true; };
        av.setFinalizer(fin);
        av.getFinalizer()();
        BOOST_CHECK(check);
    }

    /// \todo ensure that "true"|"false" are parsed correctly as booleans.

BOOST_AUTO_TEST_SUITE_END()