# Flappy Bird for LilyGO T-Display-S3

A retro Flappy Bird arcade game built for the **LilyGO T-Display-S3** (ESP32-S3) microcontroller board.

## Features
- **Zero-Flicker Double Buffering:** Rendered at ~50 FPS using LovyanGFX's `LGFX_Sprite` canvas in internal SRAM over an 8-bit parallel 8080 LCD bus.
- **Parallax Background:** Layered fluffy clouds, mountain ranges, and animated grass speed dashes.
- **Dynamic Difficulty:**
  - Starts with an easy, wide **76 px** pipe gap and **175 px** spacing between obstacles.
  - Dynamically narrows by 2 px per score point down to **44 px** for high scores.
- **Persistent High Scores:** Saved to ESP32 Non-Volatile Storage (NVS) via `Preferences`.
- **Dual Button Support:** Play with either the Boot button (GPIO 0) or Key button (GPIO 14).

## Hardware
- **Board:** LilyGO T-Display-S3 (ESP32-S3, 16MB Flash, 8MB PSRAM)
- **Display:** 1.9" ST7789V 170x320 8-bit Parallel
- **Controls:**
  - Left / Boot button: `GPIO 0`
  - Right / Key button: `GPIO 14`

## Build & Flash (arduino-cli)

### Compile
```bash
arduino-cli compile --fqbn esp32:esp32:lilygo_t_display_s3 .
```

### Upload
```bash
arduino-cli upload -p /dev/cu.usbmodem14501 --fqbn esp32:esp32:lilygo_t_display_s3 .
```
