//
// Created by erik on 5/11/18.
//

#include <interpolation.hpp>
#include <fileIO.hpp>
#include <dynamic_grid.hpp>
#include "radial_density_observer.hpp"
#include "global.hpp"
#include "initial_conditions/initial_conditions.hpp"

RadialDensityObserver::RadialDensityObserver(int resolution, std::vector<double> radii, std::string file_name) :
        ThreadLocalObserver( std::move(file_name) ),
        mRadii( std::move(radii) ),
        mResolution( resolution )
{
    if(mRadii.empty())
    {
        THROW_EXCEPTION(std::invalid_argument, "Empty list of radii supplied to RadialDensityObserver");
    }

    for(auto radius : mRadii)
    {
        if(radius <= 0) {
            THROW_EXCEPTION(std::invalid_argument, "Negative radius %1% given to RadialDensityObserver", radius);
        }
    }

    // sort the radii
    std::sort(begin(mRadii), mRadii.end());

    for(unsigned i = 0; i < mRadii.size(); ++i) {
        // create a new grid
        mCounts.emplace_back(1, resolution);
        // and initialize it to zero.
        for(auto& v : mCounts.back())
        {
            v = 0u;
        }
    }
}

void RadialDensityObserver::startTrajectory( const InitialCondition& init, std::size_t trajectory )
{
    mStartPosition = init.getState().getPosition();
    mLastPosition = gen_vect(2);
    mLastRadius = 0.0;
    mLastRadiusIndex = 0;
}

bool RadialDensityObserver::watch( const State& state, double t )
{
    const gen_vect& pos = state.getPosition();
    gen_vect delta = pos - mStartPosition;
    /// \todo we could get rid of a few sqrt calls here.
    double r = boost::numeric::ublas::norm_2(delta);
    if(r > mRadii[mLastRadiusIndex]) {
        // linear interpolation
        double s = (mRadii[mLastRadiusIndex] - mLastRadius) / (r - mLastRadius);
        auto interpol = interpolate_linear_1d(mLastPosition, delta, s);

        double angle = std::atan2(interpol[1], interpol[0]); // in (-pi, pi)
        double bin = (angle / (2*pi) + 0.5) * mResolution;
        mCounts[mLastRadiusIndex][std::floor(bin)] += 1;
        if(mLastRadiusIndex == mRadii.size() - 1)
            return false;
        else
            mLastRadiusIndex += 1;
    }

    mLastRadius = r;
    mLastPosition = delta;

    return true;
}

std::shared_ptr<ThreadLocalObserver> RadialDensityObserver::clone() const {
    return std::make_shared<RadialDensityObserver>(mResolution, mRadii, filename());
}

void RadialDensityObserver::combine(ThreadLocalObserver& other) {
    auto& source = dynamic_cast<RadialDensityObserver&>(other);
    for(unsigned i = 0; i < mCounts.size(); ++i) {
        std::transform(mCounts[i].begin(), mCounts[i].end(), source.mCounts[i].begin(), mCounts[i].begin(),
                       std::plus<std::uint32_t>());
    }
}

void RadialDensityObserver::save(std::ostream& target) {
/*! Angular Histogram save file format.
        Header: rade001\\n
        Data type     | Count | Meaning
        ---------     | ----- | -------
        int [\#R]     | 1     | Number of radii
        int           | 1     | Resolution
        double        | \#R   | Radii
        grid          | \#R   | counts
    */

    // file header
    target << "rade001\n";
    writeInteger(target, mRadii.size());
    writeInteger(target, mResolution);

    // write radii
    writeFloats(target, mRadii);

    for(const auto& counts : mCounts)
    {
        counts.dump(target);
    }
}
