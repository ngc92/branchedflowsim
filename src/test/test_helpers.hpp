#ifndef TEST_HELPERS_HPP_INCLUDED
#define TEST_HELPERS_HPP_INCLUDED

#include <iosfwd>
#include <array>
#include "vector.hpp"
class MultiIndex;

namespace std
{
	/// \todo overload in namespace std is a problem
	/// \todo definition of these functions is currently in binary_cmp.cpp
	/// \todo make something more generic
	/// \todo maybe these functions are useful enough outside of tests to add them to global.hpp?
	ostream& operator<<( std::ostream& stream, const array<int, 1>& dat );
	ostream& operator<<( std::ostream& stream, const array<int, 2>& dat );
	ostream& operator<<( std::ostream& stream, const array<int, 3>& dat );
}
namespace boost{
namespace numeric{
namespace ublas{
	bool operator==( const gen_vect& a, const gen_vect& b );
	std::ostream& operator<<( std::ostream& stream, const gen_vect& dat );
}}}

double rand_value();
double rand01();
gen_vect rand_vec( int dim );

MultiIndex createMultiIndex( std::initializer_list<int> init_list );

bool compare_files_binary(std::string f1, std::string f2, bool verbose = true);

class check_what
{
public:
	explicit check_what(std::string w) : what(std::move(w))
	{}

	template<class T>
	bool operator()(const T& exception)
	{
		BOOST_CHECK_EQUAL(what, exception.what());
		return what == exception.what();
	}

private:
	std::string what;
};

#endif // TEST_HELPERS_HPP_INCLUDED
