#include "correlation.hpp"
#include "test/test_helpers.hpp"
#include <boost/numeric/ublas/io.hpp>

#include <boost/test/unit_test.hpp>

extern trafo_matrix_t matrix_from_string_vector(const std::vector<std::string>& source);

BOOST_AUTO_TEST_SUITE(correlation_fn_test)

BOOST_AUTO_TEST_CASE( transformed_correlation_identity_test )
{
	for(int dim = 1; dim <= 3; ++dim)
	{
		auto base = makeGaussianCorrelation(0.5);
		trafo_matrix_t identity = boost::numeric::ublas::identity_matrix<double>(dim, dim);
		auto trafoed = makeTransformedCorrelation(base, identity);
		for(int i = 0; i < 100; ++i)
		{
			auto p = gen_vect(dim);
			BOOST_CHECK_EQUAL( base(p), trafoed(p) );
		}
	}
}

BOOST_AUTO_TEST_CASE( matrix_from_string_vec_test )
{
	for(int dim = 1; dim <= 3; ++dim)
	{
		// create the string that is the identity matrix
		std::vector<std::string> src;
		trafo_matrix_t reference(dim, dim);
		for(int j = 0; j < dim; ++j)
		{
			for(int k = 0; k < dim; ++k)
			{
				src.push_back(k == j ? "1" : "0");
				reference(j, k) = k == j ? 1 : 0;
			}
		}
		
		auto matrix = matrix_from_string_vector(src);
		std::cout << matrix << "\n" << reference << "\n";
		for(int j = 0; j < dim; ++j)
		{
			for(int k = 0; k < dim; ++k)
			{
				BOOST_CHECK_EQUAL( matrix(j, k), reference(j, k));
			}
		}
		
	}
	
	/// \todo add test that checks for transposedness
}


BOOST_AUTO_TEST_SUITE_END()
