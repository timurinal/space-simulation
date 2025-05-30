#ifndef CELESTIALBODY_H
#define CELESTIALBODY_H

#include <bits/unique_ptr.h>
#include <glm/glm.hpp>
#include <utility>

#include "octahedron.h"
#include "maths.h"

struct CelestialBody {
    std::string name;

    double mass;
    double radius;
    const double surfaceGravity;

    glm::dvec3 position;
    glm::dvec3 velocity;

    const unsigned int instanceId;
    static unsigned int nextId;

    Material material;
    std::unique_ptr<Octahedron> gfx;

    CelestialBody(std::string name, double mass, double radius, glm::dvec3 position,
                  glm::dvec3 velocity, Material material)
        : name(std::move(name)),
          mass(mass),
          radius(radius),
          surfaceGravity(deriveSurfaceGravity(mass, radius)),
          position(position),
          velocity(velocity),
          instanceId(nextId++),
          gfx(std::make_unique<Octahedron>(position, kmToSu(radius))) {
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
              const glm::vec3 &lightColour,
              const glm::dvec3 &relativePosition) {
        glm::vec3 posSU    = glm::vec3((position - relativePosition) / SU_IN_KM);
        gfx->setPosition(posSU);
        gfx->draw(worldToClip, cameraPos, material, lightPosWS, lightColour);
    }
};

#endif //CELESTIALBODY_H
