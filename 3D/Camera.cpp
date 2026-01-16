#include "Camera.h"
#include <algorithm>
#include <cmath>

static float toRad(float deg) { return deg * 3.1415926535f / 180.0f; }

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    return std::max(minVal, std::min(value, maxVal));
}

Camera::Camera(glm::vec3 position)
    : Position(position),
    Yaw(-90.0f),
    Pitch(0.0f),
    MovementSpeed(5.0f),
    MouseSensitivity(0.10f),
    worldUp(0.0f, 1.0f, 0.0f),
    front(0.0f, 0.0f, -1.0f)
{
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + front, up);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        Pitch = clamp(Pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::MoveForward(float dt) {
    glm::vec3 planarFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    Position += planarFront * MovementSpeed * dt;
}
void Camera::MoveBackward(float dt) {
    glm::vec3 planarFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    Position -= planarFront * MovementSpeed * dt;
}
void Camera::MoveRight(float dt) {
    glm::vec3 planarFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 planarRight = glm::normalize(glm::cross(planarFront, worldUp));
    Position += planarRight * MovementSpeed * dt;
}
void Camera::MoveLeft(float dt) {
    glm::vec3 planarFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 planarRight = glm::normalize(glm::cross(planarFront, worldUp));
    Position -= planarRight * MovementSpeed * dt;
}

void Camera::updateCameraVectors() {
    glm::vec3 f;
    f.x = std::cos(toRad(Yaw)) * std::cos(toRad(Pitch));
    f.y = std::sin(toRad(Pitch));
    f.z = std::sin(toRad(Yaw)) * std::cos(toRad(Pitch));

    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
