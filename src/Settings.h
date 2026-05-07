#pragma once

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════════
// Settings — EEPROM Persistence Manager
//
// Handles reading and writing system settings and machine state to EEPROM.
// ═══════════════════════════════════════════════════════════════════════════════

struct PresetSlot {
    float fencePos;
    float wingPos;
};

class SettingsManager {
public:
    SettingsManager();

    // Call once at startup to load defaults if first run
    void begin();

    // ── Position & Homing State ───────────────────────────────────────────────
    void  savePositions(float fencePos, float wingPos);
    float getFencePosition() const;
    float getWingPosition() const;

    void  setHomedFlag(bool homed);
    bool  isHomed() const;

    // ── UI Settings ───────────────────────────────────────────────────────────
    void    setJogIndex(uint8_t index);
    uint8_t getJogIndex() const;

    // ── Speeds ────────────────────────────────────────────────────────────────
    void setFenceSpeed(int speed);
    int  getFenceSpeed() const;

    void setWingSpeed(int speed);
    int  getWingSpeed() const;

    // ── Calibration & Offsets ─────────────────────────────────────────────────
    void  setFenceOffset(float offsetMm);
    float getFenceOffset() const;

    void  setWingOffset(float offsetMm);
    float getWingOffset() const;

    void  setFenceSkew(float skewMm);
    float getFenceSkew() const;

    void  setWingSkew(float skewMm);
    float getWingSkew() const;

    // ── Presets ───────────────────────────────────────────────────────────────
    void savePreset(uint8_t slotIndex, float fencePos, float wingPos);
    PresetSlot loadPreset(uint8_t slotIndex) const;

private:
    float _fencePosCache;
    float _wingPosCache;
};

extern SettingsManager Settings;
