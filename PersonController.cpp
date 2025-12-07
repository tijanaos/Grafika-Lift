#include "PersonController.h"
#include <cmath>
#include <iostream>

PersonController::PersonController() : personFloorIndex(FLOOR_PR), personSpeed(350.0f) {
}

void PersonController::initialize(const Floor floors[FLOOR_COUNT], float corridorLeftX, float elevatorHeight, int startFloor) {
	person.height = elevatorHeight * 0.8f;       // person is 80% of elevator height
    person.width = person.height * 0.6f;
    person.inElevator = false;
	person.facingRight = true;  // at start, facing right
    personFloorIndex = startFloor;
	person.y = floors[startFloor].yTop;   // standing on the top of the floor
	person.x = corridorLeftX + 40.0f;      // away from the elevator, on the left side
}

void PersonController::update(float deltaTime, GLFWwindow* window, const Floor floors[FLOOR_COUNT],
                             const Elevator& elevator, float corridorLeftX, float corridorMaxXOutside) {
	// 1) Movement: A and W for left/right
    float dx = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        dx -= personSpeed * deltaTime;
        person.facingRight = false;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        dx += personSpeed * deltaTime;
        person.facingRight = true;
    }

    person.x += dx;

	// 2) Cooridor boundaries
    float corridorMinX = corridorLeftX + 10.0f;

	// 3) Constrain person position based on whether they are inside or outside the elevator
	if (!person.inElevator) { // allow corridor movement
        if (person.x < corridorMinX) person.x = corridorMinX;
        if (person.x > corridorMaxXOutside) person.x = corridorMaxXOutside;

		// OUTSIDE ELEVATOR: standing on the floor
        person.y = floors[personFloorIndex].yTop;
    }
	else { // allow only elevator movement
        float insideMinX = elevator.x + 5.0f;
        float insideMaxX = elevator.x + elevator.width - person.width - 5.0f;
        if (person.x < insideMinX) person.x = insideMinX;
        if (person.x > insideMaxX) person.x = insideMaxX;

		// INSIDE ELEVATOR: align with elevator y position with an offset
        person.y = elevator.y + PERSON_FLOOR_OFFSET;
    }
}

bool PersonController::handleElevatorCall(GLFWwindow* window, const Elevator& elevator,
                                         std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor) {
	// C key press detection, call elevator
    static bool cWasPressed = false;
    bool cIsPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    bool cJustPressed = cIsPressed && !cWasPressed;
    cWasPressed = cIsPressed;

    if (cJustPressed && !person.inElevator && isInFrontOfElevator(elevator)) {
		// 1) Elevator is already on my floor -> just manage doors
        if (elevator.currentFloor == personFloorIndex) {
            return true; // Signal that doors should be opened
        }
		// 2) Elvator is not on my floor -> call it
        else {
            if (!hasTargetFloor && elevator.state == ElevatorState::Idle) {
				// elvator idle - go directly to my floor
                targetFloor = personFloorIndex;
                hasTargetFloor = true;
            }
            else {
				// already has a target floor - queue my floor if not already queued
                floorQueue.push_back(personFloorIndex);
            }
            std::cout << "Lift pozvan na sprat: " << personFloorIndex << std::endl;
            return true;
        }
    }
    return false;
}


void PersonController::handleElevatorInteraction(const Elevator& elevator, const Floor floors[FLOOR_COUNT]) {
    bool doorsAreOpen = (elevator.state == ElevatorState::DoorsOpen);
    bool elevatorAtPersonsFloor = (elevator.currentFloor == personFloorIndex);
    bool inFrontOfElevator = isInFrontOfElevator(elevator);

	// 6) If the person is not in the elevator, the doors are open, the elevator is at the person's floor
    if (!person.inElevator && doorsAreOpen && elevatorAtPersonsFloor && inFrontOfElevator) {
        person.inElevator = true;
		// positioning person inside the elevator
        person.x = elevator.x + 10.0f;
        person.y = elevator.y + PERSON_FLOOR_OFFSET;
    }

	// 7) If the person is in the elevator and the doors are open allow them to exit
    if (person.inElevator && doorsAreOpen) {
        float insideMinX = elevator.x + 5.0f;
		// if he is pressing A and is near the left door
        // Note: This requires GLFW window access, so it is in Main.cpp
    }
}

bool PersonController::isInFrontOfElevator(const Elevator& elevator) const {
    float personRight = person.x + person.width;
    float elevatorLeft = elevator.x;
	return std::fabs(personRight - elevatorLeft) < 2.0f; // tolerance of 2 pixels
}

