#include "test_helpers.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <array>
#include <random>
#include <initializer_list>
#include "multiindex.hpp"

using namespace std;
/// \todo is this possible / save?
namespace std
{
	ostream& operator<<( std::ostream& stream, const array<int, 1>& dat )
	{
		for(int i=0; i < 1; ++i)
			stream << dat[i] << " ";
		return stream;
	}
	/// \todo some generalization
	ostream& operator<<( std::ostream& stream, const array<int, 2>& dat )
	{
		for(int i=0; i < 2; ++i)
			stream << dat[i] << " ";
		return stream;
	}

	ostream& operator<<( std::ostream& stream, const array<int, 3>& dat )
	{
		for(int i=0; i < 3; ++i)
			stream << dat[i] << " ";
		return stream;
	}
}

namespace boost{
namespace numeric{
namespace ublas{
bool operator==( const gen_vect& a, const gen_vect& b )
{
	if( a.size() != b.size() )
		return false;
	for( unsigned i = 0; i < a.size(); ++i)
	{
		if( a[i] != b[i])
			return false;
	}

	return true;
}
ostream& operator<<( std::ostream& stream, const gen_vect& dat )
{
	stream << "[";
	for(const auto& d : dat)
			stream << d << " ";
	return stream << "]";
}

}}}

// random functions
double rand_value()
{
	static std::default_random_engine generator;
	static std::uniform_real_distribution<double> distribution(-100.0, 100.0);
	return distribution(generator);
}

double rand01()
{
	static std::default_random_engine generator;
	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
	return distribution(generator);
}

gen_vect rand_vec( int dim )
{
	gen_vect v(dim);
	for(int i=0; i < dim; ++i)
		v[i] = rand_value();
	return v;
}


MultiIndex createMultiIndex( std::initializer_list<int> init_list )
{
	int len = init_list.size();
	int max = 1 + *std::max_element( begin(init_list), end(init_list));
	std::cout << "MAX " << max << "\n";
    MultiIndex index(len, 0, max);
    int i = 0;
	for( int val : init_list )
	{
		const_cast<int&>(index[i++]) = val;
	}
	return index;
}
