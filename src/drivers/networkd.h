#pragma once

#include "Arduino.h"
#include "WiFi.h"
#include "esp_wifi.h"
#include "vector"

#include <ns/secrets_ns.h>

enum WiFiState {
    WIFI_UNKNOWN,    // initial / uninitialized
    WIFI_OFF,        // radio fully stopped
    WIFI_STARTING,   // init in progress
    WIFI_CONNECTED,  // successfully connected
    WIFI_ERROR       // failed to connect
};

struct WiFiScanResult {
    String ssid;
    int32_t rssi;   // signal strength in dBm
    uint8_t channel;
    bool secure;
};

class WiFiManager {
private:
    WiFiManager() : state(WIFI_OFF) {}
    WiFiState state;
    std::vector<WiFiScanResult> last_scan_result;
public:
    static WiFiManager& get() {
        static WiFiManager instance;
        return instance;
    }

    WiFiState getState() const { return state; }

    void init() {
        if(state != WIFI_OFF) return;
        state = WIFI_STARTING;
        Serial.print("(networkd.h type0):");
        //        Serial.print("connecting to wi-fi ");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        WiFi.mode(WIFI_STA);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            Serial.print(".");
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        state = (WiFi.status() == WL_CONNECTED) ? WIFI_CONNECTED : WIFI_ERROR;
        //Serial.println((state ? " failed to connect." : " connected." ));

        Serial.println((WiFi.status() == WL_CONNECTED) ? " connected." : " failed to connect.");
    }

    void deinit() {
        if(state == WIFI_OFF) return;
        WiFi.disconnect(true);
        WiFi.mode(1);//WiFi.mode(WIFI_OFF);
        esp_wifi_stop();
        esp_wifi_deinit();
        state = WIFI_OFF;
        Serial.println("wi-fi has been shut down.");
    }

    std::vector<WiFiScanResult> scan() {
        std::vector<WiFiScanResult> results;

        // Ensure radio is on (STA mode is enough for scanning)
        if(state == WIFI_OFF) {
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
        }

        Serial.println("Scanning Wi-Fi networks...");
        int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

        for (int i = 0; i < n; i++) {
            WiFiScanResult r;
            r.ssid    = WiFi.SSID(i);
            r.rssi    = WiFi.RSSI(i);
            r.channel = WiFi.channel(i);
            r.secure  = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            results.push_back(r);
        }
        if(state == WIFI_OFF) {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            esp_wifi_stop();
            esp_wifi_deinit();
        }
        WiFi.scanDelete(); // free memory
        Serial.printf("Scan complete. Found %d networks.\n", n);
        last_scan_result = results;
        return results;
    }

    std::vector<WiFiScanResult> scan_result() {
        return last_scan_result;
    }

    bool ready() { return state == WIFI_CONNECTED; }
};
