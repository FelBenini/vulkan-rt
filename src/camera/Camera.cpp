#include "camera/Camera.hpp"
#include <cmath>

Camera::Camera(float fovDegrees, float aspectRatio)
    : m_fovDegrees(fovDegrees)
    , m_aspectRatio(aspectRatio)
{
    update();
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
    m_dirty = true;
}

void Camera::setTarget(const glm::vec3& target) {
    m_forward = glm::normalize(target - m_position);
    m_dirty = true;
}

void Camera::setUp(const glm::vec3& up) {
    m_worldUp = up;
    m_dirty = true;
}

void Camera::setFOV(float degrees) {
    m_fovDegrees = degrees;
    m_dirty = true;
}

void Camera::setAspectRatio(float aspect) {
    m_aspectRatio = aspect;
    m_dirty = true;
}

void Camera::rotate(float yawDelta, float pitchDelta) {
    float maxPitch = glm::radians(89.0f);
    
    glm::vec3 forward = glm::normalize(m_orientation * glm::vec3(0, 0, -1));
    float currentPitch = std::asin(forward.y);
    
    if (currentPitch > maxPitch && pitchDelta > 0) return;
    if (currentPitch < -maxPitch && pitchDelta < 0) return;

    glm::quat yawQuat = glm::angleAxis(yawDelta, m_worldUp);

    glm::vec3 right = glm::normalize(m_orientation * glm::vec3(1, 0, 0));
    glm::quat pitchQuat = glm::angleAxis(pitchDelta, right);

    m_orientation = glm::normalize(yawQuat * pitchQuat * m_orientation);

    m_dirty = true;
}

void Camera::moveForward(float distance) {
    m_position += m_forward * distance;
    m_dirty = true;
}

void Camera::moveRight(float distance) {
    m_position += m_right * distance;
    m_dirty = true;
}

void Camera::moveUp(float distance) {
    m_position += m_worldUp * distance;
    m_dirty = true;
}

const glm::mat4& Camera::getViewMatrix() const {
    const_cast<Camera*>(this)->update();
    return m_viewMatrix;
}

const glm::mat4& Camera::getInverseViewMatrix() const {
    const_cast<Camera*>(this)->update();
    return m_inverseViewMatrix;
}

glm::vec3 Camera::getPosition() const {
    return m_position;
}

glm::vec3 Camera::getTarget() const {
    return m_position + m_forward;
}

glm::vec3 Camera::getForward() const {
    return m_forward;
}

glm::vec3 Camera::getRight() const {
    return m_right;
}

glm::vec3 Camera::getUp() const {
    return m_up;
}

float Camera::getFOV() const {
    return m_fovDegrees;
}

float Camera::getAspectRatio() const {
    return m_aspectRatio;
}

Camera::Ray Camera::generateRay(float pixelX, float pixelY) const {
    float fovRadians = m_fovDegrees * glm::pi<float>() / 180.0f;
    float tanHalfFov = std::tan(fovRadians * 0.5f);
    
    float normalizedX = (2.0f * pixelX - 1.0f) * m_aspectRatio * tanHalfFov;
    float normalizedY = (1.0f - 2.0f * pixelY) * tanHalfFov;
    
    glm::vec3 direction = glm::normalize(
        m_forward +
        m_right * normalizedX +
        m_up * normalizedY
    );
    
    return {m_position, direction};
}

void Camera::update() {
    if (!m_dirty) return;
    
    m_forward = glm::normalize(m_orientation * glm::vec3(0, 0, -1));
    m_right   = glm::normalize(m_orientation * glm::vec3(1, 0, 0));
    m_up      = glm::normalize(m_orientation * glm::vec3(0, 1, 0));

    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
    m_inverseViewMatrix = glm::inverse(m_viewMatrix);

    m_dirty = false;
}