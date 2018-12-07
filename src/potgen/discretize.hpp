#ifndef BRANCHEDFLOWSIM_DISCRETIZE_H
#define BRANCHEDFLOWSIM_DISCRETIZE_H

#include "potgen.hpp"
complex_grid discretizeFunctionForFFT(std::vector<std::size_t> grid_size, std::vector<double> support, correlation_fn F);

#endif //BRANCHEDFLOWSIM_DISCRETIZE_H
