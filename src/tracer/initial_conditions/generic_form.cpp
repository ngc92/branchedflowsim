#include "generic_form.hpp"
#include <lua.hpp>

namespace init_cond
{

/*! \page ic_cst Generic Caustic Initial Condition Generation
 *		Initial manifold form: f(x) = g(x,y) + (x^2 + y^2) / 2 Z0
 *		Initial velocity: two vectors on manifold: a = (x + d, y, f) - (x, y, f); b = (x, y + d, f) - (x, y, f)
 *			Taylor approx: a = (d, 0, df/dx * d); b = (0, d, df/dy * d)
 *			Cross product: velocity perpendicular to manifold: v = (df/dx, df/dy, 1) * d^2
 *
*/
GenericCaustic2D::GenericCaustic2D(std::size_t dim, double boundary, double scale) : 
    InitialConditionGenerator(dim, 2, "generic"),
    mState( luaL_newstate() ),
    size(boundary), /// \todo even the name is wrong, what is this?
    mUVScale(scale)
{
	luaL_openlibs( mState );
	// does not make much sense in 2d, but will work so do not disallow it
	if(dim < 2)
		BOOST_THROW_EXCEPTION( std::runtime_error("generic 2d requires at least 2 dimensions") );
}

double GenericCaustic2D::getHeight(double u, double v) const
{
	lua_getglobal(mState, "f");
	assert(lua_isfunction(mState, -1));
	lua_pushnumber(mState, u);
	lua_pushnumber(mState, v);
	if(lua_pcall(mState, 2, 1, 0))
	{
		const char* error = lua_tostring(mState, -1);
		std::cerr << error << "\n";
	}
	double result = lua_tonumber(mState, -1);
	lua_pop(mState, 1);
	return result;
}

/// generate next IC
    void GenericCaustic2D::generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& params) const
{
	double p1 = params[0];
	double p2 = params[1];

	// rescale into [x0, x1] x [y0, y1]
	/// \todo alternative: add check in IC that ics are safe
	/// \todo what exactly does this do?
	p1 = p1 *  (1 - 2*size) + size;
	p2 = p2 *  (1 - 2*size) + size;

	double p1v = mUVScale*(p1 * 2 - 1);
	double p2v = mUVScale*(p2 * 2 - 1);

	/// \todo define starting position in a better way

	ray_position[0] = 0.0;
	ray_position[1] = p1;
	ray_position[2] = p2;

	ray_velocity[0] = 1;
	ray_velocity[1] = 0;
	ray_velocity[2] = 0;

	double h = getHeight(p1v, p2v);
	ray_position[0] = h;
	const double delta = 1e-6;
	ray_velocity[1] = -(getHeight( p1v + delta, p2v ) - h) / delta;
	ray_velocity[2] = -(getHeight( p1v, p2v +delta ) - h) / delta;

	// trace back to initial plane
	double tx = (ray_position[0]) / ray_velocity[0];
    double ty1 = (ray_position[1]) / ray_velocity[1];
    double tz1 = (ray_position[2]) / ray_velocity[2];
    double ty2 = (ray_position[1] - 1) / ray_velocity[1];
    double tz2 = (ray_position[2] - 1) / ray_velocity[2];
    double ty = std::max( ty1, ty2 ); // one of them is negative, so max gets the positive one
    double tz = std::max( tz1, tz2 ); // one of them is negative, so max gets the positive one
    double t = std::min( tx, std::min(ty, tz) );
	ray_position -= t * ray_velocity;

	// make sure all positions are valid

}

void GenericCaustic2D::setFunction( std::string terms )
{
    std::string script = "function f(u, v)\n";
	script += "return " + terms + "\n end\n";
	if(luaL_dostring( mState, script.c_str() ))
	{
		const char* error = lua_tostring(mState, -1);
		std::cerr << error << "\n";
	}
	getHeight(0,0);
}

} // namespace init_cond
