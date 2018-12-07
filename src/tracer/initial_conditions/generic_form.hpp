#ifndef GENERIC_FORM_HPP_INCLUDED
#define GENERIC_FORM_HPP_INCLUDED

#include "initial_conditions.hpp"

extern "C"
{
	struct lua_State;
}

namespace init_cond
{
	///! \attention This class has not been used/tested in a long while.
class GenericCaustic2D final : public InitialConditionGenerator
{
public:
	GenericCaustic2D(std::size_t dim, double boundary, double scale);

	double getHeight(double u, double v) const;

	/// generate next IC
    void generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& params) const override;
	
	void setFunction( std::string terms );

private:
	lua_State* mState;
	double size;
	double mUVScale = 1;
};

} // namespace init_cond

#endif // GENERIC_FORM_HPP_INCLUDED
