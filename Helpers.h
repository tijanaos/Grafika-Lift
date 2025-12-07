#pragma once

#include <string>

// Helper: shutdown with message
int endProgram(std::string message);

// Helper: orthographic matrix (column-major)
void makeOrtho(float left, float right,
    float bottom, float top,
    float zNear, float zFar,
    float* outMat);

