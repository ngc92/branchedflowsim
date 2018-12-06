#ifndef PARAMETERS_H_INCLUDED
#define PARAMETERS_H_INCLUDED

namespace pargs
{
	// potential properties
	extern int dim;
	extern std::vector<int> size;
	extern double correlation_length;
	extern double strength;
	extern std::vector<std::string> correlation_function;
	extern std::string correlation_trafo;
	extern unsigned int seed;

	extern unsigned int threads;
	extern bool no_wisdom;
	extern bool print_profile;
	extern bool correlation_only;

	// generation switches
	extern int derivative_order;

	// output parameters
	extern std::string potential_outfile;
}

void parse_parameters(int argc, const char* argv[]);

#endif // PARAMETERS_H_INCLUDED
