#ifndef PHYSICS_H
#define PHYSICS_H

#include <atomic>
#include <chrono>
#include <vector>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>

#include "celestialBody.h"
#include "maths.h"

class Physics {
public:
    static void Initialise();

    static std::atomic<double> gTimeScale;
    static std::vector<CelestialBody> Bodies;

private:
    static std::vector<glm::dvec3> computeAccelerations(const std::vector<CelestialBody> &Bodies, const std::vector<glm::dvec3>& positions);
    static void updatePhysics();

    static std::thread physicsThread;
};

#endif //PHYSICS_H