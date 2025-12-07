#pragma once

#include <vector>

// Vertex structure for rendering
struct Vertex {
    float x, y;  // position in pixels
    float u, v;  // texture coordinates
};

// Floor index
enum FloorIndex {
    FLOOR_SU = 0,
    FLOOR_PR = 1,
    FLOOR_1 = 2,
    FLOOR_2 = 3,
    FLOOR_3 = 4,
    FLOOR_4 = 5,
    FLOOR_5 = 6,
    FLOOR_6 = 7
};

// Floor structure
struct Floor {
	float yBottom; // bottom edge of the platform
	float yTop;    // upper edge of the platform
};

// Elevator state enumeration
enum class ElevatorState {
    Idle,
    Moving,
    DoorsOpening,
    DoorsOpen,
    DoorsClosing,
    Stopped
};

// Elevator structure
struct Elevator {
	float x;      // bottom left corner of the cabin
	float y;      // bottom left corner of the cabin
    float width;
    float height;
    int   currentFloor;   // index in [0..7]

    ElevatorState state;  
	float speed;          // speed of elevator movement in pixels per second
	float doorOpenTimer;  // door opening/closing timer
    float doorOpenRatio; 
};

// Person structure
struct Person {
	float x;      // bottom left corner of the person
    float y;
    float width;
    float height;
    bool  inElevator;
    bool  facingRight;
};

// Button type enumeration
enum class ButtonType {
    Floor,
    OpenDoor,
    CloseDoor,
    Stop,
    Ventilation
};

// Button structure
struct Button {
    float x0, y0;
    float x1, y1;
    ButtonType type;
	int floorIndex;  // only for floor buttons, -1 otherwise
    bool pressed;
};

