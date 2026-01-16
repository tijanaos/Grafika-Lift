#pragma once
#include <deque>

enum class ElevatorState {
    Idle,
    Moving,
    DoorsOpening,
    DoorsOpen,
    DoorsClosing,
    Stopped
};

class Elevator {
public:
    Elevator(int numFloors, float floorHeight, int startFloorIdx);

    void Update(float dt);

    void RequestFloor(int floorIdx);  // dodaj sprat u red (FIFO)
    void CallToFloor(int floorIdx);   // isto kao RequestFloor za sada

    void PressOpen();        // produzi otvorena vrata +5s (samo jednom po ciklusu)
    void PressClose();       // odmah zatvori
    void PressStopToggle();  // pauza/resume dok se kreće
    void ToggleVent();       // ukljuci/iskljuci ventilaciju (auto-off na prvom target spratu)

    int CurrentFloor() const { return currentFloor; }
    float CabinBaseY() const { return cabinBaseY; }
    float DoorOpen() const { return doorOpen; }   // 0..1
    bool VentOn() const { return ventOn; }
    ElevatorState State() const { return state; }

    bool IsExactlyAtFloor(int floorIdx) const; // kabina tacno na spratu (koristi se za spoljna vrata)

private:
    int numFloors;
    float floorH;

    ElevatorState state;

    int currentFloor;
    int targetFloor;

    float cabinBaseY; // donja ivica kabine (Y)
    float doorOpen;   // 0..1

    std::deque<int> queue;

    // tajmeri / flagovi
    float doorTimer;            // koliko jos drzimo otvoreno
    bool doorExtendedThisCycle; // samo jednom po otvaranju

    // ventilacija
    bool ventOn;
    int ventAutoOffFloor; // na kom spratu se gasi

private:
    void startNextMoveIfAny();
    void arriveAtTarget();
    bool containsInQueue(int floorIdx) const;
    static int clampi(int v, int lo, int hi);
};
