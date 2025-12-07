#include "Renderer.h"
#include "Shader.h"
#include <GL/glew.h>

Renderer::Renderer(int screenWidth, int screenHeight) 
    : screenWidth(screenWidth), screenHeight(screenHeight),
      VAO(0), VBO(0), EBO(0),
      overlayVAO(0), overlayVBO(0), overlayEBO(0),
      floorsVAO(0), floorsVBO(0), floorsEBO(0),
      elevatorVAO(0), elevatorVBO(0), elevatorEBO(0),
      doorVAO(0), doorVBO(0), doorEBO(0),
      shaftVAO(0), shaftVBO(0), shaftEBO(0),
      personVAO(0), personVBO(0), personEBO(0),
      buttonVAO(0), buttonVBO(0), buttonEBO(0),
      labelVAO(0), labelVBO(0), labelEBO(0),
      cursorVAO(0), cursorVBO(0), cursorEBO(0) {
}

Renderer::~Renderer() {
}

void Renderer::initialize(const Floor floors[FLOOR_COUNT], float corridorLeftX, float corridorRightX,
                          const Elevator& elevator, float buildingBottomY, float buildingTopY) {
    setupBackgroundGeometry();
    setupOverlayGeometry();
    setupFloorsGeometry(floors, corridorLeftX, corridorRightX);
    setupElevatorGeometry(elevator);
    setupDoorGeometry();
    setupShaftGeometry(elevator, buildingBottomY, buildingTopY);
    setupPersonGeometry(Person{});
    setupButtonGeometry();
    setupLabelGeometry();
    setupCursorGeometry();
}

void Renderer::setupBackgroundGeometry() {
    Vertex vertices[8];
    float midX = screenWidth / 2.0f;

    // Left half
    vertices[0] = { 0.0f, 0.0f, 0.0f, 0.0f };
    vertices[1] = { midX, 0.0f, 1.0f, 0.0f };
    vertices[2] = { midX, (float)screenHeight, 1.0f, 1.0f };
    vertices[3] = { 0.0f, (float)screenHeight, 0.0f, 1.0f };

    // Right half
    vertices[4] = { midX, 0.0f, 0.0f, 0.0f };
    vertices[5] = { (float)screenWidth, 0.0f, 1.0f, 0.0f };
    vertices[6] = { (float)screenWidth, (float)screenHeight, 1.0f, 1.0f };
    vertices[7] = { midX, (float)screenHeight, 0.0f, 1.0f };

    unsigned int indices[12] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupOverlayGeometry() {
    Vertex overlayVertices[4];
    unsigned int overlayIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float margin = 20.0f;
    float overlayWidth = 200.0f;
    float overlayHeight = 80.0f;

    float x0 = margin;
    float y1 = screenHeight - margin;
    float y0 = y1 - overlayHeight;
    float x1 = x0 + overlayWidth;

    overlayVertices[0] = { x0, y0, 0.0f, 0.0f };
    overlayVertices[1] = { x1, y0, 1.0f, 0.0f };
    overlayVertices[2] = { x1, y1, 1.0f, 1.0f };
    overlayVertices[3] = { x0, y1, 0.0f, 1.0f };

    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    glGenBuffers(1, &overlayEBO);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, overlayEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(overlayIndices), overlayIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupFloorsGeometry(const Floor floors[FLOOR_COUNT], float corridorLeftX, float corridorRightX) {
    Vertex floorVertices[FLOOR_COUNT * 4];
    unsigned int floorIndices[FLOOR_COUNT * 6];

    for (int i = 0; i < FLOOR_COUNT; ++i) {
        int vBase = i * 4;
        int iBase = i * 6;

        float y0 = floors[i].yBottom;
        float y1 = floors[i].yTop;

        floorVertices[vBase + 0] = { corridorLeftX, y0, 0.0f, 0.0f };
        floorVertices[vBase + 1] = { corridorRightX, y0, 1.0f, 0.0f };
        floorVertices[vBase + 2] = { corridorRightX, y1, 1.0f, 1.0f };
        floorVertices[vBase + 3] = { corridorLeftX, y1, 0.0f, 1.0f };

        floorIndices[iBase + 0] = vBase + 0;
        floorIndices[iBase + 1] = vBase + 1;
        floorIndices[iBase + 2] = vBase + 2;
        floorIndices[iBase + 3] = vBase + 2;
        floorIndices[iBase + 4] = vBase + 3;
        floorIndices[iBase + 5] = vBase + 0;
    }

    glGenVertexArrays(1, &floorsVAO);
    glGenBuffers(1, &floorsVBO);
    glGenBuffers(1, &floorsEBO);

    glBindVertexArray(floorsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupElevatorGeometry(const Elevator& elevator) {
    unsigned int elevatorIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float eBottom = elevator.y;
    float eTop = elevator.y + elevator.height;
    float eLeft = elevator.x;
    float eRight = elevator.x + elevator.width;

    elevatorVertices[0] = { eLeft, eBottom, 0.0f, 0.0f };
    elevatorVertices[1] = { eRight, eBottom, 1.0f, 0.0f };
    elevatorVertices[2] = { eRight, eTop, 1.0f, 1.0f };
    elevatorVertices[3] = { eLeft, eTop, 0.0f, 1.0f };

    glGenVertexArrays(1, &elevatorVAO);
    glGenBuffers(1, &elevatorVBO);
    glGenBuffers(1, &elevatorEBO);

    glBindVertexArray(elevatorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, elevatorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(elevatorVertices), elevatorVertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elevatorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elevatorIndices), elevatorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupDoorGeometry() {
    unsigned int doorIndices[12] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);
    glGenBuffers(1, &doorEBO);

    glBindVertexArray(doorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(doorVertices), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, doorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(doorIndices), doorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupShaftGeometry(const Elevator& elevator, float buildingBottomY, float buildingTopY) {
    Vertex shaftVertices[4];
    unsigned int shaftIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float shaftPaddingX = 20.0f;
    float shaftPaddingY = 20.0f;

    float shaftLeft = elevator.x - shaftPaddingX;
    float shaftRight = elevator.x + elevator.width + shaftPaddingX;
    float shaftBottom = buildingBottomY - shaftPaddingY;
    float shaftTop = buildingTopY + shaftPaddingY;

    shaftVertices[0] = { shaftLeft, shaftBottom, 0.0f, 0.0f };
    shaftVertices[1] = { shaftRight, shaftBottom, 1.0f, 0.0f };
    shaftVertices[2] = { shaftRight, shaftTop, 1.0f, 1.0f };
    shaftVertices[3] = { shaftLeft, shaftTop, 0.0f, 1.0f };

    glGenVertexArrays(1, &shaftVAO);
    glGenBuffers(1, &shaftVBO);
    glGenBuffers(1, &shaftEBO);

    glBindVertexArray(shaftVAO);
    glBindBuffer(GL_ARRAY_BUFFER, shaftVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shaftVertices), shaftVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shaftEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(shaftIndices), shaftIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupPersonGeometry(const Person& person) {
    unsigned int personIndices[6] = { 0, 1, 2, 2, 3, 0 };

    float pLeft = person.x;
    float pRight = person.x + person.width;
    float pTop = person.y + person.height;

    personVertices[0] = { pLeft, person.y, 0.0f, 0.0f };
    personVertices[1] = { pRight, person.y, 1.0f, 0.0f };
    personVertices[2] = { pRight, pTop, 1.0f, 1.0f };
    personVertices[3] = { pLeft, pTop, 0.0f, 1.0f };

    glGenVertexArrays(1, &personVAO);
    glGenBuffers(1, &personVBO);
    glGenBuffers(1, &personEBO);

    glBindVertexArray(personVAO);
    glBindBuffer(GL_ARRAY_BUFFER, personVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(personVertices), personVertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, personEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(personIndices), personIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupButtonGeometry() {
    unsigned int buttonIndices[6] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &buttonVAO);
    glGenBuffers(1, &buttonVBO);
    glGenBuffers(1, &buttonEBO);

    glBindVertexArray(buttonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buttonEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buttonIndices), buttonIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::setupLabelGeometry() {
    unsigned int labelIndices[6] = { 0, 1, 2, 2, 3, 0 };

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
}

void Renderer::setupCursorGeometry() {
    unsigned int cursorIndices[6] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &cursorVAO);
    glGenBuffers(1, &cursorVBO);
    glGenBuffers(1, &cursorEBO);

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursorVertices), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cursorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cursorIndices), cursorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::updateElevatorGeometry(const Elevator& elevator) {
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
}

void Renderer::updateDoorGeometry(const Elevator& elevator) {
    float fullHeight = elevator.height;
    float dBottom = elevator.y;
    float dTop = elevator.y + fullHeight;
    float fullWidth = elevator.width;
    float centerX = elevator.x + fullWidth * 0.5f;
    float halfWidth = fullWidth * 0.5f;
    float slide = halfWidth * elevator.doorOpenRatio;

    float leftWallX = elevator.x;
    float rightWallX = elevator.x + fullWidth;

    float leftLeft = leftWallX;
    float leftRight = centerX - slide;
    float rightRight = rightWallX;
    float rightLeft = centerX + slide;

    doorVertices[0] = { leftLeft, dBottom, 0.0f, 0.0f };
    doorVertices[1] = { leftRight, dBottom, 0.5f, 0.0f };
    doorVertices[2] = { leftRight, dTop, 0.5f, 1.0f };
    doorVertices[3] = { leftLeft, dTop, 0.0f, 1.0f };

    doorVertices[4] = { rightLeft, dBottom, 0.5f, 0.0f };
    doorVertices[5] = { rightRight, dBottom, 1.0f, 0.0f };
    doorVertices[6] = { rightRight, dTop, 1.0f, 1.0f };
    doorVertices[7] = { rightLeft, dTop, 0.5f, 1.0f };

    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(doorVertices), doorVertices);
}

void Renderer::updatePersonGeometry(const Person& person) {
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
}

void Renderer::renderAll(Shader& shader,
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
                        bool ventilationOn) {
    shader.use();

    // Background - left half (panel)
    glBindVertexArray(VAO);
    shader.setInt("uUseTexture", 0);
    shader.setVec4("uColor", 0.25f, 0.25f, 0.30f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0));

    // Background - right half (building)
    glBindVertexArray(VAO);
    if (buildingTexture != 0) {
        shader.setInt("uUseTexture", 1);
        shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, buildingTexture);
    }
    else {
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.1f, 0.15f, 0.35f, 1.0f);
    }
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));

    // Elevator shaft
    glBindVertexArray(shaftVAO);
    shader.setInt("uUseTexture", 0);
    shader.setVec4("uColor", 0.6f, 0.6f, 0.65f, 0.35f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    // Floors
    glBindVertexArray(floorsVAO);
    shader.setInt("uUseTexture", 0);
    shader.setVec4("uColor", 0.92f, 0.92f, 0.98f, 1.0f);
    glDrawElements(GL_TRIANGLES, FLOOR_COUNT * 6, GL_UNSIGNED_INT, (void*)0);

    // Elevator cab
    glBindVertexArray(elevatorVAO);
    if (elevatorTexture != 0) {
        shader.setInt("uUseTexture", 1);
        shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, elevatorTexture);
    }
    else {
        shader.setInt("uUseTexture", 0);
        shader.setVec4("uColor", 0.8f, 0.8f, 0.85f, 1.0f);
    }
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    // Person inside elevator
    if (person.inElevator) {
        glBindVertexArray(personVAO);
        unsigned int tex = person.facingRight ? personTexture : personTextureLeft;
        if (tex != 0) {
            shader.setInt("uUseTexture", 1);
            shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
        }
        else {
            shader.setInt("uUseTexture", 0);
            shader.setVec4("uColor", 0.9f, 0.4f, 0.4f, 1.0f);
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }

    // Doors
    if (elevator.doorOpenRatio < 1.0f) {
        glBindVertexArray(doorVAO);
        if (doorTexture != 0) {
            shader.setInt("uUseTexture", 1);
            shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, doorTexture);
        }
        else {
            shader.setInt("uUseTexture", 0);
            shader.setVec4("uColor", 0.2f, 0.2f, 0.3f, 1.0f);
        }
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)0);
    }

    // Person outside elevator
    if (!person.inElevator) {
        glBindVertexArray(personVAO);
        unsigned int tex = person.facingRight ? personTexture : personTextureLeft;
        if (tex != 0) {
            shader.setInt("uUseTexture", 1);
            shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
        }
        else {
            shader.setInt("uUseTexture", 0);
            shader.setVec4("uColor", 0.9f, 0.4f, 0.4f, 1.0f);
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }

    // Buttons
    glBindVertexArray(buttonVAO);
    shader.setInt("uUseTexture", 0);
    float btnWidth = 160.0f;
    float btnHeight = 80.0f;

    for (size_t i = 0; i < buttons.size(); ++i) {
        const Button& b = buttons[i];
        bool hovered = (person.inElevator &&
            mouseX >= b.x0 && mouseX <= b.x1 &&
            mouseY >= b.y0 && mouseY <= b.y1);

        float border = 3.0f;
        buttonVertices[0] = { b.x0 - border, b.y0 - border, 0.0f, 0.0f };
        buttonVertices[1] = { b.x1 + border, b.y0 - border, 1.0f, 0.0f };
        buttonVertices[2] = { b.x1 + border, b.y1 + border, 1.0f, 1.0f };
        buttonVertices[3] = { b.x0 - border, b.y1 + border, 0.0f, 1.0f };

        glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buttonVertices), buttonVertices);
        shader.setVec4("uColor", 0.05f, 0.05f, 0.08f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        buttonVertices[0] = { b.x0, b.y0, 0.0f, 0.0f };
        buttonVertices[1] = { b.x1, b.y0, 1.0f, 0.0f };
        buttonVertices[2] = { b.x1, b.y1, 1.0f, 1.0f };
        buttonVertices[3] = { b.x0, b.y1, 0.0f, 1.0f };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buttonVertices), buttonVertices);

        float r, g, bCol;
        if (b.type == ButtonType::Floor) {
            r = 0.55f; g = 0.55f; bCol = 0.65f;
            if (b.pressed) {
                r += 0.3f; g += 0.3f; bCol += 0.3f;
            }
        }
        else if (b.type == ButtonType::Stop) {
            r = 0.7f; g = 0.2f; bCol = 0.2f;
            if (b.pressed) {
                r = 0.95f; g = 0.25f; bCol = 0.25f;
            }
        }
        else if (b.type == ButtonType::Ventilation) {
            r = 0.25f; g = 0.6f; bCol = 0.25f;
            if (b.pressed) {
                r = 0.35f; g = 0.95f; bCol = 0.35f;
            }
        }
        else {
            r = 0.55f; g = 0.55f; bCol = 0.4f;
            if (b.pressed) {
                r += 0.25f; g += 0.25f; bCol += 0.1f;
            }
        }

        if (hovered) {
            r += 0.1f; g += 0.1f; bCol += 0.1f;
        }

        if (r > 1.0f) r = 1.0f;
        if (g > 1.0f) g = 1.0f;
        if (bCol > 1.0f) bCol = 1.0f;

        shader.setVec4("uColor", r, g, bCol, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }

    // Button icons
    glBindVertexArray(labelVAO);
    shader.setInt("uUseTexture", 1);
    shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);

    float iconW = 0.6f * btnWidth;
    float iconH = 0.4f * btnHeight;

    auto drawIconOnButton = [&](int btnIndex, unsigned int tex) {
        if (btnIndex < 0 || btnIndex >= (int)buttons.size()) return;
        if (tex == 0) return;

        const Button& b = buttons[btnIndex];
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

    drawIconOnButton(openButtonIndex, openBtnTex);
    drawIconOnButton(closeButtonIndex, closeBtnTex);
    drawIconOnButton(stopButtonIndex, stopBtnTex);
    drawIconOnButton(ventilationButtonIndex, ventBtnTex);

    // Floor labels
    float labelWidthPanel = 0.45f * btnWidth;
    float labelHeightPanel = 0.35f * btnHeight;
    float labelWidthSide = 40.0f;
    float labelHeightSide = 28.0f;

    for (int f = 0; f < FLOOR_COUNT; ++f) {
        unsigned int tex = floorLabelTextures[f];
        if (tex == 0) continue;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

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

        float centerY = 0.5f * (floors[f].yBottom + floors[f].yTop);
        float lx = corridorLeftX - 50.0f;

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

    // Overlay
    if (overlayTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, overlayTexture);
        shader.setInt("uUseTexture", 1);
        shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(overlayVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }

    glBindVertexArray(0);

    // Cursor
    float cursorSize = 48.0f;
    float x0c = mouseX - cursorSize * 0.5f;
    float x1c = mouseX + cursorSize * 0.5f;
    float y0c = mouseY - cursorSize * 0.5f;
    float y1c = mouseY + cursorSize * 0.5f;

    cursorVertices[0] = { x0c, y0c, 0.0f, 0.0f };
    cursorVertices[1] = { x1c, y0c, 1.0f, 0.0f };
    cursorVertices[2] = { x1c, y1c, 1.0f, 1.0f };
    cursorVertices[3] = { x0c, y1c, 0.0f, 1.0f };

    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cursorVertices), cursorVertices);

    glBindVertexArray(cursorVAO);
    shader.use();
    unsigned int tex = ventilationOn ? cursorFanTexturePink : cursorFanTexture;
    shader.setInt("uUseTexture", 1);
    shader.setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
}

