#include "Util.h"

#include <fstream>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --------- SHADER HELPERS ---------

static unsigned int compileShader(GLenum type, const char* path)
{
    // Read source from file
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Could not open shader file: " << path << std::endl;
        return 0;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string srcStr = ss.str();
    const char* sourceCode = srcStr.c_str();
    file.close();

    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &sourceCode, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Error compiling ";
        if (type == GL_VERTEX_SHADER)  std::cout << "VERTEX";
        if (type == GL_FRAGMENT_SHADER) std::cout << "FRAGMENT";
        std::cout << " shader:\n" << infoLog << std::endl;
    }

    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program = glCreateProgram();

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Error linking shader program:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// --------- TEXTURE HELPERS ---------

unsigned int loadImageToTexture(const char* filePath) // loads .png, .jpg and creates OpenGL texture
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath, &width, &height, &channels, 0);

    if (!data) {
        std::cout << "Texture not loaded! Path: " << filePath << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    switch (channels) {
    case 1: format = GL_RED;  break;
    case 2: format = GL_RG;   break;
    case 3: format = GL_RGB;  break;
    case 4: format = GL_RGBA; break;
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, format,
        width, height, 0,
        format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Basic params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return texture;
}

GLFWcursor* loadImageToCursor(const char* filePath)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filePath, &width, &height, &channels, 0);
    if (!data) {
        std::cout << "Cursor image not loaded! Path: " << filePath << std::endl;
        return nullptr;
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = data;

    int hotspotX = width / 2;
    int hotspotY = height / 2;

    GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
    stbi_image_free(data);

    if (!cursor) {
        std::cout << "Failed to create GLFW cursor from image: " << filePath << std::endl;
    }

    return cursor;
}
