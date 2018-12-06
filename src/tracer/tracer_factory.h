#ifndef TRACER_FACTORY_H_INCLUDED
#define TRACER_FACTORY_H_INCLUDED

#include <string>
#include <memory>
#include <vector>
#include "tracer.hpp"

class Tracer;
class Potential;
enum class Integrator : int;

class TracerFactory
{
public:
	TracerFactory();

	void setPeriodicBondaries( bool pb );

	// set a potential
	void loadFile( std::string filename );
	void setPotential( Potential p );

	void setObserverConfig( std::vector<std::string> cfg );
	void setDynamicsConfig( std::vector<std::string> cfg );

	void setThreadCount( std::size_t threads ) { mThreads = threads; };
	void setErrorBounds( double abs_e, double rel_e ) { mAbsErr = abs_e; mRelErr = rel_e;};
	void setEndTime( double et ) { mEndTime = et; };
	void setTimeStep( double dt ) { mDT = dt; };
	void setIntegrator(const std::string& integrator);

	// potential
	void setPotentialStrength( double s );
	std::string getPotentialInfo() const;

	std::shared_ptr<Tracer> createTracer(  ) const;
private:
	std::string mFilename;
	bool mPeriodicBoundaries = false;
	std::size_t mThreads = -1;
	double mAbsErr;
	double mRelErr;
	Integrator mIntegrator = Integrator::RUNGE_KUTTA_CASH_KARP_54_ADAPTIVE;
	double mEndTime;
	double mDT = -1;
	std::string mPotentialDataString;

	std::shared_ptr<Potential> mPotential;
	std::vector<std::vector<std::string>> mObserverConfig;
	std::string mDynamicsType;
	std::vector<std::string> mDynamicsConfig;
};

#endif // TRACER_FACTORY_H_INCLUDED
