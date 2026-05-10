#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <RotaryEncoder.h>
#include "MotionCoordinator.h"

// ═══════════════════════════════════════════════════════════════════════════════
// UI — User Interface Manager
//
// Handles the SH1106 OLED display, Rotary Encoder inputs, and Button states.
// ═══════════════════════════════════════════════════════════════════════════════

class UIManager {
public:
    UIManager(MotionCoordinator& coordinator);

    void begin();

    // Call continuously in loop() to handle input and display updates
    void update();

    // Force UI into the boot homing prompt state
    void showBootPrompt();

    // Force a redraw of the display
    void redraw();

    // Check if the user has requested homing (clears the flag when called)
    bool isHomingRequested();

    // Tell UI homing is finished
    void notifyHomingComplete();

    // Expose encoder tick function for ISR
    void tickEncoder();

private:
    MotionCoordinator& _coordinator;
    U8G2_SH1106_128X64_NONAME_F_HW_I2C _display;
    RotaryEncoder _encoder;

    enum ViewState {
        VIEW_BOOT_PROMPT,
        VIEW_HOME,
        VIEW_MENU_MAIN,
        VIEW_MENU_OFFSETS,
        VIEW_EDIT_OFFSET,
        VIEW_EDIT_SKEW,
        VIEW_MENU_PRESETS,
        VIEW_PRESET_ACTION,
        VIEW_MENU_SPEEDS,
        VIEW_EDIT_SPEED,
        VIEW_HOMING_ACTIVE
    };

    ViewState _currentView;
    float _targetInput;
    uint32_t _lastDrawMs;

    // Menu Navigation State
    uint8_t _menuIndex;
    uint8_t _menuScrollTop;
    uint8_t _selectedSlot;
    uint8_t _editMode;
    float   _editValueFloat;
    int     _editValueInt;

    // Button states
    bool _btnBackPrev;
    bool _btnConfirmPrev;
    bool _btnEncoderPrev;
    uint32_t _btnBackPressTime;
    
    // Jog steps
    const float JOG_STEPS[3] = {0.5f, 1.0f, 5.0f};

    // UI drawing functions
    void drawBootPrompt();
    void drawHome();
    void drawMenuMain();
    void drawMenuSpeeds();
    void drawEditSpeed();
    void drawMenuOffsets();
    void drawEditOffset();
    void drawEditSkew();
    void drawMenuPresets();
    void drawPresetAction();
    void drawHomingActive();

    // Utility for scrolling lists
    void drawMenuStr(uint8_t y, bool isSelected, const char* text);

    // Input handlers
    void handleInputs();
    void handleEncoderMenuScroll(int maxIndex);
};
