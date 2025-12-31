#pragma once

#include "Arduino.h"
#include "WiFi.h"
#include "esp_wifi.h"

#include <ns/secrets_ns.h>

enum WiFiState {
    UNKNOWN,    // initial / uninitialized
    OFF,        // radio fully stopped
    STARTING,   // init in progress
    CONNECTED,  // successfully connected
    ERROR       // failed to connect
};

class WiFiManager {
private:
    WiFiManager() : state(OFF) {}
    WiFiState state;
public:
    static WiFiManager& get() {
        static WiFiManager instance;
        return instance;
    }

    WiFiState getState() const { return state; }

    void init() {
        if(state != OFF) return;
        state = STARTING;
        Serial.print("connecting to wi-fi ");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        WiFi.mode(WIFI_STA);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            Serial.print(".");
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        state = (WiFi.status() == WL_CONNECTED) ? CONNECTED : ERROR;
        Serial.println((state ? " failed to connect." : " connected." ));
    }

    void deinit() {
        if(state == OFF) return;
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        esp_wifi_stop();
        esp_wifi_deinit();
        state = OFF;
        Serial.println("wi-fi has been shut down.");
    }

    bool ready() { return state == CONNECTED; }
};
