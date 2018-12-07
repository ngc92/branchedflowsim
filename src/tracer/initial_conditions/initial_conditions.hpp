#ifndef IC2_HPP_INCLUDED
#define IC2_HPP_INCLUDED

/// \file
/// This file defines the basic classes used for initial condition generation.

#include <cstddef>
#include <functional>
#include "state.hpp"
#include "initial_conditions_fwd.hpp"
#include <mutex>
#include "multiindex.hpp"

class RayDynamics;

/// namespace for all initial condition code.
namespace init_cond
{

    class InitialCondition;

    // -----------------------------------------------------------------------------------------------------------------
    //                        Initial Condition Configuration
    // -----------------------------------------------------------------------------------------------------------------
    /*!
     * \class InitialConditionConfiguration
     * \brief This class manages the options passed to an InitialConditionGenerator.
     */
    class InitialConditionConfiguration
    {
    public:
        // energy normalization
        /// set whether to normalize energy
        InitialConditionConfiguration& setEnergyNormalization( bool ne );

        /// get whether energy will be normalized
        bool getEnergyNormalization() const;

        /// sets the dynamics which are needed to correctly normalize energy.
        InitialConditionConfiguration& setDynamics(std::shared_ptr<const RayDynamics> dynamics);

        /// gets the ray dynamics. Throws if they are not set.
        const RayDynamics& getDynamics() const;

        /// \brief set number of particles to trace
        InitialConditionConfiguration& setParticleCount( std::size_t count);

        /// get how many particles will be traced
        std::size_t getParticleCount() const;

        // support
        /// sets the support upon which tracing will take place. This tells
        /// the initial condition how to scale positions.
        InitialConditionConfiguration& setSupport( std::vector<double> support );

        /// gets the support.
        const std::vector<double>& getSupport() const;

        // offset
        /// sets the offset vector, which is added to all initial ray positions.
        /// This is in world coordinates.
        InitialConditionConfiguration& setOffset( const gen_vect& offset );

        /// gets the offset vector.
        const gen_vect& getOffset() const;

        /// sets the IC coordinates to relative.
        InitialConditionConfiguration& setUseRelativeCoordinates(bool mode=true);

        /// gets whether the IC coordinates are relative.
        bool getUseRelativeCoordinates() const;

    private:
        /// whether to normalize energy
        bool mNormalizeEnergy = true;

        /// whether to use absolute or relative ([0, 1]) coordinates
        bool mIsRelative = true;

        /// number of particles
        std::size_t mNumOfParticles = 0;

        /// the support on which the tracing will happen
        std::vector<double> mSupport;

        /// offset (world coordinates) for traced rays
        gen_vect mOffset;

        /// the ray dynamics.
        std::shared_ptr<const RayDynamics> mDynamics;
    };

    /// Type used to indicate a vector in initial manifold coordinates.
    using manifold_pos = gen_vect;

    /*! \class InitialCondition
        \brief class that represents a single initial condition.
        \details Created by InitialConditionGenerator class. This class preallocates enough memory to hold an initial
            condition. Therefore, using its iteration interface is faster than repeatedly calling
            InitialConditionGenerator::next().
    */
    class InitialCondition
    {
    public:
        ~InitialCondition();
        InitialCondition(InitialCondition&&) = default;

        /// gets the initial State
        const State& getState() const;

        /// get the delta in manifold direction \p idx
        /// \note State contains a matrix, but this is not part of a delta
        const State& getDelta( int idx ) const;

        /// gets the manifold position as index
        const std::vector<int>& getManifoldIndex( ) const;

        /// gets the manifold position as coordinates
        const manifold_pos& getManifoldCoordinates() const;


        // ------  iterator interface ------
        explicit operator bool() const;
        InitialCondition& operator++();

    private:
        friend class InitialConditionGenerator;
        /// only callable by InitialConditionGenerator
        explicit InitialCondition( InitialConditionGenerator& generator );


        /// cached current State
        State mCurrentState;
        /// deltas
        std::vector<State> mDeltas;
        /// manifold index
        std::vector<int> mManifoldIndex;
        /// normalized manifold coordinates
        manifold_pos mManifoldCoordinates;

        /// sets whether this state is valid (i.e. not yet reached the end)
        /// default constructed to false, as soon as advanced was called valid
        bool mIsValid = false;
        /// reference to generator
        InitialConditionGenerator* mGenerator;

        void advance();
    };


    // -----------------------------------------------------------------------------------------------------------------
    //                        Initial Condition Generator class interface
    // -----------------------------------------------------------------------------------------------------------------
    /*! \class InitialConditionGenerator
     *  \brief Class that is responsible for generating initial states of rays.
     *  \details This class manages the creation of ray states on the initial condition
     *          manifold. An initial condition can then be created using the next() function.
     *          The next() and advance() functions can safely be called concurrently from different threads.
     *          Before any IC can be generated, the init() method has to be called and gets supplied with
     *          a InitialConditionConfiguration config object.
     *
     *          For a thorough documentation of the generation process, see \ref ic_doc.
     */
    class InitialConditionGenerator
    {
    public:
        /// create the initial condition generator with defined world and manifold dimensions and a name.
        InitialConditionGenerator( std::size_t world_dimension, std::size_t manifold_dimension, std::string name );

        /*! \brief Gives the dimension of the system in which tracing is going to happen.
         * \details This is the dimension that position and velocity vectors of the produced
         *  initial conditions will have. Usually (though not necessarily) the manifold dimension
         *  (getManifoldDimension()) will be one less than the world dimension.
        */
        std::size_t getWorldDimension() const;

        /// gets the dimension of the initial manifold.
        std::size_t getManifoldDimension() const;

        // --------------------------------------------------------------
        // generation interface
        // ---------------------
        /// init function, has to be called before iteration starts.
        /// checks config for consistency, and set the configuration.
        /// initializes manifold position and index.
        /// \throw std::logic_error if config is invalid.
        void init(const InitialConditionConfiguration& config);

        /// creates a new iterator for going over the initial conditions. Multiple iterators can be used concurrently,
        /// and they collectively iterate over all initial rays exactly once.
        /// \throw std::logic_error, if not yet initialized.
        InitialCondition next();

        /// gets a string that identifies the type of the generator.
        const std::string& getGeneratorType() const;

        // -------------------------------------------------------------------------------------------------------------
        //  info functions
        /// get how many particles will be traced
        std::size_t getParticleCount() const;
    protected:
        /// Dimension of the world in which the dimension is run. \sa getWorldDimension()
        const std::size_t mWorldDimension;

        /// Dimension of the manifold formed by the initial conditions. \sa getManifoldDimension()
        const std::size_t mManifoldDimension;

        /*! \brief This function is called when the generator is initialized.
         * \details Its job is to set up the multi-index that will be used
         * for iteration over the initialization manifold. The multi-index will be
         * initialized (MultiIndex::init) by the calling function.
         * @param manifold_index MultiIndex of getManifoldDimension() dimensions,
         *        whose lower bound is set to zero, and upper bound is not yet set.
         */
        virtual void init_generator(MultiIndex& manifold_index);
    private:

        /// called before a new trajectory is requested.
        /// param pos the manifold position of the trajectory in coordinates.
        /// param index the manifold position as index. Changing this allows to dynamically adapt the bounds.
        virtual void next_trajectory(const manifold_pos& pos, MultiIndex& index) { (void)pos; (void)index; };

        /*! \brief Generate position anv velocity for given initial manifold position.
         * \details This function is the customization point that derived classes have to implement that
         *          executes the actual generation of a new initial ray state.
         * \param[in] manifold_position relative position of the new particle on the initial manifold.
         *            getManifoldDimension() entries in the range (0, 1)
         * \param[out] ray_position New ray's initial position
         * \param[out] ray_velocity New ray's initial velocity.
         */
        virtual void
        generate(gen_vect& ray_position, gen_vect& ray_velocity, const manifold_pos& manifold_position) const = 0;

        /*! generates a new state and normalizes the energy if required.
         *  \param pos relative position of the particle on the new manifold
         *  \param[out] state Target where to write the new state
         */
        void generateNormalized( State& state, const manifold_pos& pos) const;

        /// moved the given initial condition to the next free position.
        /// This function is called from InitialCondition::InitialCondition().
        void advance( InitialCondition& newCondition );

        /// helper function that updates the manifold position based on the manifold index.
        void updateManifoldPosition();

        // data
        // ----
        /// settings for IC generation.
        InitialConditionConfiguration mConfig;

        /// name of this generator. This has only documentation purposes.
        std::string mName;

        // relative coordinates on the manifold
        /// The discrete coordinates on the initial manifold as a MultiIndex for iteration.
        MultiIndex mManifoldIndex;
        /// The coordiantes on the initial manifold in [0, 1] range. This is automatically filled
        /// by updateManifoldPosition() from the data given in mManifoldIndex.
        manifold_pos mManifoldPosition;

        /// this mutex is locked during the generation of an initial condition.
        std::mutex mIsGenerating;

        friend void InitialCondition::advance();
    };

} // namespace init_cond

#endif // IC2_HPP_INCLUDED
