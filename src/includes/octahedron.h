#ifndef OCTAHEDRON_H
#define OCTAHEDRON_H

#include <iosfwd>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "material.h"
#include "shader.h"
#include "vertex.h"

class Octahedron {
public:
    static int CreateVertexLine(glm::vec3 from, glm::vec3 to, int steps, int v, std::vector<Vertex> & vertices);

    static int CreateLowerStrip(int steps, int vTop, int vBottom, int t, std::vector<unsigned int> & triangles);
    static int CreateUpperStrip(int steps, int vTop, int vBottom, int t, std::vector<unsigned int> & triangles);

    static void InitialiseShared(unsigned int subdivisions,
                                 const char *vertPath,
                                 const char *fragPath);

    static void ShutdownShared();

    Octahedron(const glm::vec3& worldPos, float radius = 1.0f) : position(worldPos), radius(radius) { }

    void draw(const glm::mat4& worldToClip,
              const glm::vec3& cameraPos,
              const Material& material,
              const glm::vec3& lightPos     = glm::vec3(0),
              const glm::vec3& lightColour  = glm::vec3(1)) const;

    void setPosition(const glm::vec3& p) { position = p; }
    void setRadius  (float r)            { radius = r; }
private:
    /* ---- shared GPU state (one copy for all octahedrons) - whilst this means one subdivision for each, only one copy needs to be made for however many planets ---- */
    static inline Shader *sShader = nullptr;
    static inline GLuint sVAO = 0;
    static inline GLuint sVBO = 0;
    static inline GLuint sEBO = 0;

    static inline unsigned int numTriangles = 0;

    glm::vec3 position;
    float radius;
};

#endif //OCTAHEDRON_H
