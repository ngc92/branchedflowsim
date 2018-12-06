#ifndef ODE_STATE_HPP_INCLUDED
#define ODE_STATE_HPP_INCLUDED

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <vector>
#include <cassert>

class GState;

struct vector_proxy
{
public:
	double& operator[](std::size_t idx)
	{
		assert( idx < mSize );
		return mData[idx];
	};
	const double& operator[](std::size_t idx) const
	{
		assert( idx < mSize );
		return mData[idx];
	};

	template<class T>
	vector_proxy& operator=(const T& other)
	{
		assert( other.size() == mSize );
		for(unsigned i = 0; i < other.size(); ++i)
			(*this)[i] = other[i];
		return *this;
	}

	template<class T>
	void assignTo( T& other ) const
	{
		assert( other.size() == mSize );
		for(int i = 0; i < other.size(); ++i)
			other[i] = (*this)[i];
	}

	std::size_t size() const { return mSize;};

private:
	double* mData;
	std::size_t mSize;
	vector_proxy(double* ptr, std::size_t sz) : mData(ptr), mSize(sz) {};

	friend class GState;
};

struct const_vector_proxy
{
public:
	const double& operator[](std::size_t idx) const {
		assert( idx < mSize );
		return mData[idx];
	};
	std::size_t size() const { return mSize;};

	template<class T>
	void assignTo( T& other ) const
	{
		assert( other.size() == mSize );
		for(unsigned i = 0; i < mSize; ++i)
			other[i] = (*this)[i];
	}
private:
	const double* mData;
	std::size_t mSize;

	const_vector_proxy(const double* ptr, std::size_t sz) : mData(ptr), mSize(sz) {};

	friend class GState;
};

class GState
{
public:
	GState();
	//~GState();
	GState( int D, bool m );
	//GState( const GState& cpy );
	//GState( GState&& mov );

	typedef std::vector<double> container_type;
	typedef container_type::iterator iterator;
	typedef container_type::const_iterator const_iterator;
	typedef double value_type;

	std::size_t dimension() const { return mDimension; };
	bool monodromy() const { return mHasMonodromy; };

	void resize( std::size_t size, bool monodromy );
	void init_monodromy();

	// iterator access
	const_iterator begin() const 	{ return mData.begin(); }
	const_iterator end() const		{ return mData.end(); }
	iterator begin() 				{ return mData.begin(); }
	iterator end() 					{ return mData.end(); }

	// accessing values outside of integration
	vector_proxy position() { return vector_proxy(&mData[0], mDimension); };
	const_vector_proxy position() const { return const_vector_proxy(&mData[0], mDimension); };

	vector_proxy velocity() { return vector_proxy(&mData[0] + mDimension, mDimension);};
	const_vector_proxy velocity() const { return const_vector_proxy(&mData[0] + mDimension, mDimension);};

	vector_proxy matrix() { return vector_proxy( &mData[0] + 2*mDimension, 4*mDimension*mDimension ); };
	const_vector_proxy matrix() const { return const_vector_proxy( &mData[0] + 2*mDimension, 4*mDimension*mDimension ); };
private:
	std::vector<double> mData;
	std::size_t mDimension = 0;
	bool mHasMonodromy = false;
};

// mark resizable
namespace boost { namespace numeric { namespace odeint {

template<>
struct is_resizeable< GState >
{
	typedef boost::true_type type;
	static const bool value = type::value;
};

template <class U, class V>
struct same_size_impl;

template <>
struct same_size_impl< GState , GState >
{
    static bool same_size( const GState& x , const GState& y )
    {
        return x.dimension() == y.dimension() && x.monodromy() == y.monodromy();
    }
};

template <class U, class V>
struct resize_impl;

template <>
struct resize_impl< GState , GState >
{
    static void resize( GState& x , const GState& y )
    {
    	x.resize( y.dimension(), y.monodromy() );
    }
};

} } }


#endif // ODE_STATE_HPP_INCLUDED
