#pragma once

// Global constants
const int FLOOR_COUNT = 8; // SU, PR, 1, 2, 3, 4, 5, 6
const float DOOR_ANIM_DURATION = 0.3f;   // open/close door animation in seconds
const float BASE_DOOR_OPEN_TIME = 5.0f;  
const float PERSON_FLOOR_OFFSET = 10.0f;  // when person is on a floor, offset from the floor y position

// FPS limiter
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

