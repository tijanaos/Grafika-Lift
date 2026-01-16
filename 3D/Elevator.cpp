#include "Elevator.h"
#include <cmath>

static const float MOVE_SPEED = 2.0f;    // jedinica: world units / s
static const float DOOR_SPEED = 1.8f;    // koliko brzo doorOpen ide ka 0/1 (1/s)
static const float DOOR_OPEN_TIME = 5.0f;

Elevator::Elevator(int numFloors_, float floorHeight, int startFloorIdx)
    : numFloors(numFloors_), floorH(floorHeight),
    state(ElevatorState::Idle),
    currentFloor(clampi(startFloorIdx, 0, numFloors_ - 1)),
    targetFloor(currentFloor),
    cabinBaseY(currentFloor* floorH),
    doorOpen(0.0f),
    doorTimer(0.0f),
    doorExtendedThisCycle(false),
    ventOn(false),
    ventAutoOffFloor(currentFloor)
{
}

int Elevator::clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

bool Elevator::containsInQueue(int floorIdx) const {
    for (int f : queue) if (f == floorIdx) return true;
    return false;
}

bool Elevator::IsExactlyAtFloor(int floorIdx) const {
    float y = floorIdx * floorH;
    return std::fabs(cabinBaseY - y) < 0.001f;
}

void Elevator::RequestFloor(int floorIdx) {
    floorIdx = clampi(floorIdx, 0, numFloors - 1);

    // ako je vec target ili vec u redu - ignorisi
    if (floorIdx == targetFloor && (state == ElevatorState::Moving || state == ElevatorState::Stopped)) return;
    if (containsInQueue(floorIdx)) return;

    // ako smo idle i vec smo na tom spratu i vrata zatvorena - samo otvori
    if (state == ElevatorState::Idle && floorIdx == currentFloor && doorOpen <= 0.0f) {
        state = ElevatorState::DoorsOpening;
        return;
    }

    queue.push_back(floorIdx);

    // ako trenutno miruje sa zatvorenim vratima -> odmah kreni
    if (state == ElevatorState::Idle && doorOpen <= 0.0f) {
        startNextMoveIfAny();
    }
}

void Elevator::CallToFloor(int floorIdx) {
    RequestFloor(floorIdx);
}

void Elevator::PressOpen() {
    if (state == ElevatorState::DoorsOpen && !doorExtendedThisCycle) {
        doorTimer += DOOR_OPEN_TIME;
        doorExtendedThisCycle = true;
    }
}

void Elevator::PressClose() {
    if (state == ElevatorState::DoorsOpen) {
        doorTimer = 0.0f; // isteci odmah
    }
    if (state == ElevatorState::DoorsOpening) {
        state = ElevatorState::DoorsClosing;
    }
}

void Elevator::PressStopToggle() {
    if (state == ElevatorState::Moving) state = ElevatorState::Stopped;
    else if (state == ElevatorState::Stopped) state = ElevatorState::Moving;
}

void Elevator::ToggleVent() {
    ventOn = !ventOn;
    if (ventOn) {
        // ugasi kad stigne do prvog "sledeceg" sprata
        if (state == ElevatorState::Moving || state == ElevatorState::Stopped) {
            ventAutoOffFloor = targetFloor;
        }
        else if (!queue.empty()) {
            ventAutoOffFloor = queue.front();
        }
        else {
            ventAutoOffFloor = currentFloor;
        }
    }
}

void Elevator::startNextMoveIfAny() {
    while (!queue.empty() && queue.front() == currentFloor) {
        queue.pop_front();
        // ako je neko kliknuo "sprat na kom smo" -> samo otvori vrata
        state = ElevatorState::DoorsOpening;
        return;
    }

    if (queue.empty()) {
        state = ElevatorState::Idle;
        return;
    }

    targetFloor = queue.front();
    state = ElevatorState::Moving;
}

void Elevator::arriveAtTarget() {
    cabinBaseY = targetFloor * floorH;
    currentFloor = targetFloor;

    if (!queue.empty() && queue.front() == targetFloor) {
        queue.pop_front();
    }

    // ventilacija auto-off
    if (ventOn && currentFloor == ventAutoOffFloor) {
        ventOn = false;
    }

    // otvori vrata
    state = ElevatorState::DoorsOpening;
}

void Elevator::Update(float dt) {
    if (dt <= 0.0f) return;

    switch (state) {
    case ElevatorState::Idle:
        // ako ima nesto u redu i vrata su zatvorena, kreni
        if (doorOpen <= 0.0f && !queue.empty()) startNextMoveIfAny();
        break;

    case ElevatorState::Moving: {
        float targetY = targetFloor * floorH;
        float dir = (targetY > cabinBaseY) ? 1.0f : -1.0f;

        cabinBaseY += dir * MOVE_SPEED * dt;

        // overshoot check
        if ((dir > 0.0f && cabinBaseY >= targetY) || (dir < 0.0f && cabinBaseY <= targetY)) {
            arriveAtTarget();
        }
        break;
    }

    case ElevatorState::Stopped:
        // ne radimo nista - ceka STOP ponovo
        break;

    case ElevatorState::DoorsOpening:
        doorOpen += DOOR_SPEED * dt;
        if (doorOpen >= 1.0f) {
            doorOpen = 1.0f;
            doorTimer = DOOR_OPEN_TIME;
            doorExtendedThisCycle = false;
            state = ElevatorState::DoorsOpen;
        }
        break;

    case ElevatorState::DoorsOpen:
        doorTimer -= dt;
        if (doorTimer <= 0.0f) {
            state = ElevatorState::DoorsClosing;
        }
        break;

    case ElevatorState::DoorsClosing:
        doorOpen -= DOOR_SPEED * dt;
        if (doorOpen <= 0.0f) {
            doorOpen = 0.0f;
            state = ElevatorState::Idle;
            startNextMoveIfAny();
        }
        break;
    }
}
