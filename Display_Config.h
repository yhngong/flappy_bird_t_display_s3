#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ============================================================================
// LilyGO T-Display-S3 Hardware Display Definition (ST7789 8-bit Parallel)
// ============================================================================
class LGFX_TDisplayS3 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX_TDisplayS3() {
        {
            auto cfg = _bus_instance.config();
            cfg.port       = 0;
            cfg.freq_write = 20000000;
            cfg.pin_wr     = 8;
            cfg.pin_rd     = 9;
            cfg.pin_rs     = 7;
            cfg.pin_d0     = 39;
            cfg.pin_d1     = 40;
            cfg.pin_d2     = 41;
            cfg.pin_d3     = 42;
            cfg.pin_d4     = 45;
            cfg.pin_d5     = 46;
            cfg.pin_d6     = 47;
            cfg.pin_d7     = 48;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 6;
            cfg.pin_rst          = 5;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 240;
            cfg.memory_height    = 320;
            cfg.panel_width      = 170;
            cfg.panel_height     = 320;
            cfg.offset_x         = 35;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 38;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }

    bool init() {
        pinMode(15, OUTPUT);
        digitalWrite(15, HIGH); // Enable LCD power pin
        return lgfx::LGFX_Device::init();
    }
};

extern LGFX_TDisplayS3 display;
extern LGFX_Sprite canvas;

const int SCREEN_W = 320;
const int SCREEN_H = 170;

// Onboard Hardware Pins
#define PIN_BTN1 0   // Boot button (Left side)
#define PIN_BTN2 14  // Key button  (Right side)
#define PIN_BATTERY 4 // Battery ADC pin
