/*! \file observer.hpp
    \brief tracking integration results
    \todo test and comment
*/

#ifndef OBSERVER_HPP_INCLUDED
#define OBSERVER_HPP_INCLUDED

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <iosfwd>
#include <mutex>
#include "state.hpp"
#include "initial_conditions_fwd.hpp"

class MasterObserver;
class RayDynamics;

/*! \class Observer
    \brief Base class for objects that track tracing results.
    \details provides an interface via virtual functions to allow abstraction between different observers.
            Provides functions that are called at the start of tracing, at the end and before a new trajectory is started.
*/

class Observer : public boost::noncopyable, public std::enable_shared_from_this<Observer>
{
public:
    explicit Observer(std::string file_name);

    /// \brief generic watch function that is called after each integration step.
    ///    \details The state parameter is just a pointer to an internal buffer, so
    ///            it is not save to keep the reference after the function call.
    /// \return True, if the observer wants further data points, false if it is finished.
    virtual bool watch(const State& state, double t) = 0;

    /// Initializes the Observer before tracing.
    void init(std::shared_ptr<const RayDynamics> dynamics);
    /// Returns true if the observer has been initialized.
    bool is_ready() const;

    /// \brief called when tracing starts. Currently redundant, because multiple tracings are not supported.
    /// \note This function is only called once, not once per thread.
    virtual void startTracing() {};
    /// \brief called when tracing ends, can be used to finalize data.
    /// \note This function is only called once, not once per thread.
    virtual void endTracing(std::size_t /*particleCount*/) {};

    /// \brief called when a new particle is started. Used to submit information about the
    ///     initial condition of the observed trajectory, as well as its index.
    virtual void startTrajectory(const InitialCondition& /*start*/, std::size_t /*trajectory*/) = 0;

    /// \brief called when tracing of a trajectory ends.
    /// \param final_state: The last state on the trajectory.
    virtual void endTrajectory(const State& final_state) {};

    /// set the path to the save file.
    void setFileName(std::string path);

    /// gets the name of the desired save file
    const std::string filename() const;

    /// \brief called to save the gathered results into the `target` stream. Should be in binary mode!
    virtual void save( std::ostream& target ) = 0;

    /// \brief observer pointer for a new thread
    /// \details returns an observer pointer to an observer for a new thread. If the Observer type
    ///            can sensible be copied, returns a copy, otherwise, a pointer to this Observer.
    ///            the behaviour is implemented in the ThreadLocalObserver and ThreadSharedObserver classes.
    virtual std::shared_ptr<Observer> makeThreadCopy() = 0;
protected:
    std::string mFileName;

    bool mIsInitialized = false;
    std::shared_ptr<const RayDynamics> mDynamics;
};

/*! \brief base class for thread local observers.
    \details This class is used as a base class that are local to each tracing thread.
*/
class ThreadLocalObserver : public Observer
{
public:
    explicit ThreadLocalObserver(std::string file_name);

    virtual ~ThreadLocalObserver();

    /*! \brief combines the results of the observer into the
            root object.
        \details This function is called inside the destructor of the master observer,
                so we call it upon thread exit.
    */
    void reduce();

    /*!
     * Returns whether this observer is "slaved" to a root observer. In that case you need to call
     * `reduce()` before deleting this observer.
     */
    bool is_slave() const;

    /*!
     * Returns whether this observer is root observer for other observers.
     */
    bool is_root() const;

    std::shared_ptr<Observer> makeThreadCopy() final;
private:
    /*! this variable points to the root observer for each clone, and to null
        for the original. This ensures that the clones are cleaned up beforehand
        and allows to add an additional gather phase to combine the data of
        the single observers for final processing.
        \note After the observer has been merged with its root, this variable is set
                to null.
    */
    std::shared_ptr<ThreadLocalObserver> mRootObserver;

    /// creates a new copy of the observe of this type, with the same settings.
    virtual std::shared_ptr<ThreadLocalObserver> clone() const = 0;


    /// called to combine the data of the different observers.
    /// combine data from \p other into this!
    virtual void combine( ThreadLocalObserver& other ) = 0;

    /// this variable contains the mutex for protecting access to
    boost::optional<std::mutex> mRootMutex;
};

/*! \brief base class for shared observer.
    \details This class is used as the base class for observers that are shared
            between all threads. It provides additional functions for locking the
            observer. The cloning interface is implemented to return a pointer to
            the original observer to be used in all threads.
*/
class ThreadSharedObserver : public Observer
{
public:
    explicit ThreadSharedObserver(std::string file_name);

    /// \brief gets the mutex
    /// \details gets the mutex that protects this specific observer
    std::unique_lock<std::mutex> getLock();

    /// since all threads share the same observer object, this function just returns shared_from_this
    std::shared_ptr<Observer> makeThreadCopy() final;
private:
    std::mutex mMutex;
};


/*! \page new_obs Writing a new Observer
    \section imp_choice Implementation choices.
    To add a new observer, you need to create a new class derived from the Observer base class.
    However, it is not possible to use Observer directly as a base class (it is allowed, but the
    created observer will never be able to collect any data), instead it has to be derived from either
    ThreadLocalObserver or ThreadSharedObserver.
    
    \par ThreadLocalObserver
        Use this base if your observer works mostly independently on the different trajectories. 
        The system will then automatically generate a clone of the observer for each thread.
        At the end of the program run, the original observer will call the ThreadLocalObserver::combine() method for
        each of the thread copies, which is responsible for copying the data into a single observer
        for saving.
        <ul>
        <li> Advantages:     No synchronization between observers needed so it scales better to multiple threads.
        <li> Disadvantages:  Requires to implement the additional methods from cloning and combining.
        
    \par ThreadSharedObserver
        Use this base class if your observer has to combine data from different trajectories in a 
        nontrivial way. Independently of the number of threads used to simulate the trajectories, 
        only one instance of this observer will exist, and calls to observer are mutex protected.
        <ul>
        <li>Advantages:        Easier implementation:
        <li>Disadvantages:    Due to mutex locking, somewhat slower and does not scale well if the observation
                        takes a significant amount of time compared to the trajectory calculation.
    
    \par Class stub.
    As the ThreadLocalObserver is the more complex one, but also expected to be more common, 
    we will assume in the following always a ThreadLocalObserver. A typical class definition will then
    look like
    \code    
    #include "observer.hpp"

    class YourObserver final: public ThreadLocalObserver
    {
    public:
        // c'tor and d'tor
        YourObserver( Arg1, Arg2, ... );
        ~YourObserver() = default;

        // standard observer functions
        // for documentation look at observer.hpp
        bool watch( const State& state, double t ) override;
        void start() override;
        void startTrajectory(const InitialCondition& start, std::size_t trajectory) override;
        void save( ) override;

    private:
        // thread local specific functions
        std::shared_ptr<ThreadLocalObserver> clone() const override;
        void combine(ThreadLocalObserver& other) override;

        // configuration data
        
        // collected data
    };
    \endcode
    
    Typically, you will not need to implement the start() method, nor a destructor. The purpose of the other methods 
    can be found in the documentation of Observer, and ThreadLocalObserver resp. (You can use the links in the code 
    above to go there directly). 
    For the clone method, you will typically need something like
    \code
    std::shared_ptr<ThreadLocalObserver> YourObserver::clone() const
    {
        return std::make_shared<YourObserver>( Arg1, Arg2 );
    }
    \endcode
    where you pass the same arguments as were used for originally creating the observer.
    At minimum, an observer should take a file name as a configuration argument (which has to be passed to the
    constructor of its base class anyway). This value can be accessed using the Observer::filename() function
    to pass it on correctly in the clone function (Strictly speaking, since only the original observer will be used
    for saving, the file name set for the clones does not matter, but for consistency it is nice to pass the correct
    value nonetheless).

    \section Registration
    To register a new `Observer` so that it can be specified from the command line, you need to create a builder class
    for the `Observer` and register it with the global observer factory. The builder needs to be derived from
    ObserverBuilder and implement the `create` method. For a more thorough explanation see the documentation
    of ObserverBuilder.

    \code
    class YourObserverBuilder : public ObserverBuilder {
    public:
        YourObserverBuilder() : ObserverBuilder("your_observer") {
            args().description("a description for this observer.");
            // add more arguments and make them store in member variables of this class.
        }

    private:
        std::shared_ptr<Observer> create(const Potential&) override {
        // at this point, the args::ArgumentSet has processed the arguments and their values are safely stored in
           // member variables.
            return std::make_shared<YourObserver>(
             // use what you stored in members + potential to generate the observer.
            );
        }
        // here you can put variables to receive values from the command line.
    };
    \endcode

    The observer builder needs to be registered with the global observer factory. This can be done by calling
    `getObserverFactory().add_builder<YourObserverBuilder>()` before any command line is processed. The most simple
    solution is to add your observer builder to the registry code in `observer_factory.cpp`.

    By convention your observer should expose at least the command line argument `file_name` to specify the name of the
    save file.

    \section example_obs An example observer.
    As a demonstration, the following code is for an (useless) observer that counts the number of trajectories and
    calculates the sum of a specified velocity component over all trajectory points. You can find the code in
    `example_observer.hpp` and `example_observer.cpp` resp.
    The class definition could look as follows:
    \snippet example_observer.hpp example_observer
    
    The trivial methods are implemented as follows:
    \snippet example_observer.cpp trivial
    
    The constuctor just sets the parameter, and clone creates a new object with the same arguments.
    
    The watch functions themselves are like this:
    \snippet example_observer.cpp watch
    \attention Remember to return true from the watch function, unless your observer 
            does not want to continue receiving data for this trajectory.
    
    Finally, we need the combine method, that reduces the data into a single observer and save the result. 
    In this case, the reduction is simply summing the local results.
    \snippet example_observer.cpp combine
    \note Since combine is called when the thread local Observer objects are about to be 
            destructed, it is save to gather the data from them using std::move (if possible) to prevent
            having to reallocate memory.
    
    For saving we get passed a file open in binary mode. To write binary files, the \p fileIO.hpp
    header defines some utility functions. Since we do not write much data, however, it is OK to write
    in plain text here.
    \snippet example_observer.cpp save
    
    In order to use the observer (i.e., let it be enabled from the command line), you need to create a builder that
    describes the command line and creates the observer. This builder needs to be registered with the global
    observer factory by doing `getObserverFactory().add_builder<ExampleObserverFactory>();`. For the example observer
    the builder could look like this:
    \snippet example_observer.cpp factory
*/

#endif // OBSERVER_HPP_INCLUDED
