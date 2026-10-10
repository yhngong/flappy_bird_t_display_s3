#pragma once
#include <Arduino.h>
#include "Display_Config.h"

class InputManager {
public:
    bool downLeft = false;
    bool downRight = false;
    bool justPressedLeft = false;
    bool justPressedRight = false;
    bool justReleasedLeft = false;
    bool justReleasedRight = false;

    uint32_t pressStartLeft = 0;
    uint32_t pressStartRight = 0;

    bool exitTriggered = false;
    float exitProgress = 0.0f; // 0.0 to 1.0

    void init() {
        pinMode(PIN_BTN1, INPUT_PULLUP);
        pinMode(PIN_BTN2, INPUT_PULLUP);
    }

    void update(bool enableExitHold = true) {
        uint32_t now = millis();

        bool rawLeft = (digitalRead(PIN_BTN1) == LOW);
        bool rawRight = (digitalRead(PIN_BTN2) == LOW);

        justPressedLeft = (rawLeft && !downLeft);
        justPressedRight = (rawRight && !downRight);
        justReleasedLeft = (!rawLeft && downLeft);
        justReleasedRight = (!rawRight && downRight);

        if (justPressedLeft) pressStartLeft = now;
        if (justPressedRight) pressStartRight = now;

        downLeft = rawLeft;
        downRight = rawRight;

        exitTriggered = false;
        exitProgress = 0.0f;

        if (enableExitHold) {
            uint32_t holdTime = 0;
            if (downLeft) holdTime = max(holdTime, now - pressStartLeft);
            if (downRight) holdTime = max(holdTime, now - pressStartRight);

            const uint32_t EXIT_HOLD_MS = 1500;
            if (holdTime >= 350) {
                exitProgress = (float)(holdTime - 350) / (float)(EXIT_HOLD_MS - 350);
                if (exitProgress > 1.0f) exitProgress = 1.0f;

                if (holdTime >= EXIT_HOLD_MS) {
                    exitTriggered = true;
                    exitProgress = 0.0f;
                    pressStartLeft = now + 10000;  // Prevent re-triggering until released
                    pressStartRight = now + 10000;
                }
            }
        }
    }

    void drawExitOverlay() {
        if (exitProgress > 0.05f) {
            // Draw a compact, elegant exit progress bar in the top-right corner
            int barW = 86;
            int barH = 14;
            int x = SCREEN_W - barW - 6;
            int y = 5;

            canvas.fillRoundRect(x, y, barW, barH, 4, 0x18C3);
            canvas.drawRoundRect(x, y, barW, barH, 4, TFT_WHITE);

            int fillW = (int)((barW - 4) * exitProgress);
            if (fillW > barW - 4) fillW = barW - 4;
            canvas.fillRoundRect(x + 2, y + 2, fillW, barH - 4, 2, 0xF800); // Red progress

            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("EXITING...", x + barW / 2, y + barH / 2, &fonts::Font0);
        }
    }
};

extern InputManager input;
