#ifndef CORRELATION_HPP_INCLUDED
#define CORRELATION_HPP_INCLUDED

#include <functional>
#include <string>
#include <vector>
#include "vector.hpp"
#include <boost/numeric/ublas/matrix.hpp>

typedef std::function<double(const gen_vect&)> correlation_fn;
typedef boost::numeric::ublas::c_matrix<double, 3, 3> trafo_matrix_t;

correlation_fn makeGaussianCorrelation( double corrlength );
correlation_fn makeAnisotropicGaussianCorrelation( double corrlength, gen_vect ani );
correlation_fn makeSechCorrelation( double corrlength );
correlation_fn makePowerCorrelation( double corrlength, double alpha );
correlation_fn makeLuaCorrelation( double corrlength, std::string scriptfile, const std::vector<std::string>& vars );
correlation_fn makeTransformedCorrelation( correlation_fn original, trafo_matrix_t matrix );

/// creates a correlation function according to the specification in \p specs
correlation_fn makeCorrelation( const std::vector<std::string>& specs, double length, std::string trafo );


#endif // CORRELATION_HPP_INCLUDED
