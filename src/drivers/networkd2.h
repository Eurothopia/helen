#pragma once
//network driver implementation without the Arduino WiFi.h wrapper (useless now)

#include "Arduino.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wpa2.h"  // If using WPA2 enterprise (optional)
#include "esp_netif.h"  // For network interface

#include <ns/secrets_ns.h>

enum WiFiState {
    UNKNOWN,    // initial / uninitialized
    OFF,        // radio fully stopped
    STARTING,   // init in progress
    CONNECTED,  // successfully connected
    ERROR       // failed to connect
};

struct WiFiScanResult {
    String ssid;
    int32_t rssi;   // signal strength in dBm
    uint8_t channel;
    bool secure;
};

class WiFiManager {
private:
    WiFiManager() : state(OFF), current_rssi(-1) {}
    WiFiState state;
    int current_rssi;
    wifi_config_t wifi_config;
    wifi_scan_config_t scan_config;
    std::vector<WiFiScanResult> last_scan_result;
    bool netif = false;

    void updateRSSI() {
        if (state == CONNECTED) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                current_rssi = ap_info.rssi;
            }
        } else {
            current_rssi = -1;
        }
    }

public:
    static WiFiManager& get() {
        static WiFiManager instance;
        return instance;
    }

    inline WiFiState getState() {
        // If OFF, don't query hardware, just return internal state
        if (state == OFF) return OFF;
        
        // Update internal state based on hardware
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            state = CONNECTED;
            current_rssi = ap_info.rssi;
        } else if (state == CONNECTED) {
            // We thought we were connected, but ap_info failed
            state = ERROR;
        }
        return state;
    }

    inline int getRSSI() {
        updateRSSI();
        return current_rssi;
    }

    inline void init() {
        if (state != OFF) return;
        cpu_boost(1000, 160);
        state = STARTING;
        Serial.print("(networkd.h type2):");
        
        // Initialize TCP/IP stack and event loop
        if (!netif) {
            esp_netif_init();
            esp_event_loop_create_default();
            esp_netif_create_default_wifi_sta();
            netif = true;    
        }

        // Initialize WiFi with default config
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        // Set mode to STA
        esp_wifi_set_mode(WIFI_MODE_STA);

        // Configure WiFi credentials
        memset(&wifi_config, 0, sizeof(wifi_config));
        strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
        strcpy((char*)wifi_config.sta.password, WIFI_PASSWORD);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

        // Start WiFi
        esp_wifi_start();

        // Connect
        esp_wifi_connect();

        // Wait for connection with frequent yields (10ms instead of 300ms)
        wifi_ap_record_t ap_info;
        unsigned long start = millis();
        int poll_count = 0;
        
        while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK && millis() - start < 10000) {
            vTaskDelay(10 / portTICK_PERIOD_MS);  // Yield every 10ms (30x more often)
            
            // Every 500ms, boost CPU to speed up WiFi negotiation
            //if (poll_count++ % 50 == 0) {
                //cpu_boost(100);

                if (poll_count++ % 500 == 0) Serial.print(".");
            //}
        }

        state = (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) ? CONNECTED : ERROR;
        Serial.println(state == CONNECTED ? " connected." : " failed to connect.");
    }

    inline void deinit() {
        if (state == OFF) return;
        Serial.print("(networkd.h type2): ");
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        //esp_netif_deinit();
        //esp_event_loop_delete_default();
        state = OFF;
        Serial.println("wi-fi has been shut down.");
    }

    bool ready() { return state == CONNECTED; }

    // Optional: Scanning (returns number of networks found)
    std::vector<WiFiScanResult> scan() {
        std::vector<WiFiScanResult> results;

        if (state == OFF) {
            // Radio must be initialized to at least STA mode for scanning
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            esp_wifi_init(&cfg);
            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_start();
        }

        Serial.println("Scanning Wi-Fi networks...");

        // Prepare scan config
        memset(&scan_config, 0, sizeof(scan_config));
        scan_config.show_hidden = false;

        // Start blocking scan
        esp_wifi_scan_start(&scan_config, true);

        // Get number of APs
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);

        if (ap_count == 0) {
            Serial.println("No networks found.");
            return results;
        }

        // Retrieve records
        std::vector<wifi_ap_record_t> ap_records(ap_count);
        esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());

        // Convert to user API format
        for (int i = 0; i < ap_count; i++) {
            WiFiScanResult r;
            r.ssid    = (const char*)ap_records[i].ssid;
            r.rssi    = ap_records[i].rssi;
            r.channel = ap_records[i].primary;
            r.secure  = (ap_records[i].authmode != WIFI_AUTH_OPEN);
            results.push_back(r);
        }

        if (state==OFF) {
            //return to prev state
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_wifi_deinit();
            esp_netif_deinit();
            esp_event_loop_delete_default();
        }

        Serial.printf("Scan complete. Found %u networks.\n", ap_count);
        last_scan_result = results;
        return results;
    }

    std::vector<WiFiScanResult> scan_result() {
        return last_scan_result;
    }
};