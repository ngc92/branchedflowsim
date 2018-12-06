//
// Created by eriks on 4/24/18.
//

#include "energy_error_observer.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include "dynamics/ray_dynamics.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <utility>
namespace pt = boost::property_tree;

EnergyErrorObserver::EnergyErrorObserver(std::string file_name) : ThreadLocalObserver(std::move(file_name))
{
}

void EnergyErrorObserver::startTrajectory(const InitialCondition& start, std::size_t trajectory)
{
    mInitialEnergy = mDynamics->getEnergy(start.getState());
}

void EnergyErrorObserver::endTrajectory(const State& final_state)
{
    double finalEnergy = mDynamics->getEnergy(final_state);
    double rel_err = std::abs((mInitialEnergy - finalEnergy) / mInitialEnergy);

    mCount += 1;
    mSum += rel_err;
    mMax = std::max(mMax, rel_err);
}

std::shared_ptr<ThreadLocalObserver> EnergyErrorObserver::clone() const
{
    return std::make_shared<EnergyErrorObserver>( filename() );
}

void EnergyErrorObserver::combine(ThreadLocalObserver& other)
{
    auto& data = dynamic_cast<EnergyErrorObserver&>( other );
    mSum += data.mSum;
    mCount += data.mCount;
    mMax = std::max(mMax, data.mMax);
}

void EnergyErrorObserver::save(std::ostream& target)
{
    pt::ptree tree;

    tree.put("count", mCount);
    tree.put("max", mMax);
    tree.put("sum", mSum);
    tree.put("mean", mSum / mCount);

    pt::write_json(target, tree);
}

double EnergyErrorObserver::getMaximumError() const
{
    return mMax;
}

void EnergyErrorObserver::startTracing()
{
    if(!mDynamics)
        THROW_EXCEPTION(std::logic_error, "Starting energy observation before dynamics have been set.");
}

double EnergyErrorObserver::getMeanError() const {
    return mSum / mCount;
}
