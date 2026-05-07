# FenceMate Major Refactoring Plan

## Discovery Findings

### Current System
- **MCU**: Arduino UNO R4 WiFi (limited pins)
- **Steppers**: 2x (Fence), direct GPIO, custom acceleration
- **Motors**: Both leadscrew-driven (20 TPI)
- **Display**: TFT ILI9341 (320x240 pixels) + XPT2046 touchscreen
- **UI**: Imperial/Metric dual mode support
- **Homing**: Manual redefinition only, no hardware endstops
- **Input**: TFT touchscreen with custom button/keypad UI

### User Requirements (Clarified)
1. **Stepper Library**: AccelStepper; position loss acceptable
2. **Fence Motors**: Leadscrew → Rack & Pinion (Module 1, 25-tooth pinion)
3. **Extension Wing**: 2x additional steppers (leadscrew-driven); 300mm threshold
4. **Extension Wing Control**: Dual motors for tramming/parallel adjustment
5. **UI**: I2C OLED SH1106 + rotary encoder (with push button) + back/confirm buttons
6. **Measurements**: Metric only (remove imperial code)
7. **Board**: Arduino Mega 1280 (86 I/O pins, 8KB SRAM; pin constraints + future expansion)
8. **Homing**: Sequential pair homing (fence pair → wing pair); optical endstops; home to 0.00mm

### Final Specifications (Confirmed)
✅ **Wing Leadscrew**: 8mm diameter, 2mm pitch, 4mm lead → **50 steps/mm**
✅ **Fence Rack & Pinion**: Module 1, 25-tooth → **8 steps/mm**
✅ **OLED UI Layout**: Fence position (line 1), Target cut width in large/bold (line 2-4), Settings (menu)
✅ **Encoder Input**: ±5mm per click; push toggles jog speed (0.5/1/5mm); back/confirm for navigation
✅ **Wing Max Extension**: 600mm effective cut width (300mm fence + 300mm wing) ± 10mm tolerance
✅ **Build Environment**: PlatformIO for reproducibility

---

## Technical Specifications

### Motion Calculations

**Current Fence System (Leadscrew 20 TPI)**:
- Stepper: 200 steps/revolution
- Linear: 4000 steps/inch ≈ 157.48 steps/mm
- Function: `GetAbsoluteSteps() = position_mm * 157.48`

**New Fence System (Rack & Pinion, Module 1, 25-tooth)**:
- Stepper: 200 steps/revolution
- Linear: 25mm per revolution = 8 steps/mm
- Function: `GetAbsoluteSteps() = position_mm * 8`

**New Extension Wing System (Leadscrew, 8mm Ø, 2mm pitch, 4mm lead)**:
- Stepper: 200 steps/revolution
- Linear: 4mm lead per revolution = 50 steps/mm
- Function: `GetAbsoluteSteps() = position_mm * 50`

### Extension Wing Motion Framework

**Threshold**: 300mm
- **Fence_Position ≤ 300mm**: Move fence motors alone to target; wing stationary
- **Fence_Position > 300mm**: 
  - Move fence motors to 300mm (synchronously)
  - Then move wing motors to extend by (Fence_Position - 300mm)
  - Wing can also be trammed independently (dual motors for parallel adjustment)

### Pin Mapping (Arduino Mega 1280)

| Function | Pin | Notes |
|----------|-----|-------|
| **Fence Motor 1** | | |
| - Step | 22 | |
| - Direction | 23 | |
| - Enable | 24 | |
| **Fence Motor 2** | | |
| - Step | 25 | |
| - Direction | 26 | |
| - Enable | 27 | |
| **Wing Motor 1** | | |
| - Step | 28 | |
| - Direction | 29 | |
| - Enable | 30 | |
| **Wing Motor 2** | | |
| - Step | 31 | |
| - Direction | 32 | |
| - Enable | 33 | |
| **Endstops (Optical)** | | |
| - Fence1 home | 34 | LOW = at home |
| - Fence2 home | 35 | LOW = at home |
| - Wing1 home | 36 | LOW = at home |
| - Wing2 home | 37 | LOW = at home |
| **I2C (OLED SH1106)** | | |
| - SDA | 20 | Hardware I2C |
| - SCL | 21 | Hardware I2C |
| **Encoder** | | |
| - CLK (A) | 38 | Rotary movement |
| - DT (B) | 39 | Rotary movement |
| - Push | 40 | Encoder button |
| **UI Buttons** | | |
| - Back | 41 | Cancel operation |
| - Confirm | 42 | Confirm operation |
| **SPI (if needed future)** | 50-52 | Reserved |

### Libraries & Dependencies

| Library | Purpose | Version | Notes |
|---------|---------|---------|-------|
| AccelStepper | Stepper motor control | Recommended: v1.61+ | Handles acceleration, not used for position tracking |
| u8g2 | OLED SH1106 driver | Recommended: v2.35+ | High performance, SPI or I2C mode |
| RotaryEncoder | Encoder input | Any standard library | Interrupt-driven preferred |
| Wire | I2C communication | Built-in | For OLED |
| EEPROM | Parameter storage | Built-in | Position, settings persistence |

### EEPROM Layout (New)

| Address | Type | Purpose |
|---------|------|---------|
| 0-3 | long | Fence absolute position (mm) |
| 4-7 | long | Wing absolute position (mm) |
| 8-9 | byte | Homing status flag |
| 10-13 | int | Fence motor max speed (steps/sec) |
| 14-17 | int | Wing motor max speed (steps/sec) |
| 18-21 | int | Current jog speed index (0=0.5mm, 1=1mm, 2=5mm) |
| 22+ | | Reserved for future |

### OLED UI Layout (128×64 SH1106)

```
Line 1:  "Fence: XXX.Xmm"         [small font]
Line 2:  "━━━━━━━━━━━━━━━━━━━━━"   [separator]
Line 3:  "600.0"                  [large/bold, primary input]
Line 4:  "Target Width"           [small font, label]
Line 5:  "━━━━━━━━━━━━━━━━━━━━━"   [separator]
Line 6:  "Jog: ±5mm │ Speed: ±1  " [status line]
```

**Input Handler States**:
- **Rotary Encoder**: ±5mm per detent (clockwise = increment, counter-clockwise = decrement)
- **Push Button (Encoder)**: Cycles jog speed: 0.5mm → 1mm → 5mm → 0.5mm (loops)
- **Back Button**: Cancel current input, return to previous menu state
- **Confirm Button**: Apply target width and execute fence+wing movement

**Menu Flow**:
1. Home Screen (default) → shows fence position, target width input
2. Settings (accessed via long-press Back or dedicated menu button) → motor speeds, debug info, system status

---

## Implementation Notes

**Arduino Mega 1280 Specifics**:
- Board ID for PlatformIO: `megaatmega1280`
- 84 I/O pins (vs 2560's 86), sufficient for this project
- Same pinout structure, same serial/I2C/SPI peripherals
- No changes needed to pin mapping above
- Memory: 8KB SRAM (same as 2560)
