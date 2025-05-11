#ifndef CELESTIALBODY_H
#define CELESTIALBODY_H

#include <bits/unique_ptr.h>
#include <glm/glm.hpp>

#include "billboard.h"

struct CelestialBody {
    double mass;
    double radius;
    double surfaceGravity;

    glm::vec3 position;
    glm::vec3 velocity;

    const unsigned int instanceId;
    static unsigned int nextId;

    std::unique_ptr<Billboard> gfx;

    CelestialBody(double mass, double radius, double surfaceGravity, glm::vec3 position, glm::vec3 velocity)
        : mass(mass),
          radius(radius),
          surfaceGravity(surfaceGravity),
          position(position),
          velocity(velocity),
          instanceId(nextId++),
          gfx(std::make_unique<Billboard>(position, radius)) {
    }

    CelestialBody(const CelestialBody &other) = delete; // Disable copying
    CelestialBody &operator=(const CelestialBody &other) = delete;

    CelestialBody(CelestialBody &&other) noexcept = default;

    CelestialBody &operator=(CelestialBody &&other) noexcept = default;

    ~CelestialBody() = default;

    void draw(const glm::mat4 &view,
              const glm::mat4 &proj,
              const glm::vec3 &cameraPos,
              const glm::vec3 &lightPosWS,
              const glm::vec3 &lightColour) {
        gfx->setPosition(position);
        gfx->draw(view, proj, cameraPos, lightPosWS, lightColour);
    }
};

#endif //CELESTIALBODY_H
