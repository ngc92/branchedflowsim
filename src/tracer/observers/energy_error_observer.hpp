//
// Created by eriks on 4/24/18.
//

#ifndef BRANCHEDFLOWSIM_ENERGY_ERROR_OBSERVER_HPP
#define BRANCHEDFLOWSIM_ENERGY_ERROR_OBSERVER_HPP

#include "observer.hpp"

class EnergyErrorObserver : public ThreadLocalObserver {
public:
    /// create an observer and specify the time interval for saving the particle's position
    EnergyErrorObserver( std::string file_name = "energy.json" );

    /// d'tor
    virtual ~EnergyErrorObserver() = default;

    // standard observer functions
    // for documentation look at observer.hpp
    void startTracing() override;
    bool watch( const State& state, double t ) override { return false; }
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void endTrajectory(const State& final_state) override;
    void save(std::ostream& target) override;

    double getMaximumError() const;
    double getMeanError() const;

private:
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    double mInitialEnergy;

    std::size_t mCount = 0;
    double mSum = 0.0;
    double mMax = 0.0;
};

#endif //BRANCHEDFLOWSIM_ENERGY_ERROR_OBSERVER_HPP
