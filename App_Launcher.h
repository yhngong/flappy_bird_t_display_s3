#pragma once
#include <Arduino.h>
#include <time.h>
#include "Display_Config.h"
#include "Input.h"

class AppClock;
extern AppClock appClock;
extern time_t getCurrentTime();

struct AppMetadata {
    const char* title;
    const char* label;
    uint16_t primaryColor;
    uint16_t bgColor;
};

const int TOTAL_APPS = 6;

const AppMetadata APP_INFO[TOTAL_APPS] = {
    { "Flappy Bird",    "Flappy",   0xFDE0, 0x35BF }, // Sky blue
    { "Sensor Scope",   "Sensors",  0x07E0, 0x0842 }, // Cyber slate
    { "Desk Clock",     "Clock",    0x07FF, 0x18C3 }, // Dark charcoal
    { "Space Shooter",  "Shooter",  0x07FF, 0x10A8 }, // Deep space
    { "Retro Snake",    "Snake",    0x07E0, 0x0A42 }, // Emerald green
    { "System Info",    "System",   0x07FF, 0x2967 }  // Titanium slate
};

class AppLauncher {
public:
    int selectedApp = 0;
    const int ICON_SIZE = 44;
    const int ICON_RADIUS = 10;

    void init() {}

    int getAppAtCoordinates(int tx, int ty) {
        // Exclude status bar (0..18) and bottom home indicator (162..170)
        if (ty < 18 || ty > 162) return -1;
        if (tx < 0 || tx >= 320) return -1;

        // 3 wide touch columns:
        // Col 0: 0..106 px (Flappy / Shooter)
        // Col 1: 107..213 px (Sensors / Snake)
        // Col 2: 214..319 px (Clock / System)
        int col = -1;
        if (tx < 107) col = 0;
        else if (tx < 214) col = 1;
        else col = 2;

        // 2 tall touch rows:
        // Row 0: 18..89 px (Top 3 apps)
        // Row 1: 90..162 px (Bottom 3 apps)
        int row = (ty < 90) ? 0 : 1;

        int appIdx = (row * 3) + col;
        if (appIdx >= 0 && appIdx < TOTAL_APPS) {
            return appIdx;
        }
        return -1;
    }

    int update() {
        // 1. Touch Input: Tap directly on any icon to launch!
        if (input.screenTouched) {
            int tappedApp = getAppAtCoordinates(input.touchX, input.touchY);
            if (tappedApp >= 0 && tappedApp < TOTAL_APPS) {
                selectedApp = tappedApp;
                return tappedApp; // Instantly launch tapped app!
            }
        }

        // 2. Physical Button Fallback
        // Button 1: Cycle selection
        if (input.justPressedLeft) {
            selectedApp = (selectedApp + 1) % TOTAL_APPS;
        }

        // Button 2: Launch selected
        if (input.justPressedRight) {
            return selectedApp;
        }

        return -1;
    }

    void drawStatusBar() {
        // Translucent top status bar
        canvas.fillRect(0, 0, SCREEN_W, 18, 0x10C4);
        canvas.drawFastHLine(0, 18, SCREEN_W, 0x2126);

        // San Francisco Bay Area Time (HH:MM)
        time_t now = appClock.getCurrentTime();
        struct tm* tmInfo = localtime(&now);
        char timeStr[10];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", tmInfo->tm_hour, tmInfo->tm_min);

        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(timeStr, 12, 9, &fonts::Font0);

        // Live Touch Coordinates in center
        if (input.touchX >= 0) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(0x07FF);
            canvas.drawString("T:(" + String(input.touchX) + "," + String(input.touchY) + ")", SCREEN_W / 2, 9, &fonts::Font0);
        }

        // Battery Pill on Right
        int rawMv = analogReadMilliVolts(PIN_BATTERY);
        float vbat = (rawMv * 2.0f) / 1000.0f;
        int pct = (int)((vbat - 3.3f) / (4.2f - 3.3f) * 100.0f);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;

        int bx = SCREEN_W - 32;
        int by = 4;
        canvas.drawRoundRect(bx, by, 20, 10, 2, TFT_WHITE);
        canvas.fillRect(bx + 20, by + 3, 2, 4, TFT_WHITE); // battery terminal tip
        int fillW = (pct * 16) / 100;
        uint16_t bColor = (pct > 20) ? 0x07E0 : 0xF800;
        if (fillW > 0) canvas.fillRect(bx + 2, by + 2, fillW, 6, bColor);

        canvas.setTextDatum(textdatum_t::middle_right);
        canvas.setTextColor(TFT_LIGHTGRAY);
        canvas.drawString(String(pct) + "%", bx - 5, 9, &fonts::Font0);
    }

    void drawWallpaper() {
        // iOS-style twilight wallpaper gradient
        for (int y = 0; y < SCREEN_H; y++) {
            // Gradient from deep midnight blue (0x08D2) to rich indigo purple (0x3891)
            float t = (float)y / (float)SCREEN_H;
            uint8_t r = (uint8_t)(8 + t * 40);
            uint8_t g = (uint8_t)(16 + t * 20);
            uint8_t b = (uint8_t)(45 + t * 55);
            uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            canvas.drawFastHLine(0, y, SCREEN_W, color);
        }

        // Soft blurred ambient wallpaper orbs
        canvas.fillCircle(40, 130, 60, 0x19B4);
        canvas.fillCircle(280, 45, 55, 0x3936);
        canvas.fillCircle(180, 140, 45, 0x2174);
    }

    void drawAppIcon(int appIdx, int ix, int iy) {
        const AppMetadata& app = APP_INFO[appIdx];

        // 1. Icon Drop Shadow
        canvas.fillRoundRect(ix + 2, iy + 3, ICON_SIZE, ICON_SIZE, ICON_RADIUS, 0x0842);

        // 2. Icon Squircle Base
        canvas.fillRoundRect(ix, iy, ICON_SIZE, ICON_SIZE, ICON_RADIUS, app.bgColor);
        canvas.drawRoundRect(ix, iy, ICON_SIZE, ICON_SIZE, ICON_RADIUS, 0x6B6D);

        // 3. Highlight Ring if selected via physical button
        if (selectedApp == appIdx) {
            canvas.drawRoundRect(ix - 2, iy - 2, ICON_SIZE + 4, ICON_SIZE + 4, ICON_RADIUS + 2, TFT_WHITE);
            canvas.drawRoundRect(ix - 3, iy - 3, ICON_SIZE + 6, ICON_SIZE + 6, ICON_RADIUS + 3, 0x07FF);
        }

        int cx = ix + ICON_SIZE / 2;
        int cy = iy + ICON_SIZE / 2;

        // 4. Custom Icon Graphics
        switch (appIdx) {
            case 0: { // Flappy Bird Icon
                // Green pipe fragment
                canvas.fillRect(ix + 5, iy + 18, 9, 20, 0x2DE2);
                canvas.drawRect(ix + 5, iy + 18, 9, 20, TFT_BLACK);
                canvas.fillRect(ix + 4, iy + 15, 11, 4, 0x4745);
                canvas.drawRect(ix + 4, iy + 15, 11, 4, TFT_BLACK);

                // Bird body
                canvas.fillCircle(cx + 4, cy, 9, 0xFDE0);
                canvas.drawCircle(cx + 4, cy, 9, TFT_BLACK);
                canvas.fillCircle(cx + 8, cy - 3, 3, TFT_WHITE);
                canvas.fillCircle(cx + 9, cy - 3, 2, TFT_BLACK);
                canvas.fillTriangle(cx + 10, cy - 1, cx + 18, cy + 2, cx + 10, cy + 4, 0xFC40);
                canvas.drawTriangle(cx + 10, cy - 1, cx + 18, cy + 2, cx + 10, cy + 4, TFT_BLACK);
                canvas.fillRoundRect(cx - 3, cy - 1, 8, 5, 2, TFT_WHITE);
                canvas.drawRoundRect(cx - 3, cy - 1, 8, 5, 2, TFT_BLACK);
                break;
            }
            case 1: { // Sensors Icon (Oscilloscope scope wave)
                canvas.fillRect(ix + 4, iy + 4, ICON_SIZE - 8, ICON_SIZE - 8, 0x0020);
                canvas.drawRect(ix + 4, iy + 4, ICON_SIZE - 8, ICON_SIZE - 8, 0x07E0);
                // Grid ticks
                canvas.drawFastHLine(ix + 6, cy, ICON_SIZE - 12, 0x01E0);
                canvas.drawFastVLine(cx, iy + 6, ICON_SIZE - 12, 0x01E0);
                // Pulse Wave
                canvas.drawLine(ix + 7, cy, cx - 7, cy, 0x07E0);
                canvas.drawLine(cx - 7, cy, cx - 3, cy - 10, 0x07E0);
                canvas.drawLine(cx - 3, cy - 10, cx + 3, cy + 10, 0x07E0);
                canvas.drawLine(cx + 3, cy + 10, cx + 7, cy, 0x07E0);
                canvas.drawLine(cx + 7, cy, ix + ICON_SIZE - 7, cy, 0x07E0);
                break;
            }
            case 2: { // Clock Icon (Working live analog hands!)
                canvas.fillCircle(cx, cy, 17, 0x0000);
                canvas.drawCircle(cx, cy, 17, TFT_WHITE);
                // Hour markers
                for (int h = 0; h < 12; h += 3) {
                    float a = h * 30.0f * 3.14159f / 180.0f;
                    int hx = cx + sin(a) * 14;
                    int hy = cy - cos(a) * 14;
                    canvas.drawPixel(hx, hy, TFT_WHITE);
                }
                // Live hands matching SF Bay Area time
                time_t now = appClock.getCurrentTime();
                struct tm* t = localtime(&now);
                int hr = t ? t->tm_hour : 10;
                int mn = t ? t->tm_min : 10;
                int sc = t ? t->tm_sec : 30;

                float hA = ((hr % 12) * 30.0f + mn * 0.5f) * 3.14159f / 180.0f;
                canvas.drawLine(cx, cy, cx + sin(hA) * 9, cy - cos(hA) * 9, TFT_WHITE);
                float mA = (mn * 6.0f) * 3.14159f / 180.0f;
                canvas.drawLine(cx, cy, cx + sin(mA) * 12, cy - cos(mA) * 12, 0x07FF);
                float sA = (sc * 6.0f) * 3.14159f / 180.0f;
                canvas.drawLine(cx, cy, cx + sin(sA) * 13, cy - cos(sA) * 13, 0xF800);
                canvas.fillCircle(cx, cy, 2, 0xF800);
                break;
            }
            case 3: { // Space Shooter Icon
                // Stars
                canvas.drawPixel(ix + 9, iy + 9, TFT_WHITE);
                canvas.drawPixel(ix + 35, iy + 12, 0xFD20);
                canvas.drawPixel(ix + 12, iy + 34, 0x07FF);
                // Ship
                canvas.fillTriangle(cx + 8, cy, cx - 12, cy - 8, cx - 8, cy, 0x07FF);
                canvas.fillTriangle(cx + 8, cy, cx - 12, cy + 8, cx - 8, cy, 0x07FF);
                canvas.fillRoundRect(cx - 6, cy - 3, 14, 6, 2, TFT_WHITE);
                // Engine & laser
                canvas.fillTriangle(cx - 8, cy - 2, cx - 14, cy, cx - 8, cy + 2, 0xF800);
                canvas.fillRoundRect(cx + 10, cy - 2, 6, 2, 1, 0x07FF);
                break;
            }
            case 4: { // Snake Icon
                int sx = ix + 10;
                int sy = cy;
                canvas.fillRoundRect(sx, sy, 7, 7, 2, 0x2664);
                canvas.fillRoundRect(sx + 7, sy, 7, 7, 2, 0x36E6);
                canvas.fillRoundRect(sx + 7, sy - 7, 7, 7, 2, 0x07E0); // Head
                canvas.drawPixel(sx + 11, sy - 5, TFT_BLACK);          // Eye
                // Apple
                canvas.fillCircle(ix + 32, cy - 4, 5, 0xF800);
                canvas.drawPixel(ix + 32, cy - 9, 0x07E0); // Leaf
                break;
            }
            case 5: { // System Monitor Icon (Settings Gear & Chip)
                canvas.fillRoundRect(cx - 12, cy - 12, 24, 24, 4, 0x2124);
                canvas.drawRoundRect(cx - 12, cy - 12, 24, 24, 4, 0x07FF);
                for (int p = -8; p <= 8; p += 4) {
                    canvas.drawFastHLine(cx - 15, cy + p, 3, 0xFD20);
                    canvas.drawFastHLine(cx + 12, cy + p, 3, 0xFD20);
                    canvas.drawFastVLine(cx + p, cy - 15, 3, 0xFD20);
                    canvas.drawFastVLine(cx + p, cy + 12, 3, 0xFD20);
                }
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.setTextColor(0x07FF);
                canvas.drawString("S3", cx, cy, &fonts::Font0);
                break;
            }
        }

        // 5. App Title Label below icon (with subtle drop shadow)
        canvas.setTextDatum(textdatum_t::top_center);
        canvas.setTextColor(0x18C3);
        canvas.drawString(app.label, cx + 1, iy + ICON_SIZE + 4, &fonts::Font0);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(app.label, cx, iy + ICON_SIZE + 3, &fonts::Font0);
    }

    void draw() {
        drawWallpaper();
        drawStatusBar();

        // 2 Rows x 3 Columns Layout
        const int COL_X[3] = { 32, 138, 244 };
        const int ROW_Y[2] = { 26, 92 };

        for (int i = 0; i < TOTAL_APPS; i++) {
            int col = i % 3;
            int row = i / 3;
            drawAppIcon(i, COL_X[col], ROW_Y[row]);
        }

        // iOS Home Bar Indicator at bottom center
        canvas.fillRoundRect(120, 162, 80, 4, 2, 0xAD55);

        // Touch feedback ripple
        if (input.touchX >= 0 && input.touchY >= 0) {
            canvas.drawCircle(input.touchX, input.touchY, 8, TFT_CYAN);
            canvas.drawCircle(input.touchX, input.touchY, 9, TFT_WHITE);
        }
    }
};

extern AppLauncher launcher;
