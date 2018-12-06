#ifndef DENSITY_OBSERVER_HPP_INCLUDED
#define DENSITY_OBSERVER_HPP_INCLUDED

#include "dynamic_grid.hpp"
#include "vector.hpp"
#include "observer.hpp"
#include <memory>

class DensityWorker;
class Potential;

/*! \brief class that observers trajectory density
    \details this class measures the trajectory density on a grid with user defined
            granularity. For this to work, each gridpoint should be passed by a few particles,
            otherwise, discrete traces instead of a smooth distribution will be observed.
    \note does not require any additional data in the tracer to work.
*/
class DensityObserver final: public ThreadLocalObserver
{
    typedef DynamicGrid<float> density_grid_type;
    static float default_extractor(const State&) { return 1.f; };
public:
    /// create an observer and specify the size of the density track object.
    DensityObserver( std::vector<std::size_t> size, std::vector<double> support,
                    std::string file_name,
                    bool re_center = false,
                    std::function<float(const State&)> extractor = default_extractor );

    // standard observe functions
    void startTrajectory(const InitialCondition&, std::size_t) override;
    void endTrajectory(const State& final_state) override;
    bool watch( const State& state, double t ) override;

    void endTracing(std::size_t particleCount) override;

    // save to file
    void save(std::ostream& target) override;

    // info functions
    const density_grid_type& getDensity() const;

    // type to save interpolated dot info
    struct IPDot
    {
        gen_vect pos;
        double weight;
    };

private:
    /// add an interpolated line. \p start and \p end have to be in observer coords
    void addInterpolatedLine( const gen_vect& start, const gen_vect& end, double weight );

    std::shared_ptr<ThreadLocalObserver> clone() const override;
    void combine( ThreadLocalObserver& other ) override;

    std::size_t mDimension;
    double mDPIFactor;
    std::vector<double> mScalingFactor;
    std::vector<double> mSupport;
    std::vector<std::size_t> mSize;

    double mLastTime;
    gen_vect mLastPosition;

    // cache all points into a vector before offloading them to the worker
    std::vector<IPDot> mDotCache;

    // helper class that is shared between all instances of density observer
    std::shared_ptr<DensityWorker> mWorker;

    // function that extracts the information from the game state. This
    // is what we record in the end.
    std::function<float(const State&)> mExtractFunction;
    // set to true to make trajectories centered around their starting point.
    bool mCenterOnStart;
    gen_vect mStartingPosition;

// needs to be public so make_shared can access this
public:
    DensityObserver( std::vector<std::size_t> size, std::vector<double> support,
                    std::string file_name,
                    bool re_center,
                    std::function<float(const State&)> extractor,
                    std::shared_ptr<DensityWorker> worker);
};


#endif // DENSITY_OBSERVER_HPP_INCLUDED
