#include "correlation.hpp"
#include <cmath>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "global.hpp"
#include "lua.hpp"
#include <boost/numeric/ublas/io.hpp>

correlation_fn makeGaussianCorrelation( double corrlength )
{
	double scale = -1.0 / corrlength / corrlength;
	auto f = [scale](const gen_vect& v) -> double
	{
		using boost::numeric::ublas::inner_prod;
		return std::exp(inner_prod(v, v) * scale);
	};
	return f;
}

correlation_fn makeAnisotropicGaussianCorrelation( double corrlength, gen_vect ani )
{
	// scale anisotropy factor with global correlation length and precalculate the squares
	for( auto& v : ani )
		v = v*v/corrlength/corrlength;
	
	auto f = [ani](const gen_vect& v) -> double
	{
		using boost::numeric::ublas::inner_prod;
		double sum = 0;
		for( unsigned i = 0; i < v.size(); ++i )
			sum -= v[i] * v[i] * ani[i];

		return std::exp( sum );
	};
	return f;
}

correlation_fn makeSechCorrelation( double corrlength )
{
	double scale = 1.0 / corrlength;
	auto f = [scale](const gen_vect& v) -> double
	{
		using boost::numeric::ublas::inner_prod;
		double l = std::sqrt(inner_prod(v, v)) * scale;
		// sech(x) = 1 / cosh(x)
		return 1.0 / std::cosh(l);
	};
	return f;
}

correlation_fn makePowerCorrelation( double corrlength, double alpha )
{
	double scale = 1.0 / corrlength / corrlength;
	auto f = [scale, alpha](const gen_vect& v) -> double
	{
		using boost::numeric::ublas::inner_prod;
		double l = 1 + inner_prod(v, v) * scale;
		return std::pow(l, -alpha);
	};
	return f;
}

correlation_fn makeLuaCorrelation( double corrlength, std::string scriptfile, const std::vector<std::string>& vars )
{
	auto make_lua = [=]() {
		// create state, load and compile file
		lua_State* state = luaL_newstate();
		// open math library
		luaL_requiref(state, LUA_MATHLIBNAME, luaopen_math, 1);
		lua_pop(state, 1);
		// open file
		luaL_dofile(state, scriptfile.c_str());

		// set the variables
		for (unsigned i = 0; i < vars.size(); i += 2) {
			auto name = vars.at(i);
			auto value = boost::lexical_cast<double>(vars.at(i + 1));
			lua_pushnumber(state, value);
			lua_setglobal(state, name.c_str());
		}


		// check that correlation function is set
		lua_getglobal(state, "c");
		if (!lua_isfunction(state, -1))
		THROW_EXCEPTION(std::runtime_error, "lua script does not contain a function named c");
		lua_pop(state, 1);

		return state;
	};

	// now the correlation function
	auto f = [make_lua, corrlength](const gen_vect& v) -> double
	{
        // ensure that we have a one lua interpreter per thread
		thread_local lua_State* state = make_lua();
		// get the lua function reference
		/// \todo is it possible to cache this?
		lua_getglobal(state, "c");
		assert(lua_isfunction(state, -1));

		// push the scaled vector
		for(unsigned i = 0; i < v.size(); ++i)
			lua_pushnumber(state, v[i] / corrlength);

		// call the function and handle any errors
		if(lua_pcall(state, v.size(), 1, 0))
		{
			const char* error = lua_tostring(state, -1);
			THROW_EXCEPTION( std::runtime_error, error );
		}

		// get the result and clear the stack
		double result = lua_tonumber(state, -1);
		lua_pop(state, 1);
		return result;
	};
	return f;
}

correlation_fn makeTransformedCorrelation( correlation_fn original, trafo_matrix_t matrix )
{
	auto f = [original, matrix](const gen_vect& v) -> double
	{
		return original( boost::numeric::ublas::prod(matrix, v) );
	};
	return f;
}

// make correlation function without any trafo.
correlation_fn makeCorrelation( const std::vector<std::string>& specs, double length )
{
	assert( !specs.empty() );
	std::string corr_type = specs[0];
	if(corr_type == "gauss" || corr_type == "gaussian")
		if( specs.size() == 1 )	// no further parameters: use isotropic gaussian
			return makeGaussianCorrelation( length );
		else
		{
			gen_vect ani( specs.size() - 1);
			for(unsigned i = 1; i < specs.size(); ++i)
				ani[i-1] = boost::lexical_cast<double>( specs[i] );
			return makeAnisotropicGaussianCorrelation( length, ani );
		}
	else if(corr_type == "sech" )
		return makeSechCorrelation( length );
	else if(corr_type == "pow" || corr_type == "power")
		return makePowerCorrelation( length, boost::lexical_cast<double>(specs.at(1)));
	else if(corr_type == "lua")
	{
		if( specs.size() < 2 )
			THROW_EXCEPTION( std::runtime_error, "No script file specified for lua correlation" );

		// load specifications for variables that are set in lua
		std::vector<std::string> vars(specs.begin() + 2, specs.end());
		if( vars.size() % 2 != 0 )
			THROW_EXCEPTION( std::runtime_error, "invalid variables for lua script. Use \"lua filename var1 value1 var2 value2\"" );
		return makeLuaCorrelation( length, specs.at(1), vars);
	}
	else
		THROW_EXCEPTION( std::runtime_error, "correlation type %1% not valid", corr_type);

}

// convert a vector of strings to a ublas matrix
trafo_matrix_t matrix_from_string_vector(const std::vector<std::string>& source)
{
	std::size_t dim = 0;
	switch(source.size())
	{
	case 1:
		dim = 1;
		break;
	case 4:
		dim = 2;
		break;
	case 9:
		dim = 3;
		break;
	default:
		THROW_EXCEPTION(std::runtime_error, "transformation matrix is required to be square with dim <= 3. Got %1% elements.", source.size());
	}

	std::vector<double> converted;
	trafo_matrix_t matrix(dim, dim);
	std::transform( begin(source), end(source), std::back_inserter(converted), [](const std::string& s){ return boost::lexical_cast<double>(s); } );
	for(unsigned i = 0; i < dim; ++i)
	{
		for(unsigned j = 0; j < dim; ++j)
		{
			matrix(i, j) = converted[dim*i + j];
		}
	}
	return matrix;
}

correlation_fn makeCorrelation( const std::vector<std::string>& specs, double length, std::string trafo )
{
	// easy case: no trafo
	if(trafo.empty())
		return makeCorrelation(specs, length);
	
	// otherwise, start out with untrafo'ed
	// in case we get quotes from the command line, remove them.
	auto base = makeCorrelation(specs, length);
	if(boost::starts_with(trafo, "\""))
		trafo = trafo.substr(1);
	if(boost::ends_with(trafo, "\""))
		trafo = trafo.substr(0, trafo.size() - 1);
	
	// now split
	std::vector<std::string> split;
	boost::split(split, trafo, [](char c){ return std::isspace(c); });
	trafo_matrix_t matrix = matrix_from_string_vector(split);
	
	return makeTransformedCorrelation(base, matrix);
}
