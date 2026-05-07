#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// MotorPair — Synchronised pair of AccelStepper motors
//
// SAFETY RULE: Both motors in a pair ALWAYS move together.
// Desynchronised movement WILL damage the physical mechanism.
// The ONLY exception is skewFront() / skewRear() during calibration,
// which move a single motor by tiny baby-step increments.
// ═══════════════════════════════════════════════════════════════════════════════

class MotorPair {
public:
    // homeDir: +1 = homes toward max travel (fence), -1 = homes toward min (wing)
    MotorPair(
        uint8_t frontStep,  uint8_t frontDir,  uint8_t frontEnable,
        uint8_t rearStep,   uint8_t rearDir,   uint8_t rearEnable,
        uint8_t frontEndstopPin, uint8_t rearEndstopPin,
        float   stepsPerMm,
        float   maxSpeed, float acceleration, float homingSpeed,
        int8_t  homeDir
    );

    // Call once in setup()
    void begin();

    // Move both motors to absolute position (mm). Non-blocking.
    void moveTo(float mm);

    // Call every loop() iteration. Returns true while either motor is moving.
    bool run();

    bool isRunning();

    // GRBL auto-square homing: both motors start simultaneously,
    // each stops independently when its own endstop triggers.
    // Returns true on success, false on timeout.
    bool home();

    float getPosition();          // Current position in mm (front motor)
    void  setPosition(float mm);        // Force-set position without moving
    void  stop();                       // Emergency stop both motors
    void  enable();
    void  disable();

    // Skew calibration ONLY — moves a single motor by a tiny delta (mm).
    // Do NOT use during normal operation.
    void skewFront(float deltaMm);
    void skewRear(float deltaMm);

private:
    AccelStepper _front;
    AccelStepper _rear;

    uint8_t _frontEnablePin;
    uint8_t _rearEnablePin;
    uint8_t _frontEndstopPin;
    uint8_t _rearEndstopPin;

    float   _stepsPerMm;
    float   _maxSpeed;
    float   _acceleration;
    float   _homingSpeed;
    int8_t  _homeDir;

    long  _mmToSteps(float mm)    const;
    float _stepsToMm(long steps)  const;
};
