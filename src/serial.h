#pragma once
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"

#include <definitions.h>
#include <global.h>
#include <excludable.h>

#include <key_input.h>

String process_command(String cmd) {
  cmd.trim();

  int space_index = cmd.indexOf(' ');
  //if (spaceIndex == -1) return;

  static String command;
  command = cmd.substring(0, space_index);
  String value = cmd.substring(space_index + 1);

  static String output;
  output = "> ";
  //Serial.print("processing command: ");
  //output+=command);

  if(command == "bg") {
    change_system_color(FG_COLOR, parse_color(value));
    output+= "background color set to 0x" + String(BG_COLOR);
    //framebuffer.fillSprite(BG_COLOR);
  } else if(command == "fg") {
    change_system_color(parse_color(value),BG_COLOR);
    //FG_COLOR = parse_color(value);
    output+= "foreground color set to 0x"+String(FG_COLOR);
  } else if(command=="rst") {
    output+="resetting..";
    reset();
  } else if(command=="ds") {
    output+="going into deep sleep..";
    deep_sleep();
    //reset();
  }else if(command=="brg") {
    if (value=="?") {output+=BRIGHTNESS;
    } else ledcWrite(PWM_PIN, value.toInt());
  } else if (command=="txs") {
    temp=value.toInt();
  } else if (command=="inf"||command=="info") {
    output+="uptime: "; output+=uptime(); 
    output+="s\nCPU frequency: "; output+=getCpuFrequencyMhz(); 
    output+="MHz\nAPB frequency: "; output+=getApbFrequency()/1000000; 
    output+="MHz\nXTAL frequency: "; output+=getXtalFrequencyMhz(); output+="MHz";
  } else if (command=="v") {
    output+="battery voltage: "; output+=float(VOLTAGE)/1000; output+="V";
  } else if (command=="sleep") {
    SLEEP_OVERRIDE=!SLEEP_OVERRIDE;//LAST_INPUT_TIME=millis()-KEYBOARD_TIMEOUT;
    output+="sleep override: "; output+=SLEEP_OVERRIDE;
  } else if (command=="input") {
    output+="input method: "; output+=value.toInt();
    INPUT_MODE = static_cast<input_mode_name>(value.toInt());
  } else if (command=="solar") {
    output+="solar: "; output+=analogReadMilliVolts(SOLAR_PIN);
  } else if (command=="io") {
    output+="gpio ";output+=value.toInt(); output+=": "; output+=digitalRead(value.toInt());
    
  } else if (command=="app") {
    output+="app: "; output+=value.toInt();
    switch_app(static_cast<AppID>(value.toInt()));
  } else if (command=="wl") {
    WAKE_LOCK=!WAKE_LOCK;
    output+="wakelock state: "; output+=WAKE_LOCK;
  } else if (command=="abg") {
    AUTO_BRIGHTNESS=!AUTO_BRIGHTNESS;
    output+="auto brightness state: "; output+=AUTO_BRIGHTNESS;
  } else if (command=="dbg") {
    debug=!debug;
    output+="debug: "; output+=debug;
  } else if (command=="7s") {
    n7s_fix=!n7s_fix;
    output+="n7s_fix: "; output+=n7s_fix;
  } else if (command=="ttds") {
    told_to_do_so=!told_to_do_so;
    output+="told_to_do_so: "; output+=told_to_do_so;
  } else if (command=="g7s") {
    gen7seg=!gen7seg;
    output+="gen7seg: "; output+=gen7seg;
  } else if (command=="mute") {
    mute=!mute;
    output+="mute: "; output+=mute;
  }else if (command=="als"){
    vTaskSuspend(display_daemon_handle);
    ledcWrite(PWM_PIN, 0);
    pinMode(PWM_PIN, INPUT_PULLDOWN);
    display.fillScreen(TFT_WHITE);
    delay(10);
    pinMode(PWM_PIN, INPUT);
    delay(10);
    output+="als: "; output+=analogReadMilliVolts(PWM_PIN);
    pinMode(PWM_PIN, OUTPUT);
    ledcAttach(PWM_PIN, 1000, PWM_RES);
    ledcWrite(PWM_PIN, BRIGHTNESS);
    //ledc_fade_func_install(0);
    vTaskResume(display_daemon_handle);//output+="SUPERKEY: "; output+=SUPERKEY;
  } else if (command=="fps"){
    REFRESH_RATE=value.toInt()*2;;
    REFRESH_TIME=1000/REFRESH_RATE;
    output+="fps: "; output+=REFRESH_RATE/2;
  }  else if (command=="cpu"){
    output+="cpu: "; output+=setCpuFrequencyMhz(value.toInt());
  }  else if (command=="c3"){
    output+="c3: "; output+=carrier3;
  }
  else if (command!="") {
    output+="unknown command ("; output+=command; output+=")";
  } else output = ">";
  return output;
}
