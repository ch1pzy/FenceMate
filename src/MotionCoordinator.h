#pragma once

#include "MotorPair.h"

// ═══════════════════════════════════════════════════════════════════════════════
// MotionCoordinator — Manages the high-level sequencing of the Fence and Wing
//
// Enforces the sequential rule: if target > 300mm, the fence moves to 300mm 
// and completely finishes BEFORE the wing extends.
// ═══════════════════════════════════════════════════════════════════════════════

class MotionCoordinator {
public:
    MotionCoordinator(MotorPair& fence, MotorPair& wing);

    void begin();

    // Command a move to absolute target width (mm)
    void moveToWidth(float targetMm);

    // Call continually in loop(). Returns true while movement is occurring.
    bool run();

    // Emergency stop all motion
    void stop();

    // Returns the total effective cut width based on current stepper positions
    float getCurrentWidth() const;

    // Checks if the system is currently executing a move
    bool isMoving() const;

private:
    MotorPair& _fence;
    MotorPair& _wing;

    enum State {
        IDLE,
        MOVING_FENCE_ONLY,
        MOVING_FENCE_STAGE_1, // Moving fence to 300mm
        MOVING_WING_STAGE_2   // Moving wing to remaining distance
    };

    State _state;
    float _targetWidth;
};
