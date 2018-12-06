#include "ode_state.hpp"
#include <iostream>

GState::GState() : mData(0), mDimension(0)
{
//	std::cout << "DCTOR "<< this << "\n";
}

GState::GState( int D, bool m ) : mData( 2*D + (m ? 4*D*D : 0)), mDimension(D), mHasMonodromy(m)
{
//	std::cout << "CTOR " << this << ": " << mData.size() <<  "\n";
}
void GState::resize(std::size_t new_size, bool monodromy)
{
	std::size_t alloc = 2 * new_size;
	if( monodromy )
		alloc += 4 * new_size * new_size;
//	std::cout << "RESIZE " << this << ": " << mData.size() << " -> " <<alloc << "\n";
	mData.resize( alloc );
	mDimension = new_size;
	mHasMonodromy = monodromy;
}

void GState::init_monodromy()
{
	for(unsigned i = 0; i < matrix().size(); ++i)
		matrix()[i] = 0;

	for(unsigned i = 0; i < 2*mDimension; ++i)
		matrix()[i+2*mDimension*i] = 1;
}
