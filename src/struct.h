#pragma once
#include "Arduino.h"

enum bluetooth_type {none, bluetooth_low_energy, bluetooth_classic, bluetooth_a2dp};
enum input_mode_name {CLASSIC_INPUT,T9X,ABX,CHIP8,GSX};
enum AppID {_CALCULATOR, _ABACUS, _LLM, _DINO, _TERMINAL, _GSX, _NTS};

struct APP_metadata {
    char name[20];
    input_mode_name input;
    bluetooth_type requires_bluetooth;
    bool requires_wifi;
    bool vsync;
    bool wakelock;
    bool fullscreen;
};

enum FrameEventType { FRAME_READY, STATUS_UPDATE };

struct FrameEvent {
  FrameEventType type;
  bool fullRefresh;      // e.g., true = redraw everything
  uint32_t dirtyMask;    // optional flags for partial updates
};


