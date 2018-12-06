#ifndef POTENTIAL_HPP_INCLUDED
#define POTENTIAL_HPP_INCLUDED

/*! \file potential.hpp
    \brief Defines a class that manages n-dimensional Potentials (including derivatives).
    \ingroup common
*/

#include "vector.hpp"
#include <vector>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include "global.hpp"
#include "dynamic_grid.hpp"
#include "multiindex.hpp"

/*! \class Potential
    \brief Class for managing potentials and derivatives.
    \details This class manages an n-dimensional potential along with its derivatives. It manages their
            grids for saving the raw data, contains metainformation as to the parameters of the Potential
            and adds a support definition to define the physical scale of the potential data.
    \attention This class is not optimized for speed. If you need to use data from a protential in a tight loop,
            get the data grid once and then only access that grid.
    \ingroup common
*/
class Potential final
{
    struct IndexType
    {
        IndexType(std::string n, std::vector<int> d) : name(std::move(n)), derivations(std::move(d)) {}
        std::string name;
        std::vector<int> derivations;
        bool operator<(const IndexType& other) const
        {
            if(name < other.name)
                return true;
            else if(name == other.name)
                return std::lexicographical_compare(derivations.begin(), derivations.end(), other.derivations.begin(), other.derivations.end());
            return false;
        }
    };
public:
    // commonly used typedefs
    /// type that is saved inside the potential
    typedef double value_type;
    /// type of the grid that is used to save the potential and its derivatives.
    typedef DynamicGrid<value_type> grid_type;

    // --------------------------------------------------
    //        c/d tors
    // --------------------------------------------------
    /*!    \brief square potential ctor.
        \details This constructor creates a potential that is equivalent in all dimensions, i.e.
                extents and support are the same for all dimensions.
        \param size size of the potential
        \param dimension dimension of the potential
        \param support support (in physical domain) of the potential
        \param strength Strength of the potential. Usually assumed to mean standard deviation of the potential.
    */
    Potential(std::size_t dimension, double support, std::size_t size, double strength=1);

    /*! \brief Constructs a Potential object with defined extents, support and max_order.
        \param extents extents of the potential. This is the number of elements in each dimension.
        \param support This defines the size of the Potential in physical domain.
        \param strength Strength of the potential. Usually assumed to mean standard deviation of the potential.
    */
    Potential(std::vector<std::size_t> extents, std::vector<double> support, double strength=1);

    /// user defined move constructor, because extents in defined as a const member.
    Potential( Potential&& ) noexcept;

    /// no copying
    Potential( const Potential& ) = delete;
    /// no copying
    Potential& operator=(const Potential&) = delete;

    // --------------------------------------------------
    //               info functions
    // --------------------------------------------------

    /// sets the creation info
    void setCreationInfo( std::size_t seed, std::size_t version, double corrlength);

    /// returns the size of the potential
    /// defined inline because it might be called from hot part of code
    const std::vector<std::size_t>& getExtents() const { return mExtents; };

    /// gets the real size of the domain the potential is defined on. This is [0, getSupport)^N
    const std::vector<double>& getSupport() const;

    /// get seed used to create this potential
    std::size_t getSeed() const;

    /// get potgen version
    std::size_t getPotgenVersion() const;

    /// gets potential dimension
    std::size_t getDimension() const;

    /// get the correlation length used to generate this potential
    /// returns -1 if no valid correlation length
    double getCorrelationLength() const;

    // --------------------------------------------------
    //            read access potential data
    // --------------------------------------------------

    /// returns a grid with the potential data
    const grid_type& getPotential(const std::string& name = "potential") const;

    /// returns a grid with given derivative, checking that \p deriv is in fact valid.
    const grid_type& getDerivative( std::vector<int> deriv, const std::string& name = "potential" ) const;
    
    /// returns a grid with the data of the index'th derivative
    template<class T>
    const grid_type& getDerivative( const T& index, const std::string& name = "potential" ) const
    {
        std::vector<int> index_vec(index.begin(), index.end());
        return getDerivative(index_vec, name);
    }

    /// checks whether a certain derivative is saved inside this potential
    bool hasDerivative( const MultiIndex& index, const std::string& name = "potential") const;

    /// checks whether all derivatives of a certain order are present
    bool hasDerivativesOfOrder( std::size_t order, const std::string& name = "potential" ) const;

    // --------------------------------------------------
    //            write access potential data
    // --------------------------------------------------

    /// set potential \sa setDerivative
    const grid_type& setPotential( grid_type data, const std::string& name = "potential" );

    /// sets the n'th derivative. implemented as a move operation to prevent copies of
    /// really big vectors.
    /// \return the newly created derivative grid
    const grid_type& setDerivative( const MultiIndex& index, grid_type data, const std::string& name = "potential" );
    
    const grid_type& setDerivative( std::vector<int> deriv, grid_type data, const std::string& name = "potential" );

    // --------------------------------------------------
    //               transformation
    // --------------------------------------------------

    /// multiplies all derivatives and the potential itself by a constant \p factor
    /// use empty /p name to scale all potentials simultaneously.
    void scalePotential( double factor, const std::string& name = "" );

    /// changes the size of the potential. This scaling of the definition domain affects
    /// the value of derivatives.
    /// \attention  This only changes derivatives based on the changing length scale. If
    ///                the corresponding physical quantity has to be scaled too, use 
    ///                scalePotential.
    void setSupport( const std::vector<double>&  supp, const std::string& name = "potential");

    // --------------------------------------------------
    //               strength
    // --------------------------------------------------
    /// gets the strength prefactor of the potential.
    double getStrength() const;

    /// sets the strength of the potential
    void setStrength(double new_strength);

    // --------------------------------------------------
    //               serialization
    // --------------------------------------------------

    /// writes this potential to a file
    void writeToFile( std::fstream& file ) const;

    /// creates a potential object by reading from a file
    static Potential readFromFile( std::fstream& file );


private:
    // helper functions
    /// gets the total order of the derivative index
    std::size_t getOrder( const MultiIndex& dindex ) const;

    // general data
    /// potentials dimension
    const std::size_t mDimension;
    /// seed used to generate this potential
    std::size_t mSeed;
    /// size of the potential
    const std::vector<std::size_t> mExtents;
    /// support size
    std::vector<double> mSupport;
    /// potential strength
    double mStrength = 1;
    /// correlation length
    double mCorrelationLength = -1;
    /// version of the potgen function
    std::size_t mPotgenVersion;
    /// potential and derivatives grids
    std::map<IndexType, grid_type> mData;

};

#endif // POTENTIAL_HPP_INCLUDED
