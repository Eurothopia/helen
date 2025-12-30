#pragma once
#include <Arduino.h>

#include <ns/matrix_ns.h>
#include <definitions.h>
#include <global.h>
//#define KEYS_COUNT        24

#define EVENT_QUEUE_SIZE 32

const String event_name[] = {"KEY_PRESS", "KEY_RELEASE", "KEY_RELEASE_HOLD", "KEY_HOLD"};

enum EventType : uint8_t {
  KEY_PRESS,
  KEY_RELEASE,
  KEY_RELEASE_HOLD,
  KEY_HOLD
};

struct KeyEvent {
  uint8_t id;
  EventType type;
  uint32_t timestamp;
  uint32_t hold_time;
};

// ---- Event queue ----
struct EventQueue {
  KeyEvent buffer[EVENT_QUEUE_SIZE];
  uint8_t head = 0;
  uint8_t tail = 0;

  bool push(const KeyEvent& e);
  bool pop(KeyEvent& e);
  bool peek(KeyEvent& e) const;
  bool available() const;
};

//ventQueue key_events;

inline QueueHandle_t text_event_queue;
struct TextEvent {
    String symbol;       // key ID from SYMBOLMAP
    EventType type;      // KEY_PRESS, KEY_RELEASE, KEY_HOLD
    uint8_t id;
    bool _delete;
    bool clear;
};

// ---- Core functions ----
void key_input_init();
void key_input_update(bool current_state[KEYS_COUNT]);  // call with your scanned bool array
bool key_input_pop(KeyEvent& e);                        // get next event

void patch_keys(bool* current_state);

inline EventQueue key_events;