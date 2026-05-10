#include "MotionCoordinator.h"
#include "config.h"

MotionCoordinator::MotionCoordinator(MotorPair& fence, MotorPair& wing) 
    : _fence(fence), _wing(wing), _state(IDLE), _targetWidth(0.0f) 
{
}

void MotionCoordinator::begin() {
    _state = IDLE;
}

void MotionCoordinator::moveToWidth(float targetMm) {
    _targetWidth = targetMm;

    // Safety limit checks
    if (_targetWidth > FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM) {
        _targetWidth = FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM;
    }
    if (_targetWidth < 0.0f) {
        _targetWidth = 0.0f;
    }

    if (_targetWidth <= WING_THRESHOLD_MM) {
        // Simple case: Fence only, retract wing completely
        _fence.moveTo(_targetWidth);
        _wing.moveTo(0.0f);
        _state = MOVING_FENCE_ONLY;
    } else {
        // Complex case: sequential move. Fence to threshold first.
        _fence.moveTo(WING_THRESHOLD_MM);
        _state = MOVING_FENCE_STAGE_1;
    }
}

bool MotionCoordinator::run() {
    bool fenceRunning = _fence.run();
    bool wingRunning = _wing.run();

    switch (_state) {
        case IDLE:
            break;

        case MOVING_FENCE_ONLY:
            if (!fenceRunning && !wingRunning) {
                _state = IDLE;
                _fence.disable();
                _wing.disable();
            }
            break;

        case MOVING_FENCE_STAGE_1:
            // Waiting for fence to finish its move to WING_THRESHOLD_MM
            if (!fenceRunning) {
                // Stage 1 complete. Now start Stage 2: Wing extension.
                // At this point, the fence is at the threshold. Endstop verification
                // could be done here if implemented.
                float wingTarget = _targetWidth - WING_THRESHOLD_MM;
                _wing.moveTo(wingTarget);
                _state = MOVING_WING_STAGE_2;
            }
            break;

        case MOVING_WING_STAGE_2:
            // Waiting for wing to finish its move
            if (!wingRunning) {
                _state = IDLE;
                _fence.disable();
                _wing.disable();
            }
            break;
    }

    return (_state != IDLE);
}

void MotionCoordinator::stop() {
    _fence.stop();
    _wing.stop();
    _state = IDLE;
}

float MotionCoordinator::getCurrentWidth() const {
    float fPos = _fence.getPosition();
    float wPos = _wing.getPosition();

    // If fence hasn't reached threshold, effective width is just fence
    // If fence is at threshold, effective width is fence + wing
    return fPos + wPos;
}

bool MotionCoordinator::isMoving() const {
    return _state != IDLE;
}

void MotionCoordinator::skewFence(float deltaMm) {
    _fence.skewFront(deltaMm);
}

void MotionCoordinator::skewWing(float deltaMm) {
    _wing.skewFront(deltaMm);
}
