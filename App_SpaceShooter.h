#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Display_Config.h"
#include "Input.h"

class AppSpaceShooter {
public:
    enum State { READY, PLAY, GAMEOVER };
    State state = READY;

    // Player
    float playerX = 32.0f;
    float playerY = 85.0f;
    const float PLAYER_SPEED = 3.6f;
    int playerHP = 3;
    const int MAX_HP = 3;
    uint32_t lastShootTime = 0;
    int invulnFrames = 0;

    // Lasers
    static const int MAX_BULLETS = 12;
    struct Bullet {
        float x, y;
        bool active;
    };
    Bullet bullets[MAX_BULLETS];

    // Enemies
    static const int MAX_ENEMIES = 6;
    struct Enemy {
        float x, y;
        float vx, vy;
        int hp;
        int type; // 0: Drone (fast), 1: Heavy, 2: Asteroid
        bool active;
    };
    Enemy enemies[MAX_ENEMIES];

    // Starfield
    static const int STAR_COUNT = 32;
    struct Star {
        float x, y;
        float speed;
        uint16_t color;
    };
    Star stars[STAR_COUNT];

    // Particle explosions
    static const int MAX_PARTICLES = 24;
    struct Particle {
        float x, y;
        float vx, vy;
        int life;
        uint16_t color;
        bool active;
    };
    Particle particles[MAX_PARTICLES];

    int score = 0;
    int highScore = 0;
    bool isNewHigh = false;
    uint32_t stateTimer = 0;

    Preferences prefs;

    void init() {
        prefs.begin("space", false);
        highScore = prefs.getInt("highscore", 0);

        // Init starfield
        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].x = random(0, SCREEN_W);
            stars[i].y = random(0, SCREEN_H);
            stars[i].speed = random(10, 40) / 10.0f;
            if (stars[i].speed > 3.0f) stars[i].color = TFT_WHITE;
            else if (stars[i].speed > 2.0f) stars[i].color = 0x07FF;
            else stars[i].color = 0x31A6;
        }

        reset();
    }

    void reset() {
        playerY = SCREEN_H / 2;
        playerHP = MAX_HP;
        score = 0;
        isNewHigh = false;
        invulnFrames = 0;
        state = READY;

        for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
        for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
        for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    }

    void spawnExplosion(float x, float y) {
        for (int i = 0; i < 8; i++) {
            for (int p = 0; p < MAX_PARTICLES; p++) {
                if (!particles[p].active) {
                    particles[p].active = true;
                    particles[p].x = x;
                    particles[p].y = y;
                    float angle = (random(0, 360) * 3.14159f) / 180.0f;
                    float spd = random(15, 45) / 10.0f;
                    particles[p].vx = cos(angle) * spd;
                    particles[p].vy = sin(angle) * spd;
                    particles[p].life = random(10, 20);
                    particles[p].color = (random(0, 2) == 0) ? 0xFD20 : 0xF800;
                    break;
                }
            }
        }
    }

    void spawnEnemy() {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) {
                enemies[i].active = true;
                enemies[i].x = SCREEN_W + 15;
                enemies[i].y = random(20, SCREEN_H - 20);
                enemies[i].type = random(0, 3);

                if (enemies[i].type == 0) { // Fast drone
                    enemies[i].vx = -random(26, 40) / 10.0f;
                    enemies[i].vy = (random(0, 2) == 0 ? 1 : -1) * 0.8f;
                    enemies[i].hp = 1;
                } else if (enemies[i].type == 1) { // Heavy fighter
                    enemies[i].vx = -1.6f;
                    enemies[i].vy = 0;
                    enemies[i].hp = 3;
                } else { // Asteroid
                    enemies[i].vx = -2.2f;
                    enemies[i].vy = random(-5, 6) / 10.0f;
                    enemies[i].hp = 2;
                }
                break;
            }
        }
    }

    void update() {
        // Move stars in all states
        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].x -= stars[i].speed;
            if (stars[i].x < 0) {
                stars[i].x = SCREEN_W;
                stars[i].y = random(0, SCREEN_H);
            }
        }

        // Update particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;
                particles[i].life--;
                if (particles[i].life <= 0) particles[i].active = false;
            }
        }

        bool btnPressed = (input.justPressedLeft || input.justPressedRight);

        switch (state) {
            case READY: {
                playerY = (SCREEN_H / 2) + sin(millis() * 0.005f) * 12.0f;
                if (btnPressed) {
                    state = PLAY;
                    lastShootTime = millis();
                }
                break;
            }

            case PLAY: {
                // Movement: BTN1 = UP, BTN2 = DOWN
                if (input.downLeft) {
                    playerY -= PLAYER_SPEED;
                    if (playerY < 16) playerY = 16;
                }
                if (input.downRight) {
                    playerY += PLAYER_SPEED;
                    if (playerY > SCREEN_H - 16) playerY = SCREEN_H - 16;
                }

                // Automatic laser cannon (rapid twin-fire)
                uint32_t now = millis();
                if (now - lastShootTime >= 190) {
                    lastShootTime = now;
                    // Fire 2 bullets (top and bottom wing)
                    for (int side = -1; side <= 1; side += 2) {
                        for (int b = 0; b < MAX_BULLETS; b++) {
                            if (!bullets[b].active) {
                                bullets[b].active = true;
                                bullets[b].x = playerX + 14;
                                bullets[b].y = playerY + (side * 6);
                                break;
                            }
                        }
                    }
                }

                // Update bullets
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (bullets[i].active) {
                        bullets[i].x += 7.0f;
                        if (bullets[i].x > SCREEN_W + 10) bullets[i].active = false;
                    }
                }

                // Spawn enemies periodically
                if (random(0, 35) == 0) spawnEnemy();

                // Update enemies
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        enemies[i].x += enemies[i].vx;
                        enemies[i].y += enemies[i].vy;

                        if (enemies[i].y < 12 || enemies[i].y > SCREEN_H - 12) {
                            enemies[i].vy = -enemies[i].vy;
                        }

                        // Off-screen
                        if (enemies[i].x < -20) {
                            enemies[i].active = false;
                        }

                        // Collision with player bullets
                        for (int b = 0; b < MAX_BULLETS; b++) {
                            if (bullets[b].active) {
                                float dx = bullets[b].x - enemies[i].x;
                                float dy = bullets[b].y - enemies[i].y;
                                if (abs(dx) < 14 && abs(dy) < 12) {
                                    bullets[b].active = false;
                                    enemies[i].hp--;
                                    if (enemies[i].hp <= 0) {
                                        enemies[i].active = false;
                                        spawnExplosion(enemies[i].x, enemies[i].y);
                                        score += (enemies[i].type == 1) ? 30 : 10;
                                    }
                                }
                            }
                        }

                        // Collision with player
                        if (invulnFrames == 0) {
                            float pdx = playerX - enemies[i].x;
                            float pdy = playerY - enemies[i].y;
                            if (abs(pdx) < 18 && abs(pdy) < 14) {
                                enemies[i].active = false;
                                spawnExplosion(enemies[i].x, enemies[i].y);
                                playerHP--;
                                invulnFrames = 40; // ~0.8s invulnerability
                                if (playerHP <= 0) {
                                    spawnExplosion(playerX, playerY);
                                    state = GAMEOVER;
                                    stateTimer = millis();
                                    if (score > highScore) {
                                        highScore = score;
                                        isNewHigh = true;
                                        prefs.putInt("highscore", highScore);
                                    }
                                }
                            }
                        }
                    }
                }

                if (invulnFrames > 0) invulnFrames--;
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

    void draw() {
        canvas.fillScreen(TFT_BLACK);

        // Draw starfield
        for (int i = 0; i < STAR_COUNT; i++) {
            canvas.drawPixel((int)stars[i].x, (int)stars[i].y, stars[i].color);
        }

        // Draw particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                canvas.fillCircle((int)particles[i].x, (int)particles[i].y, 2, particles[i].color);
            }
        }

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                canvas.fillRoundRect((int)bullets[i].x, (int)bullets[i].y - 1, 9, 3, 1, 0x07FF);
                canvas.drawFastHLine((int)bullets[i].x + 1, (int)bullets[i].y, 7, TFT_WHITE);
            }
        }

        // Draw enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                int ex = (int)enemies[i].x;
                int ey = (int)enemies[i].y;
                if (enemies[i].type == 0) { // Fast Red Scout
                    canvas.fillTriangle(ex - 8, ey, ex + 8, ey - 6, ex + 8, ey + 6, 0xF800);
                    canvas.drawTriangle(ex - 8, ey, ex + 8, ey - 6, ex + 8, ey + 6, TFT_WHITE);
                    canvas.fillCircle(ex, ey, 2, 0xFD20);
                } else if (enemies[i].type == 1) { // Purple Heavy
                    canvas.fillRoundRect(ex - 10, ey - 9, 20, 18, 4, 0x780F);
                    canvas.drawRoundRect(ex - 10, ey - 9, 20, 18, 4, TFT_MAGENTA);
                    canvas.fillRect(ex - 4, ey - 2, 8, 4, 0x07FF);
                } else { // Asteroid
                    canvas.fillCircle(ex, ey, 8, 0x8C51);
                    canvas.drawCircle(ex, ey, 8, 0x52AA);
                    canvas.drawPixel(ex - 2, ey - 2, 0x52AA);
                    canvas.drawPixel(ex + 3, ey + 2, 0x52AA);
                }
            }
        }

        // Draw Player Ship (unless blinking in invulnerability)
        if (state != GAMEOVER && (invulnFrames % 4 < 2)) {
            int px = (int)playerX;
            int py = (int)playerY;

            // Thruster flame
            int flameLen = random(8, 16);
            canvas.fillTriangle(px - 10, py - 4, px - 10 - flameLen, py, px - 10, py + 4, 0xFD20);
            canvas.fillTriangle(px - 10, py - 2, px - 10 - (flameLen / 2), py, px - 10, py + 2, TFT_WHITE);

            // Ship Body
            canvas.fillTriangle(px + 14, py, px - 12, py - 10, px - 8, py, 0x04DF);
            canvas.fillTriangle(px + 14, py, px - 12, py + 10, px - 8, py, 0x04DF);
            canvas.fillRoundRect(px - 8, py - 4, 20, 8, 3, TFT_WHITE);
            canvas.fillRect(px + 2, py - 2, 6, 4, 0x07FF); // Cockpit

            // Shield / HP indicators
            for (int h = 0; h < MAX_HP; h++) {
                uint16_t c = (h < playerHP) ? 0x07E0 : 0x4208;
                canvas.fillRoundRect(10 + (h * 14), 8, 10, 6, 1, c);
            }
        }

        // HUD / Overlays
        if (state == PLAY) {
            canvas.setTextDatum(textdatum_t::top_right);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("SCORE: " + String(score), SCREEN_W - 10, 6, &fonts::Font2);
        } else if (state == READY) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(0x07FF);
            canvas.drawString("SPACE SHOOTER", SCREEN_W / 2, 40, &fonts::Font4);

            canvas.setTextColor(0xFEE0);
            canvas.drawString("BTN1: UP  |  BTN2: DOWN", SCREEN_W / 2, 85, &fonts::Font2);
            canvas.setTextColor(TFT_LIGHTGRAY);
            canvas.drawString("(Auto-Cannons Active)", SCREEN_W / 2, 105, &fonts::Font0);

            if ((millis() / 450) % 2 == 0) {
                canvas.setTextColor(TFT_WHITE);
                canvas.drawString("PRESS ANY BUTTON TO LAUNCH", SCREEN_W / 2, 135, &fonts::Font2);
            }
        } else if (state == GAMEOVER) {
            int bw = 210, bh = 95;
            int bx = (SCREEN_W - bw) / 2, by = 26;
            canvas.fillRoundRect(bx + 3, by + 3, bw, bh, 8, 0x0000);
            canvas.fillRoundRect(bx, by, bw, bh, 8, 0x18C3);
            canvas.drawRoundRect(bx, by, bw, bh, 8, 0xF800);

            canvas.setTextDatum(textdatum_t::top_center);
            canvas.setTextColor(0xF800);
            canvas.drawString("MISSION FAILED", SCREEN_W / 2, by + 8, &fonts::Font4);

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

extern AppSpaceShooter appSpace;
