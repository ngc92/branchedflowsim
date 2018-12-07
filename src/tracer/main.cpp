#include <fstream>
#include <iostream>
#include <cmath>
#include <string>
#include <boost/lexical_cast.hpp>
#include "fileIO.hpp"
#include "potential.hpp"
#include "trace_args.h"
#include "tracer.hpp"
#include "tracer_factory.h"
#include "initial_conditions_fwd.hpp"
#include "observers/observer.hpp"
#include "profiling.hpp"
#include <boost/exception/diagnostic_information.hpp>
#include <chrono>

using namespace std;

void trace( const std::shared_ptr<Tracer>& tracer );
void print_duration(std::ostream& stream, std::string intro, std::chrono::high_resolution_clock::time_point start)
{
    auto dur = std::chrono::high_resolution_clock::now() - start;
    auto dur_sec = std::chrono::duration_cast<std::chrono::seconds>(dur);
    stream << intro << dur_sec.count() << "s "
           << std::chrono::duration_cast<std::chrono::milliseconds>(dur - dur_sec).count() << "ms" << std::endl;
}

int main(int argc, char* argv[])
{
	try
	{
		parse_parameters(argc, argv);

		setMaximumMemoryAvailable( targs::memory_avail * 1024 * 1024 );

		/// \todo allow rand init
		srand(0);

		// ugly and security risk
		system(("mkdir -p " + targs::result_file).c_str());


		// set all parameters in the tracer factory
		auto start = std::chrono::high_resolution_clock::now();
		TracerFactory factory;
		factory.loadFile( targs::potential_source_file );
		factory.setPeriodicBondaries( targs::periodic );
		if(targs::override_strength)
		{
			factory.setPotentialStrength( targs::POTENTIAL_STRENGTH );
		}
		factory.setObserverConfig( targs::observers );
		factory.setDynamicsConfig( targs::dynamics );
		factory.setThreadCount( targs::thread_count );
		factory.setErrorBounds( targs::abs_err_bound, targs::rel_err_bound );
		factory.setEndTime( targs::end_time );
		factory.setIntegrator( targs::integrator );
		factory.setTimeStep( targs::time_step );

		// save general data file
		std::fstream gdata(targs::result_file+"/config.txt", std::fstream::out);
		gdata << "# command line\n";
		for(int i = 0; i < argc; ++i)
			gdata << argv[i] << " ";
		gdata << "\n\n# potential data\n";
		gdata << factory.getPotentialInfo() << "\n";
		gdata << "\n# tracing info\n";
		gdata << "\n  energy normalization " << !targs::no_norm_energy << std::endl;

		std::cout << "potinfo: " << factory.getPotentialInfo() << std::endl;

		// create the tracer and do tracing
		std::size_t total_particles = 0;
		std::shared_ptr<Tracer> tracer = factory.createTracer( );
        print_duration(std::cout, "setup took ", start);

		trace( tracer );
		total_particles = tracer->getTracedParticleCount();
		gdata << "# particles " << total_particles << "\n";
	} catch (std::exception& e)
	{
		std::cerr << boost::diagnostic_information(e) << "\n";
        return EXIT_FAILURE;
	}

    return EXIT_SUCCESS;
}

void trace( const std::shared_ptr<Tracer>& tracer )
{
	// initial condition generator
	auto generator = createInitialConditionGenerator( tracer->getDimension(), targs::incoming_wave );
	auto start = std::chrono::high_resolution_clock::now();
    InitialConditionConfiguration config;
    config.setParticleCount(targs::NUM_OF_PARTICLES).setEnergyNormalization(!targs::no_norm_energy);
	auto result = tracer->trace( generator, config);
    print_duration(std::cout, "calculation took ", start);

	std::cout << "maximum energy deviation: " <<  result.mMaximumEnergyDeviation * 100 << "% \n";
	if(result.mMaximumEnergyDeviation > 1e-3) {
		std::cout << "this is an indicator for numerical problems and could mean that the potential resolution is too"
				  " low or its strength too high. The mean energy deviation was " << result.mMeanEnergyDeviation * 100
				<< "%.\n";
	}
	for(const auto& o : tracer->getObservers())
	{
		try {
			std::string filename = targs::result_file + "/" + o->filename();
			std::fstream out(filename, std::fstream::out | std::fstream::binary);
			if (!out.is_open()) {
				THROW_EXCEPTION(std::runtime_error, "could not create data file %1% : %2%", filename,
								std::strerror(errno));
			} else {
				o->save(out);
			}
		// catch the exception here, so in case one observer cannot be saved we still might save the others.
		} catch (const std::exception& error) {
			std::cerr << boost::diagnostic_information(error) << "\n";
		}
	}
}
