#pragma once
#include <Arduino.h>
#include "Display_Config.h"
#include "Input.h"

struct AppMetadata {
    const char* title;
    const char* subtitle;
    uint16_t primaryColor;
    uint16_t accentColor;
};

const int TOTAL_APPS = 6;

const AppMetadata APP_INFO[TOTAL_APPS] = {
    { "FLAPPY BIRD",     "Flap through pipes & beat high score",  0xFDE0, 0x3E24 },
    { "SENSOR DEMO",     "Live Oscilloscope & ADC Sensor Scope",  0x07E0, 0x03EF },
    { "WI-FI CLOCK",     "Digital & Analog NTP Desk Clock",       0x07FF, 0x0861 },
    { "SPACE SHOOTER",   "Twin-laser galactic alien battle",      0x07FF, 0xF800 },
    { "RETRO SNAKE",     "Classic 2-button arcade snake",         0x07E0, 0xFD20 },
    { "HARDWARE MONITOR","Battery, Wi-Fi & ESP32-S3 stats",       0x9E7F, 0x051D }
};

class AppLauncher {
public:
    int selectedApp = 0;

    void init() {}

    int update() {
        // Button 1: Next App (cycle 0..5)
        if (input.justPressedLeft) {
            selectedApp = (selectedApp + 1) % TOTAL_APPS;
        }

        // Button 2: Launch App
        if (input.justPressedRight) {
            return selectedApp;
        }

        // Touch on screen in Launcher:
        if (input.screenTouched) {
            if (input.touchX > 90 && input.touchX < 290 && input.touchY > 25 && input.touchY < 135) {
                return selectedApp; // Tap card to launch
            } else {
                selectedApp = (selectedApp + 1) % TOTAL_APPS; // Tap to cycle
            }
        }

        return -1;
    }

    void drawBatteryIndicator() {
        int rawMv = analogReadMilliVolts(PIN_BATTERY);
        float vbat = (rawMv * 2.0f) / 1000.0f;
        int pct = (int)((vbat - 3.3f) / (4.2f - 3.3f) * 100.0f);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;

        int bx = SCREEN_W - 55;
        int by = 6;
        canvas.drawRoundRect(bx, by, 22, 10, 2, TFT_WHITE);
        canvas.fillRect(bx + 22, by + 2, 2, 6, TFT_WHITE);
        
        int fillW = (pct * 18) / 100;
        uint16_t bColor = (pct > 20) ? 0x07E0 : 0xF800;
        if (fillW > 0) {
            canvas.fillRect(bx + 2, by + 2, fillW, 6, bColor);
        }

        canvas.setTextDatum(textdatum_t::top_right);
        canvas.setTextColor(TFT_LIGHTGRAY);
        canvas.drawString(String((int)pct) + "%", bx - 4, by, &fonts::Font0);
    }

    void drawAppIcon(int appIdx, int cx, int cy) {
        float bob = sin(millis() * 0.005f) * 3.0f;
        int by = cy + (int)bob;

        switch (appIdx) {
            case 0: { // Flappy Bird Icon
                canvas.fillRect(cx - 32, by - 12, 14, 28, 0x2DE2);
                canvas.drawRect(cx - 32, by - 12, 14, 28, TFT_BLACK);
                canvas.fillRect(cx - 34, by - 16, 18, 6, 0x4745);
                canvas.drawRect(cx - 34, by - 16, 18, 6, TFT_BLACK);

                canvas.fillCircle(cx + 4, by, 11, 0xFDE0);
                canvas.drawCircle(cx + 4, by, 11, TFT_BLACK);
                canvas.fillCircle(cx + 9, by - 3, 4, TFT_WHITE);
                canvas.fillCircle(cx + 10, by - 3, 2, TFT_BLACK);
                canvas.fillTriangle(cx + 12, by - 1, cx + 20, by + 2, cx + 12, by + 5, 0xFC40);
                canvas.drawTriangle(cx + 12, by - 1, cx + 20, by + 2, cx + 12, by + 5, TFT_BLACK);
                canvas.fillRoundRect(cx - 5, by - 2, 10, 6, 2, TFT_WHITE);
                canvas.drawRoundRect(cx - 5, by - 2, 10, 6, 2, TFT_BLACK);
                break;
            }
            case 1: { // Sensor Demo Icon (Oscilloscope scope wave)
                canvas.fillRect(cx - 24, by - 16, 48, 32, 0x0020);
                canvas.drawRect(cx - 24, by - 16, 48, 32, 0x07E0);
                // Sine wave
                for (int x = -20; x <= 20; x += 2) {
                    int wy = (int)(sin((x + millis() * 0.02f) * 0.3f) * 8.0f);
                    canvas.drawPixel(cx + x, by + wy, 0x07E0);
                }
                break;
            }
            case 2: { // Clock Icon
                canvas.fillCircle(cx, by, 18, 0x0861);
                canvas.drawCircle(cx, by, 18, 0x07FF);
                // Hands
                canvas.drawLine(cx, by, cx, by - 10, TFT_WHITE);
                canvas.drawLine(cx, by, cx + 9, by, 0x07FF);
                canvas.fillCircle(cx, by, 2, 0xFD20);
                break;
            }
            case 3: { // Space Shooter Icon
                canvas.fillTriangle(cx + 12, by, cx - 16, by - 11, cx - 10, by, 0x07FF);
                canvas.fillTriangle(cx + 12, by, cx - 16, by + 11, cx - 10, by, 0x07FF);
                canvas.fillRoundRect(cx - 8, by - 4, 18, 8, 3, TFT_WHITE);
                canvas.fillTriangle(cx - 10, by - 3, cx - 18, by, cx - 10, by + 3, 0xF800);
                canvas.fillRoundRect(cx + 15, by - 4, 8, 2, 1, 0x07FF);
                canvas.fillRoundRect(cx + 15, by + 4, 8, 2, 1, 0x07FF);
                break;
            }
            case 4: { // Snake Icon
                int kx = cx - 18;
                canvas.fillRoundRect(kx, by, 10, 10, 2, 0x2664);
                canvas.fillRoundRect(kx + 10, by, 10, 10, 2, 0x36E6);
                canvas.fillRoundRect(kx + 10, by - 10, 10, 10, 2, 0x47E8);
                canvas.fillRoundRect(kx + 20, by - 10, 10, 10, 2, 0x07E0);
                canvas.fillCircle(kx + 26, by - 7, 2, TFT_BLACK);
                canvas.fillCircle(kx + 42, by - 5, 6, 0xF800);
                break;
            }
            case 5: { // Hardware Monitor Icon
                canvas.fillRoundRect(cx - 16, by - 16, 32, 32, 4, 0x2124);
                canvas.drawRoundRect(cx - 16, by - 16, 32, 32, 4, 0x07FF);
                for (int p = -10; p <= 10; p += 5) {
                    canvas.drawFastHLine(cx - 20, by + p, 4, 0xFD20);
                    canvas.drawFastHLine(cx + 16, by + p, 4, 0xFD20);
                    canvas.drawFastVLine(cx + p, by - 20, 4, 0xFD20);
                    canvas.drawFastVLine(cx + p, by + 16, 4, 0xFD20);
                }
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.setTextColor(0x07FF);
                canvas.drawString("S3", cx, by, &fonts::Font2);
                break;
            }
        }
    }

    void draw() {
        canvas.fillScreen(0x0841); // Deep slate background

        // Top Status Bar
        canvas.fillRect(0, 0, SCREEN_W, 22, 0x10A2);
        canvas.drawFastHLine(0, 22, SCREEN_W, 0x2124);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07FF);
        canvas.drawString("LILYGO", 8, 4, &fonts::Font2);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(" OS", 56, 4, &fonts::Font2);

        // App counter [ X / 6 ]
        canvas.setTextDatum(textdatum_t::top_center);
        canvas.setTextColor(TFT_LIGHTGRAY);
        canvas.drawString("[" + String(selectedApp + 1) + " / " + String(TOTAL_APPS) + "]", SCREEN_W / 2, 5, &fonts::Font0);

        drawBatteryIndicator();

        // Central Card Showcase
        int cardW = 276;
        int cardH = 92;
        int cardX = (SCREEN_W - cardW) / 2;
        int cardY = 32;

        const AppMetadata& app = APP_INFO[selectedApp];

        canvas.fillRoundRect(cardX + 3, cardY + 3, cardW, cardH, 8, 0x0000);
        canvas.fillRoundRect(cardX, cardY, cardW, cardH, 8, 0x18C3);
        canvas.drawRoundRect(cardX, cardY, cardW, cardH, 8, app.primaryColor);

        // App Icon
        drawAppIcon(selectedApp, cardX + 46, cardY + cardH / 2);

        // App Title & Subtitle
        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(app.primaryColor);
        canvas.drawString(app.title, cardX + 94, cardY + 16, &fonts::Font4);

        canvas.setTextColor(TFT_LIGHTGRAY);
        canvas.drawString(app.subtitle, cardX + 94, cardY + 44, &fonts::Font0);

        // Pagination Dots
        int dotSpacing = 14;
        int dotsStartX = (SCREEN_W - ((TOTAL_APPS - 1) * dotSpacing)) / 2;
        int dotsY = cardY + cardH - 12;
        for (int i = 0; i < TOTAL_APPS; i++) {
            int dx = dotsStartX + (i * dotSpacing);
            if (i == selectedApp) {
                canvas.fillCircle(dx, dotsY, 3, app.primaryColor);
            } else {
                canvas.drawCircle(dx, dotsY, 2, 0x4A69);
            }
        }

        // Bottom Navigation Bar
        canvas.fillRect(0, SCREEN_H - 22, SCREEN_W, 22, 0x10A2);
        canvas.drawFastHLine(0, SCREEN_H - 22, SCREEN_W, 2124);

        canvas.setTextDatum(textdatum_t::bottom_left);
        canvas.setTextColor(0xFD20);
        canvas.drawString("BTN1: NEXT", 12, SCREEN_H - 4, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::bottom_right);
        canvas.setTextColor(0x07E0);
        canvas.drawString("BTN2: LAUNCH", SCREEN_W - 12, SCREEN_H - 4, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::bottom_center);
        canvas.setTextColor(0x7BC0);
        canvas.drawString("(Hold 1.5s in app to exit)", SCREEN_W / 2, SCREEN_H - 4, &fonts::Font0);
    }
};

extern AppLauncher launcher;
