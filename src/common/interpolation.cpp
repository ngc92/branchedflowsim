#include "interpolation.hpp"
#include "dynamic_grid.hpp"

// put into anonymous namespace to prevent ODR problems!
namespace
{
    template<int N>
    using index_t = std::array<int, N>;

    template<int N>
    using fvec_t = std::array<double, N>;

    template<int D>
    struct GetOffsetOf
    {
        static inline std::size_t index_of(const int* index, const std::size_t* extends)
        {
            std::size_t offset = 0;
            for(int i = 0; i < D; ++i)
            {
                /// \todo ensure that index is positive and remove the additional branch
                auto mod = (index[i] % extends[i]);
                offset = offset * extends[i] + (mod < 0 ? mod + extends[i] : mod);
            }
            return offset;
        }
    };

    template<int DIMENSION, int N>
    struct dim_reduce_interpolate;

    // forward declaration to set HOT attribute
    template<int DIMENSION>
    inline static double linearInterpolateF( const default_grid& grid, const double* v) HOT_FUNCTION FLATTEN_FN;

    template<int D>
    inline static double linearInterpolateF( const default_grid& grid, const double* v)
    {
        // here we ensure that the grid we work on is actually periodic.
        /// \todo is this a good idea? for non-periodic tracing, we actually ensure
        ///        that we end just before the boundary, so all operations are safe.
        assert( grid.getAccessMode() == TransformationType::PERIODIC );
        index_t<D> arr;
        for(unsigned int i = 0; i < D; ++i)
            arr[i] = std::floor(v[i]);

        return dim_reduce_interpolate<D, D-1>::inter(grid, arr, v);
    }

    template<int DIMENSION, int N>
    struct dim_reduce_interpolate
    {
        typedef double result_t;

        typedef dim_reduce_interpolate<DIMENSION, N-1> reduced_type;
        static_assert(N < DIMENSION, "vector subscript index must be lower than DIMENSION" );

        /// recursive template that defines n-dimensional interpolation in terms of (n-1) dimensional interpolation
        /// \param grid Data to interpolate
        /// \param base helper variable needed for recursion, for upper level call initialize with floor(v)
        /// \param v Position where to calculate the interpolation
        static result_t inter( const default_grid& grid, index_t<DIMENSION>& base, const double* v)
        {
            /// \todo this could be optimized
            auto v1 = reduced_type::inter( grid, base, v );
            base[N]++;
            auto v2 = reduced_type::inter( grid, base, v );
            base[N]--;
            return interpolate_linear_1d( v1, v2, v[N] - base[N] );
        }

    };

    template<int DIMENSION>
    struct dim_reduce_interpolate<DIMENSION, -1>
    {
        // recursion end
        static double inter( const default_grid& grid, const index_t<DIMENSION>& base, const double* /*v*/)
        {
            auto index = grid.getOffsetWithMode<GetOffsetOf<DIMENSION>>( base );
            return grid[index];
        }
    };

    // ------------------------------------------------------------
    //                interpolated drawing
    // ------------------------------------------------------------

    // helper template
    template<int DIM /*!< dimension of the problem */,
             int POS /*!< current index in dimension recursion, ends at -1 */>
    struct AddDotRecurse
    {
        template<class Grid>
        static void draw(Grid& dens, fvec_t<DIM>& df, index_t<DIM>& offset, double weight)
        {
            constexpr int sub = DIM-POS-1;
            AddDotRecurse<DIM, POS-1>::draw(dens, df, offset, weight);
            offset[sub]++;
            df[sub] = 1.0 - df[sub];
            AddDotRecurse<DIM, POS-1>::draw(dens, df, offset, weight);
            df[sub] = 1.0 - df[sub];
            offset[sub]--;
        }
    };

    /// recursion end. multiplies the weights and adds to the density map.
    template<int DIM>
    struct AddDotRecurse<DIM, -1>
    {
        template<class Grid>
        static void draw( Grid& dens, const fvec_t<DIM>& df, const index_t<DIM>& offset, double weight)
        {
            for(int i = 0; i < DIM; ++i)
                weight *= df[i];
            dens(offset) += weight;
        }
    };

    /// convenience helper template to start the recursion above. correctly initializes
    /// the position template parameter, and calculates offsets and deltas.
    template<int DIM>
    struct AddDot
    {
        template<class Grid>
        inline static void draw(Grid& dens, const gen_vect& pos, double weight)
        {
            // check dimension consistency
            assert( dens.getDimension() == pos.size() );
            assert( dens.getDimension() == DIM );

            // calculate offset and deltas
            // gen_vect only uses data on stack, so fast
            fvec_t<DIM> df;
            index_t<DIM> offset;
            for(unsigned i = 0; i < DIM; ++i)
            {
                offset[i] = std::floor( pos[i] );
                df[i] = 1.0 - (pos[i] - offset[i]);
            }
            AddDotRecurse<DIM, DIM-1>::draw(dens, df, offset, weight);
        }
    };
    template<class Grid>
    inline void drawInterpolatedDot(size_t dimension, Grid& grid, const gen_vect& pos, double weight ) HOT_FUNCTION FLATTEN_FN;

    template<class Grid>
    inline void drawInterpolatedDot(std::size_t dimension, Grid& grid, const gen_vect& pos, double weight )
    {
        assert( dimension == grid.getDimension() );
        // dimension dispatch.
        switch(dimension)
        {
        case 1:
            AddDot<1>::draw(grid, pos, weight);
            return;
        case 2:
            AddDot<2>::draw(grid, pos, weight);
            return;
        case 3:
            AddDot<3>::draw(grid, pos, weight);
            return;
        }

        assert(false);
        __builtin_unreachable();
    }
} // end anonymous namespace

// now the externally visible functions

double linearInterpolate( const default_grid& grid, const gen_vect& v)
{
    /// \todo check dimension of v
    return linearInterpolate(grid, &v[0]);
}

// select the correct template algorithm depending on the dimension of grid.
double linearInterpolate( const default_grid& grid, const double* v)
{
    switch( grid.getDimension() )
    {
    case 1:
        return linearInterpolateF<1>( grid, v );
    case 2:
        return linearInterpolateF<2>( grid, v );
    case 3:
        return linearInterpolateF<3>( grid, v );
    }
    assert(false);
    __builtin_unreachable();
}

void drawInterpolatedDot( DynamicGrid<float>& grid, const gen_vect& pos, double weight )
{
    return drawInterpolatedDot(grid.getDimension(), grid, pos, weight);
}
