//
// Created by erik on 2/19/18.
//

#include <boost/test/unit_test.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "initial_conditions/planar_wave.hpp"
#include "initial_conditions/radial_wave.hpp"
#include "initial_conditions/init_factory.hpp"
#include "test_helpers.hpp"
#include "utils.hpp"

using zero_vec = boost::numeric::ublas::zero_vector<double>;

using namespace init_cond;

BOOST_AUTO_TEST_SUITE(cmdline_init_cond)
    /*
     * Check that the command line construction of planar initial conditions works as expected.
     * This entails correct defaults and parsing of initial velocity and origin from the command line.
     * We also test that mismatching dimensions are detected and diagnosed correctly.
     *
     * The command line interface is currently missing a method to set the spanning vectors!
     */
    BOOST_AUTO_TEST_CASE(planar)
    {
        using InitType = PlanarWave;

        // without arguments
        auto ic = getInitialConditionFactory().create("planar", {}, 2);
        InitType& planar_2d_default = dynamic_cast<InitType&>(*ic);
        BOOST_REQUIRE_EQUAL(planar_2d_default.getGeneratorType(), "planar");
        BOOST_REQUIRE_EQUAL(planar_2d_default.getWorldDimension(), 2);
        BOOST_CHECK_EQUAL(planar_2d_default.getInitialVelocity()[0], 1.0);
        BOOST_CHECK_EQUAL(planar_2d_default.getInitialVelocity()[1], 0.0);
        BOOST_CHECK_EQUAL(planar_2d_default.getOrigin()[0], 0.0);
        BOOST_CHECK_EQUAL(planar_2d_default.getOrigin()[1], 0.0);

        // full, valid specification
        ic = getInitialConditionFactory().create("planar", {"vel", "0", "1", "origin",  "0.3",  "0.2"}, 2);
        InitType& planar_2d_set = dynamic_cast<InitType&>(*ic);
        BOOST_CHECK_EQUAL(planar_2d_set.getInitialVelocity()[0], 0.0);
        BOOST_CHECK_EQUAL(planar_2d_set.getInitialVelocity()[1], 1.0);
        BOOST_CHECK_EQUAL(planar_2d_set.getOrigin()[0], 0.3);
        BOOST_CHECK_EQUAL(planar_2d_set.getOrigin()[1], 0.2);

        // dimension mismatch
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("planar", {"vel", "0"}, 2), std::invalid_argument,
                              check_what("1 dimensional initial velocity supplied for 2 dimensional world"));
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("planar", {"pos", "0", "1"}, 3), std::invalid_argument,
                              check_what("2 dimensional origin supplied for 3 dimensional world"));

        // very long vector
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("planar", {"pos", "0", "1", "5"}, 2),
                              std::invalid_argument, check_what("3 dimensional origin supplied for 2 dimensional world"));
    }

    /*
     * Check that the command line construction of random planar initial conditions works as expected.
     * This entails correct defaults and parsing of initial velocity and origin from the command line.
     * We also test that mismatching dimensions are detected and diagnosed correctly.
     */
    BOOST_AUTO_TEST_CASE(random_planar)
    {
        using InitType = RandomPlanar;

        // without arguments
        auto ic = getInitialConditionFactory().create("random_planar", {}, 2);
        InitType& planar_2d_default = dynamic_cast<InitType&>(*ic);
        BOOST_REQUIRE_EQUAL(planar_2d_default.getGeneratorType(), "random_planar");
        BOOST_REQUIRE_EQUAL(planar_2d_default.getWorldDimension(), 2);
        BOOST_CHECK(!planar_2d_default.getFixedVelocity());
        BOOST_CHECK(!planar_2d_default.getFixedPosition());

        // full, valid specification
        ic = getInitialConditionFactory().create("random_planar", {"vel", "0", "1", "origin",  "0.3",  "0.2"}, 2);
        InitType& planar_2d_set = dynamic_cast<InitType&>(*ic);
        BOOST_CHECK_EQUAL(planar_2d_set.getFixedVelocity().value()[0], 0.0);
        BOOST_CHECK_EQUAL(planar_2d_set.getFixedVelocity().value()[1], 1.0);
        BOOST_CHECK_EQUAL(planar_2d_set.getFixedPosition().value()[0], 0.3);
        BOOST_CHECK_EQUAL(planar_2d_set.getFixedPosition().value()[1], 0.2);

        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("random_planar", {"vel"}, 2),
                              std::invalid_argument, check_what("No value supplied for vector 'velocity'"));

        // dimension mismatch
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("random_planar", {"pos", "0", "1", "3"}, 2),
                              std::invalid_argument,
                              check_what("3 dimensional origin supplied for 2 dimensional world"));
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("random_planar", {"velocity", "0"}, 2),
                              std::invalid_argument,
                              check_what("1 dimensional velocity supplied for 2 dimensional world"));

        // very long vector
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("random_planar", {"pos", "0", "1", "5"}, 2),
                              std::invalid_argument, check_what("3 dimensional origin supplied for 2 dimensional world"));
    }

    /*
     * Check that the command line construction of two dimensional radial initial conditions works as expected.
     * This entails correct defaults and parsing of the origin from the command line.
     * We also test that mismatching dimensions are detected and diagnosed correctly.
     */
    BOOST_AUTO_TEST_CASE(radial_2d)
    {
        using InitType = RadialWave2D;

        // without arguments
        auto ic = getInitialConditionFactory().create("radial", {}, 2);
        InitType& default_2d = dynamic_cast<InitType&>(*ic);
        BOOST_REQUIRE_EQUAL(default_2d.getGeneratorType(), "radial");
        BOOST_REQUIRE_EQUAL(default_2d.getWorldDimension(), 2);
        BOOST_CHECK_EQUAL(default_2d.getOrigin()[0], 0.5);
        BOOST_CHECK_EQUAL(default_2d.getOrigin()[1], 0.5);

        // full, valid specification
        ic = getInitialConditionFactory().create("radial", {"origin",  "0.3",  "0.2"}, 2);
        InitType& radial_2d_set = dynamic_cast<InitType&>(*ic);
        BOOST_CHECK_EQUAL(radial_2d_set.getOrigin()[0], 0.3);
        BOOST_CHECK_EQUAL(radial_2d_set.getOrigin()[1], 0.2);

        // dimension mismatch
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("radial", {"pos", "0.1"}, 2), std::invalid_argument,
                              check_what("1 dimensional origin supplied for 2 dimensional world"));

        // very long vector
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("radial", {"pos", "0", "1", "5"}, 2),
                              std::invalid_argument, check_what("3 dimensional origin supplied for 2 dimensional world"));
    }

    /*
     * Check that the command line construction of three dimensional radial initial conditions works as expected.
     * This entails correct defaults and parsing of the origin from the command line.
     * We also test that mismatching dimensions are detected and diagnosed correctly.
     */
    BOOST_AUTO_TEST_CASE(radial_3d)
    {
        using InitType = RadialWave3D;

        // without arguments
        auto ic = getInitialConditionFactory().create("radial", {}, 3);
        InitType& default_3d = dynamic_cast<InitType&>(*ic);
        BOOST_REQUIRE_EQUAL(default_3d.getGeneratorType(), "radial_3d");
        BOOST_REQUIRE_EQUAL(default_3d.getWorldDimension(), 3);
        BOOST_CHECK_EQUAL(default_3d.getOrigin()[0], 0.5);
        BOOST_CHECK_EQUAL(default_3d.getOrigin()[1], 0.5);
        BOOST_CHECK_EQUAL(default_3d.getOrigin()[2], 0.5);

        // full, valid specification
        ic = getInitialConditionFactory().create("radial", {"origin",  "0.3",  "0.2", "0.9"}, 3);
        InitType& radial_3d_set = dynamic_cast<InitType&>(*ic);
        BOOST_CHECK_EQUAL(radial_3d_set.getOrigin()[0], 0.3);
        BOOST_CHECK_EQUAL(radial_3d_set.getOrigin()[1], 0.2);
        BOOST_CHECK_EQUAL(radial_3d_set.getOrigin()[2], 0.9);

        // dimension mismatch
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("radial", {"pos", "0.1"}, 3), std::invalid_argument,
                              check_what("1 dimensional origin supplied for 3 dimensional world"));

        // very long vector
        BOOST_CHECK_EXCEPTION(getInitialConditionFactory().create("radial", {"pos", "0", "1", "5"}, 2),
                              std::invalid_argument, check_what("3 dimensional origin supplied for 2 dimensional world"));
    }

    /*
     * RandomRadial currently has no command line configuration, so there is nothing to test.
     */


BOOST_AUTO_TEST_SUITE_END()