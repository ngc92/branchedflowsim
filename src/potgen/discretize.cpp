#include <future>
#include <thread>
#include "discretize.hpp"

template<std::size_t DIM>
void fillGrid(complex_grid& grid, MultiIndex index, const std::vector<double>& scale, const correlation_fn& F)
{
    auto& gridsize = grid.getExtents();
    // setup corr fn argument cache
    gen_vect point(DIM);

    /// \todo make sure boundaries are ok, so that F is sampled symmetrically
    ///			it seems to work now, but i am not exactly sure why
    for(; index.valid(); ++index)
    {
        for(unsigned i = 0; i < DIM; ++i)
        {
            // this is faster than doing modulo
            int p = index[i];
            auto half_size = static_cast<int>(gridsize[i]/2);
            if(p >= half_size)
                p -= gridsize[i];

            point[i] = p * scale[i];
        }
        grid( index ) = F( point );
    }
}

void fillGrid_generic(complex_grid& grid, MultiIndex index, const std::vector<double>& scale, correlation_fn F)
{
    switch(grid.getDimension()) {
        case 1:
            fillGrid<1>(grid, index, scale, F);
            break;
        case 2:
            fillGrid<2>(grid, index, scale, F);
            break;
        case 3:
            fillGrid<3>(grid, index, scale, F);
            break;
        default:
        THROW_EXCEPTION(std::logic_error, "unsupported dimension");
    }
}



// old: 512^3: 13118ms
complex_grid discretizeFunctionForFFT(std::vector<std::size_t> gridsize, std::vector<double> support, correlation_fn F)
{
    if( gridsize.size() != support.size() )
        THROW_EXCEPTION( std::invalid_argument, "grid dimension %1% does not match dimension of support %2%",
                         gridsize.size(), support.size());

    PROFILE_BLOCK("discretize");

    std::size_t dimension = gridsize.size();

    // setup iteration index
    MultiIndex index( dimension );
    for(unsigned i = 0; i < dimension; ++i)
    {
        // check that gridsize is even
        if( gridsize[i] % 2 == 1 )
            THROW_EXCEPTION( std::invalid_argument, "trying to discretize odd-sized grid");
        index.setLowerBoundAt( i, 0 );
        index.setUpperBoundAt( i, (int)gridsize[i] );
    }

    // transform support into scale vector
    for(unsigned i = 0; i < dimension; ++i)
    {
        support[i] /= double(gridsize[i]);
    }

    // setup grid
    complex_grid grid( gridsize, TransformationType::IDENTITY );

    // now distribute computation to many threads
    std::vector<std::future<void>> tasks;
    auto sub_indices = index.split(std::thread::hardware_concurrency());
    for(const auto& sub : sub_indices)
    {
        tasks.push_back( std::async(std::launch::async, fillGrid_generic, std::ref(grid), sub, std::ref(support), std::ref(F)) );
    }

    for(unsigned i = 0; i < tasks.size(); ++i)
    {
        tasks[i].get();
    }

    grid.setAccessMode( TransformationType::FFT_INDEX );

    return std::move(grid);
}
