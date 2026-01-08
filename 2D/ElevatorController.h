#pragma once

#include "Types.h"
#include "Constants.h"
#include <vector>

// Controls elevator state and movement
class ElevatorController {
public:
    ElevatorController();
    
    void initialize(const Floor floors[FLOOR_COUNT], float elevatorX, float floorSpacing, int startFloor, float elevatorWidth, float elevatorHeight);
    
    // Update elevator state based on deltaTime
    void update(float deltaTime, const Floor floors[FLOOR_COUNT], std::vector<int>& floorQueue, 
                bool& hasTargetFloor, int& targetFloor, bool& ventilationOn,
                int floorButtonIndex[FLOOR_COUNT], std::vector<Button>& buttons,
                int ventilationButtonIndex);
    
    // Request floor (from button panel or person call)
    bool requestFloor(int floorIndex, std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor);
    
    // Door control
    void openDoors();
    void closeDoors();
    void extendDoorTimer();
    
    // Stop control
    void toggleStop();
    
    // Getters
    Elevator& getElevator() { return elevator; }
    const Elevator& getElevator() const { return elevator; }
    
    bool isDoorExtendedThisCycle() const { return doorExtendedThisCycle; }
    void setDoorExtendedThisCycle(bool value) { doorExtendedThisCycle = value; }

private:
    Elevator elevator;
    bool doorExtendedThisCycle;
    
    void processMovingState(float deltaTime, const Floor floors[FLOOR_COUNT], 
                           std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor,
                           bool& ventilationOn, int floorButtonIndex[FLOOR_COUNT],
                           std::vector<Button>& buttons, int ventilationButtonIndex);
    void processDoorsOpeningState(float deltaTime);
    void processDoorsOpenState(float deltaTime);
    void processDoorsClosingState(float deltaTime);
};

