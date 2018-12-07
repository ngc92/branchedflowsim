#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/join.hpp>
#include "observers/observer_factory.hpp"
#include "dynamics/dynamics_factory.hpp"
#include "initial_conditions/init_factory.hpp"
#include "profiling.hpp"

namespace po = boost::program_options;

namespace targs
{
	int NUM_OF_PARTICLES = 1000;
	double POTENTIAL_STRENGTH = 0.1;
	bool override_strength = false;
	bool trace_all_sides = false;
	std::string potential_source_file;
	std::vector<std::string> incoming_wave = {"planar"};
	std::string result_file = ".";
	std::vector<std::string> observers = {"density"};
	std::vector<std::string> dynamics = {"particle_potential"};
	bool no_norm_energy;
	double abs_err_bound = 1e-6;
	double rel_err_bound = 1e-6;
	double end_time = 1.0;
	double time_step = -1;
	unsigned thread_count = -1;
	std::size_t memory_avail = -1;
	bool periodic = false;
	std::string integrator;

	void parse_parameters(int argc, char* argv[])
	{
		memory_avail = getMaximumMemoryAvailable() / 1024 / 1024;
		
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "Produce help message. Prints documentation for possible values of incoming, dynamics and observers arguments.")
			("num-particles,n", po::value<int>(&NUM_OF_PARTICLES)->default_value(1000), "Number of particles to trace")
			("potential_strength,s", po::value<double>(&POTENTIAL_STRENGTH), "Override the strength given in the potential file.")
			("periodic", po::bool_switch(&periodic)->default_value(false), "use periodic boundary conditions, causes density observer to crash!")
			("potential", po::value<std::string>(&potential_source_file), "File from which to load the potential")
			("incoming", po::value<std::vector<std::string>>(&incoming_wave)->default_value(incoming_wave, "plane")->multitoken(), "Form of incoming wavefront: plane|sphere|...")
			("observers", po::value<std::vector<std::string>>(&observers)->composing()->multitoken(), "Observers to attach to the tracing. See below for a short description of possible observers.")
			("dynamics", po::value<std::vector<std::string>>(&dynamics)->composing()->multitoken()->default_value(dynamics, dynamics[0]), "The ray dynamics to use for the simulation. See below for a short description of possible dynamics.")
			("rel-err-bound", po::value<double>(&rel_err_bound)->default_value(rel_err_bound), "Maximum relative error for adaptive integration")
			("abs-err-bound", po::value<double>(&abs_err_bound)->default_value(abs_err_bound), "Maximum absolute error for adaptive integration")
			("end-time,e", po::value<double>(&end_time)->default_value(end_time), "Particle time after which the integration is stopped.")
			("result-path,r", po::value<std::string>(&result_file)->default_value("result"), "Target file path")
			("no-norm-energy", po::bool_switch(&no_norm_energy)->default_value(false), "Do not normalize the particles starting energy.")
			("threads,t", po::value<unsigned>(&thread_count)->default_value(-1), "Maximum number of threads to use for computation.")
			("memory", po::value<std::size_t>(&memory_avail)->default_value( memory_avail ), "Maximum memory the programme is allowed to use, in MB.")
			("integrator", po::value<std::string>(&integrator)->default_value( "adaptive" ), "The integrator to use. One of (adaptive, euler)")
			("time-step", po::value<double>(&time_step), "The time step for the integrator.")
		;

		po::positional_options_description p;
		p.add("potential", -1);
		po::variables_map vm;
		try
		{
			po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
			po::notify(vm);
		} catch( std::exception& ex )
		{
			std::cerr << "error parsing command line parameters!\n";
			std::cerr << boost::diagnostic_information(ex, true) << "\n\n";
			exit( EXIT_FAILURE );
		}

		override_strength = vm.count("potential_strength") != 0;

		if (vm.count("help")) {
            std::cout << desc << "\n";
            std::cout << "Observers: \n";
            getObserverFactory().printHelp(  );

            std::cout << "Dynamics: \n";
            getDynamicsFactory().printHelp(  );

            std::cout << "Incomings: \n";
            init_cond::getInitialConditionFactory().printHelp(  );
			
			exit( EXIT_SUCCESS);
		}
	}
}

void parse_parameters(int argc, char* argv[])
{
	targs::parse_parameters(argc, argv);
}

