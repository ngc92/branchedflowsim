//
// Created by erik on 5/11/18.
//

#ifndef BRANCHEDFLOWSIM_RADIAL_DENSITY_OBSERVER_HPP
#define BRANCHEDFLOWSIM_RADIAL_DENSITY_OBSERVER_HPP

#include "observer.hpp"
#include "dynamic_grid.hpp"

class RadialDensityObserver final: public ThreadLocalObserver
{
public:
    /// create an observer and specify the time interval for saving the particle's position
    RadialDensityObserver( int resolution, std::vector<double> radii, std::string file_name = "trajectory.dat" );

    /// d'tor
    virtual ~RadialDensityObserver() = default;

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save(std::ostream& target) override;

private:
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    // configuration
    std::vector<double> mRadii;
    int mResolution;

    // cache
    gen_vect mLastPosition;
    gen_vect mStartPosition;

    double mLastRadius = 0;
    std::size_t mLastRadiusIndex = 0;

    std::vector<DynamicGrid<std::uint32_t>> mCounts;
};


#endif //BRANCHEDFLOWSIM_RADIAL_DENSITY_OBSERVER_HPP
