#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "camera.h"
#include "shader.h"

struct AtmosphereSettings {
    float atmosphereRadius;
    float planetRadius;
    int numInScatteringPoints = 10;
    int numOpticalDepthPoints = 10;
    float densityFalloff = 0.0f;
    glm::vec3 wavelengths = { 700, 530, 440 };
    float scatteringStrength = 1;
};

class Atmosphere {
public:
    static void Initialise(Shader* shader) {
        sShader = shader;
    }

    Atmosphere() {
        glGenVertexArrays(1, &sVAO);
    }

    void render(AtmosphereSettings settings, Camera *camera, glm::vec3 atmospherePosition, glm::vec3 sunPosition) {
        sShader->bind();
        sShader->setMat4("inv_proj", camera->getInvProjectionMatrix());
        sShader->setMat4("inv_view", camera->getInvViewMatrix());
        sShader->setVec3("cameraPos", camera->Position);
        sShader->setInt("MainTex", 0);
        sShader->setInt("DepthTex", 1);

        sShader->setVec3("sunPosition", sunPosition);

        sShader->setVec3("atmospherePosition", atmospherePosition);

        sShader->setFloat("atmosphereRadius", settings.atmosphereRadius);
        sShader->setFloat("planetRadius", settings.planetRadius);
        sShader->setInt("numInScatteringPoints", settings.numInScatteringPoints);
        sShader->setInt("numOpticalDepthPoints", settings.numOpticalDepthPoints);
        sShader->setFloat("densityFalloff", settings.densityFalloff);

        float scatterR = pow(400 / settings.wavelengths.r, 4) * settings.scatteringStrength;
        float scatterG = pow(400 / settings.wavelengths.g, 4) * settings.scatteringStrength;
        float scatterB = pow(400 / settings.wavelengths.b, 4) * settings.scatteringStrength;
        sShader->setVec3("scatteringCoefficients", glm::vec3(scatterR, scatterG, scatterB));

        glDisable(GL_CULL_FACE);
        glBindVertexArray(sVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }
private:
    static inline Shader* sShader = nullptr;
    GLuint sVAO    = 0;
};

#endif //ATMOSPHERE_H
