#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// FenceMate v2 — Central Configuration
// Board: Geeetech GT2560 Rev A (ATmega1280)
// ALL pin assignments and tunable constants live here.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Stepper Pins (GT2560 Rev A) ───────────────────────────────────────────────
// Fence uses X (front) and Z (rear) driver slots
#define FENCE_FRONT_STEP     25      //75
#define FENCE_FRONT_DIR      23      //77
#define FENCE_FRONT_ENABLE   27      //73

#define FENCE_REAR_STEP      37     //53
#define FENCE_REAR_DIR       39     //70
#define FENCE_REAR_ENABLE    35     //55

// Wing uses Y (front) and E0 (rear) driver slots
#define WING_FRONT_STEP      31     //59
#define WING_FRONT_DIR       33     //57
#define WING_FRONT_ENABLE    29     //71

#define WING_REAR_STEP       43     //41
#define WING_REAR_DIR        45     //39
#define WING_REAR_ENABLE     41     //51

// ── Endstop Pins (optical, LOW = triggered) ───────────────────────────────────
// Fence homes OUTWARD (max travel, away from blade) → X-MAX, Z-MAX
#define FENCE_FRONT_ENDSTOP  24   // X-MAX  76
#define FENCE_REAR_ENDSTOP   32   // Z-MAX  58

// Wing homes INWARD (retracted / root position) → Y-MIN, Y-MAX
#define WING_FRONT_ENDSTOP   26   // Y-MIN  74
#define WING_REAR_ENDSTOP    28   // Y-MAX  72

// Spare endstop pins (available for safety limit switches later)
// X-MIN = 22,  Z-MIN = 30

// ── Encoder & Buttons ─────────────────────────────────────────────────────────
#define ENC_CLK       50 //2   // Rotary encoder CLK — INT0 (interrupt-capable)
#define ENC_DT        52 //3   // Rotary encoder DT  — INT1 (interrupt-capable)
#define ENC_SW        42 //4   // Rotary encoder push button
#define BTN_BACK      12 //5   // Back / cancel button
#define BTN_CONFIRM   13 //6   // Confirm / execute button
 
// I²C: SDA = pin 20, SCL = pin 21  (hardware I²C, Wire handles automatically)

// ── Motor Enable Polarity ─────────────────────────────────────────────────────
#define MOTOR_ENABLE_ACTIVE  LOW   // A4988/DRV8825: LOW = enabled
#define MOTOR_ENABLE_IDLE    HIGH

// ── Endstop Polarity ──────────────────────────────────────────────────────────
#define ENDSTOP_TRIGGERED    LOW   // Optical endstop: LOW when beam broken

// ── Motion: Steps Per mm (1/16 microstepping on all drivers) ─────────────────
// Fence: Rack & Pinion, Module 1, 25-tooth → 8 steps/mm × 16 = 128
#define FENCE_STEPS_PER_MM    128.0f
// Wing: Leadscrew 8mm Ø, 2mm pitch, 4mm lead → 50 steps/mm × 16 = 800
#define WING_STEPS_PER_MM     800.0f

// ── Motion: Travel Limits ─────────────────────────────────────────────────────
#define WING_THRESHOLD_MM     300.0f   // Below this: fence-only. Above: fence+wing
#define WING_MAX_EXTENSION_MM 300.0f   // Max wing travel beyond threshold
#define FENCE_MAX_TRAVEL_MM   300.0f   // Physical max fence travel

// ── Motion: Speed & Acceleration ─────────────────────────────────────────────
#define FENCE_MAX_SPEED       5000.0f  // steps/sec
#define FENCE_ACCELERATION    2000.0f  // steps/sec²
#define FENCE_HOMING_SPEED    1000.0f  // steps/sec during homing

#define WING_MAX_SPEED        8000.0f
#define WING_ACCELERATION     3000.0f
#define WING_HOMING_SPEED     2000.0f

#define HOMING_TIMEOUT_MS     30000UL  // 30 s max per axis before error

// ── UI: Jog Steps ─────────────────────────────────────────────────────────────
#define JOG_STEP_COUNT  3
// Defined as array in config.cpp / main — avoids multiple-definition in headers

// ── EEPROM Layout ─────────────────────────────────────────────────────────────
#define EEPROM_FENCE_POS      0    // float  4B  fence absolute position (mm)
#define EEPROM_WING_POS       4    // float  4B  wing absolute position (mm)
#define EEPROM_HOMED_FLAG     8    // byte   1B  1 = homed since last power-on
#define EEPROM_JOG_INDEX      9    // byte   1B  jog step index (0-2)
#define EEPROM_FENCE_SPEED    10   // float  4B  fence max speed override
#define EEPROM_WING_SPEED     14   // float  4B  wing max speed override
#define EEPROM_FENCE_OFFSET   18   // float  4B  fence calibration offset (mm)
#define EEPROM_WING_OFFSET    22   // float  4B  wing calibration offset (mm)
#define EEPROM_FENCE_SKEW     26   // float  4B  fence skew offset (mm)
#define EEPROM_WING_SKEW      30   // float  4B  wing skew offset (mm)
#define EEPROM_PRESETS_BASE   34   // 6 slots × 8B = 48B
#define PRESET_SLOT_COUNT     6
#define PRESET_SLOT_SIZE      8    // bytes: 4B fence pos + 4B wing pos

// ── Debug ─────────────────────────────────────────────────────────────────────
// #define DEBUG_SERIAL            // Uncomment to enable serial debug output
