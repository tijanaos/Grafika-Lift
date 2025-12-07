#include "ElevatorController.h"
#include "ButtonPanel.h"
#include <iostream>

ElevatorController::ElevatorController() : doorExtendedThisCycle(false) {
}

void ElevatorController::initialize(const Floor floors[FLOOR_COUNT], float elevatorX, float floorSpacing, int startFloor, float elevatorWidth, float elevatorHeight) {
    elevator.x = elevatorX;
    elevator.width = elevatorWidth;
    elevator.height = elevatorHeight;
    elevator.currentFloor = startFloor;
    elevator.y = floors[startFloor].yTop;
    elevator.state = ElevatorState::Idle;
    elevator.speed = 260.0f;
    elevator.doorOpenTimer = 0.0f;
    elevator.doorOpenRatio = 0.0f;
    doorExtendedThisCycle = false;
}

void ElevatorController::update(float deltaTime, const Floor floors[FLOOR_COUNT], std::vector<int>& floorQueue,
                               bool& hasTargetFloor, int& targetFloor, bool& ventilationOn,
                               int floorButtonIndex[FLOOR_COUNT], std::vector<Button>& buttons,
                               int ventilationButtonIndex) {
	// If elevator is in  idle and has no target floor,
	// something is in the waiting queue, take the next floor from the queue.
    if (elevator.state == ElevatorState::Idle &&
        !hasTargetFloor &&
        !floorQueue.empty()) {
        targetFloor = floorQueue.front();
        floorQueue.erase(floorQueue.begin());
        hasTargetFloor = true;
    }

	// If elevator is idle and has a target floor different from current, start moving
    if (elevator.state == ElevatorState::Idle &&
        hasTargetFloor && targetFloor != elevator.currentFloor) {
        elevator.state = ElevatorState::Moving;
    }

    switch (elevator.state) {
    case ElevatorState::Moving:
        processMovingState(deltaTime, floors, floorQueue, hasTargetFloor, targetFloor,
                          ventilationOn, floorButtonIndex, buttons, ventilationButtonIndex);
        break;
    case ElevatorState::DoorsOpening:
        processDoorsOpeningState(deltaTime);
        break;
    case ElevatorState::DoorsOpen:
        processDoorsOpenState(deltaTime);
        break;
    case ElevatorState::DoorsClosing:
        processDoorsClosingState(deltaTime);
        break;
    case ElevatorState::Stopped:
		// stopped, nothing to do
        break;
    case ElevatorState::Idle:
    default:
		// nothing to do
        break;
    }
}

void ElevatorController::processMovingState(float deltaTime, const Floor floors[FLOOR_COUNT],
                                           std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor,
                                           bool& ventilationOn, int floorButtonIndex[FLOOR_COUNT],
                                           std::vector<Button>& buttons, int ventilationButtonIndex) {
    float targetY = floors[targetFloor].yTop;
    float dir = (targetY > elevator.y) ? 1.0f : -1.0f;
    float step = elevator.speed * deltaTime * dir;
    elevator.y += step;

	// so it does not overshoot the target
    if ((dir > 0.0f && elevator.y >= targetY) ||
        (dir < 0.0f && elevator.y <= targetY)) {
        elevator.y = targetY;
        elevator.currentFloor = targetFloor;

		// unpress the floor button for the arrived floor
        if (targetFloor >= 0 && targetFloor < FLOOR_COUNT) {
            int fb = floorButtonIndex[targetFloor];
            if (fb >= 0 && fb < (int)buttons.size()) {
                buttons[fb].pressed = false;
            }
        }

		// ventilation shuts down when elevator arrives
        ventilationOn = false;
        if (ventilationButtonIndex >= 0 && ventilationButtonIndex < (int)buttons.size()) {
            buttons[ventilationButtonIndex].pressed = false;
        }

		// take next target floor from the queue, if any
        if (!floorQueue.empty()) {
            targetFloor = floorQueue.front();
            floorQueue.erase(floorQueue.begin());
            hasTargetFloor = true;
        }
        else {
            hasTargetFloor = false;
        }

		// start door opening animation
        elevator.state = ElevatorState::DoorsOpening;
        elevator.doorOpenRatio = 0.0f;
        doorExtendedThisCycle = false;
    }
}

void ElevatorController::processDoorsOpeningState(float deltaTime) {
    elevator.doorOpenRatio += deltaTime / DOOR_ANIM_DURATION;
    if (elevator.doorOpenRatio >= 1.0f) {
        elevator.doorOpenRatio = 1.0f;
        elevator.state = ElevatorState::DoorsOpen;
        elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
		doorExtendedThisCycle = false; // new cycle of door open
    }
}

void ElevatorController::processDoorsOpenState(float deltaTime) {
    elevator.doorOpenTimer -= deltaTime;
    if (elevator.doorOpenTimer <= 0.0f) {
        elevator.doorOpenTimer = 0.0f;
        elevator.state = ElevatorState::DoorsClosing;
    }
}

void ElevatorController::processDoorsClosingState(float deltaTime) {
    elevator.doorOpenRatio -= deltaTime / DOOR_ANIM_DURATION;
    if (elevator.doorOpenRatio <= 0.0f) {
        elevator.doorOpenRatio = 0.0f;
        elevator.state = ElevatorState::Idle;
    }
}

bool ElevatorController::requestFloor(int floorIndex, std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor) {
    if (floorIndex == elevator.currentFloor) {
		// Elevator is already on my floor -> only manage doors
        if (elevator.state == ElevatorState::Idle ||
            elevator.state == ElevatorState::DoorsClosing) {
            openDoors();
        }
        else if (elevator.state == ElevatorState::DoorsOpen) {
            extendDoorTimer();
        }
        return true;
    }
    else {
		// Elevator is NOT on my floor -> standardly add request
        if (!hasTargetFloor) {
            if (elevator.state == ElevatorState::Idle) {
				// elevator idle -> set target floor and start moving
                targetFloor = floorIndex;
                hasTargetFloor = true;
            }
            else if (elevator.state == ElevatorState::DoorsOpen || 
                     elevator.state == ElevatorState::DoorsClosing ||
                     elevator.state == ElevatorState::DoorsOpening) {
				// doors are open/closing -> set target floor
                targetFloor = floorIndex;
                hasTargetFloor = true;
            }
            else {
				// already moving -> add floor to queue
                floorQueue.push_back(floorIndex);
            }
        }
        else {
			// already has target floor -> add to queue
            floorQueue.push_back(floorIndex);
        }
        std::cout << "Lift pozvan na sprat: " << floorIndex << std::endl;
        return true;
    }
}

void ElevatorController::openDoors() {
    elevator.state = ElevatorState::DoorsOpening;
    elevator.doorOpenRatio = 0.0f;
    elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
    doorExtendedThisCycle = false;
}

void ElevatorController::closeDoors() {
    if (elevator.state == ElevatorState::DoorsOpen) {
        elevator.doorOpenTimer = 0.0f;
        elevator.state = ElevatorState::DoorsClosing;
    }
}

void ElevatorController::extendDoorTimer() {
    elevator.doorOpenTimer = BASE_DOOR_OPEN_TIME;
}

void ElevatorController::toggleStop() {
    if (elevator.state == ElevatorState::Moving) {
        elevator.state = ElevatorState::Stopped;
    }
    else if (elevator.state == ElevatorState::Stopped) {
        elevator.state = ElevatorState::Idle;
    }
}

