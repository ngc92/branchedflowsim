#include "example_observer.hpp"
#include "observers/observer_factory.hpp"
#include <fstream>
#include <boost/lexical_cast.hpp>

/*! \file example_observer.cpp
    This file defines an observer that does nothing useful, but is
    used as a tutorial demonstration on ho to write a new observer.
*/


//! [trivial] 
// ExampleObserver.cpp
ExampleObserver::ExampleObserver( std::string file_name, int n ) :
        ThreadLocalObserver( std::move(file_name) ),
        mN(n) { }

std::shared_ptr<ThreadLocalObserver> ExampleObserver::clone() const
{
    return std::make_shared<ExampleObserver>( filename(), mN );
}
//! [trivial] 

//! [watch] 
bool ExampleObserver::watch( const State& state, double t )
{
    // TODO check that mN is a valid index
    mSum += state.getVelocity()[mN];
    return true;    // continue this trajectory
}

void ExampleObserver::startTrajectory(const InitialCondition& start, std::size_t trajectory)
{
    mCount += 1; // count this trajectory.
}
//! [watch] 

//! [combine] 
void ExampleObserver::combine(ThreadLocalObserver& other)
{
    // first, we cast the given observer to the actual type.
    auto& data = dynamic_cast<ExampleObserver&>( other );
    
    mCount += data.mCount;
    mSum += data.mSum;
}

//! [combine] 
//! [save] 
void ExampleObserver::save( std::ostream& save_file )
{
    save_file << mCount << "\t" << mSum << "\n";
}
//! [save] 

//! [factory]

class ExampleObserverFactory : public ObserverBuilder {
public:
    ExampleObserverFactory() : ObserverBuilder("example", false)
    {
        args().description("An observer just for demo purposes.");
        args() << args::ArgumentSpec("n").positional().store(n).description("An integer value for n")
               << args::ArgumentSpec("file_name").optional().store(file_name).description(
                "Name of the file in which the results will be saved.");
    }
    std::shared_ptr<Observer> create(const Potential& potential) {
        return std::make_shared<ExampleObserver>( std::move(file_name), n );
    }
private:
    int n;
    std::string file_name = "example.dat";
};

//! [factory]
