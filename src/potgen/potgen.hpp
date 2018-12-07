#ifndef POTGEN_HPP_INCLUDED
#define POTGEN_HPP_INCLUDED

#include <array>
#include <vector>
#include <functional>
#include <complex>

#include "vector.hpp"
#include "potential.hpp"

// forward declarations
template<class V>
class DynamicGrid;

typedef DynamicGrid<complex_t> complex_grid;
typedef DynamicGrid<double> default_grid;
///! typedef for a correlation function type. Takes its arguments by reference, as \p gen_vect objects allocate dynamic memory
typedef std::function<double(const gen_vect&)> correlation_fn;

struct PGOptions
{
	// potgen options
	bool randomize = true;
	unsigned int maxDerivativeOrder = 1;
	std::size_t randomSeed = 1u;
	double corrlength = -1;
	correlation_fn cor_fun;	//!< correlation function. no sensible default value, so must be set
	bool verbose = false;

	std::size_t numThreads = 1;
};

Potential generatePotential( std::vector<std::size_t> sizes, std::vector<double> support, const PGOptions& opt );

#endif // POTGEN_HPP_INCLUDED
