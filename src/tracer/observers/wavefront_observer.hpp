#ifndef WAVEFRONTOBSERVER_H_INCLUDED
#define WAVEFRONTOBSERVER_H_INCLUDED

#include "observer.hpp"
#include <vector>

class WavefrontObserver : public ThreadSharedObserver
{
    public:
        WavefrontObserver(double time, std::string file_name="wavefront.ply");

        void startTrajectory(const InitialCondition& start, std::size_t) override;

        void save(std::ostream& target) override;

    private:
        virtual bool watch(const State& state, double t);

        double mStopTime;

        const InitialCondition* mCachedInitialCondition;

        struct pos_t
        {
            std::size_t pid;
            gen_vect position;
            std::vector<int> initial_manifold;
            gen_vect uv_coords;
        };

        std::vector<pos_t> mManifoldPositions;
};

#endif // WAVEFRONTOBSERVER_H_INCLUDED
