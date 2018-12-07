#include "tracer_factory.h"
#include "potential.hpp"
#include "tracer.hpp"
#include "observers/observer_factory.hpp"
#include "dynamics/dynamics_factory.hpp"
#include "dynamics/ray_dynamics.hpp"
#include <boost/lexical_cast.hpp>
#include <fstream>
#include "fileIO.hpp"
#include "interpolation.hpp"
#include "factory/factory.hpp"

using std::fstream;

TracerFactory::TracerFactory() = default;

void TracerFactory::loadFile( std::string filename )
{
	mFilename = std::move(filename);
	fstream ifile(mFilename, fstream::in | std::fstream::binary);
	if( !ifile.is_open() )
	{
		std::cerr << "could not open potential file " << mFilename << "\n";
		exit( EXIT_FAILURE );
	}

	setPotential(Potential::readFromFile(ifile));
}

void TracerFactory::setPotential( Potential p )
{
	mPotential = std::make_shared<Potential>( std::move(p) );
}

void TracerFactory::setPeriodicBondaries( bool pb )
{
	mPeriodicBoundaries = pb;
}

std::shared_ptr<Tracer> TracerFactory::createTracer() const
{
	if(!mPotential)
		THROW_EXCEPTION( std::runtime_error, "trying to create tracer, but no potential has been set!");

	// figure out whether we need monodromy
	// create observers
	bool monodromy = std::any_of(begin(mObserverConfig), end(mObserverConfig),
				[](const std::vector<std::string>& options ){
					return getObserverFactory().get_builder(options.front())->need_monodromy(); }
	);
	auto dynamics = getDynamicsFactory().create(mDynamicsType, mDynamicsConfig, *mPotential, mPeriodicBoundaries,
												monodromy);
		
	auto tracer = std::make_shared<Tracer>( *mPotential, std::move(dynamics));

	tracer->setMaxThreads( mThreads );

	// create observers
	for(const auto& cfg : mObserverConfig)
	{
		std::vector<std::string> options;
		std::copy(cfg.begin() + 1, cfg.end(), std::back_inserter(options));
		auto observer = getObserverFactory().create(cfg.front(), options, *mPotential);
		tracer->addObserver( std::move(observer) );
	}

	tracer->setErrorBounds( mAbsErr, mRelErr );
	tracer->setEndTime( mEndTime );
	tracer->setIntegrator(mIntegrator);

	if(mDT > 0) {
		tracer->setTimeStep(mDT);
	}

	return tracer;
}

void TracerFactory::setPotentialStrength( double strength )
{ ;
	mPotential->setStrength(strength);
}

std::string TracerFactory::getPotentialInfo() const
{
	std::stringstream potential_data_str;
	potential_data_str << "size: " << mPotential->getExtents()[0];
	for(unsigned i = 1; i < mPotential->getDimension(); ++i)
		potential_data_str << "x" << mPotential->getExtents()[i];
	potential_data_str << "\nsupport: " << mPotential->getSupport()[0];
	for(unsigned i = 1; i < mPotential->getDimension(); ++i)
		potential_data_str << "x" << mPotential->getSupport()[i];
	potential_data_str << "\nseed: " << mPotential->getSeed() << "\n";
	potential_data_str << "corr length: " << mPotential->getCorrelationLength() << "\n";
	potential_data_str << "strength: " << mPotential->getStrength() << "\n";
	potential_data_str << "version: " << mPotential->getPotgenVersion() << "\n";
	return potential_data_str.str();
}

void TracerFactory::setObserverConfig( std::vector<std::string> cfg )
{
	mObserverConfig.clear();
	std::vector<std::string> known_observers = getObserverFactory().getTypes();
	for( const auto& str : cfg )
	{
		if( std::count(known_observers.begin(), known_observers.end(), str) != 0 )
		{
			mObserverConfig.emplace_back();
		}

		if(mObserverConfig.empty())
			THROW_EXCEPTION(std::runtime_error, "%1% is not an observer name.", str);
		mObserverConfig.back().push_back(str);
	}
}

void TracerFactory::setDynamicsConfig( std::vector<std::string> cfg )
{
	mDynamicsConfig.clear();
	mDynamicsType = cfg[0];
	std::move(begin(cfg)+1, end(cfg), std::back_inserter(mDynamicsConfig) );
}

void TracerFactory::setIntegrator(const std::string& integrator) {
    if(integrator == "adaptive") {
        mIntegrator = Integrator::RUNGE_KUTTA_CASH_KARP_54_ADAPTIVE;
    } else if(integrator == "euler") {
        mIntegrator = Integrator::EULER_CONST;
    } else {
        THROW_EXCEPTION( std::runtime_error, "Unknown integrator %1%", integrator);
    }
}
