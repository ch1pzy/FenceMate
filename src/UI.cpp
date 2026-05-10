#include "UI.h"
#include "config.h"
#include "Settings.h"

// Define I2C OLED display (U8g2)
UIManager::UIManager(MotionCoordinator& coordinator) 
    : _coordinator(coordinator), 
      _display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE),
      _encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::TWO03),
      _currentView(VIEW_HOME),
      _targetInput(0.0f),
      _lastDrawMs(0),
      _menuIndex(0),
      _menuScrollTop(0),
      _selectedSlot(0),
      _editMode(0),
      _editValueFloat(0.0f),
      _editValueInt(0),
      _btnBackPrev(true),
      _btnConfirmPrev(true),
      _btnEncoderPrev(true),
      _btnBackPressTime(0)
{
}

void UIManager::begin() {
    _display.begin();
    pinMode(ENC_SW, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);
    pinMode(BTN_CONFIRM, INPUT_PULLUP);

    _targetInput = _coordinator.getCurrentWidth();
}

void UIManager::showBootPrompt() {
    _currentView = VIEW_BOOT_PROMPT;
    redraw();
}

void UIManager::tickEncoder() {
    _encoder.tick();
}

void UIManager::update() {
    handleInputs();

    // Limit redraw rate to ~10 FPS to save CPU
    uint32_t now = millis();
    if (now - _lastDrawMs > 100) {
        redraw();
        _lastDrawMs = now;
    }
}

void UIManager::redraw() {
    _display.clearBuffer();

    switch (_currentView) {
        case VIEW_BOOT_PROMPT:   drawBootPrompt(); break;
        case VIEW_HOME:          drawHome(); break;
        case VIEW_MENU_MAIN:     drawMenuMain(); break;
        case VIEW_MENU_OFFSETS:  drawMenuOffsets(); break;
        case VIEW_EDIT_OFFSET:   drawEditOffset(); break;
        case VIEW_EDIT_SKEW:     drawEditSkew(); break;
        case VIEW_MENU_PRESETS:  drawMenuPresets(); break;
        case VIEW_PRESET_ACTION: drawPresetAction(); break;
        case VIEW_MENU_SPEEDS:   drawMenuSpeeds(); break;
        case VIEW_EDIT_SPEED:    drawEditSpeed(); break;
        case VIEW_HOMING_ACTIVE: drawHomingActive(); break;
    }

    _display.sendBuffer();
}

bool UIManager::isHomingRequested() {
    return _currentView == VIEW_HOMING_ACTIVE;
}

void UIManager::notifyHomingComplete() {
    _currentView = VIEW_HOME;
    _targetInput = _coordinator.getCurrentWidth();
    redraw();
}

// ── Utility ───────────────────────────────────────────────────────────────────

void UIManager::handleEncoderMenuScroll(int maxIndex) {
    int dir = (int)_encoder.getDirection();
    if (dir != 0) {
        if (dir > 0 && _menuIndex < maxIndex) _menuIndex++;
        if (dir < 0 && _menuIndex > 0) _menuIndex--;

        // Adjust scroll window (shows 3 items max)
        if (_menuIndex < _menuScrollTop) _menuScrollTop = _menuIndex;
        if (_menuIndex > _menuScrollTop + 2) _menuScrollTop = _menuIndex - 2;
    }
}

void UIManager::drawMenuStr(uint8_t y, bool isSelected, const char* text) {
    if (isSelected) {
        _display.drawBox(0, y - 9, 128, 11);
        _display.setDrawColor(0); // Black text on white box
    } else {
        _display.setDrawColor(1); // White text
    }
    _display.drawStr(2, y, text);
    _display.setDrawColor(1); // Reset
}

// ── Input Handling ────────────────────────────────────────────────────────────

void UIManager::handleInputs() {
    bool btnBack = digitalRead(BTN_BACK) == LOW;
    bool btnConfirm = digitalRead(BTN_CONFIRM) == LOW;
    bool btnEncoder = digitalRead(ENC_SW) == LOW;

    bool backPressed = (btnBack && !_btnBackPrev);
    bool confirmPressed = (btnConfirm && !_btnConfirmPrev);
    bool encoderPressed = (btnEncoder && !_btnEncoderPrev);

    if (backPressed) _btnBackPressTime = millis();
    bool backLongPressed = false;
    if (btnBack && _btnBackPrev && (millis() - _btnBackPressTime > 1000)) {
        backLongPressed = true;
        _btnBackPressTime = millis() + 5000;
    }

    _btnBackPrev = btnBack;
    _btnConfirmPrev = btnConfirm;
    _btnEncoderPrev = btnEncoder;

    int dir = (int)_encoder.getDirection();

    switch (_currentView) {
        case VIEW_BOOT_PROMPT:
            if (confirmPressed) _currentView = VIEW_HOMING_ACTIVE;
            else if (backPressed) _currentView = VIEW_HOME;
            break;

        case VIEW_HOME:
            if (dir != 0) {
                float step = JOG_STEPS[Settings.getJogIndex()];
                _targetInput += (dir > 0) ? step : -step;
                if (_targetInput < 0.0f) _targetInput = 0.0f;
                if (_targetInput > FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM) {
                    _targetInput = FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM;
                }
            }
            if (encoderPressed) {
                uint8_t idx = (Settings.getJogIndex() + 1) % JOG_STEP_COUNT;
                Settings.setJogIndex(idx);
            }
            if (confirmPressed) _coordinator.moveToWidth(_targetInput);
            if (backPressed) _targetInput = _coordinator.getCurrentWidth();
            if (backLongPressed) {
                _currentView = VIEW_MENU_MAIN;
                _menuIndex = 0;
                _menuScrollTop = 0;
            }
            break;

        case VIEW_MENU_MAIN:
            handleEncoderMenuScroll(2); // 3 items: 0=Offsets, 1=Presets, 2=Speeds
            if (backPressed) _currentView = VIEW_HOME;
            if (encoderPressed || confirmPressed) {
                if (_menuIndex == 0) { _currentView = VIEW_MENU_OFFSETS; _menuIndex = 0; _menuScrollTop = 0; }
                else if (_menuIndex == 1) { _currentView = VIEW_MENU_PRESETS; _menuIndex = 0; _menuScrollTop = 0; }
                else if (_menuIndex == 2) { _currentView = VIEW_MENU_SPEEDS; _menuIndex = 0; _menuScrollTop = 0; }
            }
            break;

        case VIEW_MENU_OFFSETS:
            handleEncoderMenuScroll(3); // 0=Fence Offset, 1=Wing Offset, 2=Fence Skew, 3=Wing Skew
            if (backPressed) { _currentView = VIEW_MENU_MAIN; _menuIndex = 0; _menuScrollTop = 0; }
            if (encoderPressed || confirmPressed) {
                _editMode = _menuIndex;
                if (_editMode == 0) _editValueFloat = Settings.getFenceOffset();
                else if (_editMode == 1) _editValueFloat = Settings.getWingOffset();
                else if (_editMode == 2) _editValueFloat = Settings.getFenceSkew();
                else if (_editMode == 3) _editValueFloat = Settings.getWingSkew();
                
                _currentView = (_editMode < 2) ? VIEW_EDIT_OFFSET : VIEW_EDIT_SKEW;
            }
            break;

        case VIEW_EDIT_OFFSET:
            if (dir != 0) _editValueFloat += (dir > 0) ? 0.1f : -0.1f;
            if (backPressed) _currentView = VIEW_MENU_OFFSETS;
            if (encoderPressed || confirmPressed) {
                if (_editMode == 0) Settings.setFenceOffset(_editValueFloat);
                else Settings.setWingOffset(_editValueFloat);
                _currentView = VIEW_MENU_OFFSETS;
            }
            break;

        case VIEW_EDIT_SKEW:
            if (dir != 0) {
                float delta = (dir > 0) ? 0.1f : -0.1f;
                _editValueFloat += delta;
                if (_editMode == 2) _coordinator.skewFence(delta);
                else _coordinator.skewWing(delta);
            }
            if (backPressed) _currentView = VIEW_MENU_OFFSETS;
            if (encoderPressed || confirmPressed) {
                if (_editMode == 2) Settings.setFenceSkew(_editValueFloat);
                else Settings.setWingSkew(_editValueFloat);
                _currentView = VIEW_MENU_OFFSETS;
            }
            break;

        case VIEW_MENU_PRESETS:
            handleEncoderMenuScroll(PRESET_SLOT_COUNT - 1);
            if (backPressed) { _currentView = VIEW_MENU_MAIN; _menuIndex = 1; _menuScrollTop = 0; }
            if (encoderPressed || confirmPressed) {
                _selectedSlot = _menuIndex;
                _currentView = VIEW_PRESET_ACTION;
                _menuIndex = 0; _menuScrollTop = 0;
            }
            break;

        case VIEW_PRESET_ACTION:
            handleEncoderMenuScroll(1); // 0=Load, 1=Save Current
            if (backPressed) { _currentView = VIEW_MENU_PRESETS; _menuIndex = _selectedSlot; _menuScrollTop = _selectedSlot > 2 ? _selectedSlot - 2 : 0; }
            if (encoderPressed || confirmPressed) {
                if (_menuIndex == 0) {
                    // Load
                    PresetSlot p = Settings.loadPreset(_selectedSlot);
                    if (p.fencePos > 0.0f || p.wingPos > 0.0f) { // Simple check for non-empty
                        _targetInput = p.fencePos + p.wingPos;
                        _coordinator.moveToWidth(_targetInput);
                    }
                    _currentView = VIEW_HOME;
                } else {
                    // Save
                    Settings.savePreset(_selectedSlot, Settings.getFencePosition(), Settings.getWingPosition());
                    _currentView = VIEW_MENU_PRESETS;
                    _menuIndex = _selectedSlot; 
                    _menuScrollTop = _selectedSlot > 2 ? _selectedSlot - 2 : 0;
                }
            }
            break;

        case VIEW_MENU_SPEEDS:
            handleEncoderMenuScroll(1); // 0=Fence Speed, 1=Wing Speed
            if (backPressed) { _currentView = VIEW_MENU_MAIN; _menuIndex = 2; _menuScrollTop = 0; }
            if (encoderPressed || confirmPressed) {
                _editMode = _menuIndex;
                _editValueInt = (_editMode == 0) ? Settings.getFenceSpeed() : Settings.getWingSpeed();
                _currentView = VIEW_EDIT_SPEED;
            }
            break;

        case VIEW_EDIT_SPEED:
            if (dir != 0) _editValueInt += (dir > 0) ? 100 : -100;
            if (_editValueInt < 100) _editValueInt = 100;
            if (backPressed) _currentView = VIEW_MENU_SPEEDS;
            if (encoderPressed || confirmPressed) {
                if (_editMode == 0) Settings.setFenceSpeed(_editValueInt);
                else Settings.setWingSpeed(_editValueInt);
                _currentView = VIEW_MENU_SPEEDS;
            }
            break;

        case VIEW_HOMING_ACTIVE:
            // Handled externally
            break;
    }
}

// ── Drawing Functions ─────────────────────────────────────────────────────────

void UIManager::drawBootPrompt() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 15, "Run Homing Routine?");
    _display.drawStr(10, 45, "[Confirm] to Run");
    _display.drawStr(10, 60, "[Back] to Skip");
}

void UIManager::drawHome() {
    _display.setFont(u8g2_font_6x10_tf);
    
    // Line 1: Actual position
    _display.setCursor(0, 10);
    _display.print("Pos: ");
    // Display position with global offsets applied visually
    float actualPos = _coordinator.getCurrentWidth();
    actualPos += Settings.getFenceOffset();
    if (actualPos > WING_THRESHOLD_MM) actualPos += Settings.getWingOffset();
    
    _display.print(actualPos, 1);
    _display.print(" mm");
    if (!Settings.isHomed()) _display.print(" (!)");

    _display.drawLine(0, 15, 128, 15);

    // Line 2-4: Target Input
    _display.setFont(u8g2_font_logisoso24_tf); 
    _display.setCursor(10, 45);
    _display.print(_targetInput, 1);

    _display.setFont(u8g2_font_4x6_tf);
    _display.drawStr(10, 53, "Target Width");

    _display.drawLine(0, 56, 128, 56);

    // Status bar
    _display.setFont(u8g2_font_5x7_tf);
    _display.setCursor(0, 64);
    _display.print("Jog: ");
    _display.print(JOG_STEPS[Settings.getJogIndex()], 1);
    _display.print("mm");

    if (_coordinator.isMoving()) _display.print("  [MOVING]");
}

void UIManager::drawMenuMain() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawBox(0, 0, 128, 13);
    _display.setDrawColor(0);
    _display.drawStr(24, 10, "-- SETTINGS --");
    _display.setDrawColor(1);

    const char* items[] = {"1. Offsets", "2. Presets", "3. Speeds"};
    for (int i = 0; i < 3; i++) {
        int idx = _menuScrollTop + i;
        if (idx < 3) drawMenuStr(25 + i * 14, _menuIndex == idx, items[idx]);
    }
}

void UIManager::drawMenuOffsets() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawBox(0, 0, 128, 13);
    _display.setDrawColor(0);
    _display.drawStr(24, 10, "-- OFFSETS --");
    _display.setDrawColor(1);

    char buf[32];
    char fBuf[10];
    
    for (int i = 0; i < 3; i++) {
        int idx = _menuScrollTop + i;
        if (idx == 0) {
            dtostrf(Settings.getFenceOffset(), 4, 1, fBuf);
            snprintf(buf, sizeof(buf), "1. Fence Off: %s", fBuf);
        } else if (idx == 1) {
            dtostrf(Settings.getWingOffset(), 4, 1, fBuf);
            snprintf(buf, sizeof(buf), "2. Wing Off:  %s", fBuf);
        } else if (idx == 2) {
            dtostrf(Settings.getFenceSkew(), 4, 1, fBuf);
            snprintf(buf, sizeof(buf), "3. Fence Skew:%s", fBuf);
        } else if (idx == 3) {
            dtostrf(Settings.getWingSkew(), 4, 1, fBuf);
            snprintf(buf, sizeof(buf), "4. Wing Skew: %s", fBuf);
        }
        if (idx < 4) drawMenuStr(25 + i * 14, _menuIndex == idx, buf);
    }
}

void UIManager::drawEditOffset() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 15, _editMode == 0 ? "Edit Fence Offset:" : "Edit Wing Offset:");
    
    _display.setFont(u8g2_font_logisoso16_tf);
    _display.setCursor(20, 45);
    if (_editValueFloat > 0) _display.print("+");
    _display.print(_editValueFloat, 1);
    _display.print(" mm");
}

void UIManager::drawEditSkew() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 15, _editMode == 2 ? "Live Fence Skew:" : "Live Wing Skew:");
    
    _display.setFont(u8g2_font_logisoso16_tf);
    _display.setCursor(20, 45);
    if (_editValueFloat > 0) _display.print("+");
    _display.print(_editValueFloat, 1);
    _display.print(" mm");
}

void UIManager::drawMenuPresets() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawBox(0, 0, 128, 13);
    _display.setDrawColor(0);
    _display.drawStr(24, 10, "-- PRESETS --");
    _display.setDrawColor(1);

    char buf[32];
    char fBuf[10];
    for (int i = 0; i < 3; i++) {
        int idx = _menuScrollTop + i;
        if (idx < PRESET_SLOT_COUNT) {
            PresetSlot p = Settings.loadPreset(idx);
            if (p.fencePos == 0.0f && p.wingPos == 0.0f) {
                snprintf(buf, sizeof(buf), "Slot %d: [Empty]", idx + 1);
            } else {
                dtostrf(p.fencePos + p.wingPos, 4, 1, fBuf);
                snprintf(buf, sizeof(buf), "Slot %d: [%s]", idx + 1, fBuf);
            }
            drawMenuStr(25 + i * 14, _menuIndex == idx, buf);
        }
    }
}

void UIManager::drawPresetAction() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 15, "Slot Action:");
    
    drawMenuStr(35, _menuIndex == 0, "1. Load Preset");
    drawMenuStr(50, _menuIndex == 1, "2. Save Current");
}

void UIManager::drawMenuSpeeds() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawBox(0, 0, 128, 13);
    _display.setDrawColor(0);
    _display.drawStr(24, 10, "-- SPEEDS --");
    _display.setDrawColor(1);

    char buf[32];
    snprintf(buf, sizeof(buf), "1. Fence: %d", Settings.getFenceSpeed());
    drawMenuStr(25, _menuIndex == 0, buf);

    snprintf(buf, sizeof(buf), "2. Wing:  %d", Settings.getWingSpeed());
    drawMenuStr(39, _menuIndex == 1, buf);
}

void UIManager::drawEditSpeed() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 15, _editMode == 0 ? "Fence Speed:" : "Wing Speed:");
    
    _display.setFont(u8g2_font_logisoso16_tf);
    _display.setCursor(20, 45);
    _display.print(_editValueInt);
}

void UIManager::drawHomingActive() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 30, "Homing Active...");
    _display.drawStr(10, 50, "Please stand clear.");
}
