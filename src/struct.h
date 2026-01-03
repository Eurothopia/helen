#pragma once
#include "Arduino.h"
#include "apps/app_config.h"

enum bluetooth_type {none, bluetooth_low_energy, bluetooth_classic, bluetooth_a2dp};
enum input_mode_name {CLASSIC_INPUT,T9X,ABX,CHIP8,GSX};
enum AppID {_CALCULATOR, _ABACUS, _LLM, _DINO, _TERMINAL, _GSX, _NTS};

using AppFn = void (*)(void *);

struct AppDescriptor {
  AppID id;
  const char *name;
  AppFn function;
  const AppConfig *config;
  input_mode_name input_mode;
  bluetooth_type requires_bluetooth;
  bool wakelock;
};

enum FrameEventType { FRAME_READY, STATUS_UPDATE, CLEAR_DISPLAY};

struct FrameEvent {
  FrameEventType type;
  bool fullRefresh;      // e.g., true = redraw everything
  uint32_t dirtyMask;    // optional flags for partial updates
};


