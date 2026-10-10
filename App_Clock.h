#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
#include "Display_Config.h"
#include "Input.h"

class AppClock {
public:
    // Default: San Francisco Bay Area (Pacific Time with auto PST/PDT DST)
    const char* TZ_PACIFIC = "PST8PDT,M3.2.0,M11.1.0";
    const char* TZ_NAME = "SF Bay Area (Pacific)";

    bool ntpSynced = false;
    bool syncing = false;
    uint32_t syncStartTime = 0;
    String syncStatusMsg = "Press BTN2 to Sync NTP";

    // Hardware/Software clock initialized to build time in San Francisco
    time_t baseTime = 0;
    uint32_t baseMillis = 0;

    Preferences prefs;

    void init() {
        // Set ESP32 system timezone to San Francisco Bay Area
        setenv("TZ", TZ_PACIFIC, 1);
        tzset();

        if (baseTime == 0) {
            parseCompileTime();
        }
    }

    void parseCompileTime() {
        struct tm t = {0};
        const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        char s_month[5];
        int day, year, hour, min, sec;
        sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
        sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

        t.tm_year = year - 1900;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = sec;
        t.tm_isdst = -1; // Auto-determine DST for Pacific Time
        for (int m = 0; m < 12; m++) {
            if (strncmp(s_month, months[m], 3) == 0) {
                t.tm_mon = m;
                break;
            }
        }
        baseTime = mktime(&t);
        baseMillis = millis();
    }

    time_t getCurrentTime() {
        if (ntpSynced) {
            time_t now;
            time(&now);
            return now;
        } else {
            return baseTime + ((millis() - baseMillis) / 1000);
        }
    }

    void startNtpSync() {
        syncing = true;
        syncStartTime = millis();
        syncStatusMsg = "Connecting to Wi-Fi...";

        Preferences netPrefs;
        netPrefs.begin("wifi_cfg", true);
        String ssid = netPrefs.getString("ssid", "");
        String pass = netPrefs.getString("pass", "");
        netPrefs.end();

        if (ssid.length() > 0) {
            WiFi.begin(ssid.c_str(), pass.c_str());
        } else {
            syncStatusMsg = "Scanning Wi-Fi...";
            WiFi.begin();
        }
    }

    void update() {
        // Button 2: Trigger Wi-Fi NTP Sync
        if (input.justPressedRight && !syncing) {
            startNtpSync();
        }

        // Handle background Wi-Fi NTP sync
        if (syncing) {
            uint32_t elapsed = millis() - syncStartTime;
            if (WiFi.status() == WL_CONNECTED) {
                syncStatusMsg = "Querying NTP atomic clock...";
                configTzTime(TZ_PACIFIC, "pool.ntp.org", "time.nist.gov");

                struct tm timeinfo;
                if (getLocalTime(&timeinfo, 500)) {
                    ntpSynced = true;
                    syncing = false;
                    syncStatusMsg = "NTP Synced (Pacific Time)!";
                } else if (elapsed > 10000) {
                    syncing = false;
                    syncStatusMsg = "NTP Timeout. Using RTC.";
                }
            } else if (elapsed > 12000) {
                syncing = false;
                syncStatusMsg = "Wi-Fi not connected (see Dashboard)";
            }
        }
    }

    void draw() {
        canvas.fillScreen(0x0002); // Deep midnight blue

        // Header
        canvas.fillRect(0, 0, SCREEN_W, 22, 0x10A2);
        canvas.drawFastHLine(0, 22, SCREEN_W, 0x07FF);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(0x07FF);
        canvas.drawString("DESK CLOCK & CALENDAR", 8, 4, &fonts::Font2);

        canvas.setTextDatum(textdatum_t::top_right);
        if (ntpSynced) {
            canvas.setTextColor(0x07E0);
            canvas.drawString("[NTP PACIFIC]", SCREEN_W - 8, 4, &fonts::Font2);
        } else {
            canvas.setTextColor(0xFD20);
            canvas.drawString("[RTC PACIFIC]", SCREEN_W - 8, 4, &fonts::Font2);
        }

        // Get time components
        time_t tVal = getCurrentTime();
        struct tm* tmInfo = localtime(&tVal);

        // Big Digital Clock (HH:MM:SS)
        int cx = 112;
        int cy = 34;
        canvas.fillRoundRect(cx, cy, 198, 62, 6, 0x0861);
        canvas.drawRoundRect(cx, cy, 198, 62, 6, 0x04DF);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec);

        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(0x07FF);
        canvas.drawString(timeBuf, cx + 99, cy + 30, &fonts::Font6);

        // Date string
        const char* days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
        const char* mons[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
        char dateBuf[48];
        snprintf(dateBuf, sizeof(dateBuf), "%s, %s %02d, %04d", days[tmInfo->tm_wday], mons[tmInfo->tm_mon], tmInfo->tm_mday, tmInfo->tm_year + 1900);

        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(dateBuf, cx + 8, cy + 70, &fonts::Font2);

        // Timezone badge
        canvas.setTextDatum(textdatum_t::top_right);
        canvas.setTextColor(0xFEE0);
        canvas.drawString("San Francisco (PT)", cx + 192, cy + 70, &fonts::Font2);

        // Analog Clock Face on left
        drawAnalogClock(54, 82, 38, tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec);

        // Sync Status banner
        canvas.setTextDatum(textdatum_t::top_left);
        canvas.setTextColor(TFT_LIGHTGRAY);
        canvas.drawString(syncStatusMsg, cx + 8, cy + 90, &fonts::Font0);

        // Bottom Navigation Bar
        canvas.fillRect(0, SCREEN_H - 18, SCREEN_W, 18, 0x10A2);
        canvas.setTextDatum(textdatum_t::bottom_left);
        canvas.setTextColor(0xFD20);
        canvas.drawString("BAY AREA / PACIFIC TIME", 10, SCREEN_H - 3, &fonts::Font0);

        canvas.setTextDatum(textdatum_t::bottom_right);
        canvas.setTextColor(0x07E0);
        canvas.drawString("BTN2: SYNC VIA WI-FI", SCREEN_W - 10, SCREEN_H - 3, &fonts::Font0);
    }

    void drawAnalogClock(int ox, int oy, int r, int h, int m, int s) {
        // Dial Face
        canvas.fillCircle(ox, oy, r, 0x0861);
        canvas.drawCircle(ox, oy, r, 0x04DF);
        canvas.drawCircle(ox, oy, r - 1, 0x07FF);

        // Hour tick marks
        for (int i = 0; i < 12; i++) {
            float ang = (i * 30.0f) * 3.14159f / 180.0f;
            int x1 = ox + sin(ang) * (r - 4);
            int y1 = oy - cos(ang) * (r - 4);
            int x2 = ox + sin(ang) * (r - 1);
            int y2 = oy - cos(ang) * (r - 1);
            canvas.drawLine(x1, y1, x2, y2, TFT_WHITE);
        }

        // Hour Hand
        float hAng = ((h % 12) * 30.0f + m * 0.5f) * 3.14159f / 180.0f;
        int hx = ox + sin(hAng) * (r * 0.52f);
        int hy = oy - cos(hAng) * (r * 0.52f);
        canvas.drawLine(ox, oy, hx, hy, TFT_WHITE);
        canvas.drawLine(ox + 1, oy, hx + 1, hy, TFT_WHITE);

        // Minute Hand
        float mAng = (m * 6.0f) * 3.14159f / 180.0f;
        int mx = ox + sin(mAng) * (r * 0.75f);
        int my = oy - cos(mAng) * (r * 0.75f);
        canvas.drawLine(ox, oy, mx, my, 0x07FF);
        canvas.drawLine(ox + 1, oy, mx + 1, my, 0x07FF);

        // Second Hand (Thin red)
        float sAng = (s * 6.0f) * 3.14159f / 180.0f;
        int sx = ox + sin(sAng) * (r * 0.85f);
        int sy = oy - cos(sAng) * (r * 0.85f);
        canvas.drawLine(ox, oy, sx, sy, 0xF800);

        // Center Pin
        canvas.fillCircle(ox, oy, 2, 0xFD20);
    }
};

extern AppClock appClock;
