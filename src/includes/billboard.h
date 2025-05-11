#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <glm/glm.hpp>
#include "shader.h"

/*  A light‑weight, RAII billboard that always faces the camera.
 *  Geometry and shader are shared by every instance; only
 *  position and radius are per‑object.
 */
class Billboard
{
public:
    // call once at start‑up (after an OpenGL context exists)
    static void InitialiseShared(const char* vertPath,
                                 const char* fragPath);

    // optional tidy‑up, e.g. before glfwTerminate()
    static void ShutdownShared();

    // create an individual billboard
    Billboard(const glm::vec3& worldPos, float radius = 1.0f);
    ~Billboard() = default;                                    // nothing to free

    // draw this billboard
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& cameraPos,
              const glm::vec3& lightPosWS   = glm::vec3(0),
              const glm::vec3& lightColour  = glm::vec3(1)) const;

    // helpers
    void setPosition(const glm::vec3& p) { mPosWS = p; }
    void setRadius  (float r)            { mRadius = r; }

private:
    glm::vec3 mPosWS;
    float     mRadius;

    /* ---- shared GPU state (one copy for all billboards) ---- */
    static inline Shader*   sShader = nullptr;
    static inline GLuint    sVAO    = 0;
    static inline GLuint    sVBO    = 0;
    static inline GLuint    sEBO    = 0;
};

#endif //BILLBOARD_H
