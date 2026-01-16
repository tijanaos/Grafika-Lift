#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position;

    float Yaw;   // levo/desno
    float Pitch; // gore/dole

    float MovementSpeed;
    float MouseSensitivity;

    Camera(glm::vec3 position = glm::vec3(0.0f, 1.7f, 5.0f));

    glm::mat4 GetViewMatrix() const;

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Kretanje je po XZ (planarno), kao u specifikaciji
    void MoveForward(float dt);
    void MoveBackward(float dt);
    void MoveRight(float dt);
    void MoveLeft(float dt);

    glm::vec3 Front() const { return front; }

private:
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    void updateCameraVectors();
};
