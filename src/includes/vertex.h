#ifndef VERTEX_H
#define VERTEX_H
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

struct Vertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
};

#endif //VERTEX_H
