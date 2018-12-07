#include "initial_conditions.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <thread>
#include <boost/numeric/conversion/cast.hpp>
//#include "profile.hpp"
#include "dynamics/ray_dynamics.hpp"

// concrete IC implementations
#include "init_factory.hpp"

using namespace init_cond;

// getters and setters
// -------------------
InitialConditionConfiguration& InitialConditionConfiguration::setEnergyNormalization(bool ne )
{
    mNormalizeEnergy = ne;
    return *this;
}

bool init_cond::InitialConditionConfiguration::getEnergyNormalization() const
{
    return mNormalizeEnergy;
}

InitialConditionConfiguration& InitialConditionConfiguration::setDynamics(std::shared_ptr<const RayDynamics> dynamics)
{
    mDynamics = move(dynamics);
    return *this;
}

InitialConditionConfiguration& InitialConditionConfiguration::setParticleCount(std::size_t count)
{
    mNumOfParticles = count;
    return *this;
}

std::size_t InitialConditionConfiguration::getParticleCount() const
{
    return mNumOfParticles;
}

InitialConditionConfiguration& InitialConditionConfiguration::setSupport(std::vector<double> support)
{
    mSupport = std::move(support);
    return *this;
}

const std::vector<double>& InitialConditionConfiguration::getSupport() const
{
    return mSupport;
}

const RayDynamics& InitialConditionConfiguration::getDynamics() const {
    if(mDynamics) {
        return *mDynamics;
    }
    THROW_EXCEPTION(std::logic_error, "No ray dynamics have been set.");
}

InitialConditionConfiguration& InitialConditionConfiguration::setUseRelativeCoordinates(bool mode)
{
    mIsRelative = mode;
    return *this;
}

bool InitialConditionConfiguration::getUseRelativeCoordinates() const
{
    return mIsRelative;
}

InitialConditionConfiguration& InitialConditionConfiguration::setOffset(const gen_vect& offset)
{
    mOffset = offset;
    return *this;
}

const gen_vect& InitialConditionConfiguration::getOffset() const
{
    return mOffset;
}

// ************************************************
//                     BASE CLASS
// ************************************************

// -----------------------------------------------------------------------------------------------------------------------------
//                        Initial Condition Generator class implementation
// -----------------------------------------------------------------------------------------------------------------------------

// constructor
// -----------

InitialConditionGenerator::InitialConditionGenerator( std::size_t world_dimension, std::size_t manifold_dimension,
                                                      std::string name ) :
        mWorldDimension(world_dimension),
        mManifoldDimension(manifold_dimension),
        mName( std::move(name) ),
        mManifoldIndex(manifold_dimension)
{
    if( mManifoldDimension == 0 || mManifoldDimension > 2*mWorldDimension )
        THROW_EXCEPTION( std::logic_error, "incompatible dimensions: manifold %1%; world %2%", mManifoldDimension, mWorldDimension );
}

std::size_t InitialConditionGenerator::getWorldDimension() const
{
    return mWorldDimension;
}

// iterator interface
// ------------------
void InitialConditionGenerator::init(const InitialConditionConfiguration& config)
{
    // check validity num of particles
    if(config.getParticleCount() < 1)
        THROW_EXCEPTION( std::logic_error, "trying to init ICG even though no particles are set!" );

    if(config.getEnergyNormalization()) {
        // check whether we can get the dynamics.
        try {
            config.getDynamics();
        }
        catch(std::logic_error& err)
        {
            THROW_EXCEPTION(std::logic_error, "trying to init ICG even though dynamics is not set!");
        }
    }

    if(config.getSupport().size() != mWorldDimension) {
        THROW_EXCEPTION(std::logic_error, "Got %1% dimensional support in %2% dimensional world!",
                        config.getSupport().size(), mWorldDimension);
    }

    if(config.getOffset().size() != mWorldDimension) {
        THROW_EXCEPTION(std::logic_error, "Got %1% dimensional offset in %2% dimensional world!",
                        config.getOffset().size(), mWorldDimension);
    }

    mConfig = config;

    mManifoldPosition.resize( mManifoldDimension );
    mManifoldIndex.setLowerBound(0);
    init_generator( mManifoldIndex );
    mManifoldIndex.init();

    updateManifoldPosition();
}

void InitialConditionGenerator::generateNormalized( State& state, const manifold_pos& pos ) const
{
    generate(state.editPos(), state.editVel(), pos);

    // it should be basically impossible to fuck this up,
    // so an assert seems enough here.
    assert(state.getPosition().size() == mWorldDimension);
    assert(state.getVelocity().size() == mWorldDimension);

    // if so desired, rescale state into support
    if(mConfig.getUseRelativeCoordinates())
    {
        for(unsigned i = 0; i < mConfig.getSupport().size(); ++i) 
        {
            state.editPos()[i] *= mConfig.getSupport()[i];
        }
    }

    state.editPos() += mConfig.getOffset();
    /// \todo some rudimentary state checking, at least optionally

    // now that we have the final position and velocity: normalize energy
    if(mConfig.getEnergyNormalization())
    {
        mConfig.getDynamics().normalizeEnergy(state, 0.5);
    }

}

void InitialConditionGenerator::advance( InitialCondition& newCondition )
{
    // since this function is only called from the InitialCondition itself,
    // it is almost impossible to screw this condition up, so we do an assert
    // instead of throwing an exception.
    assert(newCondition.mGenerator == this);

    // for the rest of the function, lock the mutex.
    std::unique_lock<std::mutex> lock(mIsGenerating);
    if(! mManifoldIndex.valid() )
    {
        newCondition.mIsValid = false;
        return;
    }

    next_trajectory(mManifoldPosition, mManifoldIndex);

    generateNormalized(newCondition.mCurrentState, mManifoldPosition);
    newCondition.mIsValid = true;

    // deltas
    double STEP = 1e-5;
    for(unsigned i = 0; i < getManifoldDimension(); ++i)
    {
        newCondition.mManifoldIndex[i] = mManifoldIndex[i];
        newCondition.mManifoldCoordinates[i] = mManifoldPosition[i];
        mManifoldPosition[i] += STEP;
        generateNormalized( newCondition.mDeltas[i], mManifoldPosition);
        mManifoldPosition[i] -= STEP;

        // subtract state and calculate derivative as difference quotient
        newCondition.mDeltas[i].editVel() -= newCondition.mCurrentState.getVelocity();
        newCondition.mDeltas[i].editVel() /= STEP;
        newCondition.mDeltas[i].editPos() -= newCondition.mCurrentState.getPosition();
        newCondition.mDeltas[i].editPos() /= STEP;
    }

    mManifoldIndex.increment();
    if(mManifoldIndex.valid()) {
        updateManifoldPosition();
    }
}

InitialCondition InitialConditionGenerator::next()
{
    if(mConfig.getParticleCount() < 1)
        THROW_EXCEPTION( std::logic_error, "InitialConditionGenerator has not been initialized." );
    return InitialCondition( *this );
}

const std::string& InitialConditionGenerator::getGeneratorType() const
{
    return mName;
}

std::size_t InitialConditionGenerator::getManifoldDimension() const
{
    return mManifoldDimension;
}

std::size_t InitialConditionGenerator::getParticleCount() const
{
    return mConfig.getParticleCount();
}

void InitialConditionGenerator::init_generator(MultiIndex& manifold_index)
{
    double root_count = std::pow(getParticleCount(), 1.0 / getManifoldDimension());
    auto bound = boost::numeric_cast<int>( std::floor(root_count) );
    manifold_index.setUpperBound(bound);
}

void InitialConditionGenerator::updateManifoldPosition()
{
    for(unsigned i = 0; i < getManifoldDimension(); ++i)
    {
        mManifoldPosition[i] = (mManifoldIndex[i] + .5) / (mManifoldIndex.getUpperBound()[i]);
    }
}

// *****************************************************************************
//                            Initial Condition Class
// *****************************************************************************

InitialCondition::InitialCondition( InitialConditionGenerator& gen ) :
    mCurrentState(gen.getWorldDimension() ), mGenerator( &gen )
{
    // this hack is necessary because State does not have a copy constructor, which is required for resize
    for(unsigned i = 0; i < mGenerator->getManifoldDimension(); ++i)
    {
        mDeltas.emplace_back( mGenerator->getWorldDimension() );
    }

    mManifoldIndex.resize( mGenerator->getManifoldDimension() );
    mManifoldCoordinates.resize( mGenerator->getManifoldDimension() );

    // advance to first value
    ++(*this);
}

InitialCondition::~InitialCondition() = default;

const State& InitialCondition::getState() const
{
    return mCurrentState;
}

const State& InitialCondition::getDelta( int idx ) const
{
    return mDeltas[idx];
}

const std::vector<int>& InitialCondition::getManifoldIndex( ) const
{
    return mManifoldIndex;
}

const manifold_pos & InitialCondition::getManifoldCoordinates() const
{
    return mManifoldCoordinates;
}

InitialCondition::operator bool() const
{
    return mIsValid;
}

InitialCondition& InitialCondition::operator++()
{
    advance();
    return *this;
}

void InitialCondition::advance()
{
    mGenerator->advance(*this);
}

// ----------------------------------------------------------------------------------------
//                                     creation function
// ----------------------------------------------------------------------------------------

namespace init_cond
{
    InitCondGenPtr createInitialConditionGenerator(std::size_t dim, const std::vector<std::string>& arguments)
    {
        std::string type = arguments.at(0);
        auto config = std::vector<std::string>(begin(arguments) + 1, end(arguments));
        InitCondGenPtr base = init_cond::getInitialConditionFactory().create(type, config, dim);

        return std::move(base);
    }
}
