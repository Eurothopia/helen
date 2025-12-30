#pragma once
#include "math.h"
#include "Arduino.h"
#include "esp_system.h"
#include "tinyexpr.h"


#include <definitions.h>
#include <global.h>
//#include <apps/_xoxo.h>

#include <ns/battery_ns.h>
#include <ns/matrix_ns.h> //keymaps

uint32_t uptime(uint32_t value = -1) {
    if (value!=-1) uptime_offset = value;
    return (millis()+uptime_offset)/1000;
}
void enable_on_key_wakeup() {
    pinMode(PINMAP[0],OUTPUT); pinMode(PINMAP[1],INPUT_PULLDOWN);
    digitalWrite(PINMAP[0], HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(PINMAP[0]));
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PINMAP[1]), 1);
}


String status(String input="", uint8_t priority=0, int timeout=0) {
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

void switch_app(AppID new_AppID) {
    static String buf = ""; buf="APP: "; buf+=AppName[new_AppID]; FOCUSED_APP = new_AppID; just_switched_apps=true;  status(buf, 10, 1000);
    fullscreen = applist[static_cast<int>(new_AppID)].fullscreen;
}

void change_system_color(int FG_COLOR2C, int BG_COLOR2C) {
  color_change=true;
  FG_COLOR=FG_COLOR2C; BG_COLOR=BG_COLOR2C;
  framebuffer.fillSprite(BG_COLOR);
}
void flash_string(String string, int count, int x, int y,bool revert=false, int ms=500) {
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

void reset() {
    flash_string("RESETTING",2,X_MIDDLE, Y_MIDDLE, false, 250);
    ledcWrite(PWM_PIN, 0);//display.writecommand(ST7789_SLPIN); 
    ESP.restart();
}

void deep_sleep(bool enable_key_wake=true, int32_t wake_time=-1)  {
    Serial.println("entering deep sleep");
    vTaskSuspend(input_daemon_handle);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (enable_key_wake) enable_on_key_wakeup();
    //if (wake_time!=-1)   esp_sleep_enable_timer_wakeup(wake_time);
    display.writecommand(ST7789_SLPIN); 
    esp_deep_sleep_start();
    Serial.print("entering sleep failed?");
    vTaskResume(input_daemon_handle);
}

uint16_t parse_color(String hex) {
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



int nearest_perc (int target, const int arr[], const int size) {
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
int mV2PERCENTAGE (int b_voltage) {
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

String expr_eval(const String &expr) {
  int err;
  String expr2 = expr;
  expr2.replace('x', '*');
  double v = te_interp(expr2.c_str(), &err);

  if (err != 0) {
    return "SYNTAX ERROR";// (" + String(err) + ")";
  }

  // convert result to string
  String buf = String(v, 2);
  if (buf.endsWith(".00")) buf.remove(buf.length() - 3);
  return buf;   // 10 = precision, adjust if needed
}


#define DRAW_H(x, y, w, c) program_frame.drawFastHLine((x),(y),(w),(c))
#define DRAW_V(x, y, h, c) program_frame.drawFastVLine((x),(y),(h),(c))
void N7(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
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
void N7S(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
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

void N7S_AA(int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
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

enum _bool {nope, yeah};