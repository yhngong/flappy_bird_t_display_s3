#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Display_Config.h"
#include "Input.h"

class AppSnake {
public:
    static const int CELL_SIZE = 8;
    static const int GRID_W = 40; // 40 * 8 = 320
    static const int GRID_H = 19; // 19 * 8 = 152 (y: 18 to 170)
    static const int GRID_OFFSET_Y = 18;

    enum State { READY, PLAY, GAMEOVER };
    State state = READY;

    // Directions: 0: UP, 1: RIGHT, 2: DOWN, 3: LEFT
    int dir = 1; // start moving Right
    int nextDir = 1;

    struct Point {
        int8_t x, y;
    };

    static const int MAX_SNAKE_LEN = 140;
    Point snake[MAX_SNAKE_LEN];
    int snakeLen = 4;

    Point food;
    Point bonusFood;
    bool hasBonus = false;
    uint32_t bonusSpawnTime = 0;

    int score = 0;
    int highScore = 0;
    bool isNewHigh = false;
    uint32_t stateTimer = 0;
    uint32_t lastMoveTime = 0;
    int moveDelay = 110; // ms per tick

    Preferences prefs;

    void init() {
        prefs.begin("snake", false);
        highScore = prefs.getInt("highscore", 0);
        reset();
    }

    void reset() {
        snakeLen = 4;
        dir = 1;
        nextDir = 1;
        score = 0;
        isNewHigh = false;
        hasBonus = false;
        moveDelay = 115;
        state = READY;

        // Initial snake position in center
        int startX = GRID_W / 3;
        int startY = GRID_H / 2;
        for (int i = 0; i < snakeLen; i++) {
            snake[i] = { (int8_t)(startX - i), (int8_t)startY };
        }

        spawnFood();
    }

    void spawnFood() {
        bool valid = false;
        while (!valid) {
            food.x = random(1, GRID_W - 1);
            food.y = random(1, GRID_H - 1);
            valid = true;
            for (int i = 0; i < snakeLen; i++) {
                if (snake[i].x == food.x && snake[i].y == food.y) {
                    valid = false;
                    break;
                }
            }
        }
    }

    void spawnBonus() {
        hasBonus = true;
        bonusSpawnTime = millis();
        bonusFood.x = random(2, GRID_W - 2);
        bonusFood.y = random(2, GRID_H - 2);
    }

    void update() {
        // 2-Button Controls:
        // Left Button: Turn Left (-1)
        // Right Button: Turn Right (+1)
        if (input.justPressedLeft) {
            nextDir = (dir + 3) % 4; // Turn Left (counter-clockwise)
        }
        if (input.justPressedRight) {
            nextDir = (dir + 1) % 4; // Turn Right (clockwise)
        }

        bool btnPressed = (input.justPressedLeft || input.justPressedRight);

        switch (state) {
            case READY: {
                if (btnPressed) {
                    state = PLAY;
                    lastMoveTime = millis();
                }
                break;
            }

            case PLAY: {
                uint32_t now = millis();

                // Check bonus food expiry (6 seconds)
                if (hasBonus && (now - bonusSpawnTime > 6000)) {
                    hasBonus = false;
                }

                if (now - lastMoveTime >= (uint32_t)moveDelay) {
                    lastMoveTime = now;
                    dir = nextDir;

                    // Calculate new head
                    Point newHead = snake[0];
                    if (dir == 0) newHead.y--;
                    else if (dir == 1) newHead.x++;
                    else if (dir == 2) newHead.y++;
                    else if (dir == 3) newHead.x--;

                    // Wall collision
                    if (newHead.x < 0 || newHead.x >= GRID_W || newHead.y < 0 || newHead.y >= GRID_H) {
                        triggerDeath();
                        return;
                    }

                    // Self collision
                    for (int i = 0; i < snakeLen; i++) {
                        if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
                            triggerDeath();
                            return;
                        }
                    }

                    // Check food consumption
                    bool ate = false;
                    if (newHead.x == food.x && newHead.y == food.y) {
                        score += 10;
                        ate = true;
                        if (snakeLen < MAX_SNAKE_LEN) snakeLen++;
                        spawnFood();

                        // Speed up slightly as snake grows
                        if (moveDelay > 55) moveDelay -= 2;

                        // Spawn bonus food every 5 food items
                        if ((score / 10) % 5 == 0 && !hasBonus) {
                            spawnBonus();
                        }
                    } else if (hasBonus && newHead.x == bonusFood.x && newHead.y == bonusFood.y) {
                        score += 35;
                        hasBonus = false;
                        if (snakeLen < MAX_SNAKE_LEN) snakeLen++;
                    }

                    // Move body
                    for (int i = snakeLen - 1; i > 0; i--) {
                        snake[i] = snake[i - 1];
                    }
                    snake[0] = newHead;
                }
                break;
            }

            case GAMEOVER: {
                if (millis() - stateTimer > 500 && btnPressed) {
                    reset();
                }
                break;
            }
        }
    }

    void triggerDeath() {
        state = GAMEOVER;
        stateTimer = millis();
        if (score > highScore) {
            highScore = score;
            isNewHigh = true;
            prefs.putInt("highscore", highScore);
        }
    }

    void draw() {
        canvas.fillScreen(0x0164); // Dark forest green

        // Top Header
        canvas.fillRect(0, 0, SCREEN_W, GRID_OFFSET_Y, 0x0A85);
        canvas.drawFastHLine(0, GRID_OFFSET_Y, SCREEN_W, 0x1AE9);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07E0);
        canvas.drawString("SNAKE", 8, 3, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::top_center);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString("SCORE: " + String(score), SCREEN_W / 2, 3, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::top_right);
        canvas.setTextColor(0xFD20);
        canvas.drawString("BEST: " + String(highScore), SCREEN_W - 8, 3, &fonts::Font0);

        // Draw Food (Apple)
        int fx = food.x * CELL_SIZE;
        int fy = GRID_OFFSET_Y + (food.y * CELL_SIZE);
        canvas.fillCircle(fx + CELL_SIZE / 2, fy + CELL_SIZE / 2, 3, 0xF800);
        canvas.drawPixel(fx + CELL_SIZE / 2 + 1, fy + 1, 0x07E0); // Leaf

        // Draw Bonus Star (Golden fruit)
        if (hasBonus) {
            int bx = bonusFood.x * CELL_SIZE;
            int by = GRID_OFFSET_Y + (bonusFood.y * CELL_SIZE);
            if ((millis() / 150) % 2 == 0) {
                canvas.fillCircle(bx + CELL_SIZE / 2, by + CELL_SIZE / 2, 4, 0xFD20);
                canvas.drawCircle(bx + CELL_SIZE / 2, by + CELL_SIZE / 2, 4, TFT_WHITE);
            }
        }

        // Draw Snake Body
        for (int i = snakeLen - 1; i >= 0; i--) {
            int sx = snake[i].x * CELL_SIZE;
            int sy = GRID_OFFSET_Y + (snake[i].y * CELL_SIZE);

            if (i == 0) { // Head
                canvas.fillRoundRect(sx, sy, CELL_SIZE, CELL_SIZE, 2, 0x07E0);
                // Eyes
                canvas.drawPixel(sx + 2, sy + 2, TFT_BLACK);
                canvas.drawPixel(sx + 5, sy + 2, TFT_BLACK);
            } else { // Body segments (gradient from light green to darker green)
                uint16_t segColor = (i % 2 == 0) ? 0x2664 : 0x36E6;
                canvas.fillRoundRect(sx + 1, sy + 1, CELL_SIZE - 2, CELL_SIZE - 2, 1, segColor);
            }
        }

        // State Overlays
        if (state == READY) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(0x07E0);
            canvas.drawString("RETRO SNAKE", SCREEN_W / 2, 60, &fonts::Font4);

            canvas.setTextColor(0xFEE0);
            canvas.drawString("BTN1: TURN LEFT  |  BTN2: TURN RIGHT", SCREEN_W / 2, 100, &fonts::Font2);

            if ((millis() / 450) % 2 == 0) {
                canvas.setTextColor(TFT_WHITE);
                canvas.drawString("PRESS ANY BUTTON TO SLITHER", SCREEN_W / 2, 135, &fonts::Font2);
            }
        } else if (state == GAMEOVER) {
            int bw = 210, bh = 95;
            int bx = (SCREEN_W - bw) / 2, by = 35;
            canvas.fillRoundRect(bx + 3, by + 3, bw, bh, 8, 0x0000);
            canvas.fillRoundRect(bx, by, bw, bh, 8, 0x18C3);
            canvas.drawRoundRect(bx, by, bw, bh, 8, 0x07E0);

            canvas.setTextDatum(textdatum_t::top_center);
            canvas.setTextColor(0xF800);
            canvas.drawString("GAME OVER", SCREEN_W / 2, by + 8, &fonts::Font4);

            canvas.setTextDatum(textdatum_t::top_left);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("SCORE: " + String(score), bx + 24, by + 42, &fonts::Font2);
            canvas.drawString("BEST:  " + String(highScore), bx + 24, by + 60, &fonts::Font2);

            if (isNewHigh) {
                canvas.setTextDatum(textdatum_t::top_right);
                canvas.setTextColor(0xFD20);
                canvas.drawString("NEW!", bx + bw - 20, by + 42, &fonts::Font2);
            }

            if ((millis() / 400) % 2 == 0) {
                canvas.setTextDatum(textdatum_t::bottom_center);
                canvas.setTextColor(TFT_WHITE);
                canvas.drawString("PRESS BUTTON TO RETRY", SCREEN_W / 2, SCREEN_H - 4, &fonts::Font2);
            }
        }
    }
};

extern AppSnake appSnake;
