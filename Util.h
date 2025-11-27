#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Shader helpers
unsigned int createShader(const char* vsSource, const char* fsSource);

// Texture helpers (kao na vezbama)
unsigned int loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);
