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
static const float CABIN_W = 2.4f;
static const float CABIN_H = 2.2f;
static const float CABIN_D = 2.8f;
static const float DOOR_THICK = 0.06f;

// Otvor (portal) na zidu sprata (ulaz u lift)
static const float PORTAL_W = 2.4f;
static const float PORTAL_H = 2.2f;

// Vrata na zidu sprata (to su "spoljna" vrata lifta)
static const float HALL_DOOR_THICK = 0.08f;

// Kabinska klizna vrata (2 krila)
static const float CABIN_DOOR_GAP = 0.02f;     // mala rupa između krila
static const float CABIN_DOOR_DEPTH = 0.05f;   // debljina krila po X (jer su na strani ka hodniku)

// ---------------- Panel u kabini ----------------
static const float PANEL_W = 0.6f;
static const float PANEL_H = 1.2f;
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
    { BTN_F0, -0.14f, +0.50f, 0.22f, 0.12f },
    { BTN_F1, +0.14f, +0.50f, 0.22f, 0.12f },

    { BTN_F2, -0.14f, +0.32f, 0.22f, 0.12f },
    { BTN_F3, +0.14f, +0.32f, 0.22f, 0.12f },

    { BTN_F4, -0.14f, +0.14f, 0.22f, 0.12f },
    { BTN_F5, +0.14f, +0.14f, 0.22f, 0.12f },

    { BTN_F6, -0.14f, -0.04f, 0.22f, 0.12f },
    { BTN_F7, +0.14f, -0.04f, 0.22f, 0.12f },

    { BTN_OPEN,  -0.14f, -0.22f, 0.22f, 0.12f },
    { BTN_CLOSE, +0.14f, -0.22f, 0.22f, 0.12f },

    { BTN_STOP,  -0.14f, -0.40f, 0.22f, 0.12f },
    { BTN_VENT,  +0.14f, -0.40f, 0.22f, 0.12f },
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
    return HALL_W * 0.5f + SHAFT_W * 0.5f - 0.35f;
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
    float wallX = HALL_W * 0.5f;
    float x = cam.Position.x;
    float z = cam.Position.z;

    // Proširi opseg detekcije da bi "hvatao" trenutak prolaska kroz vrata
    bool nearX = std::fabs(x - wallX) < 1.0f;
    bool nearZ = std::fabs(z - 0.0f) < (PORTAL_W * 0.6f);
    bool doorsOpen = elev.DoorOpen() > 0.80f;
    bool atFloor = elev.IsExactlyAtFloor(floorFromCameraY());

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
    if (!gInElevator) return -1; // Klik radi samo kad smo unutra

    float shaftX = getShaftX();
    float cabinBaseY = elev.CabinBaseY();

    float leftWallZ = -CABIN_D * 0.5f;
    float panelCenterZ = leftWallZ + (PANEL_THICK * 0.5f) + 0.01f;
    glm::vec3 panelCenter(shaftX, cabinBaseY + 1.2f, panelCenterZ);

    glm::vec3 N(0.0f, 0.0f, 1.0f); // Normala ka +Z
    glm::vec3 O = cam.Position;
    glm::vec3 D = glm::normalize(cameraForwardFromView(cam));

    float denom = glm::dot(N, D);
    if (std::fabs(denom) < 1e-4f) return -1;

    glm::vec3 planeP = panelCenter + N * (PANEL_THICK * 0.5f + BTN_THICK * 0.5f);
    float t = glm::dot(planeP - O, N) / denom;
    if (t < 0.0f) return -1;

    glm::vec3 hit = O + D * t;
    glm::vec3 rel = hit - panelCenter;

    // Za levi zid: lokalno u = rel.x, v = rel.y
    float u = rel.x;
    float v = rel.y;

    if (std::fabs(u) > PANEL_W * 0.5f || std::fabs(v) > PANEL_H * 0.5f) return -1;

    for (const PanelBtn& b : gPanelBtns) {
        // Skaliraj i ovde proveru ako si smanjila dugmad
        if (pointInRect(u, v, b.cx * (PANEL_W / 0.6f), b.cy * (PANEL_H / 1.2f), b.w * 0.7f, b.h * 0.7f)) {
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

// Crta panel + dugmad (sa hover highlight) , jako komplikovano jer crta teksturu na vrh dugmeta a ne sa strabe
static void drawElevatorPanel(GLint uM, GLint uColor, GLint uUseTex, GLint uTransparent,
    const GLuint* btnTextures, const Elevator& elev)
{
    // --- Jedan mali VAO/VBO za "nalepnicu" (quad) sa ispravnim atributima:
    // layout(location=0)=pos, (1)=col, (2)=tex
    static GLuint sQuadVAO = 0, sQuadVBO = 0;
    if (sQuadVAO == 0)
    {
        // 2 trougla, quad u XY ravni, UV 0..1, boja bela
        const float quad[] = {
            //  x     y    z      r g b a        u   v
            -0.5f,-0.5f,0.0f,   1,1,1,1,      0,0,
             0.5f,-0.5f,0.0f,   1,1,1,1,      1,0,
             0.5f, 0.5f,0.0f,   1,1,1,1,      1,1,

            -0.5f,-0.5f,0.0f,   1,1,1,1,      0,0,
             0.5f, 0.5f,0.0f,   1,1,1,1,      1,1,
            -0.5f, 0.5f,0.0f,   1,1,1,1,      0,1,
        };

        glGenVertexArrays(1, &sQuadVAO);
        glGenBuffers(1, &sQuadVBO);

        glBindVertexArray(sQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        const GLsizei stride = (3 + 4 + 2) * (GLsizei)sizeof(float);

        glEnableVertexAttribArray(0); // pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glEnableVertexAttribArray(1); // col
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2); // tex
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // zapamti koji je VAO trenutno bound (kod tebe je to cube VAO u main render loop-u)
    GLint prevVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);

    float shaftX = getShaftX();
    float cabinBaseY = elev.CabinBaseY();

    float leftWallZ = -CABIN_D * 0.5f;
    float panelCenterZ = leftWallZ + (PANEL_THICK * 0.5f) + 0.01f;

    glm::vec3 panelCenter(shaftX, cabinBaseY + 1.2f, panelCenterZ);

    // 1) Pozadina panela
    glUniform1i(uUseTex, 0);
    glUniform1i(uTransparent, 0);
    glUniform4f(uColor, 0.15f, 0.15f, 0.17f, 1.0f);
    drawBox(uM, panelCenter, glm::vec3(PANEL_W, PANEL_H, PANEL_THICK));

    glm::vec3 N(0.0f, 0.0f, 1.0f);
    float faceOffset = (PANEL_THICK * 0.5f + BTN_THICK * 0.5f);

    for (const PanelBtn& b : gPanelBtns)
    {
        bool hover = (b.id == gHoverBtn);

        glm::vec3 btnPos(
            panelCenter.x + b.cx,
            panelCenter.y + b.cy,
            panelCenter.z + N.z * faceOffset
        );

        // 2) Telo dugmeta (bez teksture)
        glUniform1i(uUseTex, 0);
        glUniform1i(uTransparent, 0);
        if (hover) glUniform4f(uColor, 0.70f, 0.70f, 0.72f, 1.0f);
        else       glUniform4f(uColor, 0.50f, 0.50f, 0.52f, 1.0f);

        drawBox(uM, btnPos, glm::vec3(b.w, b.h, BTN_THICK));

        // 3) Ikonica (tekstura) kao JEDAN QUAD, tačno na PREDNJOJ strani dugmeta
        GLuint tex = btnTextures[b.id];
        if (tex != 0)
        {
            glUniform1i(uUseTex, 1);
            glUniform1i(uTransparent, 1);
            glUniform4f(uColor, 1.0f, 1.0f, 1.0f, 1.0f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);

            // Najbitniji fix: stavi nalepnicu ISPRED prednje face dugmeta
            float zFront = btnPos.z + (BTN_THICK * 0.5f) + 0.0015f;

            glm::mat4 M(1.0f);
            M = glm::translate(M, glm::vec3(btnPos.x, btnPos.y, zFront));
            M = glm::scale(M, glm::vec3(b.w * 0.75f, b.h * 0.75f, 1.0f));
            glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));

            glBindVertexArray(sQuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // vrati cube VAO da sledeci drawBox radi normalno
            glBindVertexArray((GLuint)prevVAO);
        }
    }

    glUniform1i(uUseTex, 0);
    glUniform1i(uTransparent, 0);
    glBindVertexArray((GLuint)prevVAO);
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

// Crta pravougaonu tablicu sa teksturom oznake sprata
static void drawFloorSign(GLint uM, GLint uUseTex, GLint uColor, GLint uTransparent,
    GLuint texture, const glm::vec3& pos, float width, float height) {
    if (texture != 0) {
        glUniform1i(uUseTex, 1);
        glUniform1i(uTransparent, 1);  // omogući transparency za PNG
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform4f(uColor, 1.0f, 1.0f, 1.0f, 1.0f);

        // Tablica je tanka (po X osi jer je na zidu)
        drawBox(uM, pos, glm::vec3(0.02f, -height, -width));

        glUniform1i(uUseTex, 0);
        glUniform1i(uTransparent, 0);
    }
}
// za iscrtavanje tekstura preko dugmadi na panelu
static GLuint gQuadVAO = 0, gQuadVBO = 0;

static void initQuad()
{
    if (gQuadVAO) return;

    // pozicije + UV (quad u XY ravni, centriran u 0)
    float v[] = {
        // x, y, z,   u, v
        -0.5f,-0.5f,0,  0,0,
         0.5f,-0.5f,0,  1,0,
         0.5f, 0.5f,0,  1,1,

        -0.5f,-0.5f,0,  0,0,
         0.5f, 0.5f,0,  1,1,
        -0.5f, 0.5f,0,  0,1,
    };

    glGenVertexArrays(1, &gQuadVAO);
    glGenBuffers(1, &gQuadVBO);

    glBindVertexArray(gQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

static void drawTexturedQuad(GLuint shader, GLint uM, GLuint tex,
    const glm::vec3& pos, const glm::vec2& size,
    float zOffset)
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, pos + glm::vec3(0, 0, zOffset));
    M = glm::scale(M, glm::vec3(size.x, size.y, 1.0f));

    glUseProgram(shader);
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glBindVertexArray(gQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
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

    // --- uniform lokacije za teksture ---
    int uTex = glGetUniformLocation(shader, "uTex");
    int uUseTex = glGetUniformLocation(shader, "useTex");
    int uTexScale = glGetUniformLocation(shader, "uTexScale");
    int uTransparent = glGetUniformLocation(shader, "transparent");

    // default stanje
    glUniform1i(uTex, 0);
    glUniform1i(uUseTex, 0);
    glUniform2f(uTexScale, 1.0f, 1.0f);
    glUniform1i(uTransparent, 0);

    // --- ucitaj teksture (iz res foldera) ---
    GLuint texFloor = loadImageToTexture("res/pod2.jpg");
    GLuint texWall = loadImageToTexture("res/zid.jpg");

    // Oznake spratova
    GLuint texFloorSigns[NUM_FLOORS];
    texFloorSigns[0] = loadImageToTexture("res/floor_SU.png");
    texFloorSigns[1] = loadImageToTexture("res/floor_PR.png");
    texFloorSigns[2] = loadImageToTexture("res/floor1.png");
    texFloorSigns[3] = loadImageToTexture("res/floor2.png");
    texFloorSigns[4] = loadImageToTexture("res/floor3.png");
    texFloorSigns[5] = loadImageToTexture("res/floor4.png");
    texFloorSigns[6] = loadImageToTexture("res/floor5.png");
    texFloorSigns[7] = loadImageToTexture("res/floor6.png");

    // --- U main funkciji, kod ucitavanja tekstura ---
    GLuint texPanelBtns[12]; // 0-7 su spratovi, 8 OPEN, 9 CLOSE, 10 STOP, 11 VENT
    texPanelBtns[0] = loadImageToTexture("res/floor_SU.png");
    texPanelBtns[1] = loadImageToTexture("res/floor_PR.png");
    texPanelBtns[2] = loadImageToTexture("res/floor1.png");
    texPanelBtns[3] = loadImageToTexture("res/floor2.png");
    texPanelBtns[4] = loadImageToTexture("res/floor3.png");
    texPanelBtns[5] = loadImageToTexture("res/floor4.png");
    texPanelBtns[6] = loadImageToTexture("res/floor5.png");
    texPanelBtns[7] = loadImageToTexture("res/floor6.png");
    texPanelBtns[8] = loadImageToTexture("res/open.png");
    texPanelBtns[9] = loadImageToTexture("res/close.png");
    texPanelBtns[10] = loadImageToTexture("res/stop.png");
    texPanelBtns[11] = loadImageToTexture("res/fan.png");

    // --- Kamera ---
    float prY = 2.0f * FLOOR_H; // PR je index 1: SU(0), PR(1)
    Camera camera(glm::vec3(-2.0f, prY + 1.7f, 4.0f));
    gCamera = &camera;

    Elevator elevator(NUM_FLOORS, FLOOR_H, ELEV_START_FLOOR_IDX);
    gElev = &elevator;

    // --- Kocka VAO (format: pos(3), col(4), tex(2)) ---
    float v[] = {
        // Prednja (z = +0.5)
        -0.5f,  0.5f,  0.5f,   1,0,0,1,   1,0,  
         0.5f,  0.5f,  0.5f,   1,0,0,1,   0,0,  
         0.5f, -0.5f,  0.5f,   1,0,0,1,   0,1,  
        -0.5f, -0.5f,  0.5f,   1,0,0,1,   1,1,  

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

        // --- Unutar main while petlje, pre crtanja ---

        // 1. Provera da li si ušla u lift hodanjem
        if (!gInElevator) {
            float shaftX = getShaftX();
            // Ako je kamera prešla granicu hodnika i ušla u prostor kabine po X osi
            // i ako su vrata otvorena (to proverava isAtElevatorEntrance)
            if (gCamera->Position.x > (HALL_W * 0.5f - 0.2f) && isAtElevatorEntrance(*gCamera, *gElev)) {
                gInElevator = true;
            }
        }
        // 2. Provera da li si izašla iz lifta hodanjem
        else {
            float wallX = HALL_W * 0.5f;
            // Ako si u liftu, ali si krenula nazad ka hodniku (X osa se smanjuje)
            if (gCamera->Position.x < (wallX - 0.3f)) {
                gInElevator = false;
            }
        }

        // 3. Logika za "zaključavanje" unutar lifta dok se on kreće ili dok si unutra
        if (gInElevator && gCamera) {
            // Y osa uvek prati pod lifta
            gCamera->Position.y = elevator.CabinBaseY() + 1.7f;

            // OGRANIČENJE KRETANJA UNUTAR KABINE
            // Ne dozvoljavamo kameri da izađe kroz zadnji ili bočne zidove kabine
            float shaftX = getShaftX();
            float minX = shaftX - CABIN_W * 0.4f; // blizu vrata
            float maxX = shaftX + CABIN_W * 0.4f; // zadnji zid
            float minZ = -CABIN_D * 0.4f;
            float maxZ = CABIN_D * 0.4f;

            if (gCamera->Position.x > maxX) gCamera->Position.x = maxX;
            // Ako su vrata zatvorena, ne možeš ni napred (ka hodniku)
            if (gElev->DoorOpen() < 0.1f) {
                if (gCamera->Position.x < minX) gCamera->Position.x = minX;
            }

            if (gCamera->Position.z < minZ) gCamera->Position.z = minZ;
            if (gCamera->Position.z > maxZ) gCamera->Position.z = maxZ;
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

            // POD (tekstura)
            if (texFloor != 0) {
                glUniform1i(uUseTex, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texFloor);
                glUniform2f(uTexScale, 1.0f, 1.0f);      // ponavljanje (tweak po ukusu)
                glUniform4f(uColor, 1, 1, 1, 1);            // bez bojenja (ne tintuj)
            }
            else {
                glUniform1i(uUseTex, 0);
                glUniform4f(uColor, 0.75f, 0.75f, 0.78f, 1.0f);
            }

            drawBox(uM,
                glm::vec3(0.0f, y - SLAB_THICK * 0.5f, 0.0f),
                glm::vec3(HALL_W, SLAB_THICK, HALL_D)
            );

            // posle poda vrati na "bez teksture" ako želiš da sledeće bude boja
            glUniform1i(uUseTex, 0);
            glUniform2f(uTexScale, 1.0f, 1.0f);

            // ZIDOVI (tekstura)
            if (texWall != 0) {
                glUniform1i(uUseTex, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texWall);
                glUniform2f(uTexScale, 1.0f, 1.0f);
                glUniform4f(uColor, 1, 1, 1, 1);
            }
            else {
                glUniform1i(uUseTex, 0);
                glUniform4f(uColor, 0.55f, 0.55f, 0.60f, 1.0f);
            }

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
            // PREDNJI ZID (na +Z) - Zatvara hodnik sa suprotne strane od zadnjeg zida
            drawBox(uM,
                glm::vec3(0.0f, y + WALL_H * 0.5f, HALL_D * 0.5f),
                glm::vec3(HALL_W, WALL_H, WALL_THICK)
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
            glUniform1i(uUseTex, 0);
            glUniform2f(uTexScale, 1.0f, 1.0f);


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

            // OZNAKA SPRATA iznad vrata
            float signWidth = 0.4f;   // širina tablice
            float signHeight = 0.25f;  // visina tablice
            float signY = portalYCenter + PORTAL_H * 0.5f + 0.2f;  // malo iznad otvora
            float signX = doorX - 0.05f;  // malo prema hodniku da se vidi

            drawFloorSign(uM, uUseTex, uColor, uTransparent,
                texFloorSigns[i],
                glm::vec3(signX, signY, 0.0f),
                signWidth, signHeight);
        }

        // ---------- Okvir okna lifta (NE kao puna kocka, da se vidi kabina) ----------
        float shaftX = getShaftX();
        // ---------- Kabina lifta (obojena) ----------
        float cabinBaseY = elevator.CabinBaseY();
        float openCabin = elevator.DoorOpen();


        // telo kabine - teksturisano (da se jasno razlikuje od hodnika)
        if (texWall != 0) {
            glUniform1i(uUseTex, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texWall);
            glUniform2f(uTexScale, 2.0f, 2.0f);
            glUniform4f(uColor, 0.65f, 0.65f, 0.75f, 1.0f); // malo "metalno/hladno"
        }
        else {
            glUniform1i(uUseTex, 0);
            glUniform4f(uColor, 0.20f, 0.20f, 0.22f, 1.0f);
        }
        // --- NOVI KOD ZA ŠUPLJU KABINU ---
        float ct = 0.02f; // debljina zidova kabine

        if (texWall != 0) {
            glUniform1i(uUseTex, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texWall);
            glUniform2f(uTexScale, 1.0f, 1.0f);
            glUniform4f(uColor, 0.65f, 0.65f, 0.75f, 1.0f);
        }

        // 1. ZADNJI ZID (naspram vrata, na +X strani okna)
        drawBox(uM,
            glm::vec3(shaftX + CABIN_W * 0.5f, cabinBaseY + CABIN_H * 0.5f, 0.0f),
            glm::vec3(ct, CABIN_H, CABIN_D)
        );

        // 2. LEVI ZID (posmatrano iznutra ka vratima, to je -Z strana)
        drawBox(uM,
            glm::vec3(shaftX, cabinBaseY + CABIN_H * 0.5f, -CABIN_D * 0.5f),
            glm::vec3(CABIN_W, CABIN_H, ct)
        );

        // 3. DESNI ZID (na kom stoji panel, to je +Z strana)
        drawBox(uM,
            glm::vec3(shaftX, cabinBaseY + CABIN_H * 0.5f, CABIN_D * 0.5f),
            glm::vec3(CABIN_W, CABIN_H, ct)
        );

        // 4. PLAFON
        drawBox(uM,
            glm::vec3(shaftX, cabinBaseY + CABIN_H, 0.0f),
            glm::vec3(CABIN_W, ct, CABIN_D)
        );

        // POD KABINE (već imaš kod ispod, ali ga možeš integrisati ovde)
        if (texFloor != 0) {
            glUniform1i(uUseTex, 1);
            glBindTexture(GL_TEXTURE_2D, texFloor);
            drawBox(uM,
                glm::vec3(shaftX, cabinBaseY + 0.01f, 0.0f),
                glm::vec3(CABIN_W, 0.02f, CABIN_D)
            );
        }
        // PREDNJU STRANU (X-) NE CRTAMO - tako ostaje rupa za vrata!

        glUniform1i(uUseTex, 0);
        glUniform2f(uTexScale, 1.0f, 1.0f);


        // pod unutar kabine (preko donje strane kabine)
        if (texFloor != 0) {
            glUniform1i(uUseTex, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texFloor);
            glUniform2f(uTexScale, 2.0f, 2.0f);
            glUniform4f(uColor, 1.0f, 1.0f, 1.0f, 1.0f);

            drawBox(uM,
                glm::vec3(shaftX, cabinBaseY + 0.02f, 0.0f),
                glm::vec3(CABIN_W - 0.10f, 0.04f, CABIN_D - 0.10f)
            );
        }
        // dalje objekti u boji
        glUniform1i(uUseTex, 0);
        glUniform2f(uTexScale, 1.0f, 1.0f);


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

        drawElevatorPanel(uM, uColor, uUseTex, uTransparent, texPanelBtns, elevator);
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
