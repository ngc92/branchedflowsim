#include <iostream>
#include <cmath>
#include <fstream>
#include <functional>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include "fft.hpp"
#include "potgen.hpp"
#include "fileIO.hpp"
#include "potgen_args.h"
#include "profiling.hpp"
#include "correlation.hpp"
#include "discretize.hpp"

int main(int argc, const char* argv[])
{
	parse_parameters(argc, argv);

	PGOptions opt;

    // Get options from command line
	opt.randomSeed         = pargs::seed;
	opt.maxDerivativeOrder = pargs::derivative_order;
	opt.corrlength         = pargs::correlation_length;
	opt.numThreads         = pargs::threads;
	opt.verbose            = pargs::print_profile;

	
	// check that dimension is valid
	if( pargs::dim < 1 || pargs::dim > 3)
	{
		std::cerr << "invalid dimension " << pargs::dim << " specified\n";
		return EXIT_FAILURE;
	}
	
    try
	{
		// create output file, so if sth goes wrong we do not need to wait for the computation to finish
		// to issue an error
		std::fstream save(pargs::potential_outfile, std::fstream::out | std::fstream::binary);
		if( !save.good())
		{
			std::cerr << "could not open result file " << pargs::potential_outfile << " " << std::strerror(errno) << "\n";
			return EXIT_FAILURE;
		}

		// generate the correlation function
		opt.cor_fun = makeCorrelation( pargs::correlation_function, pargs::correlation_length, pargs::correlation_trafo);

		std::vector<std::size_t> extents(pargs::size.begin(), pargs::size.end());
		if(extents.size() == 1)
			extents.resize( pargs::dim, extents[0] );

		if( extents.size() != pargs::dim )
		{
			std::cerr << "Invalid number of size factors\n";
			exit(EXIT_FAILURE);
		}

		// debug output
		std::cout << "generate potential of size " << extents[0];
		for(int i = 1; i < pargs::dim; ++i)
			std::cout << "x"<<extents[i];
		std::cout << "\n";

		// make support area
		// we use the same aspect ratio as for the extents
		std::vector<double> support(pargs::dim);
		double min_ext = *std::min_element( extents.begin(), extents.end() );
		for(int i = 0; i < pargs::dim; ++i)
		{
			support[i] = (double)extents[i] / min_ext;
		}

		if(pargs::correlation_only)
		{
			auto grid = discretizeFunctionForFFT(extents, support, opt.cor_fun);

            // convert to real
            default_grid real_pot(extents, TransformationType::FFT_INDEX);

            auto it = real_pot.begin();
            for(const auto& value : grid)
            {
                *it = std::real(value);
                ++it;
            }


			std::cout << "saving correlation to " << pargs::potential_outfile << "\n";
            real_pot.dump(save);
			save.close();
		}
		else
		{
			auto pot = generatePotential(extents, support, opt);

			auto &pot_data = pot.getPotential();

			// control result
			std::cout << "Avg: " << std::accumulate(pot_data.begin(), pot_data.end(), 0.0) / (double) (pot_data.size())
					  << "\n";

			double variance = 0;
			double min = 0;
			double max = 0;;
			for (auto val : pot_data) {
				if (val > max) max = val;
				if (val < min) min = val;
				variance += val * val / pot_data.size();
			}

			std::cout << "Var: " << variance << "\n";

			pot.setStrength(pargs::strength);

			std::cout << "saving potential to " << pargs::potential_outfile << "\n";
			char write_buffer[1024 * 512];
			save.rdbuf()->pubsetbuf(write_buffer, sizeof(write_buffer));
			pot.writeToFile(save);
			save.close();
		}

		if( !pargs::no_wisdom )
		{
			saveFFTWisdom();
		}

		// profiling output
		if( pargs::print_profile )
		{
			ProfileRecord::print_profiling_data();
		}
	} catch ( boost::exception& e )
	{
		std::cerr << "an exception occurred: " << boost::diagnostic_information(e) << "\n";
		return 1;
	}
}
