#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Util.h"
#include "Camera.h"

#include "Elevator.h"
#include <cmath>
#include <algorithm>

static bool gInElevator = false;

static int gHoverBtn = -1;   // koje dugme panel-a "gađaš" pogledom (centar ekrana)


// Dimenzije kabine lifta
static const int ELEV_START_FLOOR_IDX = 2; // početak: sprat "1" (SU=0, PR=1, 1=2)
static const float CABIN_W = 1.6f;
static const float CABIN_H = 2.4f;
static const float CABIN_D = 2.0f;
static const float DOOR_THICK = 0.06f;

// Otvor (portal) na zidu sprata (ulaz u lift)
static const float PORTAL_W = 1.4f;
static const float PORTAL_H = 2.2f;

// Vrata na zidu sprata (to su "spoljna" vrata lifta)
static const float HALL_DOOR_THICK = 0.08f;

// Kabinska klizna vrata (2 krila)
static const float CABIN_DOOR_GAP = 0.02f;     // mala rupa između krila
static const float CABIN_DOOR_DEPTH = 0.05f;   // debljina krila po X (jer su na strani ka hodniku)

// ---------------- Panel u kabini ----------------
static const float PANEL_W = 1.0f;
static const float PANEL_H = 1.8f;
static const float PANEL_THICK = 0.03f;

static const float BTN_THICK = 0.03f;

enum PanelBtnId {
    BTN_F0 = 0, BTN_F1, BTN_F2, BTN_F3, BTN_F4, BTN_F5, BTN_F6, BTN_F7, // spratovi 0..7
    BTN_OPEN = 8,
    BTN_CLOSE = 9,
    BTN_STOP = 10,
    BTN_VENT = 11
};

struct PanelBtn {
    int id;
    float cx, cy;  // lokalno na panelu (x,y)
    float w, h;
};

// Raspored:
//  Row0: SU, PR
//  Row1: 1, 2
//  Row2: 3, 4
//  Row3: 5, 6
//  Row4: OPEN, CLOSE
//  Row5: STOP, VENT
static const PanelBtn gPanelBtns[] = {
    { BTN_F0,  -0.25f, +0.55f, 0.34f, 0.22f }, { BTN_F1, +0.25f, +0.55f, 0.34f, 0.22f },
    { BTN_F2,  -0.25f, +0.25f, 0.34f, 0.22f }, { BTN_F3, +0.25f, +0.25f, 0.34f, 0.22f },
    { BTN_F4,  -0.25f, -0.05f, 0.34f, 0.22f }, { BTN_F5, +0.25f, -0.05f, 0.34f, 0.22f },
    { BTN_F6,  -0.25f, -0.35f, 0.34f, 0.22f }, { BTN_F7, +0.25f, -0.35f, 0.34f, 0.22f },

    { BTN_OPEN,  -0.25f, -0.65f, 0.34f, 0.20f }, { BTN_CLOSE, +0.25f, -0.65f, 0.34f, 0.20f },
    { BTN_STOP,  -0.25f, -0.85f, 0.34f, 0.20f }, { BTN_VENT,  +0.25f, -0.85f, 0.34f, 0.20f },
};

// -------------------- Globalno za mouse callback --------------------
static Camera* gCamera = nullptr;
static bool gFirstMouse = true;
static float gLastX = 0.0f;
static float gLastY = 0.0f;

static float gDeltaTime = 0.0f;
static float gLastFrame = 0.0f;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (gFirstMouse) {
        gLastX = (float)xpos;
        gLastY = (float)ypos;
        gFirstMouse = false;
    }

    float xoffset = (float)xpos - gLastX;
    float yoffset = gLastY - (float)ypos; // obrnuto: gore je pozitivan pitch

    gLastX = (float)xpos;
    gLastY = (float)ypos;

    if (gCamera) gCamera->ProcessMouseMovement(xoffset, yoffset);
}

// ---------- ETAPA 2: dimenzije scene ----------
static const int NUM_FLOORS = 8;          // SU, PR, 1..6
static const float FLOOR_H = 3.0f;        // visina sprata
static const float SLAB_THICK = 0.12f;

static const float HALL_W = 12.0f;        // širina hodnika (x)
static const float HALL_D = 10.0f;        // dubina hodnika (z)
static const float WALL_THICK = 0.20f;
static const float WALL_H = 2.6f;

// okno lifta desno
static const float SHAFT_W = 2.2f;
static const float SHAFT_D = 2.6f;

// Centar okna (mora biti ISTO kao u crtanju)
static float getShaftX() {
    return HALL_W * 0.5f + SHAFT_W * 0.5f + 0.6f;
}


static Elevator* gElev = nullptr;

static int floorFromCameraY() {
    // kamera je na visini +1.7, uzmi "pod" sprata kao reference
    float yFloor = gCamera ? (gCamera->Position.y - 1.7f) : 0.0f;
    int idx = (int)std::round(yFloor / FLOOR_H);
    if (idx < 0) idx = 0;
    if (idx > NUM_FLOORS - 1) idx = NUM_FLOORS - 1;
    return idx;
}

static bool isAtElevatorEntrance(const Camera& cam, const Elevator& elev) {
    // ulaz je na desnom zidu (x ~ HALL_W/2), oko portala po Z
    float wallX = HALL_W * 0.5f;
    float doorX = wallX - (WALL_THICK * 0.5f) - (HALL_DOOR_THICK * 0.5f) - 0.01f;

    // Kamera pozicija
    float x = cam.Position.x;
    float z = cam.Position.z;

    // blizu vrata po X
    bool nearX = std::fabs(x - doorX) < 0.6f;

    // unutar širine portala po Z
    bool nearZ = std::fabs(z - 0.0f) < (PORTAL_W * 0.5f);

    // vrata moraju biti stvarno otvorena
    bool doorsOpen = elev.DoorOpen() > 0.90f;

    // i lift mora biti na tom spratu (spoljna vrata se otvaraju samo tad)
    int floorIdx = floorFromCameraY();
    bool atFloor = elev.IsExactlyAtFloor(floorIdx);

    return nearX && nearZ && doorsOpen && atFloor;
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (!gElev) return;

    // Spratovi: F1..F8 = SU, PR, 1,2,3,4,5,6
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F8) {
        int idx = key - GLFW_KEY_F1; // 0..7
        gElev->RequestFloor(idx);
        return;
    }

    // Pozovi lift na sprat na kom si (dev test) - C
    if (key == GLFW_KEY_C) {
        // Ako si van lifta -> pozovi lift na svoj sprat
        if (!gInElevator) {
            gElev->CallToFloor(floorFromCameraY());
        }
        // Ako si na ulazu i vrata su otvorena -> udji/izadji
        else {
            // ako si unutra i vrata otvorena, izlazak je dozvoljen
            // (izlazak ćemo rešiti tako što te teleportujemo malo u hodnik)
            if (gCamera && isAtElevatorEntrance(*gCamera, *gElev)) {
                gInElevator = false;
                // izbaci malo u hodnik (ka -X)
                gCamera->Position.x -= 1.2f;
            }
        }
        return;
    }

    if (key == GLFW_KEY_ENTER) {
        if (!gInElevator && gCamera && isAtElevatorEntrance(*gCamera, *gElev)) {
            gInElevator = true;

            // teleport u kabinu: centar kabine, malo unutra
            float shaftX = getShaftX();
            float cabinBaseY = gElev->CabinBaseY();

            gCamera->Position.x = shaftX;                 // u sredinu okna
            gCamera->Position.y = cabinBaseY + 1.7f;      // visina očiju
            gCamera->Position.z = 0.0f;                   // centar kabine
        }
        return;
    }

    // Vrata: E = OPEN (extend), Q = CLOSE
    if (key == GLFW_KEY_E) { gElev->PressOpen(); return; }
    if (key == GLFW_KEY_Q) { gElev->PressClose(); return; }

    // STOP: Space (pauza/resume tokom kretanja)
    if (key == GLFW_KEY_SPACE) { gElev->PressStopToggle(); return; }

    // Vent: V (auto-off na prvom target spratu)
    if (key == GLFW_KEY_V) { gElev->ToggleVent(); return; }
}


static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (!gCamera) return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) gCamera->MoveForward(gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) gCamera->MoveBackward(gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) gCamera->MoveRight(gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) gCamera->MoveLeft(gDeltaTime);
}

static void drawCube() {
    // VAO ima 24 verteksa (6 strana * 4), crtamo fan po strani
    for (int i = 0; i < 6; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
}

// helper za crtanje "kvadra" iz kocke (pozicija + skala)
static void drawBox(GLint uM, const glm::vec3& pos, const glm::vec3& scale) {
    glm::mat4 M(1.0f);
    M = glm::translate(M, pos);
    M = glm::scale(M, scale);
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));
    drawCube();
}

static glm::vec3 cameraForwardFromView(const Camera& cam) {
    // forward iz view matrice (ne zavisi od toga da li Camera ima "Front" polje)
    glm::mat4 invV = glm::inverse(cam.GetViewMatrix());
    glm::vec3 camPlusZ = glm::normalize(glm::vec3(invV[2])); // kamera +Z (unazad)
    return -camPlusZ; // forward
}

static bool pointInRect(float x, float y, float cx, float cy, float w, float h) {
    return (x >= cx - w * 0.5f && x <= cx + w * 0.5f &&
        y >= cy - h * 0.5f && y <= cy + h * 0.5f);
}

// Vraca id dugmeta (0..11) ili -1 ako ne "gađa" panel.
static int hitTestPanelCenterRay(const Camera& cam, const Elevator& elev) {
    if (!gInElevator) return -1;

    float shaftX = getShaftX();
    float cabinBaseY = elev.CabinBaseY();

    // Panel na DESNOM zidu kabine (x = +CABIN_W/2), okrenut ka unutra (-X)
    float rightWallX = shaftX + CABIN_W * 0.5f;
    float panelCenterX = rightWallX - (PANEL_THICK * 0.5f) - 0.01f;

    // malo napred po Z da bude prirodnije (može 0.0f ako hoćeš centar)
    glm::vec3 panelCenter(panelCenterX, cabinBaseY + 1.35f, 0.35f);

    glm::vec3 N(-1.0f, 0.0f, 0.0f); // normal panel-a (ka unutra)
    glm::vec3 O = cam.Position;
    glm::vec3 D = glm::normalize(cameraForwardFromView(cam));

    float denom = glm::dot(N, D);
    if (std::fabs(denom) < 1e-4f) return -1;

    // ciljaj "lice" dugmadi (malo ispred panela ka unutra)
    glm::vec3 planeP = panelCenter + N * (PANEL_THICK * 0.5f + BTN_THICK * 0.5f);

    float t = glm::dot(planeP - O, N) / denom;
    if (t < 0.0f) return -1;

    glm::vec3 hit = O + D * t;
    glm::vec3 rel = hit - panelCenter;

    // Na ovom panelu: lokalno (u, v) = (rel.z, rel.y)
    float u = rel.z; // horizontalno po Z
    float v = rel.y; // vertikalno po Y

    if (std::fabs(u) > PANEL_W * 0.5f || std::fabs(v) > PANEL_H * 0.5f) return -1;

    for (const PanelBtn& b : gPanelBtns) {
        if (pointInRect(u, v, b.cx, b.cy, b.w, b.h)) {
            return b.id;
        }
    }
    return -1;
}


static void activatePanelButton(int id) {
    if (!gElev) return;

    if (id >= BTN_F0 && id <= BTN_F7) {
        gElev->RequestFloor(id); // 0..7
        return;
    }
    if (id == BTN_OPEN) { gElev->PressOpen(); return; }
    if (id == BTN_CLOSE) { gElev->PressClose(); return; }
    if (id == BTN_STOP) { gElev->PressStopToggle(); return; }
    if (id == BTN_VENT) { gElev->ToggleVent(); return; }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    if (!gCamera || !gElev) return;
    if (!gInElevator) return;

    int id = hitTestPanelCenterRay(*gCamera, *gElev);
    if (id != -1) {
        activatePanelButton(id);
    }
}

// Crta panel + dugmad (sa hover highlight)
static void drawElevatorPanel(GLint uM, GLint uColor, const Elevator& elev) {
    if (!gInElevator) return;

    float shaftX = getShaftX();
    float cabinBaseY = elev.CabinBaseY();

    float rightWallX = shaftX + CABIN_W * 0.5f;
    float panelCenterX = rightWallX - (PANEL_THICK * 0.5f) - 0.01f;

    glm::vec3 panelCenter(panelCenterX, cabinBaseY + 1.35f, 0.35f);

    // pozadina panela (debljina po X, visina po Y, širina po Z)
    glUniform4f(uColor, 0.10f, 0.10f, 0.12f, 1.0f);
    drawBox(uM, panelCenter, glm::vec3(PANEL_THICK, PANEL_H, PANEL_W));

    glm::vec3 N(-1.0f, 0.0f, 0.0f); // ka unutra
    float faceOffset = (PANEL_THICK * 0.5f + BTN_THICK * 0.5f);

    for (const PanelBtn& b : gPanelBtns) {
        bool hover = (b.id == gHoverBtn);

        float r = 0.65f, g = 0.65f, bl = 0.70f;
        if (b.id == BTN_OPEN) { r = 0.25f; g = 0.80f; bl = 0.35f; }
        if (b.id == BTN_CLOSE) { r = 0.85f; g = 0.25f; bl = 0.25f; }
        if (b.id == BTN_STOP) { r = 0.90f; g = 0.55f; bl = 0.20f; }
        if (b.id == BTN_VENT) { r = 0.25f; g = 0.55f; bl = 0.90f; }

        if (hover) {
            r = std::min(1.0f, r + 0.25f);
            g = std::min(1.0f, g + 0.25f);
            bl = std::min(1.0f, bl + 0.25f);
        }

        glUniform4f(uColor, r, g, bl, 1.0f);

        // Lokalno: b.cx ide po Z, b.cy ide po Y
        glm::vec3 btnPos(
            panelCenter.x + N.x * faceOffset,
            panelCenter.y + b.cy,
            panelCenter.z + b.cx
        );

        // dugme: debljina po X, visina po Y, širina po Z
        drawBox(uM, btnPos, glm::vec3(BTN_THICK, b.h, b.w));
    }
}


// HUD crosshair u centru (u NDC prostoru)
static void drawCrosshairHUD(GLint uM, GLint uV, GLint uP, GLint uColor) {
    glDisable(GL_DEPTH_TEST);

    glm::mat4 V2(1.0f);
    glm::mat4 P2 = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(V2));
    glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(P2));

    glUniform4f(uColor, 1.0f, 1.0f, 1.0f, 1.0f);

    // horizontalna linija
    drawBox(uM, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.06f, 0.004f, 0.001f));
    // vertikalna linija
    drawBox(uM, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.004f, 0.06f, 0.001f));

    glEnable(GL_DEPTH_TEST);
}


int main() {
    if (!glfwInit()) {
        std::cout << "GLFW nije inicijalizovan.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// fullscreen dimenzije
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int wWidth = mode->width;
    int wHeight = mode->height;

    // Fullscreen window (pravi fullscreen, ne samo maximize)
    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "Lift 3D - Etapa 2", monitor, nullptr);
    if (!window) {
        std::cout << "Prozor nije napravljen.\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // FPS kamera: zaključaj kursor i koristi mouse movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);


    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW nije mogao da se ucita.\n";
        return 3;
    }

    glViewport(0, 0, wWidth, wHeight);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shader = createShader("basic.vert", "basic.frag");
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "uTex"), 0);
    glUniform1i(glGetUniformLocation(shader, "useTex"), 0);
    glUniform1i(glGetUniformLocation(shader, "transparent"), 0);


    // --- Kamera ---
    float prY = 1.0f * FLOOR_H; // PR je index 1: SU(0), PR(1)
    Camera camera(glm::vec3(-2.0f, prY + 1.7f, 4.0f));
    gCamera = &camera;

    Elevator elevator(NUM_FLOORS, FLOOR_H, ELEV_START_FLOOR_IDX);
    gElev = &elevator;


    // --- Kocka VAO (format: pos(3), col(4), tex(2)) ---
    float v[] = {
        // Prednja (z = +0.5)
         0.5f,  0.5f,  0.5f,   1,0,0,1,   0,0,
        -0.5f,  0.5f,  0.5f,   1,0,0,1,   1,0,
        -0.5f, -0.5f,  0.5f,   1,0,0,1,   1,1,
         0.5f, -0.5f,  0.5f,   1,0,0,1,   0,1,

         // Leva (x = -0.5)
         -0.5f,  0.5f,  0.5f,   0,0,1,1,   0,0,
         -0.5f,  0.5f, -0.5f,   0,0,1,1,   1,0,
         -0.5f, -0.5f, -0.5f,   0,0,1,1,   1,1,
         -0.5f, -0.5f,  0.5f,   0,0,1,1,   0,1,

         // Donja (y = -0.5)
          0.5f, -0.5f,  0.5f,   1,1,1,1,   0,0,
         -0.5f, -0.5f,  0.5f,   1,1,1,1,   1,0,
         -0.5f, -0.5f, -0.5f,   1,1,1,1,   1,1,
          0.5f, -0.5f, -0.5f,   1,1,1,1,   0,1,

          // Gornja (y = +0.5)
           0.5f,  0.5f,  0.5f,   1,1,0,1,   0,0,
           0.5f,  0.5f, -0.5f,   1,1,0,1,   1,0,
          -0.5f,  0.5f, -0.5f,   1,1,0,1,   1,1,
          -0.5f,  0.5f,  0.5f,   1,1,0,1,   0,1,

          // Desna (x = +0.5)
           0.5f,  0.5f,  0.5f,   0,1,0,1,   0,0,
           0.5f, -0.5f,  0.5f,   0,1,0,1,   1,0,
           0.5f, -0.5f, -0.5f,   0,1,0,1,   1,1,
           0.5f,  0.5f, -0.5f,   0,1,0,1,   0,1,

           // Zadnja (z = -0.5)
            0.5f,  0.5f, -0.5f,   1,0.5f,0,1,   0,0,
            0.5f, -0.5f, -0.5f,   1,0.5f,0,1,   1,0,
           -0.5f, -0.5f, -0.5f,   1,0.5f,0,1,   1,1,
           -0.5f,  0.5f, -0.5f,   1,0.5f,0,1,   0,1,
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    int stride = (3 + 4 + 2) * (int)sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Uniform lokacije
    int uM = glGetUniformLocation(shader, "uM");
    int uV = glGetUniformLocation(shader, "uV");
    int uP = glGetUniformLocation(shader, "uP");
    int uColor = glGetUniformLocation(shader, "uColor");

    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        float current = (float)glfwGetTime();
        gDeltaTime = current - gLastFrame;
        gLastFrame = current;

        processInput(window);
        elevator.Update(gDeltaTime);

        if (gInElevator && gCamera) {
            // drži kameru "zaključanu" u kabini (X,Z), a Y prati kabinu
            float shaftX = getShaftX();
            gCamera->Position.x = shaftX;
            gCamera->Position.y = elevator.CabinBaseY() + 1.7f;
            // Z ostavljamo kako jeste (možeš da se "pomeraš" u kabini), ili zaključaš na 0:
            // gCamera->Position.z = 0.0f;
        }

        gHoverBtn = -1;
        if (gCamera && gElev && gInElevator) {
            gHoverBtn = hitTestPanelCenterRay(*gCamera, *gElev);
        }



        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Perspektiva (spec traži perspektivu)
        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        float aspect = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;

        glm::mat4 P = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);
        glm::mat4 V = camera.GetViewMatrix();

        glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(V));
        glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(P));

        glBindVertexArray(VAO);

        // ---------- Spratovi ----------
        for (int i = 0; i < NUM_FLOORS; i++) {
            float y = i * FLOOR_H;

            // POD (svetlo sivo)
            glUniform4f(uColor, 0.75f, 0.75f, 0.78f, 1.0f);
            drawBox(uM,
                glm::vec3(0.0f, y - SLAB_THICK * 0.5f, 0.0f),
                glm::vec3(HALL_W, SLAB_THICK, HALL_D)
            );

            // ZIDOVI (srednje sivo)
            glUniform4f(uColor, 0.55f, 0.55f, 0.60f, 1.0f);

            // zadnji zid (na -Z)
            drawBox(uM,
                glm::vec3(0.0f, y + WALL_H * 0.5f, -HALL_D * 0.5f),
                glm::vec3(HALL_W, WALL_H, WALL_THICK)
            );

            // levi zid (na -X)
            drawBox(uM,
                glm::vec3(-HALL_W * 0.5f, y + WALL_H * 0.5f, 0.0f),
                glm::vec3(WALL_THICK, WALL_H, HALL_D)
            );

            // DESNI ZID (na +X) sa otvorom ka liftu

            float wallX = HALL_W * 0.5f;
            float portalYCenter = y + PORTAL_H * 0.5f;      // otvor počinje od poda sprata
            float topH = WALL_H - PORTAL_H;
            if (topH < 0.1f) topH = 0.1f;

            // 1) deo iznad otvora
            drawBox(uM,
                glm::vec3(wallX, portalYCenter + PORTAL_H * 0.5f + topH * 0.5f, 0.0f),
                glm::vec3(WALL_THICK, topH, HALL_D)
            );

            // 2) levi bočni stub (oko otvora) - po Z osi
            float sideW = (HALL_D - PORTAL_W) * 0.5f;
            if (sideW < 0.1f) sideW = 0.1f;

            // stub na -Z strani otvora
            drawBox(uM,
                glm::vec3(wallX, portalYCenter, -(PORTAL_W * 0.5f + sideW * 0.5f)),
                glm::vec3(WALL_THICK, PORTAL_H, sideW)
            );

            // stub na +Z strani otvora
            drawBox(uM,
                glm::vec3(wallX, portalYCenter, +(PORTAL_W * 0.5f + sideW * 0.5f)),
                glm::vec3(WALL_THICK, PORTAL_H, sideW)
            );

            // Spoljna vrata lifta na spratu (za sada ZATVORENA)
            // Stojimo malo unutar hodnika (pomeri po X ka unutra)
            glUniform4f(uColor, 0.80f, 0.80f, 0.85f, 1.0f);

            float doorX = wallX - (WALL_THICK * 0.5f) - (HALL_DOOR_THICK * 0.5f) - 0.01f;

            float open = (elevator.IsExactlyAtFloor(i) ? elevator.DoorOpen() : 0.0f);

            // otvara se od sredine ka spolja
            float zShift = open * (PORTAL_W * 0.25f);

            float leftZ = -PORTAL_W * 0.25f - zShift;
            float rightZ = PORTAL_W * 0.25f + zShift;

            drawBox(uM,
                glm::vec3(doorX, portalYCenter, leftZ),
                glm::vec3(HALL_DOOR_THICK, PORTAL_H, PORTAL_W * 0.5f - CABIN_DOOR_GAP)
            );

            drawBox(uM,
                glm::vec3(doorX, portalYCenter, rightZ),
                glm::vec3(HALL_DOOR_THICK, PORTAL_H, PORTAL_W * 0.5f - CABIN_DOOR_GAP)
            );


        }

        // ---------- Okvir okna lifta (NE kao puna kocka, da se vidi kabina) ----------
        float buildingH = (NUM_FLOORS - 1) * FLOOR_H + WALL_H;
        float shaftX = HALL_W * 0.5f + SHAFT_W * 0.5f + 0.6f;

        glUniform4f(uColor, 0.35f, 0.35f, 0.38f, 1.0f);

        float pillarW = 0.15f;
        drawBox(uM, glm::vec3(shaftX - SHAFT_W * 0.5f, buildingH * 0.5f, 0.0f),
            glm::vec3(pillarW, buildingH, SHAFT_D));
        drawBox(uM, glm::vec3(shaftX + SHAFT_W * 0.5f, buildingH * 0.5f, 0.0f),
            glm::vec3(pillarW, buildingH, SHAFT_D));

        // zadnja “ploča” okna
        drawBox(uM, glm::vec3(shaftX, buildingH * 0.5f, -SHAFT_D * 0.5f),
            glm::vec3(SHAFT_W, buildingH, 0.10f));

        // ---------- Kabina lifta (obojena) ----------
        float cabinBaseY = elevator.CabinBaseY();
        float openCabin = elevator.DoorOpen();


        // telo kabine (tamno)
        glUniform4f(uColor, 0.20f, 0.20f, 0.22f, 1.0f);
        drawBox(uM,
            glm::vec3(shaftX, cabinBaseY + CABIN_H * 0.5f, 0.0f),
            glm::vec3(CABIN_W, CABIN_H, CABIN_D)
        );

        // Kabinska vrata (2 krila) na strani ka hodniku (X- strana kabine)
    // Za sad zatvorena, stoje u sredini, dele otvor po Z.
        glUniform4f(uColor, 0.70f, 0.70f, 0.75f, 1.0f);

        float cabinDoorX = shaftX - CABIN_W * 0.5f + CABIN_DOOR_DEPTH * 0.5f;
        float cabinDoorY = cabinBaseY + CABIN_H * 0.5f;

        float zShiftCab = openCabin * (CABIN_D * 0.25f);
        float leftZc = -CABIN_D * 0.25f - zShiftCab;
        float rightZc = CABIN_D * 0.25f + zShiftCab;

        // krilo 1
        drawBox(uM,
            glm::vec3(cabinDoorX, cabinDoorY, leftZc),
            glm::vec3(CABIN_DOOR_DEPTH, CABIN_H, CABIN_D * 0.5f - CABIN_DOOR_GAP)
        );

		// krilo 2
        drawBox(uM,
            glm::vec3(cabinDoorX, cabinDoorY, rightZc),
            glm::vec3(CABIN_DOOR_DEPTH, CABIN_H, CABIN_D * 0.5f - CABIN_DOOR_GAP)
        );

        drawElevatorPanel(uM, uColor, elevator);
        glUniform4f(uColor, 0.9f, 0.2f, 0.9f, 1.0f);

        drawCrosshairHUD(uM, uV, uP, uColor);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
