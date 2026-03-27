#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
public:
    Camera(float fovDegrees = 60.0f, float aspectRatio = 16.0f / 9.0f);

    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);
    void setFOV(float degrees);
    void setAspectRatio(float aspect);

    void rotate(float yaw, float pitch);
    void moveForward(float distance);
    void moveRight(float distance);
    void moveUp(float distance);

    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getInverseViewMatrix() const;
    glm::vec3 getPosition() const;
    glm::vec3 getTarget() const;
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    float getFOV() const;
    float getAspectRatio() const;

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };
    Ray generateRay(float pixelX, float pixelY) const;

private:
    void update();

    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::quat m_orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;
    
    float m_fovDegrees = 60.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_inverseViewMatrix = glm::mat4(1.0f);
    bool m_dirty = true;
};