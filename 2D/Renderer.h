#pragma once

#include <GL/glew.h>
#include "Types.h"
#include "Constants.h"
#include <vector>

// Forward declarations
struct Elevator;
struct Person;
struct Button;
class Shader;

// Rendering system for the elevator simulation
class Renderer {
public:
    Renderer(int screenWidth, int screenHeight);
    ~Renderer();

    // Initialize all rendering resources
    void initialize(const Floor floors[FLOOR_COUNT], float corridorLeftX, float corridorRightX,
                   const Elevator& elevator, float buildingBottomY, float buildingTopY);

    // Update dynamic geometry
    void updateElevatorGeometry(const Elevator& elevator);
    void updateDoorGeometry(const Elevator& elevator);
    void updatePersonGeometry(const Person& person);

    // Render functions
    void renderAll(Shader& shader, 
                   unsigned int buildingTexture,
                   unsigned int elevatorTexture,
                   unsigned int doorTexture,
                   unsigned int personTexture,
                   unsigned int personTextureLeft,
                   unsigned int overlayTexture,
                   unsigned int cursorFanTexture,
                   unsigned int cursorFanTexturePink,
                   const Floor floors[FLOOR_COUNT],
                   const Elevator& elevator,
                   const Person& person,
                   const std::vector<Button>& buttons,
                   unsigned int floorLabelTextures[FLOOR_COUNT],
                   unsigned int openBtnTex,
                   unsigned int closeBtnTex,
                   unsigned int stopBtnTex,
                   unsigned int ventBtnTex,
                   int floorButtonIndex[FLOOR_COUNT],
                   int openButtonIndex,
                   int closeButtonIndex,
                   int stopButtonIndex,
                   int ventilationButtonIndex,
                   float mouseX, float mouseY,
                   float corridorLeftX,
                   bool ventilationOn);

private:
    int screenWidth;
    int screenHeight;

    // VAOs, VBOs, EBOs
    unsigned int VAO, VBO, EBO;
    unsigned int overlayVAO, overlayVBO, overlayEBO;
    unsigned int floorsVAO, floorsVBO, floorsEBO;
    unsigned int elevatorVAO, elevatorVBO, elevatorEBO;
    unsigned int doorVAO, doorVBO, doorEBO;
    unsigned int shaftVAO, shaftVBO, shaftEBO;
    unsigned int personVAO, personVBO, personEBO;
    unsigned int buttonVAO, buttonVBO, buttonEBO;
    unsigned int labelVAO, labelVBO, labelEBO;
    unsigned int cursorVAO, cursorVBO, cursorEBO;

    // Geometry data
    Vertex elevatorVertices[4];
    Vertex doorVertices[8];
    Vertex personVertices[4];
    Vertex buttonVertices[4];
    Vertex labelVertices[4];
    Vertex cursorVertices[4];

    // Helper functions
    void setupBackgroundGeometry();
    void setupOverlayGeometry();
    void setupFloorsGeometry(const Floor floors[FLOOR_COUNT], float corridorLeftX, float corridorRightX);
    void setupElevatorGeometry(const Elevator& elevator);
    void setupDoorGeometry();
    void setupShaftGeometry(const Elevator& elevator, float buildingBottomY, float buildingTopY);
    void setupPersonGeometry(const Person& person);
    void setupButtonGeometry();
    void setupLabelGeometry();
    void setupCursorGeometry();
};

