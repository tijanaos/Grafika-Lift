#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "Shader.h"   // NOVO
#include "Util.h"     // Ostavljamo, iako ga sada ne koristimo

// Globalna promenljiva za deltaTime (u sekundama)
float deltaTime = 0.0f;

// Pomoćna funkcija za lepo gašenje programa sa porukom
int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

// Pomoćna funkcija: ortho matrica (column-major, kako OpenGL očekuje)
void makeOrtho(float left, float right,
    float bottom, float top,
    float zNear, float zFar,
    float* outMat) {
    // Reset na identitet
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
    float x, y;  // pozicija u pikselima
    float u, v;  // tex koordinate
};

int main()
{
    // Inicijalizacija GLFW i postavljanje na verziju 3 sa programabilnim pajplajnom
    if (!glfwInit()) {
        return endProgram("GLFW nije uspeo da se inicijalizuje.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FULLSCREEN: uzmi primarni monitor i njegov video mode
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

    // Formiranje prozora preko celog ekrana
    GLFWwindow* window = glfwCreateWindow(
        screenWidth,
        screenHeight,
        "Lift projekat",
        monitor,          // FULLSCREEN
        nullptr
    );
    if (window == NULL) {
        return endProgram("Prozor nije uspeo da se kreira.");
    }

    glfwMakeContextCurrent(window);

    // Inicijalizacija GLEW
    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW nije uspeo da se inicijalizuje.");
    }

    // Podesi viewport da pokrije ceo ekran
    glViewport(0, 0, screenWidth, screenHeight);

    // Boja pozadine
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Kreiranje šejdera
    Shader shader("basic.vert", "basic.frag");

    // Orto projekcija 0..width, 0..height
    float projection[16];
    makeOrtho(0.0f, (float)screenWidth,
        0.0f, (float)screenHeight,
        -1.0f, 1.0f,
        projection);

    shader.use();
    shader.setMat4("uProjection", projection);
    shader.setInt("uTexture", 0); // samo da bude podešen, iako još ne koristimo teksturu

    // --- Geometrija: dva pravougaonika (leva i desna polovina ekrana) ---

    Vertex vertices[8];

    // Leva polovina: od x=0 do x=screenWidth/2, po celoj visini
    float midX = screenWidth / 2.0f;

    // Leva polovina (4 temena)
    vertices[0] = { 0.0f,       0.0f,        0.0f, 0.0f }; // dole levo
    vertices[1] = { midX,       0.0f,        1.0f, 0.0f }; // dole desno
    vertices[2] = { midX,  (float)screenHeight, 1.0f, 1.0f }; // gore desno
    vertices[3] = { 0.0f,  (float)screenHeight, 0.0f, 1.0f }; // gore levo

    // Desna polovina (4 temena)
    vertices[4] = { midX,       0.0f,        0.0f, 0.0f }; // dole levo
    vertices[5] = { (float)screenWidth, 0.0f,        1.0f, 0.0f }; // dole desno
    vertices[6] = { (float)screenWidth, (float)screenHeight, 1.0f, 1.0f }; // gore desno
    vertices[7] = { midX,  (float)screenHeight, 0.0f, 1.0f }; // gore levo

    unsigned int indices[12] = {
        // Leva polovina
        0, 1, 2,
        2, 3, 0,
        // Desna polovina
        4, 5, 6,
        6, 7, 4
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Pozicija (location = 0)
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // Tex koordinate (location = 1)
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

    // FPS limiter
    const double TARGET_FPS = 75.0;
    const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS; // ~0.013333s po frejmu

    // GLAVNA PETLJA
    while (!glfwWindowShouldClose(window))
    {
        double frameStartTime = glfwGetTime();

        // ESC gasi program
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Čišćenje bafera
        glClear(GL_COLOR_BUFFER_BIT);

        // Crtanje scene
        shader.use();
        glBindVertexArray(VAO);

        // Leva polovina – panel (npr. tamno crvena)
        shader.setVec4("uColor", 0.4f, 0.1f, 0.15f, 1.0f);
        shader.setInt("uUseTexture", 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0));

        // Desna polovina – zgrada (svetlija siva/plava)
        shader.setVec4("uColor", 0.3f, 0.3f, 0.4f, 1.0f);
        shader.setInt("uUseTexture", 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));

        glBindVertexArray(0);

        // Zamena bafera i obrada događaja
        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS limiter
        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - frameStartTime;

        if (frameDuration < TARGET_FRAME_TIME) {
            double sleepTime = TARGET_FRAME_TIME - frameDuration;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));

            frameEndTime = glfwGetTime();
            frameDuration = frameEndTime - frameStartTime;
        }

        deltaTime = static_cast<float>(frameDuration);
        // Debug: možeš da proveriš FPS
        // std::cout << "FPS: " << (1.0 / frameDuration) << std::endl;
    }

    glfwTerminate();
    return 0;
}
