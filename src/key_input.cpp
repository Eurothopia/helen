#include "key_input.h"
#include "ns/matrix_ns.h"

// Global queue


// Internal state
static bool prev_state[KEYS_COUNT] = {0};
static bool prev_hold_state[KEYS_COUNT] = {0};
static uint32_t last_change_time[KEYS_COUNT] = {0};
static uint32_t press_start_time[KEYS_COUNT] = {0};
static uint32_t hold_press_start_time[KEYS_COUNT] = {0};
static uint32_t first_hold[KEYS_COUNT] = {0};
static uint8_t hold_repeat_count[KEYS_COUNT] = {0};

// ---- Event queue implementation ----
bool EventQueue::push(const KeyEvent& e) {
  uint8_t next = (head + 1) % EVENT_QUEUE_SIZE;
  if (next == tail) return false; // full
  buffer[head] = e;
  head = next;
  return true;
}

bool EventQueue::pop(KeyEvent& e) {
  if (head == tail) return false; // empty
  e = buffer[tail];
  tail = (tail + 1) % EVENT_QUEUE_SIZE;
  return true;
}

bool EventQueue::peek(KeyEvent &e) const {
    if (head == tail) return false; // empty
    e = buffer[tail];               // copy front element without removing
    return true;
}


bool EventQueue::available() const {
  return head != tail;
}

// ---- Initialization ----
void key_input_init() {
  memset(prev_state, 0, sizeof(prev_state));
  memset(last_change_time, 0, sizeof(last_change_time));
  memset(press_start_time, 0, sizeof(press_start_time));
}

// ---- Debounce + event generation ----
void key_input_update(bool current_state[KEYS_COUNT]) {
  uint32_t now = millis();

  for (uint8_t i = 0; i < KEYS_COUNT; i++) {
    bool curr = current_state[i];
    bool prev = prev_state[i];

    // Check if state changed
    if (curr != prev) {
      // debounce
      if (now - last_change_time[i] >= DEBOUNCE_MS) {
        prev_state[i] = curr;
        last_change_time[i] = now;

        if (curr) {
          // pressed
          if ((now - press_start_time[i]) < DOUBLE_PRESS_MS && (i>=0 && i<5)) {
            key_events.push({i, KEY_DOUBLE_PRESS, now});
          } else key_events.push({i, KEY_PRESS, now});
          press_start_time[i] = now;
        } else {
          // released
          if (prev_hold_state[i]==true) {
            key_events.push({i, KEY_RELEASE_HOLD, now});
            prev_hold_state[i]=false;
          } else {
            key_events.push({i, KEY_RELEASE, now});

          }
          first_hold[i] = false;
          hold_repeat_count[i] = 0;
         
        }
      }
    } else if (curr) {
      // held check
      uint32_t repeat_interval;
      if (hold_repeat_count[i] == 0) {
        repeat_interval = HOLD_MS;  // first repeat
      } else if (double_hold && hold_repeat_count[i] == 1) {
        repeat_interval = HOLD_MS;  // second repeat (if double_hold)
      } else {
        repeat_interval = HOLD_REPEAT_MS;  // fast repeats
      }
      
      if (now - press_start_time[i] >= repeat_interval) {
        if (!prev_hold_state[i]) hold_press_start_time[i] = now;
        key_events.push({i, KEY_HOLD, now, now-hold_press_start_time[i]});
        prev_hold_state[i]=true;
        first_hold[i] = true;
        hold_repeat_count[i]++;
        press_start_time[i] = now; // repeat hold events
      }
    }
  }
}

bool key_input_pop(KeyEvent& e) {
  return key_events.pop(e);
}

void patch_keys(bool *KEY_ARR_BOOL) {
  if (KEY_ARR_BOOL[KEY_0] || KEY_ARR_BOOL[KEY_EQUALS]) {
    //if (debug)
    KEY_ARR_BOOL[KEY_SQRT]=false;
  }
}