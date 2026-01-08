#include "Helpers.h"
#include <GLFW/glfw3.h>
#include <iostream>

// Helper: shutdown with message
int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

// Helper: orthographic matrix (column-major) used in vertex shaders
// Converts coordinates from world space to normalized OPENGL coordinates
void makeOrtho(float left, float right,
    float bottom, float top,
    float zNear, float zFar,
    float* outMat) {
    for (int i = 0; i < 16; ++i) outMat[i] = 0.0f;
    outMat[0] = 2.0f / (right - left);
    outMat[5] = 2.0f / (top - bottom);
    outMat[10] = -2.0f / (zFar - zNear);
    outMat[15] = 1.0f;

    outMat[12] = -(right + left) / (right - left);
    outMat[13] = -(top + bottom) / (top - bottom);
    outMat[14] = -(zFar + zNear) / (zFar - zNear);
}

