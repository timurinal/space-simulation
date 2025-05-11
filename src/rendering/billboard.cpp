#include "billboard.h"
#include <glad/glad.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

void Billboard::InitialiseShared(const char* vertPath,
                                 const char* fragPath)
{
    if (sShader) return;               // already initialised

    /* ---------- shader ---------- */
    sShader = new Shader(vertPath, fragPath);

    /* ---------- quad geometry (two‑tri strip) ---------- */
    const float vertices[] = {
        // pos.xy    tex.xy
        -0.5f,-0.5f, 0.0f,0.0f,
        -0.5f, 0.5f, 0.0f,1.0f,
         0.5f,-0.5f, 1.0f,0.0f,
         0.5f, 0.5f, 1.0f,1.0f
    };
    const unsigned int indices[] = { 0,1,2, 2,1,3 };

    glGenVertexArrays(1,&sVAO);
    glGenBuffers(1,&sVBO);
    glGenBuffers(1,&sEBO);

    glBindVertexArray(sVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);          // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); // uv
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Billboard::ShutdownShared()
{
    if(!sShader) return;
    glDeleteVertexArrays(1,&sVAO);
    glDeleteBuffers(1,&sVBO);
    glDeleteBuffers(1,&sEBO);
    delete sShader;
    sShader = nullptr;
}

Billboard::Billboard(const glm::vec3& worldPos, float radius)
    : mPosWS(worldPos), mRadius(radius)
{}

void Billboard::draw(const glm::mat4& view,
                     const glm::mat4& proj,
                     const glm::vec3& cameraPos,
                     const glm::vec3& lightPosWS,
                     const glm::vec3& lightColour) const
{
    sShader->bind();

    /* standard camera uniforms */
    sShader->setMat4 ("view", view);
    sShader->setMat4 ("proj", proj);

    /* per‑billboard data */
    sShader->setVec3 ("spherePosWS", mPosWS);
    sShader->setFloat("radius",      mRadius);

    /* face the camera */
    glm::vec3 viewDir = glm::normalize(cameraPos - mPosWS);
    const glm::vec3 worldUp(0,1,0);

    glm::vec3 bbRight = glm::cross(worldUp, viewDir);
    if (glm::length2(bbRight) < 1e-4f) bbRight = glm::vec3(1,0,0);
    bbRight = glm::normalize(bbRight);
    glm::vec3 bbUp = glm::cross(viewDir, bbRight);

    sShader->setVec3("bbRight",   bbRight);
    sShader->setVec3("bbUp",      bbUp);
    sShader->setVec3("bbForward", viewDir);

    /* lighting */
    sShader->setVec3("lightPosWS",  lightPosWS);
    sShader->setVec3("lightColour", lightColour);

    /* kick the GPU */
    glBindVertexArray(sVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
