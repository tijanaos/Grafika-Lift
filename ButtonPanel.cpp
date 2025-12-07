#include "ButtonPanel.h"
#include "ElevatorController.h"
#include <algorithm>
#include <iostream>

ButtonPanel::ButtonPanel() 
    : openButtonIndex(-1), closeButtonIndex(-1), stopButtonIndex(-1), ventilationButtonIndex(-1) {
    for (int i = 0; i < FLOOR_COUNT; ++i) {
        floorButtonIndex[i] = -1;
    }
}

void ButtonPanel::initialize(int screenWidth, int screenHeight) {
    createButtons(screenWidth, screenHeight);
}

void ButtonPanel::createButtons(int screenWidth, int screenHeight) {
    buttons.clear();
    buttons.reserve(12);

    float midX = screenWidth / 2.0f;
    float panelCenterX = midX / 2.0f;
    float btnWidth = 160.0f;
    float btnHeight = 80.0f;
    float rowSpacing = btnHeight + 15.0f;
	float colOffset = 90.0f; // horizontal offset left/right from center (button columns)

    float colLeftX0 = panelCenterX - colOffset - btnWidth * 0.5f;
    float colLeftX1 = colLeftX0 + btnWidth;
    float colRightX0 = panelCenterX + colOffset - btnWidth * 0.5f;
    float colRightX1 = colRightX0 + btnWidth;

    float floorStartY = (float)screenHeight - 160.0f;

	// Floor buttons 4 rows x 2 columns
    // row 0: SU (left), PR (right)
    float yTop0 = floorStartY;
    float yBot0 = yTop0 - btnHeight;
    floorButtonIndex[FLOOR_SU] = addButton(colLeftX0, yBot0, colLeftX1, yTop0, ButtonType::Floor, FLOOR_SU);
    floorButtonIndex[FLOOR_PR] = addButton(colRightX0, yBot0, colRightX1, yTop0, ButtonType::Floor, FLOOR_PR);

    // row 1: 1, 2
    float yTop1 = floorStartY - rowSpacing;
    float yBot1 = yTop1 - btnHeight;
    floorButtonIndex[FLOOR_1] = addButton(colLeftX0, yBot1, colLeftX1, yTop1, ButtonType::Floor, FLOOR_1);
    floorButtonIndex[FLOOR_2] = addButton(colRightX0, yBot1, colRightX1, yTop1, ButtonType::Floor, FLOOR_2);

    // row 2: 3, 4
    float yTop2 = floorStartY - 2.0f * rowSpacing;
    float yBot2 = yTop2 - btnHeight;
    floorButtonIndex[FLOOR_3] = addButton(colLeftX0, yBot2, colLeftX1, yTop2, ButtonType::Floor, FLOOR_3);
    floorButtonIndex[FLOOR_4] = addButton(colRightX0, yBot2, colRightX1, yTop2, ButtonType::Floor, FLOOR_4);

    // row 3: 5, 6
    float yTop3 = floorStartY - 3.0f * rowSpacing;
    float yBot3 = yTop3 - btnHeight;
    floorButtonIndex[FLOOR_5] = addButton(colLeftX0, yBot3, colLeftX1, yTop3, ButtonType::Floor, FLOOR_5);
    floorButtonIndex[FLOOR_6] = addButton(colRightX0, yBot3, colRightX1, yTop3, ButtonType::Floor, FLOOR_6);

    // Control buttons (OPEN/CLOSE, STOP/VENT)
    float ctlTop1 = floorStartY - 4.5f * rowSpacing;
    float ctlBot1 = ctlTop1 - btnHeight;

    openButtonIndex = addButton(colLeftX0, ctlBot1, colLeftX1, ctlTop1, ButtonType::OpenDoor, -1);
    closeButtonIndex = addButton(colRightX0, ctlBot1, colRightX1, ctlTop1, ButtonType::CloseDoor, -1);

    float ctlTop2 = ctlTop1 - rowSpacing;
    float ctlBot2 = ctlTop2 - btnHeight;

    stopButtonIndex = addButton(colLeftX0, ctlBot2, colLeftX1, ctlTop2, ButtonType::Stop, -1);
    ventilationButtonIndex = addButton(colRightX0, ctlBot2, colRightX1, ctlTop2, ButtonType::Ventilation, -1);
}

int ButtonPanel::addButton(float x0, float yBottom, float x1, float yTop, ButtonType type, int floorIdx) {
    Button b;
    b.x0 = x0;
    b.y0 = yBottom;
    b.x1 = x1;
    b.y1 = yTop;
    b.type = type;
    b.floorIndex = floorIdx;
    b.pressed = false;
    buttons.push_back(b);
    return (int)buttons.size() - 1;
}

void ButtonPanel::handleClick(float mouseX, float mouseY, bool personInElevator,
                             ElevatorController& elevatorController, bool& ventilationOn,
                             std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor) {
    for (size_t i = 0; i < buttons.size(); ++i) {
        Button& b = buttons[i];

        if (mouseX >= b.x0 && mouseX <= b.x1 &&
            mouseY >= b.y0 && mouseY <= b.y1) {

            switch (b.type) {
            case ButtonType::Floor: {
                int fIdx = b.floorIndex;
                if (fIdx >= 0 && fIdx < FLOOR_COUNT &&
                    !isFloorAlreadyRequested(fIdx, hasTargetFloor, targetFloor, floorQueue) &&
                    fIdx != elevatorController.getElevator().currentFloor) {
                    if (elevatorController.requestFloor(fIdx, floorQueue, hasTargetFloor, targetFloor)) {
                        b.pressed = true;
                    }
                }
                break;
            }

            case ButtonType::OpenDoor:
                if (elevatorController.getElevator().state == ElevatorState::DoorsOpen &&
                    !elevatorController.isDoorExtendedThisCycle()) {
                    elevatorController.extendDoorTimer();
                    elevatorController.setDoorExtendedThisCycle(true);
                }
                else if (elevatorController.getElevator().state == ElevatorState::Idle ||
                    elevatorController.getElevator().state == ElevatorState::DoorsClosing) {
                    elevatorController.openDoors();
                    elevatorController.setDoorExtendedThisCycle(false);
                }
                break;

            case ButtonType::CloseDoor:
                elevatorController.closeDoors();
                break;

            case ButtonType::Stop:
                if (!b.pressed) {
                    b.pressed = true;
                    elevatorController.toggleStop();
                }
                else {
                    b.pressed = false;
                    elevatorController.toggleStop();
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

bool ButtonPanel::isFloorAlreadyRequested(int floorIdx, bool hasTargetFloor, int targetFloor,
                                         const std::vector<int>& floorQueue) const {
    if (hasTargetFloor && targetFloor == floorIdx) return true;
    for (int f : floorQueue) {
        if (f == floorIdx) return true;
    }
    return false;
}

