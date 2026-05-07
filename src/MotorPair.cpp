#include "MotorPair.h"

MotorPair::MotorPair(
    uint8_t frontStep,  uint8_t frontDir,  uint8_t frontEnable,
    uint8_t rearStep,   uint8_t rearDir,   uint8_t rearEnable,
    uint8_t frontEndstopPin, uint8_t rearEndstopPin,
    float   stepsPerMm,
    float   maxSpeed, float acceleration, float homingSpeed,
    int8_t  homeDir
) :
    _front(AccelStepper::DRIVER, frontStep, frontDir),
    _rear (AccelStepper::DRIVER, rearStep,  rearDir),
    _frontEnablePin(frontEnable),
    _rearEnablePin(rearEnable),
    _frontEndstopPin(frontEndstopPin),
    _rearEndstopPin(rearEndstopPin),
    _stepsPerMm(stepsPerMm),
    _maxSpeed(maxSpeed),
    _acceleration(acceleration),
    _homingSpeed(homingSpeed),
    _homeDir(homeDir)
{
    _front.setMaxSpeed(maxSpeed);
    _front.setAcceleration(acceleration);
    _rear.setMaxSpeed(maxSpeed);
    _rear.setAcceleration(acceleration);
}

void MotorPair::begin() {
    pinMode(_frontEnablePin,    OUTPUT);
    pinMode(_rearEnablePin,     OUTPUT);
    pinMode(_frontEndstopPin,   INPUT_PULLUP);
    pinMode(_rearEndstopPin,    INPUT_PULLUP);
    disable();  // Start with motors disabled (de-energised)
}

void MotorPair::moveTo(float mm) {
    long target = _mmToSteps(mm);
    enable();
    _front.moveTo(target);
    _rear.moveTo(target);
}

bool MotorPair::run() {
    bool f = _front.run();
    bool r = _rear.run();
    return f || r;
}

bool MotorPair::isRunning() {
    return _front.isRunning() || _rear.isRunning();
}

// GRBL-style auto-square homing:
//   Both motors drive simultaneously toward their endstops.
//   Each motor stops independently the moment its own endstop triggers.
//   Any difference in trigger timing naturally squares the axis.
bool MotorPair::home() {
    // Temporarily set fast acceleration for homing approach
    _front.setMaxSpeed(_homingSpeed);
    _front.setAcceleration(_homingSpeed * 2.0f);
    _rear.setMaxSpeed(_homingSpeed);
    _rear.setAcceleration(_homingSpeed * 2.0f);

    // Command a very large move in homing direction (endstop will stop it)
    long bigMove = (long)(_homeDir * _stepsPerMm * 700.0f);
    _front.moveTo(bigMove);
    _rear.moveTo(bigMove);

    enable();

    bool frontDone = false;
    bool rearDone  = false;
    unsigned long startMs = millis();

    while (!frontDone || !rearDone) {
        if (millis() - startMs > HOMING_TIMEOUT_MS) {
            stop();
            disable();
            return false;  // Timeout — endstop not reached
        }
        if (!frontDone) {
            if (digitalRead(_frontEndstopPin) == ENDSTOP_TRIGGERED) {
                _front.setCurrentPosition(_front.currentPosition());
                _front.stop();
                frontDone = true;
            } else {
                _front.run();
            }
        }
        if (!rearDone) {
            if (digitalRead(_rearEndstopPin) == ENDSTOP_TRIGGERED) {
                _rear.setCurrentPosition(_rear.currentPosition());
                _rear.stop();
                rearDone = true;
            } else {
                _rear.run();
            }
        }
    }

    // Zero both positions at home
    _front.setCurrentPosition(0);
    _rear.setCurrentPosition(0);

    // Restore normal operating speeds
    _front.setMaxSpeed(_maxSpeed);
    _front.setAcceleration(_acceleration);
    _rear.setMaxSpeed(_maxSpeed);
    _rear.setAcceleration(_acceleration);

    disable();
    return true;
}

float MotorPair::getPosition() {
    return _stepsToMm(_front.currentPosition());
}

void MotorPair::setPosition(float mm) {
    long s = _mmToSteps(mm);
    _front.setCurrentPosition(s);
    _rear.setCurrentPosition(s);
}

void MotorPair::stop() {
    _front.stop();
    _rear.stop();
}

void MotorPair::enable() {
    digitalWrite(_frontEnablePin, MOTOR_ENABLE_ACTIVE);
    digitalWrite(_rearEnablePin,  MOTOR_ENABLE_ACTIVE);
}

void MotorPair::disable() {
    digitalWrite(_frontEnablePin, MOTOR_ENABLE_IDLE);
    digitalWrite(_rearEnablePin,  MOTOR_ENABLE_IDLE);
}

// ── Skew calibration — single-motor baby-stepping ────────────────────────────

void MotorPair::skewFront(float deltaMm) {
    enable();
    _front.moveTo(_front.currentPosition() + _mmToSteps(deltaMm));
    while (_front.run());   // Blocking — delta is always tiny (<1mm)
    // NOTE: rear motor is NOT moved — this is intentional for skew correction
    disable();
}

void MotorPair::skewRear(float deltaMm) {
    enable();
    _rear.moveTo(_rear.currentPosition() + _mmToSteps(deltaMm));
    while (_rear.run());    // Blocking — delta is always tiny (<1mm)
    // NOTE: front motor is NOT moved — this is intentional for skew correction
    disable();
}

// ── Private helpers ───────────────────────────────────────────────────────────

long MotorPair::_mmToSteps(float mm) const {
    return (long)(mm * _stepsPerMm);
}

float MotorPair::_stepsToMm(long steps) const {
    return (float)steps / _stepsPerMm;
}
