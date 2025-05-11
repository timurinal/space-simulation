#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct Material {
    glm::vec3 diffuse = glm::vec3(1);
    bool emissive = false;
    glm::vec4 emission = glm::vec4(0);

    int albedoTexture = -1;
};

#endif //MATERIAL_H
