#pragma once

#include "Types.h"
#include "Constants.h"
#include <GLFW/glfw3.h>

// Controls person movement and interaction with elevator
class PersonController {
public:
    PersonController();
    
    void initialize(const Floor floors[FLOOR_COUNT], float corridorLeftX, float elevatorHeight, int startFloor);
    
    // Update person position and handle input
    void update(float deltaTime, GLFWwindow* window, const Floor floors[FLOOR_COUNT],
                const Elevator& elevator, float corridorLeftX, float corridorMaxXOutside);
    
    // Handle elevator call (C key)
    bool handleElevatorCall(GLFWwindow* window, const Elevator& elevator, 
                           std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor);
    
    // Handle entering/exiting elevator
    void handleElevatorInteraction(const Elevator& elevator, const Floor floors[FLOOR_COUNT]);
    
    // Getters
    Person& getPerson() { return person; }
    const Person& getPerson() const { return person; }
    int getCurrentFloor() const { return personFloorIndex; }
    
    // Setters
    void setCurrentFloor(int floor) { personFloorIndex = floor; }

private:
    Person person;
    int personFloorIndex;
    float personSpeed;
    
    bool isInFrontOfElevator(const Elevator& elevator) const;
};

