#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <CST816_TouchLib.h>
#include "Display_Config.h"

#define PIN_TOUCH_SDA 18
#define PIN_TOUCH_SCL 17
#define PIN_TOUCH_INT 16
#define PIN_TOUCH_RST 21

class InputManager {
public:
    MDO::CST816Touch_SWMode touchController;
    bool touchAvailable = false;
    bool homeButtonPressed = false;
    bool screenTouched = false;
    int touchX = -1;
    int touchY = -1;

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

        // Initialize I2C for CST816 touch controller
        Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
        Wire.setClock(400000);
        touchAvailable = touchController.begin(Wire, PIN_TOUCH_INT, PIN_TOUCH_RST);
        if (touchAvailable) {
            touchController.setSwapXY(true);
            touchController.setInvertY(true, 170); // Invert Y to match LCD coordinate space (y=0 at top)
        }
    }

    void update(bool enableExitHold = true) {
        uint32_t now = millis();

        // 1. Read Physical Buttons
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
        homeButtonPressed = false;
        screenTouched = false;

        // 2. Read Capacitive Touch & Home Circle Button
        if (touchAvailable) {
            touchController.control();
            MDO::TouchScreenEventCache* pTouchCache = MDO::TouchScreenEventCache::getInstance();

            if (pTouchCache->hadGesture()) {
                MDO::TouchScreenController::gesture_t gesture = MDO::TouchScreenController::gesture_t::GESTURE_NONE;
                int gx = 0, gy = 0;
                pTouchCache->getLastGesture(gesture, gx, gy);
                if (gesture == MDO::TouchScreenController::gesture_t::GESTURE_TOUCH_BUTTON) {
                    homeButtonPressed = true;
                }
            }

            if (pTouchCache->hadTouch()) {
                int tx = 0, ty = 0;
                pTouchCache->getLastTouchPosition(tx, ty);
                touchX = tx;
                touchY = ty;

                // T-Display-S3 Touch: circle button on bezel sits beyond 315px horizontally
                if (tx >= 315) {
                    homeButtonPressed = true;
                } else {
                    screenTouched = true;
                }
            }
        }

        // 3. Exit Handling
        if (enableExitHold) {
            // Home button touched -> Instant exit to main menu!
            if (homeButtonPressed) {
                exitTriggered = true;
                return;
            }

            // Long-press backup (holding physical buttons for 1.5s)
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
                    pressStartLeft = now + 10000;
                    pressStartRight = now + 10000;
                }
            }
        }
    }

    void drawExitOverlay() {
        if (exitProgress > 0.05f) {
            int barW = 86;
            int barH = 14;
            int x = SCREEN_W - barW - 6;
            int y = 5;

            canvas.fillRoundRect(x, y, barW, barH, 4, 0x18C3);
            canvas.drawRoundRect(x, y, barW, barH, 4, TFT_WHITE);

            int fillW = (int)((barW - 4) * exitProgress);
            if (fillW > barW - 4) fillW = barW - 4;
            canvas.fillRoundRect(x + 2, y + 2, fillW, barH - 4, 2, 0xF800);

            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("EXITING...", x + barW / 2, y + barH / 2, &fonts::Font0);
        }
    }
};

extern InputManager input;
