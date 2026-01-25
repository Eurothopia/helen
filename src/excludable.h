#pragma once
#include "math.h"
#include "Arduino.h"
#include "esp_system.h"
#include "tinyexpr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Ticker.h"
#include <Preferences.h>
#include "driver/ledc.h"

#include <definitions.h>
#include <global.h>

#include <key_input.h> //clear this this messy as fuck
//#include <apps/_xoxo.h>

//#include <drivers/networkd2.h>

#include <ns/battery_ns.h>
#include <ns/matrix_ns.h> //keymaps

struct PersistedSettings {
  bool debug = false;
};

inline Preferences preferences;
inline PersistedSettings persisted_settings;
inline void load_settings() {
  if (!preferences.begin("helen", false)) {  // writable (creates namespace if missing)
    return;
  }
  persisted_settings.debug = preferences.getBool("debug", false);
  debug = persisted_settings.debug;  // Apply to global variable
  preferences.end();
}

inline void save_settings() {
  if (!preferences.begin("helen", false)) {
    return;
  }
  preferences.putBool("debug", persisted_settings.debug);
  preferences.end();
}

inline void set_debug(bool value) {
  persisted_settings.debug = value;
  debug = value;
  save_settings();
}

inline void set_brightness(uint8_t value, ledc_mode_t mode = LEDC_LOW_SPEED_MODE, ledc_channel_t channel = (ledc_channel_t)PWM_CH) {
  ledc_set_duty(mode, channel, value);
  ledc_update_duty(mode, channel);
}

inline uint32_t uptime(uint32_t value = -1) {
    if (value!=-1) uptime_offset = value;
    return (millis()+uptime_offset)/1000;
}
inline void enable_on_key_wakeup() {
    pinMode(PINMAP[0],OUTPUT); pinMode(PINMAP[1],INPUT_PULLDOWN);
    digitalWrite(PINMAP[0], HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(PINMAP[0]));
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PINMAP[1]), 1);
}


inline String status(String input="", uint8_t priority=0, int timeout=0) {
  static uint8_t last_status_priority = 0;
  static unsigned long last_status_time = 0;
  static int set_timeout = 0;
  static bool clear;
  if(input!=""&&input!="-1"&&priority>=last_status_priority) {
    last_status_priority=priority;
    set_timeout=timeout;
    last_status_time=millis();
    clear=false;
    STATUS_STRING=input;
  //} else if (input=="-1"&&priority>=last_status_priority) {
  } else if (((millis()-last_status_time>=set_timeout)||(input=="-1"&&priority>=last_status_priority))&&!clear) { //check if stale
    last_status_priority=0;
    set_timeout=0;
    clear=true;
    STATUS_STRING="";
  }
  return STATUS_STRING;
  //STATUS_STRING;
}

inline void switch_app(AppID new_AppID) {
    //xQueueReset(text_event_queue);  // clear pending text events
    const auto &app = getApp(new_AppID);
    static String buf = ""; buf = "APP: "; buf += app.name; FOCUSED_APP = new_AppID; just_switched_apps = true;  status(buf, 10, 1000);
    if (WIFI != app.config->needs_network) {
      network_commands cmd = app.config->needs_network ? wifi_init : wifi_deinit;
      xQueueSend(network_command_queue, &cmd, 0);
      WIFI = app.config->needs_network;
    }
    WAIT_FOR_DMA = app.config->vsync;
    fullscreen = app.config->fullscreen || force_fullscreen;
    #ifdef D1BIT
      program_frame.setBitmapColor(FG_COLOR, BG_COLOR);
    #endif

    xTaskNotifyGive(app_handles[static_cast<int>(new_AppID)]);
    //if (WIFI) WiFiManager::get().init();
}

inline void draw_keyboard (int id = -1) {
  static int last_id=-1;
  if(last_id!=id) {
    if (last_id==23) last_id=24;
    int x = 255 + (last_id%5)*12;
    int y = initial_px + (last_id/5)*(spacing_px);
    program_frame.drawRect(x-2, y-2, 9, 11, BG_COLOR);
  }
  program_frame.setTextDatum(TL_DATUM);
  program_frame.drawString("a b c d e", 255, initial_px);
  //program_frame.setTextDatum(MR_DATUM);
  program_frame.drawString("f g h i j", 255, initial_px+spacing_px*1);
  program_frame.drawString("k l m n o", 255, initial_px+spacing_px*2);
  program_frame.drawString("p r s t u", 255, initial_px+spacing_px*3);
  program_frame.drawString("v # _   y", 255, initial_px+spacing_px*4);
  //program_frame.drawString("a", 255, initial_px);
  if (id!=-1) {
    if (id==23) id=24;
    int x = 255 + (id%5)*12;
    int y = initial_px + (id/5)*(spacing_px);
    //program_frame.drawRect(x-2, y-2, 9, 11, FG_COLOR);
    program_frame.drawFastHLine(x-1, y-2, 7, FG_COLOR);  // top
    program_frame.drawFastHLine(x-1, y+8, 7, FG_COLOR);  // bottom
    program_frame.drawFastVLine(x-2, y-1, 9, FG_COLOR);  // left
    program_frame.drawFastVLine(x+6, y-1, 9, FG_COLOR);  // right
    last_id=id;
  }
  program_frame.drawString("a", 255, initial_px);
}

inline Ticker cpuCooldownTimer;
const uint32_t DEFAULT_FREQ = 80;  // Your standard running speed (e.g., 80MHz)
const uint32_t BOOST_FREQ = 240;   // Maximum speed

// Callback function: This runs when the timer finishes
inline void downclock() {
    setCpuFrequencyMhz(DEFAULT_FREQ);
    if (debug) Serial.printf("[CPU] Boost ended. Clock returned to %d MHz\n", DEFAULT_FREQ);
    boosting = false;
}

/**
 * @param boostFreq   The frequency to jump to (e.g., 240)
 * @param durationMs  How long to stay at that frequency (in milliseconds)
 */
inline void cpu_boost( uint32_t durationMs, int frequency=MAX_CPU_FREQ) {
    // 1. Set the high clock speed
    setCpuFrequencyMhz(frequency);
    
    // 2. Schedule the downclock function
    // .once_ms(delay, callback) will run the function once and then stop.
    // If cpu_boost is called again, it will reset the existing timer.
    cpuCooldownTimer.once_ms(durationMs, downclock);

    if (debug) Serial.printf("[CPU] Boosted to %d MHz for %d ms\n", MAX_CPU_FREQ, durationMs);
    boosting = true;
}

inline void change_system_color(int FG_COLOR2C, int BG_COLOR2C) {
  color_change=true;
  FG_COLOR=FG_COLOR2C; BG_COLOR=BG_COLOR2C;
  //display.fillScreen(BG_COLOR); //crashes things
  FrameEvent evt = {CLEAR_DISPLAY, false, 0};
    xQueueSend(frame_command_queue, &evt, 0);
}
inline void flash_string(String string, int count, int x, int y,bool revert=false, int ms=500) {
  vTaskSuspend(display_daemon_handle);
    display.fillScreen(BG_COLOR);
    display.setTextDatum(MC_DATUM);
    display.setTextSize(2);
    for (size_t i = 0; i < count; i++)
    {  //flash thrice
      display.setTextColor(BG_COLOR, BG_COLOR, true);
      display.drawString(string, 320/2, 34+(108/2));
      delay(ms);
      display.setTextColor(FG_COLOR, BG_COLOR, true);
      display.drawString(string, 320/2, 34+(108/2));
      delay(ms);
    }
  if (revert) vTaskResume(display_daemon_handle);
}

inline void reset() {
    flash_string("RESETTING",2,X_MIDDLE, Y_MIDDLE, false, 250);
    set_brightness(0);//display.writecommand(ST7789_SLPIN); 
    ESP.restart();
}

inline void deep_sleep(bool enable_key_wake=true, int32_t wake_time=-1)  {
    network_commands cmd = wifi_deinit;
    xQueueSend(network_command_queue, &cmd, 0);
    Serial.println("entering deep sleep");
    vTaskSuspend(input_daemon_handle);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (enable_key_wake) enable_on_key_wakeup();
    //if (wake_time!=-1)   esp_sleep_enable_timer_wakeup(wake_time);
    #ifndef WOKWI
        display.writecommand(ST7789_SLPIN); 
    #endif
    esp_deep_sleep_start();
    Serial.print("entering sleep failed?");
    vTaskResume(input_daemon_handle);
}



inline uint16_t parse_color(String hex) {
  // Remove # if present
  if (hex.startsWith("#")) hex.remove(0, 1);

  // Allow short (4-digit) or full (6-digit) hex
  if (hex.length() == 4) {
    // e.g. "44BA" -> RGB565
    uint16_t value = strtol(hex.c_str(), nullptr, 16);
    return value;
  } else if (hex.length() == 6) {
    // e.g. "44BA32" -> 24-bit RGB, convert to 565
    uint8_t r = strtol(hex.substring(0, 2).c_str(), nullptr, 16);
    uint8_t g = strtol(hex.substring(2, 4).c_str(), nullptr, 16);
    uint8_t b = strtol(hex.substring(4, 6).c_str(), nullptr, 16);
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  return 0; // invalid
}



inline int nearest_perc (int target, const int arr[], const int size) {
    //internal check?
    //int tmp;
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == target) {
            return i*5;
        } else if (arr[i] > target) { 
            //tmp=i;
            return (abs(arr[i]-target)<abs(arr[i-1]-target)) ? i*5 : (i-1)*5;
        } else i++;
    }
    return 979080;
    
    
}
inline int mV2PERCENTAGE (int b_voltage) {
    if (b_voltage > BATTERY_P_GLOBAL[BATTERY_P_SIZE-1] || b_voltage == BATTERY_P_GLOBAL[BATTERY_P_SIZE-1]) return 100;
    //Serial.println(); Serial.print(b_voltage); Serial.println(max);
    for (int i = 0; i < BATTERY_P_SIZE-1; i++) {
        if (b_voltage == BATTERY_P_GLOBAL[i]) {
            return i*5;
        } else if (b_voltage > BATTERY_P_GLOBAL[i] && b_voltage < BATTERY_P_GLOBAL[i+1]) {
                // 408       >= 407                 && 408       < 411
                float v1 = BATTERY_P_GLOBAL[i], v2 = BATTERY_P_GLOBAL[i+1];
                float p1 = i*5, p2 = (i+1)*5;
                return p1 + ((p2 - p1) / (v2 - v1)) * (b_voltage - v1);
        }
    }
    return 0;
}

inline String expr_eval(const String &expr) {
  int err;
  String expr2 = expr;
  expr2.replace('x', '*');
  expr2.replace("sq(", "sqrt(");
  double v = te_interp(expr2.c_str(), &err);

  if (err != 0) {
    expr2 += ")";
    err = 0;
    v = te_interp(expr2.c_str(), &err);
    if (err != 0) return "uhh idk";
  }

  // convert result to string
  String buf = String(v, 2);
  if (buf.endsWith(".00")) buf.remove(buf.length() - 3);
  return buf;   // 10 = precision, adjust if needed
}


#define DRAW_H(x, y, w, c) program_frame.drawFastHLine((x),(y),(w),(c))
#define DRAW_V(x, y, h, c) program_frame.drawFastVLine((x),(y),(h),(c))
inline void N7(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
  unsigned int num=abs(n),i,s,t,w,col,h,a,b,si=0,j=1,d=0,S1=cS,S2=5*cS,S3=2*cS,S4=7*cS,x1=(S3/2)+1,x2=(2*S1)+S2+1,y1=yLoc+x1,y3=yLoc+(2*S1)+S4+1;
  unsigned int seg[7][3]={{(S3/2)+1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
  unsigned char nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67,0x00,0x40},c=(c=abs(cS))>10?10:(c<1)?1:c,cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
  for (xLoc+=cnt*(d=(2*S1)+S2+(2*S3)+2);cnt>0;cnt--){
    for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
      col=(nums[i]&(1<<j))?fC:bC;s=(2*S1)/S3;
      if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+(S3/2),a=xLoc+seg[j][0]+S1,b=seg[j][1];b<h;b++,a-=s,w+=(2*s))DRAW_H(a,b,w,col);
      else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+S3/2,b=xLoc+seg[j][0],a=seg[j][1]+S1;b<h;b++,a-=s,w+=(2*s))DRAW_V(b,a,w,col);
      for (;b<t;b++,a+=s,w-=(2*s))seg[j][2]?DRAW_H(a,b,w,col):DRAW_V(b,a,w,col);
    }
  }
}

#define k -0.1
#define DRAW_HS(x, y, w, c) program_frame.drawLine((x) + k * ((y) - yLoc), (y), (x) + w + k * ((y) - yLoc) + 1, (y), (c))
#define DRAW_VS(x, y, h, c) program_frame.drawLine((x) + k * ((y) - yLoc), (y), (x) + k * ((y + h) - yLoc), (y) + h + 1, (c))
inline void N7S(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
  unsigned int num=abs(n),i,s,t,w,col,h,a,b,si=0,j=1,d=0,S1=cS,S2=5*cS,S3=2*cS,S4=7*cS,x1=(S3/2)+1,x2=(2*S1)+S2+1,y1=yLoc+x1,y3=yLoc+(2*S1)+S4+1;
  unsigned int seg[7][3]={{(S3/2)+1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
  unsigned char nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67,0x00,0x40},c=(c=abs(cS))>10?10:(c<1)?1:c,cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
  for (xLoc+=cnt*(d=(2*S1)+S2+(2*S3)+2);cnt>0;cnt--){
    for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
      col=(nums[i]&(1<<j))?fC:bC;s=(2*S1)/S3;
      if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+(S3/2),a=xLoc+seg[j][0]+S1,b=seg[j][1];b<h;b++,a-=s,w+=(2*s))DRAW_HS(a,b,w,col);
      else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+S3/2,b=xLoc+seg[j][0],a=seg[j][1]+S1;b<h;b++,a-=s,w+=(2*s))DRAW_VS(b,a,w,col);
      for (;b<t;b++,a+=s,w-=(2*s))seg[j][2]?DRAW_HS(a,b,w,col):DRAW_VS(b,a,w,col);
    }
  }
}



#define DRAW_AAHS(x, y, w, c) program_frame.drawWideLine((x) + k * ((y) - yLoc),    (y),    (x) + w + k * ((y) - yLoc) + 1,       (y),       1,     (c))
#define DRAW_AAVS(x, y, h, c) program_frame.drawWideLine((x) + k * ((y) - yLoc),     (y),    (x) + k * ((y + h) - yLoc),      (y) + h + 1,   1,     (c))

inline void N7S_AA(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
  unsigned int num=abs(n),i,s,t,w,col,h,a,b,si=0,j=1,d=0,S1=cS,S2=5*cS,S3=2*cS,S4=7*cS,x1=(S3/2)+1,x2=(2*S1)+S2+1,y1=yLoc+x1,y3=yLoc+(2*S1)+S4+1;
  unsigned int seg[7][3]={{(S3/2)+1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
  unsigned char nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67,0x00,0x40},c=(c=abs(cS))>10?10:(c<1)?1:c,cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
  for (xLoc+=cnt*(d=(2*S1)+S2+(2*S3)+2);cnt>0;cnt--){
    for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
      col=(nums[i]&(1<<j))?fC:bC;s=(2*S1)/S3;
      if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+(S3/2),a=xLoc+seg[j][0]+S1,b=seg[j][1];b<h;b++,a-=s,w+=(2*s))DRAW_AAHS(a,b,w,col);
      else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+S3/2,b=xLoc+seg[j][0],a=seg[j][1]+S1;b<h;b++,a-=s,w+=(2*s))DRAW_AAVS(b,a,w,col);
      for (;b<t;b++,a+=s,w-=(2*s))seg[j][2]?DRAW_AAHS(a,b,w,col):DRAW_AAVS(b,a,w,col);
    }
  }
}

inline String strip_diacritics(const String& text) {
  String result = text;
  // UTF-8 two-byte replacements (Czech diacritics are 0xC3 + second byte)
  result.replace("\xC3\xA1", "a"); result.replace("\xC3\x81", "A"); // á Á
  result.replace("\xC4\x8D", "c"); result.replace("\xC4\x8C", "C"); // č Č
  result.replace("\xC4\x8F", "d"); result.replace("\xC4\x8E", "D"); // ď Ď
  result.replace("\xC3\xA9", "e"); result.replace("\xC3\x89", "E"); // é É
  result.replace("\xC4\x9B", "e"); result.replace("\xC4\x9A", "E"); // ě Ě
  result.replace("\xC3\xAD", "i"); result.replace("\xC3\x8D", "I"); // í Í
  result.replace("\xC5\x88", "n"); result.replace("\xC5\x87", "N"); // ň Ň
  result.replace("\xC3\xB3", "o"); result.replace("\xC3\x93", "O"); // ó Ó
  result.replace("\xC5\x99", "r"); result.replace("\xC5\x98", "R"); // ř Ř
  result.replace("\xC5\xA1", "s"); result.replace("\xC5\xA0", "S"); // š Š
  result.replace("\xC5\xA5", "t"); result.replace("\xC5\xA4", "T"); // ť Ť
  result.replace("\xC3\xBA", "u"); result.replace("\xC3\x9A", "U"); // ú Ú
  result.replace("\xC5\xAF", "u"); result.replace("\xC5\xAE", "U"); // ů Ů
  result.replace("\xC3\xBD", "y"); result.replace("\xC3\x9D", "Y"); // ý Ý
  result.replace("\xC5\xBE", "z"); result.replace("\xC5\xBD", "Z"); // ž Ž
  return result;
}
inline char* strip_diacritics(const char* input) {
    if (!input) return nullptr;
    
    size_t len = strlen(input);
    char* output = (char*)malloc(len + 1);  // Allocate result buffer
    if (!output) return nullptr;
    
    size_t out_idx = 0;
    for (size_t i = 0; i < len; ) {
        // Check for UTF-8 two-byte Czech diacritics (0xC3/0xC4/0xC5 + second byte)
        if ((uint8_t)input[i] == 0xC3 && i + 1 < len) {
            uint8_t second = (uint8_t)input[i + 1];
            switch (second) {
                case 0xA1: case 0x81: output[out_idx++] = (second == 0xA1) ? 'a' : 'A'; i += 2; continue; // á Á
                case 0xA9: case 0x89: output[out_idx++] = (second == 0xA9) ? 'e' : 'E'; i += 2; continue; // é É
                case 0xAD: case 0x8D: output[out_idx++] = (second == 0xAD) ? 'i' : 'I'; i += 2; continue; // í Í
                case 0xB3: case 0x93: output[out_idx++] = (second == 0xB3) ? 'o' : 'O'; i += 2; continue; // ó Ó
                case 0xBA: case 0x9A: output[out_idx++] = (second == 0xBA) ? 'u' : 'U'; i += 2; continue; // ú Ú
                case 0xBD: case 0x9D: output[out_idx++] = (second == 0xBD) ? 'y' : 'Y'; i += 2; continue; // ý Ý
            }
        } else if ((uint8_t)input[i] == 0xC4 && i + 1 < len) {
            uint8_t second = (uint8_t)input[i + 1];
            switch (second) {
                case 0x8D: case 0x8C: output[out_idx++] = (second == 0x8D) ? 'c' : 'C'; i += 2; continue; // č Č
                case 0x8F: case 0x8E: output[out_idx++] = (second == 0x8F) ? 'd' : 'D'; i += 2; continue; // ď Ď
                case 0x9B: case 0x9A: output[out_idx++] = (second == 0x9B) ? 'e' : 'E'; i += 2; continue; // ě Ě
            }
        } else if ((uint8_t)input[i] == 0xC5 && i + 1 < len) {
            uint8_t second = (uint8_t)input[i + 1];
            switch (second) {
                case 0x88: case 0x87: output[out_idx++] = (second == 0x88) ? 'n' : 'N'; i += 2; continue; // ň Ň
                case 0x99: case 0x98: output[out_idx++] = (second == 0x99) ? 'r' : 'R'; i += 2; continue; // ř Ř
                case 0xA1: case 0xA0: output[out_idx++] = (second == 0xA1) ? 's' : 'S'; i += 2; continue; // š Š
                case 0xA5: case 0xA4: output[out_idx++] = (second == 0xA5) ? 't' : 'T'; i += 2; continue; // ť Ť
                case 0xAF: case 0xAE: output[out_idx++] = (second == 0xAF) ? 'u' : 'U'; i += 2; continue; // ů Ů
                case 0xBE: case 0xBD: output[out_idx++] = (second == 0xBE) ? 'z' : 'Z'; i += 2; continue; // ž Ž
            }
        }
        // Not a diacritic, copy as-is
        output[out_idx++] = input[i++];
    }
    output[out_idx] = '\0';
    return output;  // Caller must free()
}

enum _bool {nope, yeah};

inline void frame_ready() {
    FrameEvent evt = {FRAME_READY, true, 0};
    xQueueSend(frame_command_queue, &evt, 0);
}

inline void update_statusbar() {
    FrameEvent evt = {STATUS_UPDATE, true, 0};
    xQueueSend(frame_command_queue, &evt, 0);
    xTaskNotifyGive(display_daemon_handle);
    vTaskDelay(25);
}