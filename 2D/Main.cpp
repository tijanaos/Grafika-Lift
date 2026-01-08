#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include "Shader.h"
#include "Util.h"
#include "Helpers.h"
#include "Types.h"
#include "Constants.h"
#include "Renderer.h"
#include "ElevatorController.h"
#include "PersonController.h"
#include "ButtonPanel.h"

// Global deltaTime (seconds)
float deltaTime = 0.0f;

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

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

    // Ortho projection
    float projection[16];
    makeOrtho(0.0f, (float)screenWidth,
        0.0f, (float)screenHeight,
        -1.0f, 1.0f,
        projection);

    shader.use();
    shader.setMat4("uProjection", projection);
    shader.setInt("uTexture", 0);

    // Initialize floors
    Floor floors[FLOOR_COUNT];
    float buildingBottomY = 100.0f;
    float buildingTopY = (float)screenHeight - 160.0f;
    float floorThickness = 8.0f;
    float floorSpacing = (buildingTopY - buildingBottomY) / (FLOOR_COUNT - 1);

    for (int i = 0; i < FLOOR_COUNT; ++i) {
        float centerY = buildingBottomY + floorSpacing * i;
        floors[i].yBottom = centerY - floorThickness * 0.5f;
        floors[i].yTop = centerY + floorThickness * 0.5f;
    }

    // Initialize elevator
    float shaftMarginRight = 40.0f;
    float elevatorScale = 1.8f;
    float elevatorRightX = (float)screenWidth - shaftMarginRight;
    float elevatorX = elevatorRightX - (60.0f * elevatorScale);
    float elevatorHeight = floorSpacing * 0.6f * elevatorScale;

    ElevatorController elevatorController;
    float elevatorWidth = 60.0f * elevatorScale;
    elevatorController.initialize(floors, elevatorX, floorSpacing, FLOOR_1, elevatorWidth, elevatorHeight);

    // Initialize corridor
    float midX = screenWidth / 2.0f;
    float corridorLeftX = midX + 60.0f;
    float corridorRightX = elevatorX - 20.0f;

    // Initialize person
    PersonController personController;
    personController.initialize(floors, corridorLeftX, elevatorHeight, FLOOR_PR);

    // Initialize button panel
    ButtonPanel buttonPanel;
    buttonPanel.initialize(screenWidth, screenHeight);

    // Initialize renderer
    Renderer renderer(screenWidth, screenHeight);
    renderer.initialize(floors, corridorLeftX, corridorRightX, 
                       elevatorController.getElevator(), buildingBottomY, buildingTopY);

    // Load textures
    unsigned int overlayTexture = loadImageToTexture("textures/ime.png");
    unsigned int elevatorTexture = loadImageToTexture("textures/elevator_open.png");
    unsigned int doorTexture = loadImageToTexture("textures/elevator_door.png");
    unsigned int personTexture = loadImageToTexture("textures/person.png");
    unsigned int personTextureLeft = loadImageToTexture("textures/person_left.png");
    unsigned int buildingTexture = loadImageToTexture("textures/small_brick_wall.png");

    unsigned int floorLabelTextures[FLOOR_COUNT];
    floorLabelTextures[FLOOR_SU] = loadImageToTexture("textures/floor_SU.png");
    floorLabelTextures[FLOOR_PR] = loadImageToTexture("textures/floor_PR.png");
    floorLabelTextures[FLOOR_1] = loadImageToTexture("textures/floor1.png");
    floorLabelTextures[FLOOR_2] = loadImageToTexture("textures/floor2.png");
    floorLabelTextures[FLOOR_3] = loadImageToTexture("textures/floor3.png");
    floorLabelTextures[FLOOR_4] = loadImageToTexture("textures/floor4.png");
    floorLabelTextures[FLOOR_5] = loadImageToTexture("textures/floor5.png");
    floorLabelTextures[FLOOR_6] = loadImageToTexture("textures/floor6.png");

    unsigned int openBtnTex = loadImageToTexture("textures/open.png");
    unsigned int closeBtnTex = loadImageToTexture("textures/close.png");
    unsigned int stopBtnTex = loadImageToTexture("textures/stop.png");
    unsigned int ventBtnTex = loadImageToTexture("textures/fan.png");
    unsigned int cursorFanTexture = loadImageToTexture("textures/fan_cursor_black.png");
    unsigned int cursorFanTexturePink = loadImageToTexture("textures/fan_cursor_pink2.png");

    // Game state
    int targetFloor = elevatorController.getElevator().currentFloor;
    bool hasTargetFloor = false;
    std::vector<int> floorQueue;
    bool ventilationOn = false;
    bool doorExtendedThisCycle = false;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        double frameStartTime = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Update person movement
        float corridorMaxXOutside = elevatorController.getElevator().x - personController.getPerson().width;
        personController.update(deltaTime, window, floors, elevatorController.getElevator(),
                               corridorLeftX, corridorMaxXOutside);

        // Handle elevator call (C key)
        if (personController.handleElevatorCall(window, elevatorController.getElevator(),
                                               floorQueue, hasTargetFloor, targetFloor)) {
            if (elevatorController.getElevator().currentFloor == personController.getCurrentFloor()) {
                if (elevatorController.getElevator().state == ElevatorState::Idle ||
                    elevatorController.getElevator().state == ElevatorState::DoorsClosing) {
                    elevatorController.openDoors();
                    doorExtendedThisCycle = false;
                }
                else if (elevatorController.getElevator().state == ElevatorState::DoorsOpen) {
                    elevatorController.extendDoorTimer();
                }
            }
        }

        // Handle person entering/exiting elevator
        personController.handleElevatorInteraction(elevatorController.getElevator(), floors);
        
        // Handle exit from elevator
        if (personController.getPerson().inElevator && 
            elevatorController.getElevator().state == ElevatorState::DoorsOpen) {
            float insideMinX = elevatorController.getElevator().x + 5.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && 
                personController.getPerson().x <= insideMinX + 1.0f) {
                Person& person = personController.getPerson();
                person.inElevator = false;
                int newFloor = elevatorController.getElevator().currentFloor;
                person.x = elevatorController.getElevator().x - person.width;
                person.y = floors[newFloor].yTop;
                personController.setCurrentFloor(newFloor);
            }
        }

        // Handle button panel clicks
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        float mouseXF = static_cast<float>(mouseX);
        float mouseYGL = static_cast<float>(screenHeight) - static_cast<float>(mouseY);

        static bool leftMouseWasDown = false;
        bool leftMouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        bool leftMouseClick = leftMouseDown && !leftMouseWasDown;
        leftMouseWasDown = leftMouseDown;

        if (leftMouseClick && personController.getPerson().inElevator) {
            buttonPanel.handleClick(mouseXF, mouseYGL, personController.getPerson().inElevator,
                                   elevatorController, ventilationOn,
                                   floorQueue, hasTargetFloor, targetFloor);
        }

        // Update elevator
        int floorButtonIndices[FLOOR_COUNT];
        for (int i = 0; i < FLOOR_COUNT; ++i) {
            floorButtonIndices[i] = buttonPanel.getFloorButtonIndex(i);
        }
        elevatorController.update(deltaTime, floors, floorQueue, hasTargetFloor, targetFloor,
                                 ventilationOn,
                                 floorButtonIndices,
                                 buttonPanel.getButtons(),
                                 buttonPanel.getVentilationButtonIndex());

        // Update renderer geometry
        renderer.updateElevatorGeometry(elevatorController.getElevator());
        renderer.updateDoorGeometry(elevatorController.getElevator());
        renderer.updatePersonGeometry(personController.getPerson());

        // Clear and render
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.renderAll(shader,
                           buildingTexture,
                           elevatorTexture,
                           doorTexture,
                           personTexture,
                           personTextureLeft,
                           overlayTexture,
                           cursorFanTexture,
                           cursorFanTexturePink,
                           floors,
                           elevatorController.getElevator(),
                           personController.getPerson(),
                           buttonPanel.getButtons(),
                           floorLabelTextures,
                           openBtnTex,
                           closeBtnTex,
                           stopBtnTex,
                           ventBtnTex,
                           floorButtonIndices,
                           buttonPanel.getOpenButtonIndex(),
                           buttonPanel.getCloseButtonIndex(),
                           buttonPanel.getStopButtonIndex(),
                           buttonPanel.getVentilationButtonIndex(),
                           mouseXF, mouseYGL,
                           corridorLeftX,
                           ventilationOn);

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
    }

    glfwTerminate();
    return 0;
}
