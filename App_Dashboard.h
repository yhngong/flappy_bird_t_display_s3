#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "Display_Config.h"
#include "Input.h"

class AppDashboard {
public:
    int currentTab = 0; // 0: System Specs & Sensors, 1: Wi-Fi Scanner
    int wifiCount = -1;
    bool scanning = false;
    uint32_t lastScanTime = 0;

    void init() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        startScan();
    }

    void startScan() {
        scanning = true;
        wifiCount = -1;
        WiFi.scanNetworks(true); // Asynchronous scan
        lastScanTime = millis();
    }

    void update() {
        // Tab switching: Button 1 switches tab
        if (input.justPressedLeft) {
            currentTab = (currentTab + 1) % 2;
        }

        // On Wi-Fi tab: Button 2 triggers fresh rescan
        if (currentTab == 1 && input.justPressedRight && !scanning) {
            startScan();
        }

        // Check if async Wi-Fi scan completed
        if (scanning) {
            int16_t status = WiFi.scanComplete();
            if (status >= 0) {
                wifiCount = status;
                scanning = false;
            } else if (status == -1) {
                // Still scanning
            } else {
                scanning = false;
                wifiCount = 0;
            }
        }
    }

    void draw() {
        canvas.fillScreen(0x0842); // Cyber dark background

        // Top Navigation Bar
        canvas.fillRect(0, 0, SCREEN_W, 22, 0x10C4);
        canvas.drawFastHLine(0, 22, SCREEN_W, 0x07FF);

        // Tab Headers
        uint16_t tab0Color = (currentTab == 0) ? 0x07FF : 0x4A69;
        uint16_t tab1Color = (currentTab == 1) ? 0x07FF : 0x4A69;

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(tab0Color);
        canvas.drawString("1. SYSTEM SPECS", 10, 4, &fonts::Font2);

        canvas.setTextColor(tab1Color);
        canvas.drawString("2. WI-FI SCANNER", 140, 4, &fonts::Font2);

        if (currentTab == 0) {
            drawSystemTab();
        } else {
            drawWifiTab();
        }

        // Bottom status line
        canvas.fillRect(0, SCREEN_H - 18, SCREEN_W, 18, 0x10C4);
        canvas.drawFastHLine(0, SCREEN_H - 18, SCREEN_W, 0x2124);

        canvas.setTextDatum(textdatum_t::bottom_left);
        canvas.setTextColor(0xFD20);
        canvas.drawString("BTN1: SWITCH TAB", 10, SCREEN_H - 3, &fonts::Font0);

        if (currentTab == 1) {
            canvas.setTextDatum(textdatum_t::bottom_right);
            canvas.setTextColor(0x07E0);
            canvas.drawString("BTN2: RESCAN", SCREEN_W - 10, SCREEN_H - 3, &fonts::Font0);
        }
    }

    void drawSystemTab() {
        // Card 1: SoC & Temp
        int c1x = 10, c1y = 28, c1w = 145, c1h = 58;
        canvas.fillRoundRect(c1x, c1y, c1w, c1h, 4, 0x18E5);
        canvas.drawRoundRect(c1x, c1y, c1w, c1h, 4, 0x2967);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07FF);
        canvas.drawString("ESP32-S3 DUAL-CORE", c1x + 8, c1y + 6, &fonts::Font0);

        canvas.setTextColor(TFT_WHITE);
        canvas.drawString("Freq: " + String(ESP.getCpuFreqMHz()) + " MHz", c1x + 8, c1y + 20, &fonts::Font0);

        float tempC = temperatureRead();
        float tempF = (tempC * 9.0f / 5.0f) + 32.0f;
        canvas.drawString("Die Temp: " + String((int)tempC) + "C / " + String((int)tempF) + "F", c1x + 8, c1y + 34, &fonts::Font0);

        // Card 2: Battery & Uptime
        int c2x = 165, c2y = 28, c2w = 145, c2h = 58;
        canvas.fillRoundRect(c2x, c2y, c2w, c2h, 4, 0x18E5);
        canvas.drawRoundRect(c2x, c2y, c2w, c2h, 4, 0x2967);

        int rawMv = analogReadMilliVolts(PIN_BATTERY);
        float vbat = (rawMv * 2.0f) / 1000.0f;
        int pct = (int)((vbat - 3.3f) / (4.2f - 3.3f) * 100.0f);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;

        canvas.setTextColor(0x07E0);
        canvas.drawString("BATTERY & POWER", c2x + 8, c2y + 6, &fonts::Font0);

        canvas.setTextColor(TFT_WHITE);
        canvas.drawString("VBAT: " + String(vbat, 2) + "V (" + String(pct) + "%)", c2x + 8, c2y + 20, &fonts::Font0);

        uint32_t sec = millis() / 1000;
        int hrs = sec / 3600;
        int mins = (sec % 3600) / 60;
        int s = sec % 60;
        char upBuf[24];
        snprintf(upBuf, sizeof(upBuf), "Uptime: %02d:%02d:%02d", hrs, mins, s);
        canvas.drawString(upBuf, c2x + 8, c2y + 34, &fonts::Font0);

        // Card 3: Memory & Storage (Heap + PSRAM)
        int c3x = 10, c3y = 90, c3w = 300, c3h = 56;
        canvas.fillRoundRect(c3x, c3y, c3w, c3h, 4, 0x18E5);
        canvas.drawRoundRect(c3x, c3y, c3w, c3h, 4, 0x2967);

        canvas.setTextColor(0xFEE0);
        canvas.drawString("MEMORY & STORAGE", c3x + 8, c3y + 6, &fonts::Font0);

        uint32_t freeHeap = ESP.getFreeHeap() / 1024;
        uint32_t totalHeap = ESP.getHeapSize() / 1024;
        uint32_t freePsram = ESP.getFreePsram() / 1024;
        uint32_t totalPsram = ESP.getPsramSize() / 1024;

        canvas.setTextColor(TFT_WHITE);
        canvas.drawString("SRAM:  " + String(freeHeap) + " KB Free / " + String(totalHeap) + " KB Total", c3x + 8, c3y + 22, &fonts::Font0);
        canvas.drawString("PSRAM: " + String(freePsram) + " KB Free / " + String(totalPsram) + " KB Total (8MB)", c3x + 8, c3y + 36, &fonts::Font0);
    }

    void drawWifiTab() {
        int wx = 10, wy = 28, ww = 300, wh = 118;
        canvas.fillRoundRect(wx, wy, ww, wh, 4, 0x18E5);
        canvas.drawRoundRect(wx, wy, ww, wh, 4, 0x2967);

        if (scanning) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(0x07FF);
            canvas.drawString("SCANNING WI-FI AIRWAVES...", SCREEN_W / 2, wy + wh / 2, &fonts::Font2);
            return;
        }

        if (wifiCount <= 0) {
            canvas.setTextDatum(textdatum_t::middle_center);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString("No networks found. Press BTN2 to rescan.", SCREEN_W / 2, wy + wh / 2, &fonts::Font0);
            return;
        }

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07FF);
        canvas.drawString("FOUND " + String(wifiCount) + " NETWORKS:", wx + 10, wy + 6, &fonts::Font0);

        // Display up to 5 strongest networks
        int displayCount = min(wifiCount, 5);
        for (int i = 0; i < displayCount; i++) {
            int lineY = wy + 22 + (i * 18);
            String ssid = WiFi.SSID(i);
            if (ssid.length() > 22) ssid = ssid.substring(0, 20) + "..";
            int32_t rssi = WiFi.RSSI(i);

            // Signal bar color
            uint16_t sigColor = (rssi > -60) ? 0x07E0 : ((rssi > -75) ? 0xFD20 : 0xF800);

            canvas.setTextColor(TFT_WHITE);
            canvas.drawString(String(i + 1) + ". " + ssid, wx + 10, lineY, &fonts::Font0);

            canvas.setTextDatum(textdatum_t::top_right);
            canvas.setTextColor(sigColor);
            canvas.drawString(String(rssi) + " dBm", wx + ww - 10, lineY, &fonts::Font0);
            canvas.setTextDatum(textdatum_t::top_left);
        }
    }
};

extern AppDashboard appDashboard;
