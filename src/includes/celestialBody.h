#ifndef CELESTIALBODY_H
#define CELESTIALBODY_H

#include <bits/unique_ptr.h>
#include <glm/glm.hpp>

#include "octahedron.h"

struct CelestialBody {
    double mass;
    double radius;
    double surfaceGravity;

    glm::vec3 position;
    glm::vec3 velocity;

    const unsigned int instanceId;
    static unsigned int nextId;

    Material material;
    std::unique_ptr<Octahedron> gfx;

    CelestialBody(double mass, double radius, double surfaceGravity, glm::vec3 position, glm::vec3 velocity, Material material)
        : mass(mass),
          radius(radius),
          surfaceGravity(surfaceGravity),
          position(position),
          velocity(velocity),
          instanceId(nextId++),
          gfx(std::make_unique<Octahedron>(position, radius)) {
        this->material = material;
    }

    CelestialBody(const CelestialBody &other) = delete; // Disable copying
    CelestialBody &operator=(const CelestialBody &other) = delete;

    CelestialBody(CelestialBody &&other) noexcept = default;

    CelestialBody &operator=(CelestialBody &&other) noexcept = default;

    ~CelestialBody() = default;

    void draw(const glm::mat4 &worldToClip,
              const glm::vec3 &cameraPos,
              const glm::vec3 &lightPosWS,
              const glm::vec3 &lightColour) {
        gfx->setPosition(position);
        gfx->draw(worldToClip, cameraPos, material, lightPosWS, lightColour);
    }
};

#endif //CELESTIALBODY_H
