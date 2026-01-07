#pragma once

#include "Arduino.h"
#include "BluetoothA2DPSource.h"

#include <ns/secrets_ns.h>

enum A2DPState {
    A2DP_OFF,        // radio fully stopped
    A2DP_STARTING,   // init in progress
    A2DP_CONNECTED,  // successfully connected
    A2DP_ERROR       // failed to connect
};

class A2DPManager {
private:
    A2DPManager() : state(A2DP_OFF) {}
    A2DPState state;
    BluetoothA2DPSource a2dp_source;

public:
    static A2DPManager& get() {
        static A2DPManager instance;
        return instance;
    }

    A2DPState getState() {
        // Actively refresh state by checking connection
        if (a2dp_source.is_connected()) {
            state = A2DP_CONNECTED;
        } else if (state == A2DP_CONNECTED) {
            state = A2DP_ERROR;  // Connection lost
        }
        return state;
    }

    void init() {
        if(state != A2DP_OFF) return;
        state = A2DP_STARTING;
        Serial.print("starting a2dp source ");
        a2dp_source.start(A2DP_DEVICE_NAME);
        // Assume it connects automatically or check status
        // For simplicity, set to connected after start
        state = A2DP_CONNECTED;
        Serial.println("a2dp source started.");
    }

    void deinit() {
        if(state == A2DP_OFF) return;
        a2dp_source.end();
        state = A2DP_OFF;
        Serial.println("a2dp source stopped.");
    }

    bool ready() { return a2dp_source.is_connected(); }

    // Expose underlying source for stream callbacks
    BluetoothA2DPSource& source() { return a2dp_source; }
};