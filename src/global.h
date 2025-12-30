#pragma once
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "definitions.h"
//#include "apps/_xoxo.h"



enum AppID {_CALCULATOR, _ABACUS, _LLM, _DINO, _TERMINAL, _GSX, _NTS};
inline const String AppName[] = {"CALCULATOR", "ABACUS", "LLM", "DINO", "TERMINAL", "GSX"};
inline AppID FOCUSED_APP = _CALCULATOR;
inline bool just_switched_apps = true;

inline TaskHandle_t input_daemon_handle, serial_cx_daemon_handle, connectivity_daemon_handle, display_daemon_handle, power_daemon_handle, battery_service_handle, brightness_service_handle;
//inline TaskHandle_t app_0_handle,app_1_handle,app_2_handle,app_3_handle,app_4_handle,app_5_handle,app_6_handle,app_7_handle;
inline TaskHandle_t app_handles[20];

inline bool debug=false;

inline bool BLE=false;

inline uint8_t BRIGHTNESS = DISPLAY_DEFAULT_BRIGHTNESS; inline float FPS; inline int FG_COLOR = TFT_WHITE, BG_COLOR = TFT_BLACK, FBG_COLOR; ///bdc7 acc7 
inline bool AUTO_BRIGHTNESS=true, color_change=false;
inline int SOLAR=0;

inline uint8_t PERCENTAGE; inline int VOLTAGE; inline bool CHARGING=false, SLEEPING=false, WAKE_LOCK=false, SLEEP_OVERRIDE=false, POWER_OVERRIDE=false, DEEP_SLEEP_REQUESTED=false;
RTC_DATA_ATTR inline uint32_t uptime_offset = 0;
RTC_DATA_ATTR inline int time_into_sleep = 0;
RTC_DATA_ATTR inline int low_battery_wakeup_count = 0;
RTC_DATA_ATTR inline bool was_low_battery = false;

inline int POLLING_RATE = POLLING_RATE_DEFAULT, POLLING_TIME = 1000/POLLING_RATE;
inline int REFRESH_RATE = REFRESH_RATE_DEFAULT, REFRESH_TIME = 1000/REFRESH_RATE;

inline uint8_t KEY_ARR[24], KEY_ARR_COUNT = 0, LATCHED_KEY_ARR[24]; inline bool KEY_ARR_BOOL[24], PREV_KEY_ARR_BOOL[24], KEYBOARD_INACTIVE=false; inline int LAST_INPUT_TIME=millis(),SUPERKEY = -1;

enum input_mode_name {CLASSIC_INPUT,T9X,ABX,CHIP8,GSX};
inline input_mode_name INPUT_MODE = CLASSIC_INPUT; //


inline TFT_eSPI display = TFT_eSPI(); inline TFT_eSprite framebuffer = TFT_eSprite(&display), program_frame = TFT_eSprite(&display); 
inline String carrier = "", carrier2 = "", carrier3="", PROG_STRING="", DAEMON_STRING="", STATUS_STRING="";

inline SemaphoreHandle_t adc_mutex;


inline int temp = 3;
inline bool told_to_do_so=false;
inline bool gen7seg=true;
inline bool n7s_fix=true;
inline bool mute=false;

inline uint64_t wake_mask = 0;




/*bool debug=false;

uint8_t BRIGHTNESS = DISPLAY_DEFAULT_BRIGHTNESS; float FPS; int FG_COLOR = TFT_WHITE, BG_COLOR = TFT_BLACK; ///bdc7 acc7 
bool color_change=false;

uint8_t PERCENTAGE;int VOLTAGE; bool CHARGING=false, DISCONNECTED=false, SLEEPING=false, WAKE_LOCK=false, SLEEP_OVERRIDE=false, POWER_OVERRIDE=false;
RTC_DATA_ATTR int time_into_sleep = 0;
RTC_DATA_ATTR int low_battery_wakeup_count = 0;
RTC_DATA_ATTR bool was_low_battery = false;

int POLLING_RATE = POLLING_RATE_DEFAULT, POLLING_TIME = 1000/POLLING_RATE;
int REFRESH_RATE = REFRESH_RATE_DEFAULT, REFRESH_TIME = 1000/REFRESH_RATE;

uint8_t KEY_ARR[24], KEY_ARR_COUNT = 0, LATCHED_KEY_ARR[24]; bool KEY_ARR_BOOL[24], PREV_KEY_ARR_BOOL[24], KEYBOARD_INACTIVE=false; int LAST_INPUT_TIME=millis(),SUPERKEY = -1;

enum input_mode_name {CLASSIC_INPUT,T9X,ABX,CHIP8,GSX};
input_mode_name INPUT_MODE = CLASSIC_INPUT; //

QueueHandle_t text_event_queue;
struct TextEvent {
    String symbol;       // key ID from SYMBOLMAP
    EventType type;      // KEY_PRESS, KEY_RELEASE, KEY_HOLD
    uint8_t id;
    bool _delete;
    bool clear;
};

TFT_eSPI display = TFT_eSPI(); TFT_eSprite framebuffer = TFT_eSprite(&display); TFT_eSprite program_frame = TFT_eSprite(&display); 
String carrier = "", carrier2 = "", carrier3="", PROG_STRING="", DAEMON_STRING="", STATUS_STRING="";

TaskHandle_t input_daemon_handle, serial_cx_daemon_handle, connectivity_daemon_handle, display_daemon_handle, power_daemon_handle, battery_service_handle;
TaskHandle_t app_0_handle,app_1_handle,app_2_handle,app_3_handle,app_4_handle,app_5_handle,app_6_handle,app_7_handle;

enum AppID {_CALCULATOR, _GSX, _LLM, _DINO, _TERMINAL};
const String AppName[] = {"CALCULATOR", "GSX", "LLM", "DINO", "TERMINAL"};
volatile AppID FOCUSED_APP = _CALCULATOR;
volatile bool just_switched_apps = true;

int temp = 3;
bool told_to_do_so=false;

uint64_t wake_mask = 0;*/

