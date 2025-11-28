#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>

#include "Shader.h"
#include "Util.h"

// Global deltaTime (seconds)
float deltaTime = 0.0f;
const float DOOR_ANIM_DURATION = 0.3f;   // vreme animacije otvaranja/zatvaranja
const float BASE_DOOR_OPEN_TIME = 5.0f;  // osnovno vreme dok su vrata otvorena


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

enum class ElevatorState {
    Idle,
    Moving,
    DoorsOpening,
    DoorsOpen,
    DoorsClosing,
    Stopped
};

struct Elevator {
    float x;      // donji levi ugao kabine
    float y;      // donji levi ugao kabine
    float width;
    float height;
    int   currentFloor;   // indeks u [0..7]

    ElevatorState state;  // trenutno stanje
    float speed;          // brzina kretanja (pikseli/s)
    float doorOpenTimer;  // preostalo vreme dok su vrata otvorena
    float doorOpenRatio;  // 0 = zatvorena, 1 = potpuno otvorena
};


struct Person {
    float x;      // donji levi ugao “čoveka”
    float y;
    float width;
    float height;
    bool  inElevator;
};

enum class ButtonType {
    Floor,
    OpenDoor,
    CloseDoor,
    Stop,
    Ventilation
};

struct Button {
    float x0, y0;
    float x1, y1;
    ButtonType type;
    int floorIndex;  // za sprat dugme, inače -1
    bool pressed;
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

    // dodatna inicijalizacija stanja lifta
    elevator.state = ElevatorState::Idle;
    elevator.speed = 260.0f;         // pikseli u sekundi (podesi po želji)
    elevator.doorOpenTimer = 0.0f;
    elevator.doorOpenRatio = 0.0f;   // 0 = vrata potpuno zatvorena


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

    // 4b) Vrata lifta – pravougaonik koji se animira
    Vertex doorVertices[4];
    unsigned int doorIndices[6] = { 0, 1, 2, 2, 3, 0 };

    unsigned int doorVAO, doorVBO, doorEBO;
    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);
    glGenBuffers(1, &doorEBO);

    glBindVertexArray(doorVAO);

    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    // podatke ćemo slati dinamički u glavnoj petlji
    glBufferData(GL_ARRAY_BUFFER, sizeof(doorVertices), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, doorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(doorIndices), doorIndices, GL_STATIC_DRAW);

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

    int personFloorIndex = FLOOR_PR;                 // osoba je na pocetku u prizemlju
    float pBottom = floors[personFloorIndex].yTop;   // stoji na platformi tog sprata
    // nek bude negde levo od lifta
    person.x = corridorLeftX + 40.0f;
    person.y = pBottom;

    // BRZINA osobe (pikseli u sekundi)
    float personSpeed = 350.0f;

    // Ciljni sprat lifta – aktivni sprat ka kom se lift trenutno kreće
    int targetFloor = elevator.currentFloor;
    bool hasTargetFloor = false;

    // Red čekanja spratova koje treba obići (za panel + pozivanje C)
    std::vector<int> floorQueue;

    // Pomoćna funkcija: da li je sprat već tražen (aktuelni cilj ili u redu)
    auto isFloorAlreadyRequested = [&](int floorIdx) {
        if (hasTargetFloor && targetFloor == floorIdx) return true;
        for (int f : floorQueue) {
            if (f == floorIdx) return true;
        }
        return false;
        };

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

    // =========================
    // Panel dugmići (spratovi, vrata, STOP, ventilacija)
    // =========================
    std::vector<Button> buttons;
    buttons.reserve(12);

    int floorButtonIndex[FLOOR_COUNT];
    int openButtonIndex = -1;
    int closeButtonIndex = -1;
    int stopButtonIndex = -1;
    int ventilationButtonIndex = -1;

    // =========================
    // Tekst oznake spratova (SU, PR, 1-6) preko tekstura
    // =========================
    unsigned int floorLabelTextures[FLOOR_COUNT];
    floorLabelTextures[FLOOR_SU] = loadImageToTexture("textures/floor_SU.png");
    floorLabelTextures[FLOOR_PR] = loadImageToTexture("textures/floor_PR.png");
    floorLabelTextures[FLOOR_1] = loadImageToTexture("textures/floor1.png");
    floorLabelTextures[FLOOR_2] = loadImageToTexture("textures/floor2.png");
    floorLabelTextures[FLOOR_3] = loadImageToTexture("textures/floor3.png");
    floorLabelTextures[FLOOR_4] = loadImageToTexture("textures/floor4.png");
    floorLabelTextures[FLOOR_5] = loadImageToTexture("textures/floor5.png");
    floorLabelTextures[FLOOR_6] = loadImageToTexture("textures/floor6.png");
    // Teksture za specijalna dugmad (OPEN, CLOSE, STOP, VENT)
    unsigned int openBtnTex = loadImageToTexture("textures/open.png");
    unsigned int closeBtnTex = loadImageToTexture("textures/close.png");
    unsigned int stopBtnTex = loadImageToTexture("textures/stop.png");
    unsigned int ventBtnTex = loadImageToTexture("textures/fan.png");

    Vertex labelVertices[4];
    unsigned int labelIndices[6] = { 0, 1, 2, 2, 3, 0 };
    unsigned int labelVAO, labelVBO, labelEBO;

    glGenVertexArrays(1, &labelVAO);
    glGenBuffers(1, &labelVBO);
    glGenBuffers(1, &labelEBO);

    glBindVertexArray(labelVAO);

    glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(labelVertices), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, labelEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(labelIndices), labelIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);


    bool ventilationOn = false;
    bool doorExtendedThisCycle = false; // da OPEN produžava vreme samo jednom po ciklusu

    // Jedan VAO/VBO za sva dugmad – punićemo ga za svako posebno
    Vertex buttonVertices[4];
    unsigned int buttonIndices[6] = { 0, 1, 2, 2, 3, 0 };
    unsigned int buttonVAO, buttonVBO, buttonEBO;

    glGenVertexArrays(1, &buttonVAO);
    glGenBuffers(1, &buttonVBO);
    glGenBuffers(1, &buttonEBO);

    glBindVertexArray(buttonVAO);

    glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buttonEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buttonIndices), buttonIndices, GL_STATIC_DRAW);

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

    // Layout dugmadi na levoj polovini
    float panelCenterX = midX / 2.0f;
    float btnWidth = 160.0f;
    float btnHeight = 80.0f;
    float rowSpacing = btnHeight + 15.0f;
    float colOffset = 90.0f; // horizontalni razmak leve/desne kolone

    float colLeftX0 = panelCenterX - colOffset - btnWidth * 0.5f;
    float colLeftX1 = colLeftX0 + btnWidth;
    float colRightX0 = panelCenterX + colOffset - btnWidth * 0.5f;
    float colRightX1 = colRightX0 + btnWidth;

    float floorStartY = (float)screenHeight - 160.0f;

    auto addButton = [&](float x0b, float yBottom, float x1b, float yTopb,
        ButtonType type, int floorIdx) -> int {
            Button b;
            b.x0 = x0b;
            b.y0 = yBottom;
            b.x1 = x1b;
            b.y1 = yTopb;
            b.type = type;
            b.floorIndex = floorIdx;
            b.pressed = false;
            buttons.push_back(b);
            return (int)buttons.size() - 1;
        };

    // Sprat dugmići u 4 reda x 2 kolone:
    // red 0: SU (levo), PR (desno)
    float yTop0 = floorStartY;
    float yBot0 = yTop0 - btnHeight;
    floorButtonIndex[FLOOR_SU] = addButton(colLeftX0, yBot0, colLeftX1, yTop0, ButtonType::Floor, FLOOR_SU);
    floorButtonIndex[FLOOR_PR] = addButton(colRightX0, yBot0, colRightX1, yTop0, ButtonType::Floor, FLOOR_PR);

    // red 1: 1, 2
    float yTop1 = floorStartY - rowSpacing;
    float yBot1 = yTop1 - btnHeight;
    floorButtonIndex[FLOOR_1] = addButton(colLeftX0, yBot1, colLeftX1, yTop1, ButtonType::Floor, FLOOR_1);
    floorButtonIndex[FLOOR_2] = addButton(colRightX0, yBot1, colRightX1, yTop1, ButtonType::Floor, FLOOR_2);

    // red 2: 3, 4
    float yTop2 = floorStartY - 2.0f * rowSpacing;
    float yBot2 = yTop2 - btnHeight;
    floorButtonIndex[FLOOR_3] = addButton(colLeftX0, yBot2, colLeftX1, yTop2, ButtonType::Floor, FLOOR_3);
    floorButtonIndex[FLOOR_4] = addButton(colRightX0, yBot2, colRightX1, yTop2, ButtonType::Floor, FLOOR_4);

    // red 3: 5, 6
    float yTop3 = floorStartY - 3.0f * rowSpacing;
    float yBot3 = yTop3 - btnHeight;
    floorButtonIndex[FLOOR_5] = addButton(colLeftX0, yBot3, colLeftX1, yTop3, ButtonType::Floor, FLOOR_5);
    floorButtonIndex[FLOOR_6] = addButton(colRightX0, yBot3, colRightX1, yTop3, ButtonType::Floor, FLOOR_6);

    // Kontrolna dugmad (OPEN/CLOSE, STOP/VENT) ispod
    float ctlTop1 = floorStartY - 4.5f * rowSpacing;
    float ctlBot1 = ctlTop1 - btnHeight;

    openButtonIndex = addButton(colLeftX0, ctlBot1, colLeftX1, ctlTop1, ButtonType::OpenDoor, -1);
    closeButtonIndex = addButton(colRightX0, ctlBot1, colRightX1, ctlTop1, ButtonType::CloseDoor, -1);

    float ctlTop2 = ctlTop1 - rowSpacing;
    float ctlBot2 = ctlTop2 - btnHeight;

    stopButtonIndex = addButton(colLeftX0, ctlBot2, colLeftX1, ctlTop2, ButtonType::Stop, -1);
    ventilationButtonIndex = addButton(colRightX0, ctlBot2, colRightX1, ctlTop2, ButtonType::Ventilation, -1);



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
        // Etapa 6+7 – kretanje osobe, pozivanje lifta,
        //             ulazak/izlazak i osnova za kretanje lifta
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

        // 2) Osnovni opseg hodnika na spratu na kom je osoba (kada je VAN lifta)
        float corridorMinX = corridorLeftX + 10.0f;
        float corridorMaxXOutside = elevator.x - person.width; // “ispred” vrata

        if (!person.inElevator) {
            if (person.x < corridorMinX) person.x = corridorMinX;
            if (person.x > corridorMaxXOutside) person.x = corridorMaxXOutside;

            // osoba je van lifta – visina je određena spratom
            person.y = floors[personFloorIndex].yTop;
        }
        else {
            // osoba je u kabini – ograniči je na unutrašnjost kabine
            float insideMinX = elevator.x + 5.0f;
            float insideMaxX = elevator.x + elevator.width - person.width - 5.0f;
            if (person.x < insideMinX) person.x = insideMinX;
            if (person.x > insideMaxX) person.x = insideMaxX;

            // uvek stoji na “podu” lifta
            person.y = elevator.y;
        }

        // 3) Ažuriraj verteks podatke za osobu
        float pLeftCur = person.x;
        float pRightCur = person.x + person.width;
        float pBottomCur = person.y;
        float pTopCur = pBottomCur + person.height;

        personVertices[0].x = pLeftCur;   personVertices[0].y = pBottomCur;
        personVertices[1].x = pRightCur;  personVertices[1].y = pBottomCur;
        personVertices[2].x = pRightCur;  personVertices[2].y = pTopCur;
        personVertices[3].x = pLeftCur;   personVertices[3].y = pTopCur;

        glBindBuffer(GL_ARRAY_BUFFER, personVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(personVertices), personVertices);

        // 4) Da li je osoba "ispred" lifta (dodiruje levu ivicu kabine)?
        float personRight = pRightCur;
        float elevatorLeft = elevator.x;
        bool inFrontOfElevator = std::fabs(personRight - elevatorLeft) < 2.0f; // tolerancija ~2px

        bool doorsAreOpen = (elevator.state == ElevatorState::DoorsOpen);
        bool elevatorAtPersonsFloor = (elevator.currentFloor == personFloorIndex);

        // 5) Taster C – pozivanje lifta (edge detect: tek kad se pritisne)
        static bool cWasPressed = false;
        bool cIsPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
        bool cJustPressed = cIsPressed && !cWasPressed;
        cWasPressed = cIsPressed;

        if (cJustPressed && !person.inElevator && inFrontOfElevator) {

            // 1) Lift je već na mom spratu → samo upravljamo vratima
            if (elevator.currentFloor == personFloorIndex) {

                // Ako su vrata zatvorena / se zatvaraju → ponovo ih otvori
                if (elevator.state == ElevatorState::Idle ||
                    elevator.state == ElevatorState::DoorsClosing) {

                    elevator.state = ElevatorState::DoorsOpening;
                    elevator.doorOpenRatio = 0.0f;
                    elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
                    doorExtendedThisCycle = false;
                }
                // Ako su već otvorena – resetuj timer (da ostanu još malo)
                else if (elevator.state == ElevatorState::DoorsOpen) {
                    elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
                }
            }
            // 2) Lift NIJE na mom spratu → standardno ubaci zahtev
            else if (!isFloorAlreadyRequested(personFloorIndex)) {

                if (!hasTargetFloor && elevator.state == ElevatorState::Idle) {
                    // lift miruje – kreni odmah ka tom spratu
                    targetFloor = personFloorIndex;
                    hasTargetFloor = true;
                }
                else {
                    // već se vozi – ubaci sprat u red
                    floorQueue.push_back(personFloorIndex);
                }

                std::cout << "Lift pozvan na sprat: " << personFloorIndex << std::endl;
            }
        }



        // 6) Ako su vrata otvorena i lift je na spratu osobe, dozvoli ulazak
        if (!person.inElevator && doorsAreOpen && elevatorAtPersonsFloor && inFrontOfElevator) {
            person.inElevator = true;
            // pozicioniramo osobu unutar kabine
            person.x = elevator.x + 10.0f;
            person.y = elevator.y;
        }

        // 7) Ako je osoba u kabini i vrata su otvorena, dozvoli izlazak levo
        if (person.inElevator && doorsAreOpen) {
            float insideMinX = elevator.x + 5.0f;
            // ako drži A i približi se levoj strani, izađe na hodnik
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && person.x <= insideMinX + 1.0f) {
                person.inElevator = false;
                personFloorIndex = elevator.currentFloor;                 // sada je na tom spratu
                person.x = elevator.x - person.width;                     // odmah ispred vrata
                person.y = floors[personFloorIndex].yTop;
            }
        }
        // ======================
        // Panel – obrada klika mišem
        // ======================

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        float mouseXF = static_cast<float>(mouseX);
        float mouseYGL = static_cast<float>(screenHeight) - static_cast<float>(mouseY);

        static bool leftMouseWasDown = false;
        bool leftMouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        bool leftMouseClick = leftMouseDown && !leftMouseWasDown;
        leftMouseWasDown = leftMouseDown;

        if (leftMouseClick && person.inElevator) {
            for (size_t i = 0; i < buttons.size(); ++i) {
                Button& b = buttons[i];

                if (mouseXF >= b.x0 && mouseXF <= b.x1 &&
                    mouseYGL >= b.y0 && mouseYGL <= b.y1) {

                    switch (b.type) {
                    case ButtonType::Floor: {
                        int fIdx = b.floorIndex;
                        if (fIdx >= 0 && fIdx < FLOOR_COUNT &&
                            !isFloorAlreadyRequested(fIdx) &&
                            fIdx != elevator.currentFloor) {

                            if (!hasTargetFloor && elevator.state == ElevatorState::Idle) {
                                targetFloor = fIdx;
                                hasTargetFloor = true;
                            }
                            else {
                                floorQueue.push_back(fIdx);
                            }
                            b.pressed = true;
                        }
                        break;
                    }

                    case ButtonType::OpenDoor:
                        // produži OPEN jednom po ciklusu
                        if (elevator.state == ElevatorState::DoorsOpen && !doorExtendedThisCycle) {
                            elevator.doorOpenTimer += BASE_DOOR_OPEN_TIME;
                            doorExtendedThisCycle = true;
                        }
                        break;

                    case ButtonType::CloseDoor:
                        // odmah počni zatvaranje ako su vrata otvorena
                        if (elevator.state == ElevatorState::DoorsOpen) {
                            elevator.doorOpenTimer = 0.0f;
                            elevator.state = ElevatorState::DoorsClosing;
                        }
                        break;

                    case ButtonType::Stop:
                        if (!b.pressed) {
                            b.pressed = true;
                            if (elevator.state == ElevatorState::Moving) {
                                elevator.state = ElevatorState::Stopped;
                            }
                        }
                        else {
                            b.pressed = false;
                            if (elevator.state == ElevatorState::Stopped) {
                                elevator.state = ElevatorState::Idle;
                            }
                        }
                        break;

                    case ButtonType::Ventilation:
                        ventilationOn = !ventilationOn;
                        b.pressed = ventilationOn;
                        break;
                    }
                }
            }
        }


        // ======================
        // Etapa 7 – kretanje lifta i vrata
        // ======================

        // Ako smo u Idle, a nemamo aktivan ciljni sprat,
        // ali postoji nešto u redu čekanja, preuzmi sledeći sprat iz reda.
        if (elevator.state == ElevatorState::Idle &&
            !hasTargetFloor &&
            !floorQueue.empty()) {

            targetFloor = floorQueue.front();
            floorQueue.erase(floorQueue.begin());
            hasTargetFloor = true;
        }


        // Ako je lift u mirovanju i imamo zadat novi ciljni sprat – kreni
        if (elevator.state == ElevatorState::Idle &&
            hasTargetFloor && targetFloor != elevator.currentFloor) {
            elevator.state = ElevatorState::Moving;
        }


        switch (elevator.state) {
        case ElevatorState::Moving:
        {
            float targetY = floors[targetFloor].yTop;
            float dir = (targetY > elevator.y) ? 1.0f : -1.0f;
            float step = elevator.speed * deltaTime * dir;
            elevator.y += step;

            // da ne preskoči ciljni sprat
            if ((dir > 0.0f && elevator.y >= targetY) ||
                (dir < 0.0f && elevator.y <= targetY)) {

                elevator.y = targetY;
                elevator.currentFloor = targetFloor;

                // otpusti dugme za ovaj sprat (ako postoji)
                if (targetFloor >= 0 && targetFloor < FLOOR_COUNT) {
                    int fb = floorButtonIndex[targetFloor];
                    if (fb >= 0 && fb < (int)buttons.size()) {
                        buttons[fb].pressed = false;
                    }
                }

                // ventilacija se gasi kada lift stigne do sledećeg sprata
                ventilationOn = false;
                if (ventilationButtonIndex >= 0 && ventilationButtonIndex < (int)buttons.size()) {
                    buttons[ventilationButtonIndex].pressed = false;
                }

                // uzmi sledeći sprat iz reda, ako postoji
                if (!floorQueue.empty()) {
                    targetFloor = floorQueue.front();
                    floorQueue.erase(floorQueue.begin());
                    hasTargetFloor = true;
                }
                else {
                    hasTargetFloor = false;
                }

                // započni animaciju otvaranja
                elevator.state = ElevatorState::DoorsOpening;
                elevator.doorOpenRatio = 0.0f;
                doorExtendedThisCycle = false;
            }

            break;
        }
        case ElevatorState::DoorsOpening:
            elevator.doorOpenRatio += deltaTime / DOOR_ANIM_DURATION;
            if (elevator.doorOpenRatio >= 1.0f) {
                elevator.doorOpenRatio = 1.0f;
                elevator.state = ElevatorState::DoorsOpen;
                elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
                doorExtendedThisCycle = false; // novi ciklus otvaranja
            }
            break;

        case ElevatorState::DoorsOpen:
            elevator.doorOpenTimer -= deltaTime;
            if (elevator.doorOpenTimer <= 0.0f) {
                elevator.doorOpenTimer = 0.0f;
                elevator.state = ElevatorState::DoorsClosing;
            }
            break;

        case ElevatorState::DoorsClosing:
            elevator.doorOpenRatio -= deltaTime / DOOR_ANIM_DURATION;
            if (elevator.doorOpenRatio <= 0.0f) {
                elevator.doorOpenRatio = 0.0f;
                elevator.state = ElevatorState::Idle;
            }
            break;
        case ElevatorState::Stopped:
            // zaustavljen, ništa se ne dešava
            break;
        case ElevatorState::Idle:
        default:
            // ništa
            break;
        }

        // 8) Ažuriraj verteks podatke za kabinu lifta (jer se y menja)
        float eBottomCur = elevator.y;
        float eTopCur = elevator.y + elevator.height;
        float eLeftCur = elevator.x;
        float eRightCur = elevator.x + elevator.width;

        elevatorVertices[0].x = eLeftCur;   elevatorVertices[0].y = eBottomCur;
        elevatorVertices[1].x = eRightCur;  elevatorVertices[1].y = eBottomCur;
        elevatorVertices[2].x = eRightCur;  elevatorVertices[2].y = eTopCur;
        elevatorVertices[3].x = eLeftCur;   elevatorVertices[3].y = eTopCur;

        glBindBuffer(GL_ARRAY_BUFFER, elevatorVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(elevatorVertices), elevatorVertices);

        // 9) Ažuriraj verteks podatke za vrata na osnovu doorOpenRatio
        float doorHeight = elevator.height * (1.0f - elevator.doorOpenRatio);
        if (doorHeight < 0.0f) doorHeight = 0.0f;

        float dBottom = elevator.y;
        float dTop = elevator.y + doorHeight;
        float dLeft = elevator.x;
        float dRight = elevator.x + elevator.width;

        doorVertices[0] = { dLeft,  dBottom, 0.0f, 0.0f };
        doorVertices[1] = { dRight, dBottom, 1.0f, 0.0f };
        doorVertices[2] = { dRight, dTop,    1.0f, 1.0f };
        doorVertices[3] = { dLeft,  dTop,    0.0f, 1.0f };

        glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(doorVertices), doorVertices);

        // (sada ide glClear, crtanje itd.)
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

        // vrata lifta – tamniji pravougaonik preko kabine
        if (elevator.doorOpenRatio < 1.0f) { // kada su potpuno otvorena, gotovo da se ne vide
            glBindVertexArray(doorVAO);
            shader.setInt("uUseTexture", 0);
            shader.setVec4("uColor", 0.2f, 0.2f, 0.3f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        }


        // osoba na prizemlju
        glBindVertexArray(personVAO);
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.9f, 0.4f, 0.4f, 1.0f); // crvenkasti “čovek”
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        // panel dugmići
        glBindVertexArray(buttonVAO);
        shader.setInt("uUseTexture", 0);

        for (size_t i = 0; i < buttons.size(); ++i) {
            const Button& b = buttons[i];

            // da li je miš trenutno iznad dugmeta
            bool hovered = (person.inElevator &&
                mouseXF >= b.x0 && mouseXF <= b.x1 &&
                mouseYGL >= b.y0 && mouseYGL <= b.y1);

            // 1) Okvir – malo veći pravougaonik
            float border = 3.0f;
            buttonVertices[0] = { b.x0 - border, b.y0 - border, 0.0f, 0.0f };
            buttonVertices[1] = { b.x1 + border, b.y0 - border, 1.0f, 0.0f };
            buttonVertices[2] = { b.x1 + border, b.y1 + border, 1.0f, 1.0f };
            buttonVertices[3] = { b.x0 - border, b.y1 + border, 0.0f, 1.0f };

            glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buttonVertices), buttonVertices);

            // tamni okvir
            shader.setVec4("uColor", 0.05f, 0.05f, 0.08f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

            // 2) Unutrašnjost dugmeta
            buttonVertices[0] = { b.x0, b.y0, 0.0f, 0.0f };
            buttonVertices[1] = { b.x1, b.y0, 1.0f, 0.0f };
            buttonVertices[2] = { b.x1, b.y1, 1.0f, 1.0f };
            buttonVertices[3] = { b.x0, b.y1, 0.0f, 1.0f };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buttonVertices), buttonVertices);

            // bazne boje po tipu
            float r, g, bCol;
            if (b.type == ButtonType::Floor) {
                r = 0.55f; g = 0.55f; bCol = 0.65f;        // normalno
                if (b.pressed) {                          // sprat tražen → svetlije
                    r += 0.3f; g += 0.3f; bCol += 0.3f;
                }
            }
            else if (b.type == ButtonType::Stop) {
                r = 0.7f; g = 0.2f; bCol = 0.2f;
                if (b.pressed) {                          // aktivan STOP → jako crveno
                    r = 0.95f; g = 0.25f; bCol = 0.25f;
                }
            }
            else if (b.type == ButtonType::Ventilation) {
                r = 0.25f; g = 0.6f; bCol = 0.25f;
                if (b.pressed) {                          // uključen → svetlo zeleno
                    r = 0.35f; g = 0.95f; bCol = 0.35f;
                }
            }
            else { // Open / Close
                r = 0.55f; g = 0.55f; bCol = 0.4f;
                if (b.pressed) {                          // (ako želiš da ikad setuješ pressed)
                    r += 0.25f; g += 0.25f; bCol += 0.1f;
                }
            }

            // hover efekat – malo posvetli
            if (hovered) {
                r += 0.1f; g += 0.1f; bCol += 0.1f;
            }

            // clamp na [0,1]
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (bCol > 1.0f) bCol = 1.0f;

            shader.setVec4("uColor", r, g, bCol, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        }

        // =========================
// Ikonice za specijalna dugmad (OPEN, CLOSE, STOP, VENT)
// =========================
        glBindVertexArray(labelVAO);
        shader.setInt("uUseTexture", 1);
        shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);

        auto drawIconOnButton = [&](int btnIndex, unsigned int tex,
            float iconW, float iconH)
            {
                if (btnIndex < 0 || btnIndex >= (int)buttons.size()) return;
                if (tex == 0) return; // tekstura nije učitana

                const Button& b = buttons[btnIndex];

                // centar dugmeta
                float cx = 0.5f * (b.x0 + b.x1);
                float cy = 0.5f * (b.y0 + b.y1);

                float x0i = cx - iconW * 0.5f;
                float x1i = cx + iconW * 0.5f;
                float y0i = cy - iconH * 0.5f;
                float y1i = cy + iconH * 0.5f;

                labelVertices[0] = { x0i, y0i, 0.0f, 0.0f };
                labelVertices[1] = { x1i, y0i, 1.0f, 0.0f };
                labelVertices[2] = { x1i, y1i, 1.0f, 1.0f };
                labelVertices[3] = { x0i, y1i, 0.0f, 1.0f };

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);

                glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(labelVertices), labelVertices);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
            };

        // dimenzije ikonica unutar dugmeta (manje od samog dugmeta)
        float iconW = 0.6f * btnWidth;
        float iconH = 0.4f * btnHeight;

        drawIconOnButton(openButtonIndex, openBtnTex, iconW, iconH);
        drawIconOnButton(closeButtonIndex, closeBtnTex, iconW, iconH);
        drawIconOnButton(stopButtonIndex, stopBtnTex, iconW, iconH);
        drawIconOnButton(ventilationButtonIndex, ventBtnTex, iconW, iconH);


        // oznake spratova: na panelu (u dugmetu) i pored platformi
        glBindVertexArray(labelVAO);
        shader.setInt("uUseTexture", 1);
        shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);

        float labelWidthPanel = 0.45f * btnWidth;
        float labelHeightPanel = 0.35f * btnHeight;
        float labelWidthSide = 40.0f;
        float labelHeightSide = 28.0f;

        for (int f = 0; f < FLOOR_COUNT; ++f) {
            unsigned int tex = floorLabelTextures[f];
            if (tex == 0) continue; // tekstura nije učitana

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);

            // 1) Tekst u dugmetu na panelu
            int btnIdx = floorButtonIndex[f];
            if (btnIdx >= 0) {
                const Button& b = buttons[btnIdx];

                float cx = 0.5f * (b.x0 + b.x1);
                float cy = 0.5f * (b.y0 + b.y1);

                float x0p = cx - labelWidthPanel * 0.5f;
                float x1p = cx + labelWidthPanel * 0.5f;
                float y0p = cy - labelHeightPanel * 0.5f;
                float y1p = cy + labelHeightPanel * 0.5f;

                labelVertices[0] = { x0p, y0p, 0.0f, 0.0f };
                labelVertices[1] = { x1p, y0p, 1.0f, 0.0f };
                labelVertices[2] = { x1p, y1p, 1.0f, 1.0f };
                labelVertices[3] = { x0p, y1p, 0.0f, 1.0f };

                glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(labelVertices), labelVertices);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
            }

            // 2) Tekst pored spratova na desnoj polovini
            float centerY = 0.5f * (floors[f].yBottom + floors[f].yTop);
            float lx = corridorLeftX - 50.0f; // malo levo od platforme

            float x0s = lx - labelWidthSide * 0.5f;
            float x1s = lx + labelWidthSide * 0.5f;
            float y0s = centerY - labelHeightSide * 0.5f;
            float y1s = centerY + labelHeightSide * 0.5f;

            labelVertices[0] = { x0s, y0s, 0.0f, 0.0f };
            labelVertices[1] = { x1s, y0s, 1.0f, 0.0f };
            labelVertices[2] = { x1s, y1s, 1.0f, 1.0f };
            labelVertices[3] = { x0s, y1s, 0.0f, 1.0f };

            glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(labelVertices), labelVertices);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        }


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
