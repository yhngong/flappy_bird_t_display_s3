#pragma once
#include <Arduino.h>
#include "Display_Config.h"
#include "Input.h"

class AppSensors {
public:
    static const int SCOPE_W = 200;
    static const int SCOPE_H = 64;
    static const int SCOPE_X = 105;
    static const int SCOPE_Y = 32;

    int history[SCOPE_W];
    int histIdx = 0;
    int currentChannel = 0; // 0: Battery ADC, 1: Die Temperature, 2: Antenna / Touch ADC
    bool frozen = false;
    uint32_t lastSampleTime = 0;

    void init() {
        for (int i = 0; i < SCOPE_W; i++) history[i] = SCOPE_H / 2;
        histIdx = 0;
        frozen = false;
        analogReadResolution(12);
    }

    void update() {
        // Button 1: Switch sensor channel
        if (input.justPressedLeft) {
            currentChannel = (currentChannel + 1) % 3;
            for (int i = 0; i < SCOPE_W; i++) history[i] = SCOPE_H / 2;
        }

        // Button 2: Freeze / Unfreeze oscilloscope graph
        if (input.justPressedRight) {
            frozen = !frozen;
        }

        uint32_t now = millis();
        if (!frozen && (now - lastSampleTime >= 25)) {
            lastSampleTime = now;

            int sampleVal = 0;
            if (currentChannel == 0) { // Battery voltage
                int rawMv = analogReadMilliVolts(PIN_BATTERY) * 2;
                // Scale 3200mV..4300mV to 0..SCOPE_H
                sampleVal = map(rawMv, 3200, 4300, 2, SCOPE_H - 2);
            } else if (currentChannel == 1) { // Die temperature
                float tempC = temperatureRead();
                // Scale 20C..75C to 0..SCOPE_H
                sampleVal = map((int)tempC, 20, 75, 2, SCOPE_H - 2);
            } else { // Live ADC on floating pin GPIO 1 (acts as capacitive touch antenna)
                int raw = analogRead(1);
                sampleVal = map(raw, 0, 4095, 2, SCOPE_H - 2);
            }

            if (sampleVal < 1) sampleVal = 1;
            if (sampleVal >= SCOPE_H) sampleVal = SCOPE_H - 1;

            history[histIdx] = sampleVal;
            histIdx = (histIdx + 1) % SCOPE_W;
        }
    }

    void draw() {
        canvas.fillScreen(0x0842); // Cyber terminal black/blue

        // Header
        canvas.fillRect(0, 0, SCREEN_W, 22, 0x10C4);
        canvas.drawFastHLine(0, 22, SCREEN_W, 0x07FF);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07FF);
        canvas.drawString("SENSOR OSCILLOSCOPE", 8, 4, &fonts::Font2);

        canvas.setTextDatum(textdatum_t::top_right);
        if (frozen) {
            canvas.setTextColor(0xF800);
            canvas.drawString("[FROZEN]", SCREEN_W - 8, 4, &fonts::Font2);
        } else {
            canvas.setTextColor(0x07E0);
            canvas.drawString("[LIVE]", SCREEN_W - 8, 4, &fonts::Font2);
        }

        // Left Channel Selector Panel
        int panW = 92;
        canvas.fillRect(6, 28, panW, 72, 0x10A2);
        canvas.drawRoundRect(6, 28, panW, 72, 3, 0x2145);

        const char* chNames[3] = { "1. BATTERY", "2. DIE TEMP", "3. PIN1 ADC" };
        for (int c = 0; c < 3; c++) {
            int cy = 34 + (c * 22);
            if (c == currentChannel) {
                canvas.fillRect(8, cy - 2, panW - 4, 18, 0x03EF);
                canvas.setTextColor(TFT_WHITE);
            } else {
                canvas.setTextColor(0x6331);
            }
            canvas.drawString(chNames[c], 12, cy, &fonts::Font0);
        }

        // Right Oscilloscope Graph Display
        canvas.fillRect(SCOPE_X, SCOPE_Y, SCOPE_W, SCOPE_H, 0x0020); // Dark CRT green/black
        canvas.drawRect(SCOPE_X, SCOPE_Y, SCOPE_W, SCOPE_H, 0x07E0);

        // Grid lines
        for (int gx = SCOPE_X + 25; gx < SCOPE_X + SCOPE_W; gx += 25) {
            canvas.drawFastVLine(gx, SCOPE_Y + 1, SCOPE_H - 2, 0x01E0);
        }
        for (int gy = SCOPE_Y + 16; gy < SCOPE_Y + SCOPE_H; gy += 16) {
            canvas.drawFastHLine(SCOPE_X + 1, gy, SCOPE_W - 2, 0x01E0);
        }

        // Plot scrolling waveform
        uint16_t waveColor = (currentChannel == 0) ? 0x07E0 : ((currentChannel == 1) ? 0xF800 : 0x07FF);
        for (int x = 0; x < SCOPE_W - 1; x++) {
            int idx1 = (histIdx + x) % SCOPE_W;
            int idx2 = (histIdx + x + 1) % SCOPE_W;
            int y1 = SCOPE_Y + (SCOPE_H - history[idx1]);
            int y2 = SCOPE_Y + (SCOPE_H - history[idx2]);
            canvas.drawLine(SCOPE_X + x, y1, SCOPE_X + x + 1, y2, waveColor);
        }

        // Lower Telemetry Dashboard (Sensors readouts)
        int dashY = 104;
        canvas.fillRect(6, dashY, SCREEN_W - 12, 44, 0x10A2);
        canvas.drawRoundRect(6, dashY, SCREEN_W - 12, 44, 3, 0x2145);

        // Sensor 1: Battery
        int rawMv = analogReadMilliVolts(PIN_BATTERY) * 2;
        float vbat = rawMv / 1000.0f;
        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07E0);
        canvas.drawString("VBAT: " + String(vbat, 2) + "V (" + String(rawMv) + "mV)", 14, dashY + 6, &fonts::Font0);

        // Sensor 2: Internal Temp
        float tempC = temperatureRead();
        float tempF = (tempC * 9.0f / 5.0f) + 32.0f;
        canvas.setTextColor(0xF800);
        canvas.drawString("DIE TEMP: " + String((int)tempC) + "C / " + String((int)tempF) + "F", 14, dashY + 22, &fonts::Font0);

        // Sensor 3: Antenna pin raw
        int pinRaw = analogRead(1);
        canvas.setTextColor(0x07FF);
        canvas.drawString("PIN1 NOISE: " + String(pinRaw), 170, dashY + 6, &fonts::Font0);

        // Sensor 4: Button States
        canvas.setTextColor(TFT_WHITE);
        String btnStr = "BTN0: " + String(input.downLeft ? "HELD" : "UP") + "  BTN14: " + String(input.downRight ? "HELD" : "UP");
        canvas.drawString(btnStr, 170, dashY + 22, &fonts::Font0);

        // Bottom Controls Hint
        canvas.fillRect(0, SCREEN_H - 18, SCREEN_W, 18, 0x10C4);
        canvas.setTextDatum(textdatum_t::bottom_left);
        canvas.setTextColor(0xFD20);
        canvas.drawString("BTN1: NEXT CHANNEL", 10, SCREEN_H - 3, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::bottom_right);
        canvas.setTextColor(0x07E0);
        canvas.drawString("BTN2: FREEZE / RUN", SCREEN_W - 10, SCREEN_H - 3, &fonts::Font0);
    }
};

extern AppSensors appSensors;
