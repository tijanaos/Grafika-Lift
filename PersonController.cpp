#include "PersonController.h"
#include <cmath>
#include <iostream>

PersonController::PersonController() : personFloorIndex(FLOOR_PR), personSpeed(350.0f) {
}

void PersonController::initialize(const Floor floors[FLOOR_COUNT], float corridorLeftX, float elevatorHeight, int startFloor) {
    person.height = elevatorHeight * 0.8f;       // čovek ~80% kabine
    person.width = person.height * 0.6f;         // proporcije tela
    person.inElevator = false;
    person.facingRight = true;  // na pocetku neka gleda u desno
    personFloorIndex = startFloor;
    person.y = floors[startFloor].yTop;   // stoji na platformi tog sprata
    person.x = corridorLeftX + 40.0f;      // nek bude negde levo od lifta
}

void PersonController::update(float deltaTime, GLFWwindow* window, const Floor floors[FLOOR_COUNT],
                             const Elevator& elevator, float corridorLeftX, float corridorMaxXOutside) {
    // 1) Kretanje osobe: A = levo, W = desno
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

    // 2) Osnovni opseg hodnika na spratu na kom je osoba (kada je VAN lifta)
    float corridorMinX = corridorLeftX + 10.0f;

    if (!person.inElevator) {
        if (person.x < corridorMinX) person.x = corridorMinX;
        if (person.x > corridorMaxXOutside) person.x = corridorMaxXOutside;

        // VAN lifta: stojimo TAČNO na spratu
        person.y = floors[personFloorIndex].yTop;
    }
    else {
        float insideMinX = elevator.x + 5.0f;
        float insideMaxX = elevator.x + elevator.width - person.width - 5.0f;
        if (person.x < insideMinX) person.x = insideMinX;
        if (person.x > insideMaxX) person.x = insideMaxX;

        // U LIFTU: blago podignuta da ne "propada" kroz dno lifta
        person.y = elevator.y + PERSON_FLOOR_OFFSET;
    }
}

bool PersonController::handleElevatorCall(GLFWwindow* window, const Elevator& elevator,
                                         std::vector<int>& floorQueue, bool& hasTargetFloor, int& targetFloor) {
    // Taster C – pozivanje lifta (edge detect: tek kad se pritisne)
    static bool cWasPressed = false;
    bool cIsPressed = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    bool cJustPressed = cIsPressed && !cWasPressed;
    cWasPressed = cIsPressed;

    if (cJustPressed && !person.inElevator && isInFrontOfElevator(elevator)) {
        // 1) Lift je vec na mom spratu -> samo upravljamo vratima
        if (elevator.currentFloor == personFloorIndex) {
            return true; // Signal that doors should be opened
        }
        // 2) Lift NIJE na mom spratu → standardno ubaci zahtev
        else {
            if (!hasTargetFloor && elevator.state == ElevatorState::Idle) {
                // lift miruje – kreni odmah ka tom spratu
                targetFloor = personFloorIndex;
                hasTargetFloor = true;
            }
            else {
                // već se vozi – ubaci sprat u red
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

    // 6) Ako su vrata otvorena i lift je na spratu osobe, dozvoli ulazak
    if (!person.inElevator && doorsAreOpen && elevatorAtPersonsFloor && inFrontOfElevator) {
        person.inElevator = true;
        // pozicioniramo osobu unutar kabine
        person.x = elevator.x + 10.0f;
        person.y = elevator.y + PERSON_FLOOR_OFFSET;
    }

    // 7) Ako je osoba u kabini i vrata su otvorena, dozvoli izlazak levo
    if (person.inElevator && doorsAreOpen) {
        float insideMinX = elevator.x + 5.0f;
        // ako drži A i približi se levoj strani, izađe na hodnik
        // Note: This requires GLFW window access, so we'll handle it in Main.cpp
    }
}

bool PersonController::isInFrontOfElevator(const Elevator& elevator) const {
    float personRight = person.x + person.width;
    float elevatorLeft = elevator.x;
    return std::fabs(personRight - elevatorLeft) < 2.0f; // tolerancija ~2px
}

