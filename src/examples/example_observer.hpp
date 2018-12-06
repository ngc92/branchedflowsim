#ifndef EXAMPLE_OBSERVER_HPP_INCLUDED
#define EXAMPLE_OBSERVER_HPP_INCLUDED

/*! \file example_observer.hpp
    This file defines an observer that does nothing useful, but is
    used as a tutorial demonstration on ho to write a new observer.
*/

//! [example_observer]
// ExampleObserver.hpp
#include "observer.hpp"

class ExampleObserver final: public ThreadLocalObserver
{
public:
    // c'tor and d'tor
    ExampleObserver( std::string file_name, int n );
    ~ExampleObserver() = default;

    // standard observer functions
    // for documentation look at observer.hpp
    bool watch( const State& state, double t ) override;
    void start() override { }
    void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
    void save( std::ostream& save_file  ) override;

private:
    // thread local specific functions
    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine(ThreadLocalObserver& other) override;

    // configuration data
    int mN;
    
    // collected data
    int mCount = 0;
    double mSum = 0;
};

//! [example_observer]

#endif // EXAMPLE_OBSERVER_HPP_INCLUDED
