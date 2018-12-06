//
// Created by erik on 9/11/17.
//

#ifndef BRANCHEDFLOWSIM_UTILS_H
#define BRANCHEDFLOWSIM_UTILS_H

#include <boost/test/test_tools.hpp>
#include <fstream>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

using namespace boost::accumulators;

///! \todo there should be a test for this utility
struct UniformityCheck
{
    UniformityCheck(bool enabled, std::string name) :
            counts(NUM_BINS, 0.0),
            mEnabled(enabled),
            mName( std::move(name) )
    {

    }

    ~UniformityCheck() {
        if(!mEnabled)
            return;

        std::string message = "Checking uniformity of distribution of " + mName;
        BOOST_TEST_CHECKPOINT(message);

        // evaluate
        using sum_accumulator_t = accumulator_set<double, stats<tag::sum> >;
        using max_accumulator_t = accumulator_set<double, stats<tag::max> >;

        sum_accumulator_t s_acc;
        max_accumulator_t m_acc;

        for(unsigned i = 0; i < NUM_BINS; ++i)
        {
            s_acc(counts[i]);
        }

        // Kolmogorov–Smirnov
        double partial_sum = 0;
        double total = extract::sum(s_acc);

        for(unsigned i = 0; i < NUM_BINS; ++i) {
            partial_sum += counts[i] / total;
            m_acc( std::abs(partial_sum - (i + 0.5) / NUM_BINS) );
        }

        double KS = extract::max(m_acc);

        // value taken from
        // https://de.wikipedia.org/wiki/Kolmogorow-Smirnow-Test
        double critical_value = 1.628 / sqrt(total);

        // log the test statistics
        BOOST_TEST_MESSAGE( boost::format("Kolmogorov–Smirnov statistics %1% for %2%") % KS % mName);

        BOOST_CHECK_LE(KS, critical_value);
    }

    void put_in_bin(double pos) {
        double bin_pos = pos * NUM_BINS;
        if(bin_pos < 0) {
            BOOST_FAIL("Invalid position for binning");
        }
        auto pos_min = static_cast<unsigned>(floor(bin_pos));
        double part = bin_pos - pos_min;
        // this does a little smoothing.
        counts.at((pos_min) % NUM_BINS) += 1 - part;
        counts.at((pos_min + 1) % NUM_BINS) += part;
    }

    void save(std::string filename)
    {
        std::fstream file(filename, std::fstream::out);
        for(auto     c : counts) {
            file << c << "\n";
        }
    }

    void enable() { mEnabled = true; }

    std::vector<double> counts;
    static constexpr std::size_t NUM_BINS = 1000;
    bool                mEnabled = false;
    std::string mName;
};

#include <boost/test/unit_test.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/sum.hpp>
#include <fstream>

#endif //BRANCHEDFLOWSIM_UTILS_H
