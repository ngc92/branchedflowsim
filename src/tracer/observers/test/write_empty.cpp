//
// Created by erik on 8/16/18.
//

#include <fstream>
#include <observers/caustic_observer.hpp>
#include <observers/density_observer.hpp>
#include <observers/energy_error_observer.hpp>
#include <observers/radial_density_observer.hpp>
#include <observers/trajectory_observer.hpp>
#include <observers/velocity_transition_observer.hpp>
#include <observers/velocity_histogram_observer.hpp>
#include <observers/wavefront_observer.hpp>
#include "observers/angular_histogram_obs.hpp"

void save_observer(Observer& observer)
{
    std::ofstream out(observer.filename(), std::ofstream::out | std::ofstream::binary);
    observer.save(out);
}

int main()
{
    // Angular Histogram
    std::vector<double> time_intervals = {1.0, 2.0, 3.0};
    AngularHistogramObserver a_obs(time_intervals, 10);
    save_observer(a_obs);

    CausticObserver c_obs(3, false);
    save_observer(c_obs);

    std::vector<std::size_t> size{200, 100};
    std::vector<double> support{1.0, 2.0};
    DensityObserver d_obs(size, support, "density.dat");
    save_observer(d_obs);

    EnergyErrorObserver e_obs;
    save_observer(e_obs);

    std::vector<double> radii{1.0, 2.0, 3.0};
    RadialDensityObserver r_obs(128, radii);
    save_observer(r_obs);

    TrajectoryObserver t_obs(1.0);
    save_observer(t_obs);

    std::vector<bool> in{true, false};
    std::vector<bool> out{true, false};
    VelocityTransitionObserver v_obs(2, 1.0, 32, 0.5, 1.0, in, out, false);
    save_observer(v_obs);

    VelocityHistogramObserver vh_obs(2, time_intervals, 32);
    save_observer(vh_obs);

    WavefrontObserver w_obs(1.0);
    save_observer(w_obs);
}