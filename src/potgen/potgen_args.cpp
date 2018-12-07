#include <string>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace pargs
{
	int dim = 2;
	std::vector<int> size;
	unsigned int seed = 1u;

	unsigned int threads = 1u;
	bool no_wisdom = false;
	bool print_profile = false;
	bool correlation_only = false;

	int derivative_order = 2;

	std::string potential_outfile;
	double correlation_length = 0.1;
	double strength = 1.0;
	std::vector<std::string> correlation_function;
	std::string correlation_trafo;

	void parse_parameters(int argc, const char* argv[])
	{

		// command line options
		// Declare the supported options.
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("dimension,d", po::value<int>(&dim)->default_value( dim ), "Dimension of the generated potential. 1, 2 or 3")
			("size,s", po::value<std::vector<int>>(&size)->required()->multitoken(), "Sizes of the generated potential.")
			("strength", po::value<double>(&strength)->default_value(1.0), "Strength of the generated potential.")
			("corrlength,l", po::value<double>(&correlation_length)->default_value( correlation_length ), "Correlation length. Argument inside the exponential function exp(1/2 (r/l)^2")
			("correlation,c", po::value<std::vector<std::string>>(&correlation_function)->default_value( std::vector<std::string>({"gauss"}), "gauss")->multitoken(), "Type and parameters for correlation function")
			("trafo", po::value<std::string>(&correlation_trafo), "Transformation matrix M that is applied to the correlation function. c(x) = f(Mx), "
																	"where f is the originally specified correlation function. "
																	"Make sure to pass in quotations, e.g. --trafo \"a b c d\".")
			("seed", po::value<unsigned>(&seed)->default_value( seed ), "seed for phase randomization")
			("derivative-order", po::value<int>(&derivative_order)->default_value( derivative_order ), "highest order to which the derivatives should be calculated")
			("output,o", po::value<std::string>(&potential_outfile)->required(), "File to store the potential.")
			("threads,t", po::value<unsigned>(&threads), "Number of threads for fftw to use.")
			("no-wisdom", po::bool_switch(&no_wisdom), "Disable saving fftw wisdom.")
			("print-profile", po::bool_switch(&print_profile), "Prints profiling information after the potential generation is finished.")
			("correlation-only", po::bool_switch(&correlation_only), "Do not generate potential. Just create the correlation function.")
		;

		po::positional_options_description p;
		p.add("output", -1);

		po::variables_map vm;

		try
		{
			po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

			if (vm.count("help")) {
				std::cout << desc << "\n";
				exit(0);
			}


			po::notify(vm);
		}
		 catch( po::error& cmderr )
		{
			std::cout << "error parsing command line: " << cmderr.what() << "\n";
			std::cout << desc << "\n";
			exit(EXIT_FAILURE);
		}
	}
}

void parse_parameters(int argc, const char* argv[])
{
	pargs::parse_parameters(argc, argv);
}

