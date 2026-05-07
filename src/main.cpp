#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "Settings.h"
#include "MotorPair.h"
#include "MotionCoordinator.h"
#include "UI.h"

// ── Hardware Instantiation ───────────────────────────────────────────────────

MotorPair fence(
    FENCE_FRONT_STEP, FENCE_FRONT_DIR, FENCE_FRONT_ENABLE,
    FENCE_REAR_STEP,  FENCE_REAR_DIR,  FENCE_REAR_ENABLE,
    FENCE_FRONT_ENDSTOP, FENCE_REAR_ENDSTOP,
    FENCE_STEPS_PER_MM,
    FENCE_MAX_SPEED, FENCE_ACCELERATION, FENCE_HOMING_SPEED,
    1 // Home outward (towards max travel)
);

MotorPair wing(
    WING_FRONT_STEP, WING_FRONT_DIR, WING_FRONT_ENABLE,
    WING_REAR_STEP,  WING_REAR_DIR,  WING_REAR_ENABLE,
    WING_FRONT_ENDSTOP, WING_REAR_ENDSTOP,
    WING_STEPS_PER_MM,
    WING_MAX_SPEED, WING_ACCELERATION, WING_HOMING_SPEED,
    -1 // Home inward (towards min travel)
);

MotionCoordinator coordinator(fence, wing);
UIManager ui(coordinator);

// ── Interrupt Service Routine for Rotary Encoder ─────────────────────────────
void encoderISR() {
    ui.tickEncoder();
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    #ifdef DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println("FenceMate v2 Booting...");
    #endif

    Wire.begin(); // Required for I2C OLED

    // Initialize EEPROM settings
    Settings.begin();

    // Initialize motor driver pins
    fence.begin();
    wing.begin();

    // Initialize state logic
    coordinator.begin();

    // Initialize OLED and UI
    ui.begin();

    // Attach interrupts for encoder (Pins 2 & 3 are INT0/INT1 on ATmega2560/1280)
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_DT), encoderISR, CHANGE);

    // Show Boot Prompt
    ui.showBootPrompt();
}

// ── State Tracking ─────────────────────────────────────────────────────────────
bool _isHoming = false;

// ── Main Loop ────────────────────────────────────────────────────────────────
void loop() {
    // 1. Update UI and handle inputs
    ui.update();

    // 2. Handle Homing Request from UI
    if (ui.isHomingRequested() && !_isHoming) {
        _isHoming = true;
        
        // Ensure UI updates to show "Homing Active..."
        ui.redraw();

        // Perform GRBL-style Auto-Square Homing
        // 1. Home Fence Pair
        bool fenceHomed = fence.home();
        
        // 2. Home Wing Pair
        bool wingHomed = wing.home();

        if (fenceHomed && wingHomed) {
            Settings.setHomedFlag(true);

            // Apply Skew Offsets if any
            float fenceSkew = Settings.getFenceSkew();
            if (fenceSkew != 0.0f) {
                // Apply skew (baby-stepping one motor)
                // For example, skewFront moves only the front motor
                fence.skewFront(fenceSkew);
                // After baby-stepping, re-zero to lock the squared position
                fence.setPosition(0.0f);
            }

            float wingSkew = Settings.getWingSkew();
            if (wingSkew != 0.0f) {
                wing.skewFront(wingSkew);
                wing.setPosition(0.0f);
            }
            
            // Move to safe ready position (e.g. Fence at max, Wing retracted)
            // Fence homes outward to FENCE_MAX_TRAVEL_MM
            fence.setPosition(FENCE_MAX_TRAVEL_MM); 
            // Wing homes inward to 0.0f
            wing.setPosition(0.0f); 
        }

        _isHoming = false;
        ui.notifyHomingComplete();
    }

    // 3. Process active motions
    if (!_isHoming) {
        coordinator.run();
    }
}