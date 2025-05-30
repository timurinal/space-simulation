#include "physics.h"

// Define static members
std::atomic<double> Physics::gTimeScale{1.0};
std::vector<CelestialBody> Physics::Bodies{};
std::thread Physics::physicsThread;

void Physics::Initialise() {
    physicsThread = std::thread(&Physics::updatePhysics);
    physicsThread.detach();
}

std::vector<glm::dvec3> Physics::computeAccelerations(const std::vector<CelestialBody> &Bodies, const std::vector<glm::dvec3> &positions) {
    std::vector<glm::dvec3> accelerations(Bodies.size(), glm::dvec3(0));

    for (size_t i = 0; i < Bodies.size(); ++i) {
        for (size_t j = 0; j < Bodies.size(); ++j) {
            if (i == j) continue;

            glm::dvec3 dir = positions[j] - positions[i];
            double sqrDist = glm::length2(dir);

            if (sqrDist > 0.0001) {
                glm::dvec3 forceDir = glm::normalize(dir);
                glm::dvec3 force = forceDir * GravitationalConstant *
                                   (Bodies[i].mass * Bodies[j].mass) / sqrDist;

                accelerations[i] += force / Bodies[i].mass;
            }
        }
    }

    return accelerations;
}

void Physics::updatePhysics() {
    const double fixedTimeStep = 1.0 / 100;
    double accumulator = 0.0;
    auto lastTime = std::chrono::high_resolution_clock::now();

    // Initialise shadow state
    std::vector<glm::dvec3> positions;
    std::vector<glm::dvec3> velocities;

    for (const auto& body : Bodies) {
        positions.push_back(body.position);
        velocities.push_back(body.velocity);
    }

    std::vector<glm::dvec3> accelerations = computeAccelerations(Bodies, positions);

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(now - lastTime).count();
        lastTime = now;

        frameTime = std::min(frameTime, 0.05); // cap huge spikes
        frameTime *= gTimeScale.load(std::memory_order_relaxed);
        accumulator += frameTime;

        while (accumulator >= fixedTimeStep) {
            // Kick: update velocity by half-step
            for (size_t i = 0; i < Bodies.size(); ++i)
                velocities[i] += accelerations[i] * (fixedTimeStep * 0.5);

            // Drift: update position
            for (size_t i = 0; i < Bodies.size(); ++i)
                positions[i] += velocities[i] * fixedTimeStep;

            // Recompute accelerations at new positions
            std::vector<glm::dvec3> newAccelerations = computeAccelerations(Bodies, positions);

            // Kick: complete velocity update
            for (size_t i = 0; i < Bodies.size(); ++i)
                velocities[i] += newAccelerations[i] * (fixedTimeStep * 0.5);

            // Prepare for next iteration
            accelerations = std::move(newAccelerations);
            accumulator -= fixedTimeStep;
        }

        // Push state back to Bodies (once per frame)
        for (size_t i = 0; i < Bodies.size(); ++i) {
            Bodies[i].position = positions[i];
            Bodies[i].velocity = velocities[i];
        }

        // Allow background thread to yield
        // std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Removed to allow the simulation to run at full speed, even if it causes visually laggy results
    }
}
