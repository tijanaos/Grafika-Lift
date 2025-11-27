#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "Shader.h"
#include "Util.h"

// Global deltaTime (seconds)
float deltaTime = 0.0f;

// Helper: nice shutdown with message
int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

// Helper: orthographic matrix (column-major)
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

struct Vertex {
    float x, y;  // position in pixels
    float u, v;  // texture coordinates
};

int main()
{
    // GLFW init
    if (!glfwInit()) {
        return endProgram("GLFW nije uspeo da se inicijalizuje.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Fullscreen monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        return endProgram("Nisam uspeo da dobijem primarni monitor.");
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        return endProgram("Nisam uspeo da dobijem video mode.");
    }

    int screenWidth = mode->width;
    int screenHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(
        screenWidth,
        screenHeight,
        "Lift projekat",
        monitor,
        nullptr
    );
    if (window == NULL) {
        return endProgram("Prozor nije uspeo da se kreira.");
    }

    glfwMakeContextCurrent(window);

    // GLEW init
    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW nije uspeo da se inicijalizuje.");
    }

    // Viewport
    glViewport(0, 0, screenWidth, screenHeight);

    // Background color
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader
    Shader shader("basic.vert", "basic.frag");

    // Ortho 0..width, 0..height
    float projection[16];
    makeOrtho(0.0f, (float)screenWidth,
        0.0f, (float)screenHeight,
        -1.0f, 1.0f,
        projection);

    shader.use();
    shader.setMat4("uProjection", projection);
    shader.setInt("uTexture", 0); // texture unit 0

    // ------------------------
    // Geometry: left and right halves
    // ------------------------
    Vertex vertices[8];

    float midX = screenWidth / 2.0f;

    // Left half
    vertices[0] = { 0.0f,               0.0f,                0.0f, 0.0f }; // bottom-left
    vertices[1] = { midX,               0.0f,                1.0f, 0.0f }; // bottom-right
    vertices[2] = { midX,        (float)screenHeight,        1.0f, 1.0f }; // top-right
    vertices[3] = { 0.0f,        (float)screenHeight,        0.0f, 1.0f }; // top-left

    // Right half
    vertices[4] = { midX,               0.0f,                0.0f, 0.0f }; // bottom-left
    vertices[5] = { (float)screenWidth, 0.0f,                1.0f, 0.0f }; // bottom-right
    vertices[6] = { (float)screenWidth, (float)screenHeight, 1.0f, 1.0f }; // top-right
    vertices[7] = { midX,        (float)screenHeight,        0.0f, 1.0f }; // top-left

    unsigned int indices[12] = {
        // Left
        0, 1, 2,
        2, 3, 0,
        // Right
        4, 5, 6,
        6, 7, 4
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position (location = 0)
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // tex coords (location = 1)
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // ------------------------
    // Overlay quad (top-left corner) for signature
    // ------------------------
    Vertex overlayVertices[4];
    unsigned int overlayIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float margin = 20.0f;
    float overlayWidth = 200.0f;  // prilagodi dimenzijama PNG-a
    float overlayHeight = 80.0f;   // prilagodi dimenzijama PNG-a

    float x0 = margin;
    float y1 = screenHeight - margin;
    float y0 = y1 - overlayHeight;
    float x1 = x0 + overlayWidth;

    overlayVertices[0] = { x0, y0, 0.0f, 0.0f }; // bottom-left
    overlayVertices[1] = { x1, y0, 1.0f, 0.0f }; // bottom-right
    overlayVertices[2] = { x1, y1, 1.0f, 1.0f }; // top-right
    overlayVertices[3] = { x0, y1, 0.0f, 1.0f }; // top-left

    unsigned int overlayVAO, overlayVBO, overlayEBO;
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    glGenBuffers(1, &overlayEBO);

    glBindVertexArray(overlayVAO);

    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, overlayEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(overlayIndices), overlayIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Load signature texture (adjust path if needed)
    unsigned int overlayTexture = loadImageToTexture("textures/ime.png");

    // FPS limiter
    const double TARGET_FPS = 75.0;
    const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        double frameStartTime = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        // left half – panel
        glBindVertexArray(VAO);
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.25f, 0.25f, 0.30f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0));

        // right half – building
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.1f, 0.15f, 0.35f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));

        // overlay – signature with alpha
        if (overlayTexture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, overlayTexture);

            shader.setInt("uUseTexture", 1);
            shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);

            glBindVertexArray(overlayVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - frameStartTime;

        if (frameDuration < TARGET_FRAME_TIME) {
            double sleepTime = TARGET_FRAME_TIME - frameDuration;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
            frameEndTime = glfwGetTime();
            frameDuration = frameEndTime - frameStartTime;
        }

        deltaTime = static_cast<float>(frameDuration);
    }

    glfwTerminate();
    return 0;
}
