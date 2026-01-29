#pragma once
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "esp_heap_caps.h"


#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <_lib/heap_utils.h>

#include <key_input.h>

#include <drivers/networkd2.h>

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
  } else if (command=="img") {
    // Format: img <width>,<height>,<hex_rgb565_data>
    // Example: img 100,50,FFFF0000FFFF... (width=100, height=50, then RGB565 pixels)
    int comma1 = value.indexOf(',');
    int comma2 = value.indexOf(',', comma1 + 1);
    
    if (comma1 > 0 && comma2 > 0) {
      int width = value.substring(0, comma1).toInt();
      int height = value.substring(comma1 + 1, comma2).toInt();
      String hexData = value.substring(comma2 + 1);
      
      // Suspend current app task
      static Ticker imgRestoreTicker;
      TaskHandle_t currentApp = app_handles[static_cast<int>(FOCUSED_APP)];
      vTaskSuspend(currentApp);
      
      // Parse and draw image data
      int pixelCount = width * height;
      int hexDataLen = hexData.length();
      int pixelsReceived = hexDataLen / 4;
      
      for (int i = 0; i < pixelCount && (i * 4 + 3) < hexDataLen; i++) {
        // Extract 4 hex chars directly
        char pixelHex[5];
        pixelHex[0] = hexData[i * 4];
        pixelHex[1] = hexData[i * 4 + 1];
        pixelHex[2] = hexData[i * 4 + 2];
        pixelHex[3] = hexData[i * 4 + 3];
        pixelHex[4] = '\0';
        
        uint16_t color = (uint16_t)strtol(pixelHex, nullptr, 16);
        int x = i % width;
        int y = i / width;
        program_frame.drawPixel(x, y, color);
      }
      
      // Update display
      frame_ready();
      
      // Schedule task resume after 2000ms
      imgRestoreTicker.once_ms(2000, [currentApp]() {
        vTaskResume(currentApp);
      });
      
      output += "Image: " + String(width) + "x" + String(height) + 
                " (" + String(pixelsReceived) + "/" + String(pixelCount) + " pixels)";
    } else {
      output += "Invalid format. Use: img <width>,<height>,<hex_data>";
    }
  } else if (command!="") {
    output+="unknown command ("; output+=command; output+=")";
  } else output = ">";
  return output;
}
