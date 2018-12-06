#include "wavefront_observer.hpp"
#include "global.hpp"
#include "initial_conditions/initial_conditions.hpp"
#include <fstream>

WavefrontObserver::WavefrontObserver(double time, std::string file_name) :
        ThreadSharedObserver( std::move(file_name) ),
        mStopTime( time )
{

}

void WavefrontObserver::startTrajectory(const InitialCondition& start, std::size_t)
{
    mCachedInitialCondition = &start;
}

void WavefrontObserver::save(std::ostream& target)
{
    target << "ply\n";
    target << "format ascii 1.0\n";
    target << "element vertex " << mManifoldPositions.size() << "\n";
    target << "property float x\nproperty float y\nproperty float z\n";
    target << "property uchar red\nproperty uchar green\nproperty uchar blue\n";

    // make quad index list
    std::vector<std::vector<int>> mQuads;
    for(unsigned i = 0; i < mManifoldPositions.size(); ++i)
    {
        std::vector<int> quad;
        quad.push_back(i);
        for(unsigned j = 0; j < mManifoldPositions.size(); ++j)
        {
            if(mManifoldPositions[i].initial_manifold[0] == mManifoldPositions[j].initial_manifold[0] && mManifoldPositions[i].initial_manifold[1] + 1 == mManifoldPositions[j].initial_manifold[1])
                quad.push_back(j);
            if(mManifoldPositions[i].initial_manifold[0] + 1== mManifoldPositions[j].initial_manifold[0] && mManifoldPositions[i].initial_manifold[1] + 1 == mManifoldPositions[j].initial_manifold[1])
                quad.push_back(j);
            if(mManifoldPositions[i].initial_manifold[0] +1  == mManifoldPositions[j].initial_manifold[0] && mManifoldPositions[i].initial_manifold[1] == mManifoldPositions[j].initial_manifold[1])
                quad.push_back(j);
        }
        if( quad.size() == 4 )
            mQuads.push_back(quad);
    }

    target << "element face " << mQuads.size() << "\n";
    target << "property list uchar int vertex_index\n";
    target << "end_header\n";

    // file header
    for(const auto& p : mManifoldPositions)
    {
        for(unsigned i = 0; i < p.position.size(); ++i)
        {
            target << p.position[i] << " ";
        }

        /// \todo define nice color
        for(unsigned i = 0; i < 3; ++i)
            target << (int)(128 + 128 * (int(p.uv_coords[i] * 50 + i) % 2 )) << " ";
        target << "\n";
    }

    for(auto& p : mQuads)
    {
        target << 4 << " ";
        // sort vertices
        auto edge1 = mManifoldPositions[p[0]].position - mManifoldPositions[p[1]].position;
        auto edge2 = mManifoldPositions[p[2]].position - mManifoldPositions[p[3]].position;
        if(std::inner_product(edge1.begin(), edge1.end(), edge2.begin(), 0.0) > 0)
            std::swap(p[2], p[3]);

        target << p[0] << " " << p[1] << " " << p[2] << " " << p[3];
        target << "\n";
    }
}

bool WavefrontObserver::watch(const State& state, double t)
{
    if( t > mStopTime )
    {
        pos_t new_pos;
        new_pos.pid = -1;
        new_pos.position = state.getPosition();
        new_pos.initial_manifold = mCachedInitialCondition->getManifoldIndex();
        new_pos.uv_coords = mCachedInitialCondition->getManifoldCoordinates();
        mManifoldPositions.push_back(new_pos);

        return false;
    }

    return true;
}
