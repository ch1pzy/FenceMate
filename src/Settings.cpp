#include "Settings.h"
#include "config.h"
#include <EEPROM.h>

SettingsManager Settings;

SettingsManager::SettingsManager() : _fencePosCache(0.0f), _wingPosCache(0.0f) {
}

void SettingsManager::begin() {
    // Check if EEPROM is uninitialized (all 0xFF). If so, initialize to safe defaults.
    // We check the jog index byte. If it's 255, we assume uninitialized.
    if (EEPROM.read(EEPROM_JOG_INDEX) == 255) {
        setHomedFlag(false);
        setJogIndex(1); // Default to middle jog step
        setFenceSpeed((int)FENCE_MAX_SPEED);
        setWingSpeed((int)WING_MAX_SPEED);
        setFenceOffset(0.0f);
        setWingOffset(0.0f);
        setFenceSkew(0.0f);
        setWingSkew(0.0f);
        savePositions(0.0f, 0.0f);
        for (int i = 0; i < PRESET_SLOT_COUNT; i++) {
            savePreset(i, 0.0f, 0.0f);
        }
    } else {
        // Load cached positions
        EEPROM.get(EEPROM_FENCE_POS, _fencePosCache);
        EEPROM.get(EEPROM_WING_POS, _wingPosCache);
    }
}

void SettingsManager::savePositions(float fencePos, float wingPos) {
    if (_fencePosCache != fencePos) {
        EEPROM.put(EEPROM_FENCE_POS, fencePos);
        _fencePosCache = fencePos;
    }
    if (_wingPosCache != wingPos) {
        EEPROM.put(EEPROM_WING_POS, wingPos);
        _wingPosCache = wingPos;
    }
}

float SettingsManager::getFencePosition() const {
    return _fencePosCache;
}

float SettingsManager::getWingPosition() const {
    return _wingPosCache;
}

void SettingsManager::setHomedFlag(bool homed) {
    EEPROM.write(EEPROM_HOMED_FLAG, homed ? 1 : 0);
}

bool SettingsManager::isHomed() const {
    return EEPROM.read(EEPROM_HOMED_FLAG) == 1;
}

void SettingsManager::setJogIndex(uint8_t index) {
    EEPROM.write(EEPROM_JOG_INDEX, index);
}

uint8_t SettingsManager::getJogIndex() const {
    return EEPROM.read(EEPROM_JOG_INDEX);
}

void SettingsManager::setFenceSpeed(int speed) {
    EEPROM.put(EEPROM_FENCE_SPEED, speed);
}

int SettingsManager::getFenceSpeed() const {
    int speed;
    EEPROM.get(EEPROM_FENCE_SPEED, speed);
    return speed;
}

void SettingsManager::setWingSpeed(int speed) {
    EEPROM.put(EEPROM_WING_SPEED, speed);
}

int SettingsManager::getWingSpeed() const {
    int speed;
    EEPROM.get(EEPROM_WING_SPEED, speed);
    return speed;
}

void SettingsManager::setFenceOffset(float offsetMm) {
    EEPROM.put(EEPROM_FENCE_OFFSET, offsetMm);
}

float SettingsManager::getFenceOffset() const {
    float offsetMm;
    EEPROM.get(EEPROM_FENCE_OFFSET, offsetMm);
    return isnan(offsetMm) ? 0.0f : offsetMm;
}

void SettingsManager::setWingOffset(float offsetMm) {
    EEPROM.put(EEPROM_WING_OFFSET, offsetMm);
}

float SettingsManager::getWingOffset() const {
    float offsetMm;
    EEPROM.get(EEPROM_WING_OFFSET, offsetMm);
    return isnan(offsetMm) ? 0.0f : offsetMm;
}

void SettingsManager::setFenceSkew(float skewMm) {
    EEPROM.put(EEPROM_FENCE_SKEW, skewMm);
}

float SettingsManager::getFenceSkew() const {
    float skewMm;
    EEPROM.get(EEPROM_FENCE_SKEW, skewMm);
    return isnan(skewMm) ? 0.0f : skewMm;
}

void SettingsManager::setWingSkew(float skewMm) {
    EEPROM.put(EEPROM_WING_SKEW, skewMm);
}

float SettingsManager::getWingSkew() const {
    float skewMm;
    EEPROM.get(EEPROM_WING_SKEW, skewMm);
    return isnan(skewMm) ? 0.0f : skewMm;
}

void SettingsManager::savePreset(uint8_t slotIndex, float fencePos, float wingPos) {
    if (slotIndex >= PRESET_SLOT_COUNT) return;
    int addr = EEPROM_PRESETS_BASE + (slotIndex * PRESET_SLOT_SIZE);
    EEPROM.put(addr, fencePos);
    EEPROM.put(addr + 4, wingPos);
}

PresetSlot SettingsManager::loadPreset(uint8_t slotIndex) const {
    PresetSlot slot = {0.0f, 0.0f};
    if (slotIndex >= PRESET_SLOT_COUNT) return slot;
    int addr = EEPROM_PRESETS_BASE + (slotIndex * PRESET_SLOT_SIZE);
    EEPROM.get(addr, slot.fencePos);
    EEPROM.get(addr + 4, slot.wingPos);
    return slot;
}
