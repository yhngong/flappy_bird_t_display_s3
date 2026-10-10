# LilyGO T-Display-S3 Touch Multi-App OS 📱

A modular, multi-app firmware for the **LilyGO T-Display-S3 Touch** (ESP32-S3) microcontroller board featuring an iPhone-style home screen launcher, capacitive touch support, games, sensors, and utilities.

🔗 **GitHub Repository:** [https://github.com/yhngong/flappy_bird_t_display_s3](https://github.com/yhngong/flappy_bird_t_display_s3)

---

## 📱 iPhone-Style Home Screen
- **iOS Twilight Wallpaper:** Deep indigo gradient with soft ambient light orbs.
- **Top Status Bar:** Shows local **San Francisco Bay Area (Pacific Time)** and battery percentage pill.
- **2×3 App Icon Grid (44×44 px Squircles):**
  - Tap any app icon directly on the touchscreen to launch!
  - Physical buttons also supported (`BTN1`: cycle glowing highlight ring, `BTN2`: launch).
- **Circular Home Button (`○` on bezel):** Touch the circular bezel button from **any app** to return directly to the Home Screen.

---

## 🕹️ Included Apps

### 1. Flappy Bird
- **Buttery 50 FPS double-buffered graphics** rendered via LovyanGFX over 8-bit parallel ST7789 bus.
- **Touch-To-Flap:** Tap anywhere on the glass or press physical buttons (`GPIO 0` / `GPIO 14`).
- **Parallax Background:** Scrolling clouds, distant mountain ranges, and speed-striped ground.
- **Dynamic Difficulty:** Starts with an easy, wide **76 px** pipe gap and **175 px** spacing, gradually narrowing down to **44 px**.
- **Persistent High Score:** Saved in ESP32 NVS via `Preferences`.

### 2. Sensor Oscilloscope Demo
- **Live CRT Waveform Scope:** Visualizes real-time signals with freeze/run mode.
- **Channel 1:** Battery ADC voltage (GPIO 4 with mV readout).
- **Channel 2:** ESP32-S3 internal die temperature (°C and °F).
- **Channel 3:** Capacitive / floating antenna noise waveform on GPIO 1.
- `BTN1`: Cycle channel  |  `BTN2`: Freeze / Run scope.

### 3. Desk Clock & Calendar
- **Local Time:** Pre-configured for the **San Francisco Bay Area / Pacific Time** (`PST8PDT,M3.2.0,M11.1.0`) with automatic Daylight Saving Time.
- **Dual Display:** Large digital time digits alongside a working analog clock face.
- **Date & Calendar:** Displays Day of the Week, Month, Day, and Year.
- **Wi-Fi NTP Sync:** Press `BTN2` to connect to Wi-Fi and sync atomic time via `pool.ntp.org`.

### 4. Space Shooter
- **Retro Arcade Action:** Side-scrolling starfighter battle with parallax starfield.
- **Controls:** `BTN1: Move Up` | `BTN2: Move Down` (Twin auto-cannons active).
- **Enemies:** Fast scout drones, heavy armored fighters, and asteroids with particle burst explosions.
- **Health & High Scores:** 3-point shield system and persistent best score tracker.

### 5. Retro Snake
- **Classic Arcade Nibbler:** 2-button control scheme tailored for portable handhelds.
- **Controls:** `BTN1: Turn Left (90° CCW)` | `BTN2: Turn Right (90° CW)`.
- **Food & Bonuses:** Red apples (+10 pts) and periodic golden bonus stars (+35 pts).
- **Progressive Speed:** Game ticks accelerate as the snake grows.

### 6. Hardware Monitor & Wi-Fi Scanner
- **ESP32-S3 Telemetry:** Real-time CPU frequency (240MHz), die temperature, free SRAM, and 8MB PSRAM.
- **Power:** Battery voltage (VBAT) and calculated percentage.
- **Wi-Fi Scanner:** Scans nearby 2.4GHz Wi-Fi networks and ranks by signal strength (RSSI dBm).

---

## 🛠️ Hardware Specifications
- **Board:** LilyGO T-Display-S3 Touch (ESP32-S3, 16MB Quad SPI Flash, 8MB Octal PSRAM)
- **Display:** 1.9" ST7789V 170×320 8-bit Parallel (Intel 8080 mode)
- **Touch Controller:** CST816S Capacitive Touch (I2C: SDA `GPIO 18`, SCL `GPIO 17`, INT `GPIO 16`, RST `GPIO 21`)
- **Bezel Home Button:** Circular soft-touch button on display glass
- **Physical Buttons:**
  - Left Button: `GPIO 0` (BOOT)
  - Right Button: `GPIO 14` (KEY)
- **Battery Sensor:** `GPIO 4` (ADC divider)

---

## 🚀 Build & Flash Instructions

### Prerequisites
- [Arduino CLI](https://arduino.github.io/arduino-cli/)
- ESP32 Core 3.x (`esp32:esp32`)
- Libraries: `LovyanGFX`, `CST816_TouchLib`

### Compile
```bash
arduino-cli compile --fqbn esp32:esp32:lilygo_t_display_s3 .
```

### Upload
```bash
arduino-cli upload -p /dev/cu.usbmodem14501 --fqbn esp32:esp32:lilygo_t_display_s3 .
```
