#pragma once
//#include "TFT_eSPI.h"  
/*#include <fonts/segment40pt7b.h>
#include <fonts/segment7_sharp10pt7b.h>
#include <fonts/DSEG7Classic_Italic40pt7b.h>
#define SEG7FONT &segment40pt7b
#define SEG7ALTFONT &segment7_sharp10pt7b
#define DSEG &DSEG7Classic_Italic40pt7b*/
#include <fonts/micro4.h>
#include <fonts/micro8.h>
#include <fonts/micro13.h>
#include <fonts/micro10.h>
#include <fonts/micro20.h>
#define MICRO4  &Micro_Medium4pt7b
#define MICRO8  &Micro_Medium8pt7b
#define MICRO13 &Micro_Medium13pt7b
#define MICRO10 &Micro_Medium10pt7b
#define MICRO20 &Micro_Medium20pt7b

#define TFT_PRIMARY_COLOR TFT_RED
#define D1BIT

#define PWM_CH 0
#define PWM_TIMER 0
#define PWM_PIN 4
#define PWM_RES 12
#define PWM_MAX = (1<<PWM_RES) - 1;
#define DISPLAY_DEFAULT_BRIGHTNESS 5

inline int L_OFFSET =     7;
inline int  T_OFFSET =    7;
inline int  R_OFFSET =    0;
#define X_MIDDLE          (320/2)+12
#define Y_MIDDLE          34+(108/2)
#define STATUS_BAR_HEIGHT 17
#define VIEWPORT_HEIGHT   114

#define DISPLAY_WIDTH 320

#define VSYNC_ENABLED true

#define CURSOR_BLINK_TIME 1250

#define SOLAR_PIN    36
#define BATTERY_PIN  39
#define CHARGING_PIN 17

#define DEBOUNCE_MS  40
#define HOLD_MS      250
#define HOLD_REPEAT_MS 70
#define DOUBLE_PRESS_MS 350
#define S_HOLD_MS 1500


#define KEYBOARD_TIMEOUT_DEFAULT 180000 //15000 //
inline int KEYBOARD_TIMEOUT = KEYBOARD_TIMEOUT_DEFAULT;
#define POLLING_RATE_DEFAULT 20
#define REFRESH_RATE_DEFAULT 30

#define WIFI_TIMEOUT_MS 120000

#define SLEEPING_REFRESH_RATE 4//0.1
//#define SLEEPING_RATE 4

#define SLEEPING_TIME_MS 60*1000  //250 //10*1000 //10s
#define SLEEPING_TIME_US SLEEPING_TIME_MS*1000//2

//#define SLEEPING_TIME 250 //200


#define MAX_CPU_FREQ 240
#define DEFAULT_CPU_FREQ 80
#define SYSTEM_CORE  0
#define PROGRAM_CORE 1



//UI
        #define initial_px 9
        #define spacing_px 16