#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
    explicit Camera(float aspect)
    {
        this->aspect = aspect;

        WorldUp = glm::vec3(0, 1, 0);
        
        updateProjection();
    }

    glm::vec3 Position;
    float Yaw, Pitch;

    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    [[nodiscard]] float getAspect() const { return aspect; }
    void setAspect(const float aspect) {
        this->aspect = aspect;
        updateProjection();
    }

    [[nodiscard]] glm::mat4 worldToClip() const { return matrix_projectionView; }

    [[nodiscard]] glm::mat4 getProjectionMatrix() const { return projection; }
    [[nodiscard]] glm::mat4 getViewMatrix() const { return view; }
    [[nodiscard]] glm::mat4 getInvProjectionMatrix() const { return inv_projection; }
    [[nodiscard]] glm::mat4 getInvViewMatrix() const { return inv_view; }

    void update()
    {
        updateVectors();

        view = glm::lookAt(Position, Position + Front, Up);
        inv_view = glm::inverse(view);
        
        matrix_projectionView = projection * view;
    }

    void move(const glm::vec3& offset) { Position += offset; }
    void setPosition(const glm::vec3& pos) { Position = pos; }

    void rotate(const float yaw, const float pitch) {
        Yaw += yaw;
        Pitch += pitch;
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
    }
    void setRotation(const float yaw, const float pitch) { Yaw = yaw; Pitch = pitch; }
    
private:
    glm::mat4 projection;
    glm::mat4 view;

    glm::mat4 inv_projection;
    glm::mat4 inv_view;

    glm::mat4 matrix_projectionView;
    
    glm::vec3 WorldUp;

    float aspect;

    void updateProjection()
    {
        projection = glm::perspective(glm::radians(65.0f), aspect, 0.1f, 2500.0f);
        inv_projection = glm::inverse(projection);
    }

    void updateVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#endif //CAMERA_H