#pragma once

#include "Types.h"
#include "Constants.h"
#include <vector>

// Forward declaration
class ElevatorController;

// Manages button panel layout and interactions
class ButtonPanel {
public:
    ButtonPanel();
    
    void initialize(int screenWidth, int screenHeight);
    
    // Handle mouse click on buttons
    void handleClick(float mouseX, float mouseY, bool personInElevator,
                    ElevatorController& elevatorController, bool& ventilationOn,
                    std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor);
    
    // Check if floor is already requested
    bool isFloorAlreadyRequested(int floorIdx, bool hasTargetFloor, int targetFloor,
                                 const std::vector<int>& floorQueue) const;
    
    // Getters
    std::vector<Button>& getButtons() { return buttons; }
    const std::vector<Button>& getButtons() const { return buttons; }
    
    int getFloorButtonIndex(int floor) const { return floorButtonIndex[floor]; }
    int getOpenButtonIndex() const { return openButtonIndex; }
    int getCloseButtonIndex() const { return closeButtonIndex; }
    int getStopButtonIndex() const { return stopButtonIndex; }
    int getVentilationButtonIndex() const { return ventilationButtonIndex; }

private:
    std::vector<Button> buttons;
    int floorButtonIndex[FLOOR_COUNT];
    int openButtonIndex;
    int closeButtonIndex;
    int stopButtonIndex;
    int ventilationButtonIndex;
    
    void createButtons(int screenWidth, int screenHeight);
    int addButton(float x0, float yBottom, float x1, float yTop, ButtonType type, int floorIdx);
};

