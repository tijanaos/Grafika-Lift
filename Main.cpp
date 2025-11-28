#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>


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

const int FLOOR_COUNT = 8; // SU, PR, 1, 2, 3, 4, 5, 6

// radi čitljivosti – da znamo koji indeks kojem spratu odgovara
enum FloorIndex {
    FLOOR_SU = 0,
    FLOOR_PR = 1,
    FLOOR_1 = 2,
    FLOOR_2 = 3,
    FLOOR_3 = 4,
    FLOOR_4 = 5,
    FLOOR_5 = 6,
    FLOOR_6 = 7
};

struct Floor {
    float yBottom; // donja ivica “platforme” sprata
    float yTop;    // gornja ivica platforme
};

struct Elevator {
    float x;      // donji levi ugao kabine
    float y;      // donji levi ugao kabine
    float width;
    float height;
    int   currentFloor; // indeks u [0..7]
};

struct Person {
    float x;      // donji levi ugao “čoveka”
    float y;
    float width;
    float height;
    bool  inElevator;
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

    // =============================
    // Etapa 5 – spratovi, lift i osoba
    // =============================

    // 1) Spratovi: SU, PR, 1–6
    Floor floors[FLOOR_COUNT];

    // vertikalni opseg zgrade na desnoj polovini
    float buildingBottomY = 80.0f;
    float buildingTopY = (float)screenHeight - 80.0f;
    float floorThickness = 8.0f;

    float floorSpacing = (buildingTopY - buildingBottomY) / (FLOOR_COUNT - 1);

    for (int i = 0; i < FLOOR_COUNT; ++i) {
        float centerY = buildingBottomY + floorSpacing * i;
        floors[i].yBottom = centerY - floorThickness * 0.5f;
        floors[i].yTop = centerY + floorThickness * 0.5f;
    }

    // 2) Lift – kabina uz desnu ivicu ekrana, na pocetku na 1. spratu (FLOOR_1)
    Elevator elevator{};
    float shaftMarginRight = 40.0f;   // mali odmak od same ivice ekrana
    elevator.width = 60.0f;
    elevator.height = floorSpacing * 0.6f;

    float elevatorRightX = (float)screenWidth - shaftMarginRight;
    elevator.x = elevatorRightX - elevator.width;

    elevator.currentFloor = FLOOR_1;  // 0=SU,1=PR,2=1,...
    elevator.y = floors[elevator.currentFloor].yTop; // stoji "na" platformi tog sprata

    // 3) Koridor (platforme spratova) – prostiru se od sredine desne polovine do malo pre lifta
    //midX definisan gore
    float corridorLeftX = midX + 60.0f;
    float corridorRightX = elevator.x - 20.0f;

    // vertici/indeksi za svih 8 spratova
    Vertex floorVertices[FLOOR_COUNT * 4];
    unsigned int floorIndices[FLOOR_COUNT * 6];

    for (int i = 0; i < FLOOR_COUNT; ++i) {
        int vBase = i * 4;
        int iBase = i * 6;

        float y0 = floors[i].yBottom;
        float y1 = floors[i].yTop;

        floorVertices[vBase + 0] = { corridorLeftX,  y0, 0.0f, 0.0f }; // bottom-left
        floorVertices[vBase + 1] = { corridorRightX, y0, 1.0f, 0.0f }; // bottom-right
        floorVertices[vBase + 2] = { corridorRightX, y1, 1.0f, 1.0f }; // top-right
        floorVertices[vBase + 3] = { corridorLeftX,  y1, 0.0f, 1.0f }; // top-left

        floorIndices[iBase + 0] = vBase + 0;
        floorIndices[iBase + 1] = vBase + 1;
        floorIndices[iBase + 2] = vBase + 2;
        floorIndices[iBase + 3] = vBase + 2;
        floorIndices[iBase + 4] = vBase + 3;
        floorIndices[iBase + 5] = vBase + 0;
    }

    unsigned int floorsVAO, floorsVBO, floorsEBO;
    glGenVertexArrays(1, &floorsVAO);
    glGenBuffers(1, &floorsVBO);
    glGenBuffers(1, &floorsEBO);

    glBindVertexArray(floorsVAO);

    glBindBuffer(GL_ARRAY_BUFFER, floorsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

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

    // 4) Geometrija lifta – jedan pravougaonik
    Vertex elevatorVertices[4];
    unsigned int elevatorIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float eBottom = elevator.y;
    float eTop = elevator.y + elevator.height;
    float eLeft = elevator.x;
    float eRight = elevator.x + elevator.width;

    elevatorVertices[0] = { eLeft,  eBottom, 0.0f, 0.0f }; // bottom-left
    elevatorVertices[1] = { eRight, eBottom, 1.0f, 0.0f }; // bottom-right
    elevatorVertices[2] = { eRight, eTop,    1.0f, 1.0f }; // top-right
    elevatorVertices[3] = { eLeft,  eTop,    0.0f, 1.0f }; // top-left

    unsigned int elevatorVAO, elevatorVBO, elevatorEBO;
    glGenVertexArrays(1, &elevatorVAO);
    glGenBuffers(1, &elevatorVBO);
    glGenBuffers(1, &elevatorEBO);

    glBindVertexArray(elevatorVAO);

    glBindBuffer(GL_ARRAY_BUFFER, elevatorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(elevatorVertices), elevatorVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elevatorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elevatorIndices), elevatorIndices, GL_STATIC_DRAW);

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

    // 5) Osoba – mali pravougaonik na PR (prizemlju), van lifta
    Person person{};
    person.width = 30.0f;
    person.height = 60.0f;
    person.inElevator = false;

    const int PERSON_FLOOR = FLOOR_PR;
    float pBottom = floors[PERSON_FLOOR].yTop; // stoji na platformi prizemlja
    // nek bude negde levo od lifta
    person.x = corridorLeftX + 40.0f;
    person.y = pBottom;

    // BRZINA osobe (pikseli u sekundi)
    float personSpeed = 350.0f;

    // Ciljni sprat lifta (za etapu 6 samo ga čuvamo, kretanje je etapa 7)
    int targetFloor = elevator.currentFloor;
    bool hasTargetFloor = false;

    Vertex personVertices[4];
    unsigned int personIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float pLeft = person.x;
    float pRight = person.x + person.width;
    float pTop = person.y + person.height;

    personVertices[0] = { pLeft,  person.y, 0.0f, 0.0f }; // bottom-left
    personVertices[1] = { pRight, person.y, 1.0f, 0.0f }; // bottom-right
    personVertices[2] = { pRight, pTop,      1.0f, 1.0f }; // top-right
    personVertices[3] = { pLeft,  pTop,      0.0f, 1.0f }; // top-left

    unsigned int personVAO, personVBO, personEBO;
    glGenVertexArrays(1, &personVAO);
    glGenBuffers(1, &personVBO);
    glGenBuffers(1, &personEBO);

    glBindVertexArray(personVAO);

    glBindBuffer(GL_ARRAY_BUFFER, personVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(personVertices), personVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, personEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(personIndices), personIndices, GL_STATIC_DRAW);

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

        // ======================
// Etapa 6 – kretanje osobe i pozivanje lifta
// ======================

// 1) Kretanje osobe: A = levo, W = desno
        float dx = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            dx -= personSpeed * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            dx += personSpeed * deltaTime;
        }

        person.x += dx;

        // 2) Ograniči osobu na prizemlje – između leve ivice koridora i leve ivice lifta
        float corridorMinX = corridorLeftX + 10.0f;
        float corridorMaxX = elevator.x - person.width; // da ne uđe u kabinu u ovoj etapi

        if (person.x < corridorMinX) person.x = corridorMinX;
        if (person.x > corridorMaxX) person.x = corridorMaxX;

        // 3) Ažuriraj verteks podatke za osobu
        float pLeft = person.x;
        float pRight = person.x + person.width;
        float pBottomCur = person.y;
        float pTop = pBottomCur + person.height;

        personVertices[0].x = pLeft;   personVertices[0].y = pBottomCur;
        personVertices[1].x = pRight;  personVertices[1].y = pBottomCur;
        personVertices[2].x = pRight;  personVertices[2].y = pTop;
        personVertices[3].x = pLeft;   personVertices[3].y = pTop;

        glBindBuffer(GL_ARRAY_BUFFER, personVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(personVertices), personVertices);

        // 4) Da li je osoba "ispred" lifta (dodiruje levu ivicu kabine)?
        float personRight = pRight;
        float elevatorLeft = elevator.x;
        bool inFrontOfElevator = std::fabs(personRight - elevatorLeft) < 2.0f; // tolerancija ~2px

        // 5) Taster C – pozivanje lifta (edge detect: tek kad se pritisne)
        static bool cWasPressed = false;
        bool cIsPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
        bool cJustPressed = cIsPressed && !cWasPressed;
        cWasPressed = cIsPressed;

        if (cJustPressed && !person.inElevator && inFrontOfElevator) {
            if (elevator.currentFloor != PERSON_FLOOR) {
                targetFloor = PERSON_FLOOR;
                hasTargetFloor = true;
                std::cout << "Lift pozvan na sprat: " << targetFloor << std::endl;
            }
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

        // spratovi (platforme)
        glBindVertexArray(floorsVAO);
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.8f, 0.8f, 0.85f, 1.0f); // svetlija siva
        glDrawElements(GL_TRIANGLES, FLOOR_COUNT * 6, GL_UNSIGNED_INT, (void*)0);

        // lift kabina
        glBindVertexArray(elevatorVAO);
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.9f, 0.9f, 0.2f, 1.0f); // žućkasta kabina
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        // osoba na prizemlju
        glBindVertexArray(personVAO);
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.9f, 0.4f, 0.4f, 1.0f); // crvenkasti “čovek”
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);


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
