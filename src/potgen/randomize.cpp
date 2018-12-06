//
// Created by eriks on 6/1/17.
//

#include <thread>
#include <future>
#include "randomize.hpp"

template<std::size_t DIM>
void randomize_over_index(complex_grid& grid, std::function<double()> rnd, MultiIndex index)
{
    if(!index.valid())
        THROW_EXCEPTION(std::invalid_argument, "index %1% is invalid!", index);

    std::array<int, DIM> inverted;

    // set phases for all data point outside of 0 or max axis
    for(; index.valid(); ++index)
    {
        // set inverted index
        for(unsigned i = 0; i < DIM; ++i)
            inverted[i] = -index[i];

        auto offset = grid.getOffsetOf( index );
        auto iffset = grid.getOffsetOf( inverted );
        // check that changes only once
        // check that -iter == iter access is real
        if( offset < iffset )
        {
            // set f(x) = conj(f(-x))
            auto phase  = rnd();
            auto factor = complex_t(std::cos(phase), std::sin(phase));
            // use faster [] access because we already calculated that index
            grid[ offset ] *= factor;
            grid[ iffset ] *= std::conj(factor);
        }
        else if( offset == iffset )
        {
            // since we multiply +-1 with same probability, it does not matter that currently
            // this function is called twice for each unique point
            // use faster [] access because we already calculated the index
            grid[ offset ] *= rnd() < 0.5 ? 1 : -1;
        }
    }
}

void randomize_generic(complex_grid& grid, std::function<double()> rnd, MultiIndex index)
{
    switch(grid.getDimension()) {
        case 1:
            randomize_over_index<1>(grid, rnd, index);
            break;
        case 2:
            randomize_over_index<2>(grid, rnd, index);
            break;
        case 3:
            randomize_over_index<3>(grid, rnd, index);
            break;
        default:
        THROW_EXCEPTION(std::logic_error, "unsupported dimension");
    }
}

MultiIndex fft_indexing(const complex_grid& grid)
{
    // setup iteration index
    MultiIndex index( grid.getDimension() );
    auto grid_size = grid.getExtents();
    for( unsigned i = 0u; i < grid.getDimension(); ++i )
    {
        if( grid_size[i] % 2 != 0)
            THROW_EXCEPTION(std::logic_error, "grid size %1% (=%2%) is not divisible by two", i, grid_size[i]);
        index.setLowerBoundAt(i, -(int)(grid_size[i]/2));
        index.setUpperBoundAt(i, (int)grid_size[i]/2);
    }
    index.init();
    return index;
}

void randomizePhases(complex_grid& grid, std::uint64_t seed)
{
    PROFILE_BLOCK("randomize phases");

    // setup iteration index
    MultiIndex index = fft_indexing(grid);

    std::vector<std::future<void>> tasks;
    // figure out how many threads to use. This has to be independent of the number of
    // available hardware threads, otherwise we would compromise reproducability.
    // Thus by default we use many threads, wasting a few resources on small systems
    // but ensuring that the programme can use many-core computers efficiently.
    // However, for small grid sizes this can be hurtful, so we do a heuristic here.
    // We increase the number of threads linearly with the data size, and reach the full
    // thread count at 512*512*512 (which is the size for which at least 16 GiG of RAM 
    // will be required for the full potgen programme). For the moderate size of 
    // 256*256*256 we get 8 threads, which seems reasonable. For 128*128*128, on my Laptop
    // the calculation takes about 150ms, so that is the minimum time it makes sense for a thread
    // to run.
    unsigned thread_count = std::max(1u, std::min(64u, static_cast<unsigned>(grid.size() / (128*128*128))));
    auto sub_indices = index.split( thread_count );

    // rng to generate seeds
    std::mt19937_64 seed_engine(seed);
    std::uniform_int_distribution<std::seed_seq::result_type> seed_dist(
            std::numeric_limits<std::seed_seq::result_type>::min(),
            std::numeric_limits<std::seed_seq::result_type>::max());
    auto seeder = std::bind(seed_dist, seed_engine);

    for(auto sub : sub_indices)
    {
        // create a seed sequence using seeder
        std::array<std::seed_seq::result_type, std::mt19937_64::state_size> seq;
        // Use std::ref here, because generate takes the function object by value, and
        // we then do not modify the change of seeder.
        std::generate(begin(seq), end(seq), std::ref(seeder));
        std::seed_seq seed_seq(begin(seq), end(seq));

        // create a new rng from the seed sequence
        std::mt19937_64 generator(seed_seq);
        std::uniform_real_distribution<double> distribution(0.0, 2 * pi);
        auto rnd = std::bind(distribution, generator);

        tasks.push_back(std::async(std::launch::async, randomize_generic, std::ref(grid), rnd, sub));
    }
}
