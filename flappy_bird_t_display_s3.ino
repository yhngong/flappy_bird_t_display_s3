#include "Display_Config.h"
#include "Input.h"
#include "App_Launcher.h"
#include "App_FlappyBird.h"
#include "App_Sensors.h"
#include "App_Clock.h"
#include "App_SpaceShooter.h"
#include "App_Snake.h"
#include "App_Dashboard.h"

// Global hardware and app instances
LGFX_TDisplayS3 display;
LGFX_Sprite canvas(&display);
InputManager input;
AppLauncher launcher;
AppFlappyBird appFlappy;
AppSensors appSensors;
AppClock appClock;
AppSpaceShooter appSpace;
AppSnake appSnake;
AppDashboard appDashboard;

enum CurrentApp {
    APP_MENU = -1,
    APP_FLAPPY = 0,
    APP_SENSORS = 1,
    APP_CLOCK = 2,
    APP_SPACE = 3,
    APP_SNAKE = 4,
    APP_DASHBOARD = 5
};

CurrentApp activeApp = APP_MENU;

void setup() {
    Serial.begin(115200);

    input.init();

    // Initialize display & backlight
    display.init();
    display.setRotation(1); // Landscape 320x170
    display.setBrightness(220);

    // Create double-buffer canvas in RAM
    canvas.setColorDepth(16);
    canvas.createSprite(SCREEN_W, SCREEN_H);

    launcher.init();
}

void loop() {
    uint32_t frameStart = millis();

    // Enable 1.5s long-press exit detection only when inside an app
    bool inApp = (activeApp != APP_MENU);
    input.update(inApp);

    // Long-press exit trigger: return to main menu
    if (inApp && input.exitTriggered) {
        activeApp = APP_MENU;
        launcher.init();
    }

    if (activeApp == APP_MENU) {
        int launchSelection = launcher.update();
        if (launchSelection >= 0) {
            activeApp = (CurrentApp)launchSelection;
            switch (activeApp) {
                case APP_FLAPPY:    appFlappy.init(); break;
                case APP_SENSORS:   appSensors.init(); break;
                case APP_CLOCK:     appClock.init(); break;
                case APP_SPACE:     appSpace.init(); break;
                case APP_SNAKE:     appSnake.init(); break;
                case APP_DASHBOARD: appDashboard.init(); break;
                default: break;
            }
        }
        launcher.draw();
    } else {
        // Run active app
        switch (activeApp) {
            case APP_FLAPPY:
                appFlappy.update();
                appFlappy.draw();
                break;
            case APP_SENSORS:
                appSensors.update();
                appSensors.draw();
                break;
            case APP_CLOCK:
                appClock.update();
                appClock.draw();
                break;
            case APP_SPACE:
                appSpace.update();
                appSpace.draw();
                break;
            case APP_SNAKE:
                appSnake.update();
                appSnake.draw();
                break;
            case APP_DASHBOARD:
                appDashboard.update();
                appDashboard.draw();
                break;
            default:
                break;
        }

        // Draw exit hold feedback progress bar if holding button
        input.drawExitOverlay();
    }

    // Push rendered frame to ST7789 display
    canvas.pushSprite(0, 0);

    // Maintain stable ~50 FPS
    uint32_t elapsed = millis() - frameStart;
    if (elapsed < 20) {
        delay(20 - elapsed);
    }
}
