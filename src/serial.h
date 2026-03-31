#pragma once
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#ifdef TJPG
  #include <TJpg_Decoder.h>
#endif

#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <_lib/heap_utils.h>

#include <key_input.h>

#include <drivers/networkd2.h>

// TJpgDec callback function to render JPG to display
static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= display.height()) return 0;
  display.pushImage(x, y + FRAME_T_OFFSET, w, h, bitmap);
  return 1;
}

inline String process_command(String cmd) {
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
    } else set_brightness(value.toInt());
  } else if (command=="txs") {
    temp=value.toInt();
  } else if (command=="as") {
    a_scale=value.toInt();
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
    set_debug(!debug);
    output+="debug: "; output+=debug;
  } else if (command=="7s") {
    n7s_fix=!n7s_fix;
    output+="n7s_fix: "; output+=n7s_fix;
  } else if (command=="tt") {
    told_to_do_so=!told_to_do_so;
    output+="told to do so: "; output+=told_to_do_so;
  } else if (command=="g7s") {
    gen7seg=!gen7seg;
    output+="gen7seg: "; output+=gen7seg;
  } else if (command=="mute") {
    mute=!mute;
    output+="mute: "; output+=mute;
  } else if (command=="ff") {
    force_fullscreen=!force_fullscreen;
    output+="force_fullscreen: "; output+=force_fullscreen;
  }else if (command=="als"){
    vTaskSuspend(display_daemon_handle);
    set_brightness(0);
    pinMode(PWM_PIN, INPUT_PULLDOWN);
    display.fillScreen(TFT_WHITE);
    delay(10);
    pinMode(PWM_PIN, INPUT);
    delay(10);
    output+="als: "; output+=analogReadMilliVolts(PWM_PIN);
    pinMode(PWM_PIN, OUTPUT);
    // Re-init channel after GPIO mode change
    ledc_channel_config_t ledc_channel = {
      .gpio_num       = PWM_PIN,
      .speed_mode     = LEDC_LOW_SPEED_MODE,
      .channel        = (ledc_channel_t)PWM_CH,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = (ledc_timer_t)PWM_TIMER,
      .duty           = BRIGHTNESS,
      .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    vTaskResume(display_daemon_handle);//output+="SUPERKEY: "; output+=SUPERKEY;
  } else if (command=="fps"){
    REFRESH_RATE=value.toInt();//*2;;
    REFRESH_TIME=1000/REFRESH_RATE;
    output+="fps: "; output+=REFRESH_RATE/2;
  }  else if (command=="cpu"){
    output+="cpu: "; output+=setCpuFrequencyMhz(value.toInt());
  }  else if (command=="fap"){
    output+="fapp: "; output+=FOCUSED_APP;
  } else if (command=="c3"){
    output+="c3: "; output+=carrier3;
  } else if (command=="expr") {
    output+="result: "; output+=expr_eval(value);
  } else if (command=="heap") {
    output+="max heap: "; output+=maxheap/1000; output+="KB\n";
    output+="free heap: "; output+=getFreeHeap()/1000; output+="KB\n";
    output+="min free heap: "; output+=getMinFreeHeap()/1000; output+="KB\n";
    output+="internal free mem: "; output+=getInternalFreeHeap()/1000; output+="KB\n";
    for (size_t i = 0; i < APP_COUNT; i++) {
      Serial.print(APP_REGISTRY[i].name); Serial.print(": ");
      checkTaskStack(app_handles[i]);
    }
    
  } else if (command=="scan") {
    auto nets = WiFiManager::get().scan();
    output += "WiFi networks found: " + String(nets.size()) + "\n";
    for (auto &n : nets) {
      output += String(n.ssid.c_str()) + "  RSSI: " + String(n.rssi) + " dBm  CH:" + String(n.channel) + "  " + (n.secure ? "secured" : "open") + "\n";
    }

  } else if (command=="scan2") {
    network_commands cmd = wifi_scan;
    xQueueSend(network_command_queue, &cmd, 0);
  } else if (command=="scanr") {
    auto nets = WiFiManager::get().scan_result();
    output += "WiFi networks found: " + String(nets.size()) + "\n";
    for (auto &n : nets) {
      output += String(n.ssid.c_str()) + "  RSSI: " + String(n.rssi) + " dBm  CH:" + String(n.channel) + "  " + (n.secure ? "secured" : "open") + "\n";
    }
  } else if (command=="w") {
    if (value=="init") {
      network_commands cmd = wifi_init;
      xQueueSend(network_command_queue, &cmd, 0);
    } else if (value=="de") {
      network_commands cmd = wifi_deinit;
      xQueueSend(network_command_queue, &cmd, 0);
    } else if (value=="st") {
      output+="wifi state: "; output+=WiFiManager::get().getState();
    }
  } else if (command=="in") {
    /*struct TextEvent {
    String symbol;       // key ID from SYMBOLMAP
    EventType type;      // KEY_PRESS, KEY_RELEASE, KEY_HOLD
    uint8_t id;
    bool _delete;
    bool clear;
};*/for (size_t i = 0; i < value.length(); i++)
    {
      TextEvent text_event = {
        .symbol = String(value[i]),
        .type = KEY_RELEASE,  // Changed to KEY_RELEASE to match LLM processing
        .id = 255,  // Indicate serial input
      };

      xQueueSend(text_event_queue, &text_event, 0);/* code */
    }
  } else if (command=="inx") {
    TextEvent text_event = {
      .symbol = value,
      .type = KEY_RELEASE,  // Changed to KEY_RELEASE to match LLM processing
      .id = 255,  // Indicate serial input
    };

    xQueueSend(text_event_queue, &text_event, 0);/* code */
  } else if (command=="ts") {
    // Check input_daemon status
    if (input_daemon_handle != NULL) {
      eTaskState state = eTaskGetState(input_daemon_handle);
      output += "input_daemon: ";
      switch(state) {
        case eRunning:   output += "Running"; break;
        case eReady:     output += "Ready"; break;
        case eBlocked:   output += "Blocked"; break;
        case eSuspended: output += "Suspended"; break;
        case eDeleted:   output += "Deleted/Crashed"; break;
        default:         output += "Unknown"; break;
      }
      output += " | Stack HWM: ";
      output += uxTaskGetStackHighWaterMark(input_daemon_handle);
    } else {
      output += "input_daemon: NULL (not created)";
    }
  } else if (command=="baud") {
    int baudRate = value.toInt();
    if (baudRate >= 1200 && baudRate <= 2000000) {
      Serial.println(("Changing baud rate to ") + String(baudRate));
      Serial.flush(); // Ensure all data is sent before changing baud rate
      Serial.end();
      Serial.begin(baudRate);
      Serial.setRxBufferSize(2048);  // Increase RX buffer from 256 to 2048 bytes

      output += ("Baud rate changed to ") + String(baudRate);
    } else {
      output += F("Invalid baud rate. Please enter a value between 1200 and 2000000.");
    }
  }
  #ifdef TJPG 
    else if (command=="img") {
      TaskHandle_t currentApp = app_handles[static_cast<int>(FOCUSED_APP)];    
      vTaskResume(currentApp);
      cpu_boost(40000);

      // Format: img <width>,<height>,<jpg_size>
      // Then send binary JPG data via serial, terminated with "NULLEND"
      int comma1 = value.indexOf(',');
      int comma2 = value.indexOf(',', comma1 + 1);
      
      if (comma1 > 0 && comma2 > 0) {
        int width = value.substring(0, comma1).toInt();
        int height = value.substring(comma1 + 1, comma2).toInt();
        int jpg_size = value.substring(comma2 + 1).toInt();
        
        // Suspend current app task
        static Ticker imgRestoreTicker;

        vTaskSuspend(currentApp);
        
        // Allocate buffer for JPG data
        uint8_t* jpg_buffer = (uint8_t*)malloc(jpg_size);
        if (jpg_buffer == NULL) {
          String error_msg = ("Error: Failed to allocate JPG buffer (") + String(jpg_size) + (" bytes)");
          display.setTextColor(TFT_WHITE, TFT_BLACK);
          display.setCursor(7, FRAME_T_OFFSET);
          display.print(error_msg);
          output += error_msg;
          vTaskResume(currentApp);
          return output;
        }
        
        int bytesReceived = 0;
        String termCheck = "";
        Serial.print(F("listening for JPG data: "));
        
        // Read JPG binary data
        while (bytesReceived < jpg_size) {
          if (Serial.available()) {
            uint8_t b = Serial.read();
            jpg_buffer[bytesReceived++] = b;
            
            // Check for early termination
            termCheck += (char)b;
            if (termCheck.length() > 7) {
              termCheck.remove(0, 1);
            }
            if (termCheck.endsWith("NULLEND")) {
              bytesReceived -= 7; // Remove NULLEND from count
              break;
            }
          }
          yield(); // Prevent watchdog reset
        }
        
        // Decode and display JPG using TJpgDec
        TJpgDec.setJpgScale(1);
        TJpgDec.setSwapBytes(true);  // Ensure correct byte order for RGB565
        TJpgDec.setCallback(tft_output);
        
        uint16_t decode_result = TJpgDec.drawJpg(0, 0, jpg_buffer, bytesReceived);
        
        free(jpg_buffer);
        
        if (decode_result == 0) {
          // Schedule task resume after 10000ms only if decode was successful
          imgRestoreTicker.once_ms(10000, [currentApp]() {
            vTaskResume(currentApp);
          });
          
          output += ("Image: ") + String(width) + ("x") + String(height) + 
                    (" (") + String(bytesReceived) + (" JPG bytes decoded)");
        } else {
          String error_msg = ("JPG decode error: ") + String(decode_result);
          display.setTextColor(TFT_WHITE, TFT_BLACK);
          display.setCursor(7, FRAME_T_OFFSET);
          display.print(error_msg);
          output += error_msg;
          vTaskResume(currentApp);
        }
      } else {
        output += ("Invalid format. Use: img <width>,<height>,<jpg_size> then send JPG data with NULLEND terminator");
      }
    } 
  #endif
  else if (command!="") {
    output += ("unknown command (") + command + (")");
  } else output = ">";
  return output;
}
