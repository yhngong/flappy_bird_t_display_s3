#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Display_Config.h"
#include "Input.h"

class AppFlappyBird {
public:
    const int GROUND_Y = 146;
    const int GROUND_H = 24;

    #define FB_COLOR_SKY       0x5D9F
    #define FB_COLOR_CLOUD     0xFFFF
    #define FB_COLOR_HILL_FAR  0x6475
    #define FB_COLOR_HILL_NEAR 0x4BF0
    #define FB_COLOR_PIPE_BODY 0x3E24
    #define FB_COLOR_PIPE_LIP  0x4645
    #define FB_COLOR_PIPE_DARK 0x1A60
    #define FB_COLOR_PIPE_HI   0x76E8
    #define FB_COLOR_GROUND_GR 0x5D43
    #define FB_COLOR_GROUND_EARTH 0xCE52
    #define FB_COLOR_GROUND_STRIPE 0xBD0D

    enum State { READY, PLAY, HIT, GAMEOVER };
    State state = READY;

    const float GRAVITY = 0.22f;
    const float FLAP_STRENGTH = -3.7f;
    const float MAX_FALL_SPEED = 4.8f;
    const float SCROLL_SPEED = 1.8f;

    float birdX = 65.0f;
    float birdY = 70.0f;
    float birdVY = 0.0f;
    float birdAngle = 0.0f;
    const float BIRD_RADIUS = 7.5f;
    int wingPhase = 0;
    int wingTimer = 0;

    static const int PIPE_COUNT = 3;
    const int PIPE_WIDTH = 34;
    const int INITIAL_PIPE_GAP = 76;
    const int MIN_PIPE_GAP = 44;
    const int PIPE_SPACING = 175;

    struct Pipe {
        float x;
        int gapY;
        int gapSize;
        bool scored;
    };
    Pipe pipes[PIPE_COUNT];

    struct Cloud {
        float x;
        int y;
        int radius;
        float speed;
    };
    Cloud clouds[4];

    float groundScroll = 0.0f;
    float hillScroll = 0.0f;

    int score = 0;
    int highScore = 0;
    bool isNewHigh = false;
    uint32_t stateTimer = 0;
    int flashFrames = 0;

    Preferences prefs;

    void init() {
        prefs.begin("flappy", false);
        highScore = prefs.getInt("highscore", 0);

        clouds[0] = { 40.0f,  18, 14, 0.35f };
        clouds[1] = { 130.0f, 32, 11, 0.25f };
        clouds[2] = { 220.0f, 22, 16, 0.40f };
        clouds[3] = { 310.0f, 38, 10, 0.20f };

        reset();
    }

    void reset() {
        birdX = 65.0f;
        birdY = 65.0f;
        birdVY = 0.0f;
        birdAngle = 0.0f;
        score = 0;
        isNewHigh = false;
        state = READY;

        for (int i = 0; i < PIPE_COUNT; i++) {
            spawnPipe(i, SCREEN_W + 40 + (i * PIPE_SPACING));
        }
    }

    int getCurrentGap() {
        int gap = INITIAL_PIPE_GAP - (score * 2);
        return (gap < MIN_PIPE_GAP) ? MIN_PIPE_GAP : gap;
    }

    void spawnPipe(int idx, float startX) {
        pipes[idx].x = startX;
        int gap = getCurrentGap();
        pipes[idx].gapSize = gap;
        int halfGap = gap / 2;
        int minY = halfGap + 12;
        int maxY = GROUND_Y - halfGap - 12;
        pipes[idx].gapY = (maxY < minY) ? (GROUND_Y / 2) : random(minY, maxY + 1);
        pipes[idx].scored = false;
    }

    void update() {
        bool flapPressed = (input.justPressedLeft || input.justPressedRight || input.screenTouched);

        switch (state) {
            case READY: {
                birdY = 68.0f + sin(millis() * 0.006f) * 7.0f;
                birdAngle = 0.0f;
                wingTimer++;
                if (wingTimer >= 6) {
                    wingTimer = 0;
                    wingPhase = (wingPhase + 1) % 3;
                }
                groundScroll -= 1.0f;
                if (groundScroll <= -16.0f) groundScroll += 16.0f;
                for (int i = 0; i < 4; i++) {
                    clouds[i].x -= clouds[i].speed;
                    if (clouds[i].x < -40) clouds[i].x = SCREEN_W + 30;
                }
                if (flapPressed) {
                    birdVY = FLAP_STRENGTH;
                    state = PLAY;
                }
                break;
            }

            case PLAY: {
                if (flapPressed) {
                    birdVY = FLAP_STRENGTH;
                    wingPhase = 2;
                }
                updatePhysics();
                break;
            }

            case HIT: {
                birdVY += GRAVITY * 1.5f;
                birdY += birdVY;
                birdAngle += 15.0f;
                if (birdAngle > 90.0f) birdAngle = 90.0f;

                if (birdY >= GROUND_Y - BIRD_RADIUS) {
                    birdY = GROUND_Y - BIRD_RADIUS;
                    state = GAMEOVER;
                    stateTimer = millis();

                    if (score > highScore) {
                        highScore = score;
                        isNewHigh = true;
                        prefs.putInt("highscore", highScore);
                    }
                }
                break;
            }

            case GAMEOVER: {
                if (millis() - stateTimer > 500 && flapPressed) {
                    reset();
                }
                break;
            }
        }
    }

    void updatePhysics() {
        birdVY += GRAVITY;
        if (birdVY > MAX_FALL_SPEED) birdVY = MAX_FALL_SPEED;
        birdY += birdVY;

        if (birdVY < 0) {
            birdAngle = -22.0f;
        } else {
            birdAngle += 3.5f;
            if (birdAngle > 75.0f) birdAngle = 75.0f;
        }

        wingTimer++;
        if (wingTimer >= 5) {
            wingTimer = 0;
            wingPhase = (wingPhase + 1) % 3;
        }

        groundScroll -= SCROLL_SPEED;
        if (groundScroll <= -16.0f) groundScroll += 16.0f;

        hillScroll -= (SCROLL_SPEED * 0.3f);
        if (hillScroll <= -80.0f) hillScroll += 80.0f;

        for (int i = 0; i < 4; i++) {
            clouds[i].x -= clouds[i].speed;
            if (clouds[i].x < -40) clouds[i].x = SCREEN_W + 30;
        }

        for (int i = 0; i < PIPE_COUNT; i++) {
            pipes[i].x -= SCROLL_SPEED;

            if (!pipes[i].scored && pipes[i].x + PIPE_WIDTH < birdX) {
                pipes[i].scored = true;
                score++;
            }

            if (pipes[i].x < -PIPE_WIDTH - 10) {
                float rightmostX = 0;
                for (int j = 0; j < PIPE_COUNT; j++) {
                    if (pipes[j].x > rightmostX) rightmostX = pipes[j].x;
                }
                spawnPipe(i, rightmostX + PIPE_SPACING);
            }

            float px = pipes[i].x;
            float gapTop = pipes[i].gapY - (pipes[i].gapSize / 2);
            float gapBottom = pipes[i].gapY + (pipes[i].gapSize / 2);

            if (birdX + BIRD_RADIUS > px && birdX - BIRD_RADIUS < px + PIPE_WIDTH) {
                if (birdY - BIRD_RADIUS < gapTop || birdY + BIRD_RADIUS > gapBottom) {
                    state = HIT;
                    flashFrames = 3;
                    birdVY = -2.0f;
                    return;
                }
            }
        }

        if (birdY >= GROUND_Y - BIRD_RADIUS) {
            birdY = GROUND_Y - BIRD_RADIUS;
            state = HIT;
            flashFrames = 3;
        }
        if (birdY < BIRD_RADIUS) {
            birdY = BIRD_RADIUS;
            birdVY = 0;
        }
    }

    void draw() {
        if (flashFrames > 0) {
            flashFrames--;
            canvas.fillScreen(TFT_WHITE);
            return;
        }

        drawBackground();

        for (int i = 0; i < PIPE_COUNT; i++) {
            if (pipes[i].x > -PIPE_WIDTH - 20 && pipes[i].x < SCREEN_W + 20) {
                drawPipe((int)pipes[i].x, pipes[i].gapY, pipes[i].gapSize);
            }
        }

        drawGround();
        drawBird((int)birdX, (int)birdY, birdAngle, wingPhase);

        if (state == PLAY) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(TFT_BLACK);
            canvas.drawString(String(score), SCREEN_W / 2 + 2, 24 + 2, &fonts::Font4);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString(String(score), SCREEN_W / 2, 24, &fonts::Font4);
        } else if (state == READY) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(0x1940);
            canvas.drawString("FLAPPY BIRD", SCREEN_W / 2 + 2, 32 + 2, &fonts::Font4);
            canvas.setTextColor(0xFEE0);
            canvas.drawString("FLAPPY BIRD", SCREEN_W / 2, 32, &fonts::Font4);

            if ((millis() / 450) % 2 == 0) {
                canvas.setTextDatum(textdatum_t::middle_center);
                canvas.setTextColor(TFT_WHITE);
                canvas.drawString("PRESS ANY BUTTON TO JUMP", SCREEN_W / 2, 115, &fonts::Font2);
            }

            canvas.setTextDatum(textdatum_t::top_left);
            canvas.setTextColor(TFT_WHITE, FB_COLOR_SKY);
            canvas.drawString("BEST: " + String(highScore), 8, 8, &fonts::Font2);
        } else if (state == GAMEOVER) {
            drawScoreBadge();
        }
    }

    void drawBackground() {
        canvas.fillRect(0, 0, SCREEN_W, 90, FB_COLOR_SKY);
        canvas.fillRect(0, 90, SCREEN_W, GROUND_Y - 90, 0x759F);

        for (int i = -1; i < 5; i++) {
            int hx = (int)(i * 80 + hillScroll);
            canvas.fillCircle(hx + 40, GROUND_Y + 12, 45, FB_COLOR_HILL_FAR);
        }
        for (int i = -1; i < 6; i++) {
            int hx = (int)(i * 65 + (hillScroll * 1.4f));
            canvas.fillCircle(hx + 30, GROUND_Y + 8, 30, FB_COLOR_HILL_NEAR);
        }

        for (int i = 0; i < 4; i++) {
            int cx = (int)clouds[i].x;
            int cy = clouds[i].y;
            int r = clouds[i].radius;
            canvas.fillCircle(cx, cy, r, FB_COLOR_CLOUD);
            canvas.fillCircle(cx - r * 0.7f, cy + 2, r * 0.75f, FB_COLOR_CLOUD);
            canvas.fillCircle(cx + r * 0.7f, cy + 2, r * 0.75f, FB_COLOR_CLOUD);
            canvas.fillRect(cx - r * 0.8f, cy + 2, (int)(r * 1.6f), (int)(r * 0.8f), FB_COLOR_CLOUD);
        }
    }

    void drawGround() {
        canvas.fillRect(0, GROUND_Y, SCREEN_W, 4, FB_COLOR_GROUND_GR);
        canvas.drawFastHLine(0, GROUND_Y + 4, SCREEN_W, 0x3BC0);
        canvas.fillRect(0, GROUND_Y + 5, SCREEN_W, GROUND_H - 5, FB_COLOR_GROUND_EARTH);
        for (int x = (int)groundScroll - 16; x < SCREEN_W + 16; x += 16) {
            canvas.drawLine(x, GROUND_Y + 5, x + 6, GROUND_Y + GROUND_H - 1, FB_COLOR_GROUND_STRIPE);
            canvas.drawLine(x + 1, GROUND_Y + 5, x + 7, GROUND_Y + GROUND_H - 1, FB_COLOR_GROUND_STRIPE);
        }
    }

    void drawPipe(int x, int gapY, int gapSize) {
        int gapTop = gapY - (gapSize / 2);
        int gapBottom = gapY + (gapSize / 2);
        int lipH = 9;
        int lipOverhang = 3;

        if (gapTop > 0) {
            int shaftH = gapTop - lipH;
            if (shaftH > 0) {
                canvas.fillRect(x, 0, PIPE_WIDTH, shaftH, FB_COLOR_PIPE_BODY);
                canvas.fillRect(x + 3, 0, 4, shaftH, FB_COLOR_PIPE_HI);
                canvas.fillRect(x + PIPE_WIDTH - 6, 0, 5, shaftH, FB_COLOR_PIPE_DARK);
                canvas.drawFastVLine(x, 0, shaftH, TFT_BLACK);
                canvas.drawFastVLine(x + PIPE_WIDTH - 1, 0, shaftH, TFT_BLACK);
            }
            int lipX = x - lipOverhang;
            int lipW = PIPE_WIDTH + (lipOverhang * 2);
            int lipY = gapTop - lipH;
            canvas.fillRect(lipX, lipY, lipW, lipH, FB_COLOR_PIPE_LIP);
            canvas.fillRect(lipX + 3, lipY, 5, lipH, FB_COLOR_PIPE_HI);
            canvas.fillRect(lipX + lipW - 7, lipY, 6, lipH, FB_COLOR_PIPE_DARK);
            canvas.drawRect(lipX, lipY, lipW, lipH, TFT_BLACK);
        }

        int botShaftY = gapBottom + lipH;
        int botShaftH = GROUND_Y - botShaftY;
        if (botShaftH > 0) {
            canvas.fillRect(x, botShaftY, PIPE_WIDTH, botShaftH, FB_COLOR_PIPE_BODY);
            canvas.fillRect(x + 3, botShaftY, 4, botShaftH, FB_COLOR_PIPE_HI);
            canvas.fillRect(x + PIPE_WIDTH - 6, botShaftY, 5, botShaftH, FB_COLOR_PIPE_DARK);
            canvas.drawFastVLine(x, botShaftY, botShaftH, TFT_BLACK);
            canvas.drawFastVLine(x + PIPE_WIDTH - 1, botShaftY, botShaftH, TFT_BLACK);
        }
        int lipX = x - lipOverhang;
        int lipW = PIPE_WIDTH + (lipOverhang * 2);
        int lipY = gapBottom;
        canvas.fillRect(lipX, lipY, lipW, lipH, FB_COLOR_PIPE_LIP);
        canvas.fillRect(lipX + 3, lipY, 5, lipH, FB_COLOR_PIPE_HI);
        canvas.fillRect(lipX + lipW - 7, lipY, 6, lipH, FB_COLOR_PIPE_DARK);
        canvas.drawRect(lipX, lipY, lipW, lipH, TFT_BLACK);
    }

    void drawBird(int bx, int by, float angle, int wing) {
        canvas.fillCircle(bx, by, 9, 0xFDE0);
        canvas.drawCircle(bx, by, 9, TFT_BLACK);
        canvas.fillCircle(bx - 3, by + 3, 5, 0xFFF5);
        canvas.fillCircle(bx + 4, by - 3, 4, TFT_WHITE);
        canvas.drawCircle(bx + 4, by - 3, 4, TFT_BLACK);
        canvas.fillCircle(bx + 5, by - 3, 2, TFT_BLACK);
        canvas.fillCircle(bx + 6, by - 4, 1, TFT_WHITE);
        canvas.fillTriangle(bx + 7, by - 1, bx + 15, by + 2, bx + 7, by + 5, 0xFC40);
        canvas.drawTriangle(bx + 7, by - 1, bx + 15, by + 2, bx + 7, by + 5, TFT_BLACK);
        canvas.drawFastHLine(bx + 8, by + 2, 6, TFT_BLACK);

        if (wing == 0) {
            canvas.fillRoundRect(bx - 8, by - 1, 9, 6, 2, TFT_WHITE);
            canvas.drawRoundRect(bx - 8, by - 1, 9, 6, 2, TFT_BLACK);
        } else if (wing == 1) {
            canvas.fillRoundRect(bx - 7, by + 2, 8, 6, 2, TFT_WHITE);
            canvas.drawRoundRect(bx - 7, by + 2, 8, 6, 2, TFT_BLACK);
        } else {
            canvas.fillRoundRect(bx - 8, by - 5, 9, 6, 2, TFT_WHITE);
            canvas.drawRoundRect(bx - 8, by - 5, 9, 6, 2, TFT_BLACK);
        }
    }

    void drawScoreBadge() {
        int bw = 200, bh = 95;
        int bx = (SCREEN_W - bw) / 2, by = 26;
        canvas.fillRoundRect(bx + 3, by + 3, bw, bh, 8, 0x18C3);
        canvas.fillRoundRect(bx, by, bw, bh, 8, 0xDF57);
        canvas.drawRoundRect(bx, by, bw, bh, 8, TFT_BLACK);
        canvas.drawRoundRect(bx + 2, by + 2, bw - 4, bh - 4, 6, 0xFEF5);

        canvas.setTextDatum(textdatum_t::top_center);
        canvas.setTextColor(0xD000);
        canvas.drawString("GAME OVER", SCREEN_W / 2, by + 8, &fonts::Font4);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(TFT_BLACK);
        canvas.drawString("SCORE: " + String(score), bx + 22, by + 40, &fonts::Font2);
        canvas.drawString("BEST:  " + String(highScore), bx + 22, by + 58, &fonts::Font2);

        if (isNewHigh) {
            canvas.setTextDatum(textdatum_t::top_right);
            canvas.setTextColor(0xD0A0);
            canvas.drawString("NEW!", bx + bw - 18, by + 42, &fonts::Font2);
        }

        if ((millis() / 400) % 2 == 0) {
            canvas.setTextDatum(textdatum_t::bottom_center);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("PRESS BUTTON TO RETRY", SCREEN_W / 2, SCREEN_H - 4, &fonts::Font2);
        }
    }
};

extern AppFlappyBird appFlappy;
