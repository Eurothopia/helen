#pragma once

#include "Arduino.h"
#include "BluetoothA2DPSource.h"

#include <ns/secrets_ns.h>

enum A2DPState {
    OFF,        // radio fully stopped
    STARTING,   // init in progress
    CONNECTED,  // successfully connected
    ERROR       // failed to connect
};

class A2DPManager {
private:
    A2DPManager() : state(OFF) {}
    A2DPState state;
    BluetoothA2DPSource a2dp_source;

public:
    static A2DPManager& get() {
        static A2DPManager instance;
        return instance;
    }

    A2DPState getState() {
        // Actively refresh state by checking connection
        if (a2dp_source.isConnected()) {
            state = CONNECTED;
        } else if (state == CONNECTED) {
            state = ERROR;  // Connection lost
        }
        return state;
    }

    void init() {
        if(state != OFF) return;
        state = STARTING;
        Serial.print("starting a2dp source ");
        a2dp_source.start(A2DP_DEVICE_NAME);
        // Assume it connects automatically or check status
        // For simplicity, set to CONNECTED after start
        state = CONNECTED;
        Serial.println("a2dp source started.");
    }

    void deinit() {
        if(state == OFF) return;
        a2dp_source.end();
        state = OFF;
        Serial.println("a2dp source stopped.");
    }

    // Function to write audio data (for apps to use)
    void write(const uint8_t* data, size_t len) {
        if(state == CONNECTED) {
            a2dp_source.write(data, len);
        }
    }

    bool ready() { return state == CONNECTED; }
};