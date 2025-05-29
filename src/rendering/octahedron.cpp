#include "octahedron.h"

#include "vertex.h"

#include <glm/ext/scalar_constants.hpp>

#define VEC3_UP glm::vec3(0,1,0)
#define VEC3_RIGHT glm::vec3(1,0,0)
#define VEC3_FORWARD glm::vec3(0,0,1)
#define VEC3_DOWN glm::vec3(0,-1,0)
#define VEC3_LEFT glm::vec3(-1,0,0)
#define VEC3_BACK glm::vec3(0,0,-1)

int Octahedron::CreateVertexLine(glm::vec3 from, glm::vec3 to, int steps, int v, std::vector<Vertex> &vertices) {
    for (int i = 1; i <= steps; i++) {
        vertices[v++] = Vertex(glm::mix(from, to, static_cast<float>(i) / steps));
    }
    return v;
}


void Octahedron::CreateLowerStrip(int steps, int vTop, int vBottom, std::vector<unsigned int> &triangles) {
    for (int i = 1; i < steps; i++) {
        triangles.push_back(vBottom);
        triangles.push_back(vTop - 1);
        triangles.push_back(vTop);

        triangles.push_back(vBottom++);
        triangles.push_back(vTop++);
        triangles.push_back(vBottom);
    }
    triangles.push_back(vBottom);
    triangles.push_back(vTop - 1);
    triangles.push_back(vTop);
}
void Octahedron::CreateUpperStrip(int steps, int vTop, int vBottom, std::vector<unsigned int> &triangles) {
    triangles.push_back(vBottom);
    triangles.push_back(vTop - 1);
    triangles.push_back(++vBottom);
    for (int i = 1; i <= steps; i++) {
        triangles.push_back(vTop - 1);
        triangles.push_back(vTop);
        triangles.push_back(vBottom);

        triangles.push_back(vBottom);
        triangles.push_back(vTop++);
        triangles.push_back(++vBottom);
    }
}

void Octahedron::InitialiseShared(unsigned int subdivisions, const char *vertPath, const char *fragPath) {
    sShader = new Shader(vertPath, fragPath);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> triangles;

    int resolution = 1 << subdivisions;
    std::size_t vertexCount   = (resolution + 1) * (resolution + 1) * 4
                          - (resolution * 2 - 1) * 3;
    std::size_t triangleCount = (1u << (subdivisions * 2 + 3)) * 3;

    numTriangles = triangleCount;

    vertices.resize(vertexCount);
    triangles.reserve(triangleCount);

    //  --------------------    Create the octahedron    --------------------  \\

    const std::array<glm::vec3, 4> DIRECTIONS = {
        VEC3_LEFT,
        VEC3_BACK,
        VEC3_RIGHT,
        VEC3_FORWARD
    };

    int v = 0, vBottom = 0;
    for (int i = 0; i < 4; i++) {
        vertices[v++] = Vertex(VEC3_DOWN);
    }

    for (int i = 1; i <= resolution; i++) {
        float progress = static_cast<float>(i) / resolution;
        glm::vec3 from, to;
        to = glm::mix(VEC3_DOWN, VEC3_FORWARD, progress);
        vertices[v++] = Vertex(to);
        for (int d = 0; d < 4; d++) {
            from = to;
            to = glm::mix(VEC3_DOWN, DIRECTIONS[d], progress);
            CreateLowerStrip(i, v, vBottom, triangles);
            v = CreateVertexLine(from, to, i, v, vertices);
            vBottom += i > 1 ? i - 1 : 1;
        }
        vBottom = v - 1 - i * 4;
    }

    for (int i = resolution - 1; i >= 1; i--) {
        float progress = static_cast<float>(i) / resolution;
        glm::vec3 from, to;
        to = glm::mix(VEC3_UP, VEC3_FORWARD, progress);
        vertices[v++] = Vertex(to);
        for (int d = 0; d < 4; d++) {
            from = to;
            to = glm::mix(VEC3_UP, DIRECTIONS[d], progress);
            CreateUpperStrip(i, v, vBottom, triangles);
            v = CreateVertexLine(from, to, i, v, vertices);
            vBottom += i + 1;
        }
        vBottom = v - 1 - i * 4;
    }

    for (int i = 0; i < 4; i++) {
        triangles.push_back(vBottom);
        triangles.push_back(v);
        triangles.push_back(++vBottom);
        vertices[v++].position = VEC3_UP;
    }

    //  --------------------       Post processing       --------------------  \\

    // Calculate normals and project vertices onto the sphere
    for (auto &vertex: vertices) {
        vertex.normal = vertex.position = glm::normalize(vertex.position);
    }
    // Calculate uvs
    int uvI = 0;
    float previousX = 1.0f;
    for (auto &vertex: vertices) {
        glm::vec3 v = vertex.position;
        if (v.x == previousX) vertices[uvI - 1].uv.x = 1.0f;
        previousX = v.x;
        glm::vec2 uv;
        uv.x = atan2(v.x, v.z) / (-2 * glm::pi<float>());
        if (uv.x < 0) uv.x += 1;
        uv.y = asin(v.y) / glm::pi<float>() + 0.5f;
        vertex.uv = uv;
        uvI++;
    }
    // Adjust horizontal coordinates of polar vertices
    vertices[vertices.size() - 4].uv.x = vertices[0].uv.x = 0.125f;
    vertices[vertices.size() - 3].uv.x = vertices[1].uv.x = 0.375f;
    vertices[vertices.size() - 2].uv.x = vertices[2].uv.x = 0.625f;
    vertices[vertices.size() - 1].uv.x = vertices[3].uv.x = 0.875f;

    glGenVertexArrays(1, &sVAO);
    glGenBuffers(1, &sVBO);
    glGenBuffers(1, &sEBO);

    glBindVertexArray(sVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), triangles.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE, 8 * sizeof(float), (void *) 0); // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3,GL_FLOAT,GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float))); // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2,GL_FLOAT,GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float))); // uv
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Octahedron::ShutdownShared() {
    if(!sShader) return;
    glDeleteVertexArrays(1,&sVAO);
    glDeleteBuffers(1,&sVBO);
    glDeleteBuffers(1,&sEBO);
    delete sShader;
    sShader = nullptr;
}

void Octahedron::draw(const glm::mat4 &worldToClip, const glm::vec3 &cameraPos, const Material& material,
                      const glm::vec3 &lightPos, const glm::vec3 &lightColour) const {
    sShader->bind();
    sShader->setMat4("worldToClip", worldToClip);
    sShader->setVec3("cameraPos", cameraPos);
    sShader->setVec3("lightPos", lightPos);
    sShader->setVec3("lightColour", lightColour);

    sShader->setVec3 ("spherePos", position);
    sShader->setFloat("radius",      radius);

    sShader->setVec3("material.diffuse", material.diffuse);
    sShader->setInt("material.emissive", material.emissive);
    sShader->setVec4("material.emission", material.emission);

    sShader->setInt("material.hasTexture", material.albedoTexture <= 0 ? 0 : 1);
    sShader->setInt("material.albedoTex", 0);

    if (material.albedoTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.albedoTexture);
    }

    glBindVertexArray(sVAO);
    glDrawElements(GL_TRIANGLES, numTriangles, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
