#include "UI.h"
#include "config.h"
#include "Settings.h"

// Define I2C OLED display (U8g2)
// U8G2_SH1106_128X64_NONAME_F_HW_I2C -> Full framebuffer, hardware I2C
UIManager::UIManager(MotionCoordinator& coordinator) 
    : _coordinator(coordinator), 
      _display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE),
      _encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::TWO03),
      _currentView(VIEW_HOME),
      _targetInput(0.0f),
      _lastDrawMs(0),
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
        case VIEW_BOOT_PROMPT:  drawBootPrompt(); break;
        case VIEW_HOME:         drawHome(); break;
        case VIEW_MENU_MAIN:    drawMenuMain(); break;
        case VIEW_HOMING_ACTIVE:drawHomingActive(); break;
        // Stubs for other menus
        default: break;
    }

    _display.sendBuffer();
}

bool UIManager::isHomingRequested() {
    if (_currentView == VIEW_HOMING_ACTIVE) {
        return true;
    }
    return false;
}

void UIManager::notifyHomingComplete() {
    _currentView = VIEW_HOME;
    _targetInput = _coordinator.getCurrentWidth();
    redraw();
}

// ── Input Handling ────────────────────────────────────────────────────────────

void UIManager::handleInputs() {
    bool btnBack = digitalRead(BTN_BACK) == LOW;
    bool btnConfirm = digitalRead(BTN_CONFIRM) == LOW;
    bool btnEncoder = digitalRead(ENC_SW) == LOW;

    // Detect button presses (falling edge)
    bool backPressed = (btnBack && !_btnBackPrev);
    bool confirmPressed = (btnConfirm && !_btnConfirmPrev);
    bool encoderPressed = (btnEncoder && !_btnEncoderPrev);

    // Track long press for back button (to enter settings)
    if (backPressed) {
        _btnBackPressTime = millis();
    }
    bool backLongPressed = false;
    if (btnBack && _btnBackPrev && (millis() - _btnBackPressTime > 1000)) {
        backLongPressed = true;
        _btnBackPressTime = millis() + 5000; // Prevent rapid re-trigger
    }

    _btnBackPrev = btnBack;
    _btnConfirmPrev = btnConfirm;
    _btnEncoderPrev = btnEncoder;

    // Route inputs to current view handler
    if (_currentView == VIEW_BOOT_PROMPT) {
        if (confirmPressed) {
            _currentView = VIEW_HOMING_ACTIVE;
            // The main loop will handle the actual homing call when view == VIEW_HOMING_ACTIVE
        } else if (backPressed) {
            _currentView = VIEW_HOME; // Skip homing
        }
    } 
    else if (_currentView == VIEW_HOME) {
        // Read encoder
        int dir = (int)_encoder.getDirection();
        if (dir != 0) {
            float step = JOG_STEPS[Settings.getJogIndex()];
            _targetInput += (dir > 0) ? step : -step;
            if (_targetInput < 0.0f) _targetInput = 0.0f;
            if (_targetInput > FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM) {
                _targetInput = FENCE_MAX_TRAVEL_MM + WING_MAX_EXTENSION_MM;
            }
        }

        if (encoderPressed) {
            // Cycle jog index
            uint8_t idx = Settings.getJogIndex();
            idx = (idx + 1) % JOG_STEP_COUNT;
            Settings.setJogIndex(idx);
        }

        if (confirmPressed) {
            _coordinator.moveToWidth(_targetInput);
        }

        if (backPressed) {
            _targetInput = _coordinator.getCurrentWidth(); // Reset target
        } else if (backLongPressed) {
            _currentView = VIEW_MENU_MAIN;
        }
    }
    else if (_currentView == VIEW_MENU_MAIN) {
        if (backPressed) {
            _currentView = VIEW_HOME;
        }
        // Basic menu navigation to be implemented...
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
    _display.print(_coordinator.getCurrentWidth(), 1);
    _display.print(" mm");
    if (!Settings.isHomed()) {
        _display.print(" (!)"); // Unhomed warning
    }

    _display.drawLine(0, 15, 128, 15);

    // Line 2-4: Target Input
    _display.setFont(u8g2_font_logisoso24_tf); // Large bold font
    _display.setCursor(10, 45);
    _display.print(_targetInput, 1);

    // Target Label
    _display.setFont(u8g2_font_4x6_tf);
    _display.drawStr(10, 53, "Target Width");

    _display.drawLine(0, 56, 128, 56);

    // Status bar
    _display.setFont(u8g2_font_5x7_tf);
    _display.setCursor(0, 64);
    _display.print("Jog: ");
    _display.print(JOG_STEPS[Settings.getJogIndex()], 1);
    _display.print("mm");

    if (_coordinator.isMoving()) {
        _display.print("  [MOVING]");
    }
}

void UIManager::drawMenuMain() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(0, 10, "-- Settings --");
    _display.drawStr(0, 25, "1. Speeds");
    _display.drawStr(0, 40, "2. Offsets");
    _display.drawStr(0, 55, "3. Presets");
    // To be fleshed out with scrolling menu logic
}

void UIManager::drawHomingActive() {
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(10, 30, "Homing Active...");
    _display.drawStr(10, 50, "Please stand clear.");
}
