#include "observer_factory.hpp"
#include "angular_histogram_obs.hpp"
#include "caustic_observer.hpp"
#include "density_observer.hpp"
#include "trajectory_observer.hpp"
#include "velocity_histogram_observer.hpp"
#include "velocity_transition_observer.hpp"
#include "wavefront_observer.hpp"
#include "potential.hpp"
#include "factory/builder_base.hpp"
#include "radial_density_observer.hpp"
#include <fstream>


namespace {

    class AngularHistogramBuilder : public ObserverBuilder {
    public:
        AngularHistogramBuilder() : ObserverBuilder("angle_histogram", false) {
            BuilderBaseType::args().description("bins the ray angles and creates histograms for all supplied time points.");
            BuilderBaseType::args() << args::ArgumentSpec("times").positional().optional().store(file_path).description(
                    "File from which to read the times at which the histograms are created. "
                            "If not supplied, histograms are distributed linearly between 0 and 1 in 0.01 steps.");

            BuilderBaseType::args() << args::ArgumentSpec("interval").positional().optional().store(interval).description(
                    "Size of the angular bins in radians.");
            BuilderBaseType::args() << args::ArgumentSpec("file_name").optional().store(file_name).description(
                    "Name of the file in which the angle histograms will be saved.");
        }
    private:

        std::shared_ptr<Observer> create(const Potential&) override {
            std::vector<double> times;
            for (int i = 1; i <= 100; ++i)
                times.push_back((double) i / 100.0);

            if (!file_path.empty()) {
                times.clear();
                std::fstream source(file_path);
                if (!source)
                THROW_EXCEPTION(std::runtime_error,
                                "could not open file %1% as source for angular histogram observer times",
                                file_path);

                double time;
                while (source >> time) {
                    times.push_back(time);
                }
            }

            return std::make_shared<AngularHistogramObserver>(std::move(times), interval, std::move(file_name));
        }
        
        double interval = 0.01;
        std::string file_path;
        std::string file_name = "angle_histograms.dat";
    };


    class CausticObserverBuilder : public ObserverBuilder {
    public:
        CausticObserverBuilder() : ObserverBuilder("caustics", true) {
            BuilderBaseType::args().description("records all caustics (including ray velocity and origin).");
            BuilderBaseType::args() << args::ArgumentSpec("break_on_first").positional().optional().store(break_on_first).description(
                    "If true, only the first caustic is recorded.");
            BuilderBaseType::args() << args::ArgumentSpec("file_name").optional().store(file_name).description(
                    "Name of the file in which the caustics will be saved.");
        }

    private:
        std::shared_ptr<Observer> create(const Potential& potential) override {
            return std::make_shared<CausticObserver>(potential.getDimension(), break_on_first, std::move(file_name));
        }
        
        bool break_on_first = false;
        std::string file_name = "caustics.dat";
    };


    class DensityObserverBuilder : public ObserverBuilder {
    public:
        DensityObserverBuilder() : ObserverBuilder("density", false) 
        {
            BuilderBaseType::args().description("records the ray density rho(x), or more generally "
                                          " f(x, v) * rho(x)");
            BuilderBaseType::args() << args::ArgumentSpec("center").alias("c").store_constant(center, true).optional().description(
                    "passing center causes all rays to be shifted such that "
                            "they start in the center of the image."
                   )
                                    << args::ArgumentSpec("size").alias("s").store_many(size).optional().description(
                              "'s'|'size' Int|Int...\n"
                              "Resolution of the density grid. Defaults to the resolution of the potential. "
                              "If only a single number is supplied, this is used for all dimensions."
                   )
                   << args::ArgumentSpec("support").alias("supp").store_many(support).optional().description(
                              "Support on which the density is recorded. Defaults to [0, 1]^d."
                   )
                   << args::ArgumentSpec("extractor").alias("e").store_many(extractor).optional().description(
                              "'e'|'extractor' 'density'|'vel' dir.\n"
                              "If set to density, records the ray density. If set to vel, the velocity density "
                              "(i.e. the flux rho*v) is recorded. dir determines the component of the velocity that is used"
                              " (e.g. density vel 0 records the x component of the flux density."
                   )
                   << args::ArgumentSpec("file_name").optional().store(file_name).description(
                              "Name of the file in which the density will be saved."
                   );
        }
        
    private:
        std::shared_ptr<Observer> create(const Potential& potential) override {

            if (size.empty()) {
                size = potential.getExtents();
            } else if (size.size() == 1) {
                size.resize(potential.getDimension(), size.front());
            }
            if (size.size() != potential.getDimension()) {
                THROW_EXCEPTION(std::runtime_error, "invalid size specified for density observer");
            }

            if (support.empty()) {
                support = potential.getSupport();
            } else if (support.size() == 1) {
                std::fill(begin(support), end(support), support.front());
            }
            if (support.size() != potential.getDimension()) {
                THROW_EXCEPTION(std::runtime_error, "invalid support specified for density observer");
            }

            std::function<float(const State&)> extractor_fn = [](const State&) { return 1.f; };
            // cannot use density here because that is already the name of an observer,
            // so things would get screwed up.
            if (extractor.at(0) == "dens") {
                if (extractor.size() != 1)
                THROW_EXCEPTION(std::runtime_error, "density extraction does not take additional "
                        "parameters (got %1%)", extractor.at(1));
            } else if (extractor.at(0) == "vel" || extractor.at(0) == "velocity") {
                if (extractor.size() != 2)
                THROW_EXCEPTION(std::runtime_error, "wrong number of args() (%1%) for velocity "
                        "extraction in density observer, expect 1.", extractor.size() - 1);

                auto dir = boost::lexical_cast<std::size_t>(extractor.at(1));
                if (dir >= support.size())
                THROW_EXCEPTION(std::runtime_error, "invalid direction %1% for velocity extraction "
                        "in density observer", dir);

                extractor_fn = [dir](const State& s) { return s.getVelocity()[dir]; };
                file_name = "velocity" + extractor.at(1) + ".dat";
            } else // unknown extractor
            {
                THROW_EXCEPTION(std::runtime_error, "unknown extractor %1% specified in density observer",
                                extractor.at(0));
            }

            /*! \todo
             *  extractor is still badly specified.
             */

            return std::make_shared<DensityObserver>(size, support, std::move(file_name), center, extractor_fn);
        }
        
        bool center = false;
        std::vector<std::size_t> size;
        std::vector<double> support;
        std::vector<std::string> extractor = {"dens"};
        std::string file_name = "density.dat";
    };


    class TrajectoryObserverBuilder : public ObserverBuilder {
    public:
        TrajectoryObserverBuilder() : ObserverBuilder("trajectory", false)
        {
            BuilderBaseType::args().description("records trajectory points in a defined time interval.");
            BuilderBaseType::args() << args::ArgumentSpec("interval").positional().store(interval).
                    optional().description("Time interval between recorded points.")
                                    << args::ArgumentSpec("file_name").optional().store(file_name).description(
                            "Name of the file in which the trajectories will be saved.");
        }

    private:
        std::shared_ptr<Observer> create(const Potential&) {
            return std::make_shared<TrajectoryObserver>(interval, std::move(file_name));
        }

        double interval = 0.01;
        std::string file_name = "trajectory.dat";
    };


    class VelHistObserverBuilder : public ObserverBuilder {
    public:
        VelHistObserverBuilder() : ObserverBuilder("velocity_histogram", false)
        {
            BuilderBaseType::args().description("bins the ray velocities and creates histograms for all supplied time points.");
            BuilderBaseType::args() << args::ArgumentSpec("times").positional().store(times_file).optional().description(
                    "File from which to read the times at which the histograms are created. "
                            "If not supplied, histograms are distributed linearly between 0 and 1 in 0.02 steps.")
                                    << args::ArgumentSpec("bins").positional().store(bin_count).optional().description(
                              "Number of velocity bins.")
                                    << args::ArgumentSpec("file_name").optional().store(file_name).description(
                              "Name of the file in which the velocity histograms will be saved.");
        }

    private:
        std::shared_ptr<Observer> create(const Potential& potential) {
            std::vector<double> times;
            for (int i = 1; i <= 50; ++i)
                times.push_back((double) i / 50.0);

            if (!times_file.empty()) {
                times.clear();
                std::fstream source(times_file, std::fstream::in);
                if (!source)
                THROW_EXCEPTION(std::runtime_error,
                                "could not open file %1% as source for angular histogram observer times",
                                times_file);

                double time;
                while (source >> time) {
                    times.push_back(time);
                }
            }
            return std::make_shared<VelocityHistogramObserver>(potential.getDimension(), std::move(times), bin_count,
                                                               std::move(file_name));
        }

        std::size_t bin_count = 100;
        std::string times_file;
        std::string file_name = "velocity_histograms.dat";
    };


    class VelocityTransitionObserverBuilder : public ObserverBuilder {
    public:
        VelocityTransitionObserverBuilder() : ObserverBuilder("velocity_transitions", false)
        {
            BuilderBaseType::args().description("Counts the transitions of velocities (or projections of the velocity) "
                                          "in a given time interval.");
            BuilderBaseType::args() << args::ArgumentSpec("interval").positional().store(interval).description(
                    "Time interval of recorded transitions.")
                                    << args::ArgumentSpec("bin_count").positional().store(bin_count).optional().description(
                              "Number of velocity bins.")
                                    << args::ArgumentSpec("start_time").store(start_time).optional().description(
                              "Start time of detection interval.")
                      << args::ArgumentSpec("end_time").store(end_time).optional().description(
                              "End time of detection interval.")
                      << args::ArgumentSpec("mode").store(mode).optional().description(
                              "Bit field that specifies which coordinates of pre and post velocity to record.")
                      << args::ArgumentSpec("increment").store_constant(increment, true).optional().description(
                              "Use velocity increments instead of absolute velocities.")
                      << args::ArgumentSpec("file_name").optional().store(file_name).description(
                              "Name of the file in which the velocity transitions will be saved.");
        }

    private:

        std::shared_ptr<Observer> create(const Potential& potential) {
            auto dim = potential.getDimension();

            std::vector<bool> in(dim, true);
            std::vector<bool> out(dim, true);
            in[0] = false;
            out[0] = false;

            if (!mode.empty()) {
                std::transform(begin(mode), begin(mode) + dim, begin(in), [](char c) { return c == '1'; });
                std::transform(begin(mode) + dim, end(mode), begin(out), [](char c) { return c == '1'; });
            }
            return std::make_shared<VelocityTransitionObserver>(potential.getDimension(), interval, bin_count,
                                                                start_time, end_time,
                                                                in, out, increment, std::move(file_name));
        }

        std::size_t bin_count = 100;
        double interval;
        double start_time = 0.0;
        double end_time = 1e100;
        std::string mode;
        bool increment = false;
        std::string file_name = "velocity_transitions.dat";
    };


    class WavefrontObserverFactory : public ObserverBuilder {
    public:
        WavefrontObserverFactory() : ObserverBuilder("wavefront", false)
        {
            BuilderBaseType::args().description("records the wavefront at a certain time as a 3d mesh.");
            BuilderBaseType::args() << args::ArgumentSpec("time").positional().store(pos).description(
                    "Time at which the complete wavefront is recorded.")
                                    << args::ArgumentSpec("file_name").optional().store(file_name).description(
                              "Name of the file in which the wavefront mesh will be saved.");
        }
    private:
        std::shared_ptr<Observer> create(const Potential&) {
            return std::make_shared<WavefrontObserver>(pos, std::move(file_name));
        }

        double pos;
        std::string file_name = "wavefront.ply";
    };

    class RadialDensityObserverFactory : public ObserverBuilder {
    public:
        RadialDensityObserverFactory() : ObserverBuilder("radial_density", false)
        {
            BuilderBaseType::args().description("records ray densities at given radii from ray staring point. "
                                                "Currently only supports 2D tracing.");
            BuilderBaseType::args() << args::ArgumentSpec("resolution").positional().store(res).description(
                    "Number of angular bins.")
                                    << args::ArgumentSpec("radii").store_many(radii).description(
                                            "Radii at which the density is recorded.")
                                    << args::ArgumentSpec("file_name").optional().store(file_name).description(
                                            "Name of the save file.");
        }
    private:
        std::shared_ptr<Observer> create(const Potential&) final {
            return std::make_shared<RadialDensityObserver>(res, std::move(radii), std::move(file_name));
        }

        int res;
        std::vector<double> radii;
        std::string file_name = "angular_density.dat";
    };
}

ObserverFactory& getObserverFactory() {
    static ObserverFactory factory;
    static bool init = false;
    if(!init) {
        factory.add_builder<AngularHistogramBuilder>();
        factory.add_builder<CausticObserverBuilder>();
        factory.add_builder<DensityObserverBuilder>();
        factory.add_builder<WavefrontObserverFactory>();
        factory.add_builder<VelocityTransitionObserverBuilder>();
        factory.add_builder<VelHistObserverBuilder>();
        factory.add_builder<TrajectoryObserverBuilder>();
        factory.add_builder<RadialDensityObserverFactory>();
        init = true;
    }
    return factory;
}
