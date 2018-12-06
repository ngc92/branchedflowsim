#include "initial_conditions/radial_wave.hpp"
#include <cmath>
#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/sum.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <fstream>
#include <iostream>

using namespace boost::accumulators;

using accumulator_t = accumulator_set<double, stats<tag::sum, tag::mean, tag::variance, tag::max, tag::min > >;

BOOST_AUTO_TEST_SUITE(radial_init_tests)

BOOST_AUTO_TEST_CASE(phi_dist)
{
    using namespace boost;

    init_cond::RadialWave3D wave(3);
    wave.setEnergyNormalization(false);
    wave.setParticleCount(1000000);
    wave.init();
    
    std::size_t num_bins = 180;
    
    std::vector<std::size_t> counts(num_bins, 0);
    // fix range
    for(InitialCondition cond = wave.next(); cond; ++cond)
    {
        auto& vel = cond.getState().getVelocity();
        double x = vel[0];
        double y = vel[1];
        double z = vel[2];
        double c = std::sqrt(1 - z*z);
        double phi = std::atan2(y/c, x/c);
        //double azi = std::asin( z ) * 90 / std::acos(0);
        double phi_deg = std::floor(phi * 90 / std::acos(0) + 180);
        ++counts.at( int(phi_deg * num_bins / 360) );
    }
    
    std::fstream res("result.txt", std::fstream::out);
    
    accumulator_t acc;
    
    for(int i = 0; i < num_bins; ++i)
    {
        res << i << " " << counts[i] << std::endl; 
        acc(counts[i]);
    }
    
    double mx = max(acc);
    double mi = min(acc);
    double disx = (mx / mean(acc)-1) * 100;
    double disi = (1 - mi / mean(acc)) * 100;
    double max_relative_error = std::max(disx, disi);
    
    double mean_relative_error = std::sqrt(variance(acc) / sum(acc)) / num_bins * 100;
    
    // check that mean error is lower than .025%
    BOOST_CHECK( mean_relative_error < 0.025 );
    
    // check that maximum error is lower than 4.2%
    BOOST_CHECK( max_relative_error < 4.2 );

    std::cout << "mean relative error: " << mean_relative_error << "%\n";
    std::cout << "max relative error: "  << max_relative_error << "%\n";
}

BOOST_AUTO_TEST_SUITE_END()
