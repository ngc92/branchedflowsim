#ifndef PARAMETERS_H_INCLUDED
#define PARAMETERS_H_INCLUDED

namespace targs
{
	extern int NUM_OF_PARTICLES;
	extern double POTENTIAL_STRENGTH;
	extern bool override_strength;
	extern std::string potential_source_file;
	extern std::vector<std::string> incoming_wave;
	extern std::string result_file;
	extern std::vector<std::string> observers;
	extern std::vector<std::string> dynamics;
	extern double abs_err_bound;
	extern double rel_err_bound;
	extern double end_time;
	extern double time_step;
	extern bool no_norm_energy;
	extern bool periodic;
	extern unsigned thread_count;
	extern std::size_t memory_avail;
	extern std::string integrator;
}

void parse_parameters(int argc, char* argv[]);

#endif // PARAMETERS_H_INCLUDED
