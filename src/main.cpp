#include "Arduino.h"
#include "TFT_eSPI.h"      // Hardware-specific library
#include "SPI.h"
#include "algorithm"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp32-hal-cpu.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "Ticker.h"
//#include "WiFi.h"
//#include "esp_wifi.h"


#include <definitions.h>
#include <global.h>
#include <excludable.h>

#include <ns/battery_ns.h>
#include <ns/matrix_ns.h> //keymaps
#include <ns/secrets_ns.h>
#include <matrix_core.h>
#include <key_input.h>
#include <serial.h>

#include <misc/images.h>

#ifndef NO_WIFI
#include <drivers/networkd2.h>
#endif
#include <drivers/BLEd.h>
#include <drivers/ble_service.h>

void bootISR() {
  status((digitalRead(0) ? "BOOTLOADER ON STANDBY" : "-1"), 20, 15000);
  Serial.print(status());
}
void chargeISR() { CHARGING = !digitalRead(CHARGING_PIN);}
void pwm_init(TimerHandle_t ass) {
  ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_CH, DISPLAY_DEFAULT_BRIGHTNESS);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_CH);
}
void boot_init(TimerHandle_t ass) {attachInterrupt(digitalPinToInterrupt(0), bootISR, CHANGE);}

void print_ext1_wake_pins() {
  uint64_t mask = esp_sleep_get_ext1_wakeup_status();
  String output = "";
  
  for (int i = 0; i < 64; i++) {
    if (wake_mask & (1ULL << i)) {
      output += digitalRead(i) ? "1" : "0";
    }
  }
  
  if (debug) {
    Serial.print("EXT1 wake pins: ");
    Serial.println(output);
  }
}

void printWakeMaskPins(uint64_t mask) {
    Serial.print("[wake_pins] GPIOs: ");
    bool first = true;

    for (int gpio = 0; gpio < 64; ++gpio) {
        if (mask & (1ULL << gpio)) {
            if (!first) Serial.print(", ");
            Serial.print(gpio);
            first = false;
        }
    }

    if (first) Serial.print("(none)");
    Serial.println();
}


void set_gpio_wakeups() {

//OUT_HIGH: 0,3,5,6,7,10
//INPUT: 1,2,4[NO_PD],8,9

  const int OUT_HIGH[] = {PINMAP[0], PINMAP[3], PINMAP[5],PINMAP[6],PINMAP[7],PINMAP[10]};    // pins you can drive high
  const int INPUT_WAKE[] = {PINMAP[1],PINMAP[2],PINMAP[8],PINMAP[9]};  // input-only or inputs for wakeup
  const int INPUT_ONLY_WAKE[] = {};
  //{PINMAP[4]};

  // Set outputs high
  for (int i=0; i<sizeof(OUT_HIGH)/sizeof(OUT_HIGH[0]); i++){
      pinMode(OUT_HIGH[i], OUTPUT);
      digitalWrite(OUT_HIGH[i], HIGH);
  }

  // Configure inputs with pull-down and wakeup on HIGH
  for (int i=0; i<sizeof(INPUT_WAKE)/sizeof(INPUT_WAKE[0]); i++){
      pinMode(INPUT_WAKE[i], INPUT_PULLDOWN);
      wake_mask |= 1ULL << INPUT_WAKE[i];//esp_sleep_enable_ext1_wakeup(1ULL << INPUT_WAKE[i], ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  for (int i=0; i<sizeof(INPUT_ONLY_WAKE)/sizeof(INPUT_ONLY_WAKE[0]); i++){
      pinMode(INPUT_ONLY_WAKE[i], INPUT);
      wake_mask |= 1ULL << INPUT_ONLY_WAKE[i];//esp_sleep_enable_ext1_wakeup(1ULL << INPUT_ONLY_WAKE[i], ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  //void print_wake_mask_binary() {
    if (debug) {
      Serial.print("wake_mask: ");
      Serial.print((uint32_t)(wake_mask >> 32), BIN); 
    
      Serial.print("::");
    
      Serial.println((uint32_t)(wake_mask & 0xFFFFFFFF), BIN); 
      printWakeMaskPins(wake_mask);
    }
    
   // print_ext1_wake_pins(); //print their state
//}
  esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  //Serial.print("setting up gpio input_only..");
}

void render_status() {
  if(!fullscreen) {
    status_frame.setTextSize(1);
    status_frame.setTextColor(FG_COLOR, BG_COLOR, true);
    //status_frame.fillRect(L_OFFSET,6,320,STATUS_BAR_HEIGHT, BG_COLOR);
    status_frame.fillRect(0,0,320,STATUS_BAR_HEIGHT+6, BG_COLOR);
    
    // Battery voltage
    status_frame.setCursor(L_OFFSET, T_OFFSET);
    //status_frame.print("test");
    if (!debug) {
      if(CHARGING){
        status_frame.print("[charging]");
      } else {
          status_frame.print(PERCENTAGE); status_frame.print("% "); 
          if(!mute){status_frame.print("["); status_frame.print(float(VOLTAGE)/1000); status_frame.print("V]"); }

      }
    } else {
      if (!CHARGING) {
        status_frame.drawBitmap(L_OFFSET, T_OFFSET, image_battery_bits, 26, 8, FG_COLOR);
        status_frame.fillRect(L_OFFSET+2, T_OFFSET+2, PERCENTAGE/5, 4, FG_COLOR);
        //status_frame.setCursor(L_OFFSET+28, T_OFFSET);
        //status_frame.print(PERCENTAGE); status_frame.print("% "); 
      } else status_frame.drawBitmap(L_OFFSET, T_OFFSET-1, image_battery_charging_bits,  26, 10, FG_COLOR);//status_frame.pushImage(L_OFFSET, T_OFFSET, 26, 10, image_battery_charging, FG_COLOR);
    }

    //middle_string="";

    status_frame.setTextDatum(MC_DATUM);
    //if (status()!="") middle_string = status();
    //status_frame.drawString(middle_string,X_MIDDLE,12);
    status_frame.drawString(status(),X_MIDDLE,12);
    // Uptime


    status_frame.setTextDatum(MR_DATUM);
    String t = String(SLEEPING ? "Zz " : "") 

    + ((boosting) ? "X " : "") 

#ifndef NO_WIFI
    + ((WiFiManager::get().getState()==WIFI_CONNECTED) ? "W " : "") 
    + ((WiFiManager::get().getState()==WIFI_ERROR) ? "W! " : "") 
    + ((WiFiManager::get().getState()==WIFI_STARTING) ? ".. " : "") 
#endif
    
    + ((BLEManager::get().getState()==BLE_CONNECTED) ? "B " : "")
    + ((BLEManager::get().getState()==BLE_IDLE) ? "Bi " : "")
    + ((BLEManager::get().getState()==BLE_ERROR) ? "B! " : "")
    
    + String(uptime()) + "s ";
    //String t =(SLEEPING ? "Zz " : "") + (mute ? "" : String(SOLAR) + "mV ");
    status_frame.drawString(t,320-R_OFFSET,12);
    //status bar
  //} else status_frame.fillRect(0,0,DISPLAY_WIDTH,STATUS_BAR_HEIGHT,BG_COLOR);

    status_frame.pushSprite(0,32);    
  } else if (debug && status()!="") {
    status_frame.fillRect(0,0,320,STATUS_BAR_HEIGHT+6, TFT_TRANSPARENT);
    status_frame.setTextColor(FG_COLOR, TFT_TRANSPARENT, true);
    status_frame.setTextDatum(MC_DATUM);
    //if (status()!="") middle_string = status();
    //status_frame.drawString(middle_string,X_MIDDLE,12);
    status_frame.drawString(status(),X_MIDDLE,12);
    status_frame.pushSprite(0,32, TFT_TRANSPARENT); 
  }
}

bool superkey(KeyEvent event_key) {
  bool block = false;
  static bool hold = false;
  static uint32_t s_hold_timeout = 0;
  static int just_switched_to_key_id = -1;
  static int last_hold_id = -1;
  switch (event_key.type) {
    case KEY_PRESS:
      //Serial.printf("key %d pressed\n", event_key.id);
      if (SUPERKEY[0]) {
        if (debug) Serial.print("keyevent >> switching app");
        switch_app(static_cast<AppID>(SYMBOL_MAP[event_key.id].toInt()));block=true;//,forward_block=true;
        just_switched_to_key_id = event_key.id;
      }
      if(hold) SUPERKEY[last_hold_id] = false;
      //last_hold_id=-1;
      break;
    case KEY_RELEASE_HOLD:
      if (event_key.id>=0 && event_key.id<5) SUPERKEY[event_key.id] = false;
      break;
    case KEY_RELEASE:
      if (just_switched_to_key_id == event_key.id) {
        block=true;
      }
      just_switched_to_key_id = -1;
      break;
    case KEY_HOLD:
      //Serial.printf("key %d held\n", event_key.id);
      //check for superkeys
      if (event_key.id>=0 && event_key.id<5) {
        SUPERKEY[event_key.id] = true;
        //status("SUPERKEY: "+SYMBOL_MAP[event_key.id], 9, POLLING_TIME+POLLING_TIME/2);
        if (SUPERKEY[4]){
          if (BG_COLOR==0xb3c2) {
            //Serial.print("attempt reversiu g color change");
            change_system_color(0xFFFF,0x0000);
            R_OFFSET=0;
          } else {change_system_color(0x0000,0xb3c2);R_OFFSET=10;}
          //BG_COLOR=0xb3c2; FG_COLOR=0x0000;
        } else if (SUPERKEY[3]&&SUPERKEY[2]) {
          static long last_change = -1000;

          if (millis()-last_change>800) {
            AUTO_BRIGHTNESS=!AUTO_BRIGHTNESS;
            status("BRIGHNTESS: " + String(AUTO_BRIGHTNESS ? "AUTOMATIC" : "MANUAL"), 12, 1000);
            last_change = millis();
          }
        } else if(SUPERKEY[3])  {
          static long last_change = -1000;
          if (millis()-last_change>300) {
            BRIGHTNESS+=int(1+BRIGHTNESS*0.2);
            Serial.print("[input-daemon] increasing brightness to "); Serial.println(BRIGHTNESS);
            ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_CH, (uint32_t)BRIGHTNESS, 70, LEDC_FADE_NO_WAIT);
          }
        } else if(SUPERKEY[2])  {
          static long last_change = -1000;
          if (millis()-last_change>300) {
            BRIGHTNESS-=int((1+BRIGHTNESS*0.2));
            Serial.print("[input-daemon] decreasing brightness to "); Serial.println(BRIGHTNESS);
            ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_CH, (uint32_t)BRIGHTNESS, 70, LEDC_FADE_NO_WAIT);
          }
        } else if (SUPERKEY[1]) {
          //Serial.print("hold time: "); Serial.println(event_key.hold_time);
          #define off_timeout 4
          if (event_key.hold_time>off_timeout*1000) {
            DEEP_SLEEP_REQUESTED=true;
            xTaskNotifyGive(power_daemon_handle);
            //Serial.println("entering deep sleep"); //
            //deep_sleep();
            //process_command("ds");
          } else if (event_key.hold_time>1000) {status("ENTER SLEEP ("+String(off_timeout-(event_key.hold_time/1000))+"s)?", 10, 1000);}
        //delete event
        //event_text.delete=true;
        //xQueueSend(text_event_queue, &event_text, 0);
        }
        block=true;
      }
      break;
    case KEY_DOUBLE_PRESS:
      //if (event_key.id>=0 && event_key.id<5) {
      memset(SUPERKEY, 0, sizeof(SUPERKEY));
      SUPERKEY[event_key.id] = true;
      last_hold_id=event_key.id;
      //s_hold_timeout=millis()+S_HOLD_MS;
      hold = true;

        status("S_HOLD: "+event_key.id,8,1500);
      //}
      break;
  }
  return block;
}

void serial_cx_daemon(void *parameters);
void input_daemon(void *parameters);
void power_daemon(void *parameters);
void battery_service(void *parameters);
void brightness_service(void *parameters);
void connectivity_daemon(void *parameters);
void device_status_daemon(void *parameters);
void display_daemon(void *parameters);
void display_daemon_vsync(void *parameters);

void setup() {
  Serial.begin(115200);
  Serial.print("===========");
  esp_reset_reason_t reason = esp_reset_reason();Serial.printf("\nreset reason: %d\n===========\n", reason);
  load_settings();
  if (reason==8 && was_low_battery==true) {Serial.println("waking up from low battery"); low_battery_wakeup_count++;}
  
  if (debug) Serial.println("[WARN] debug mode enabled");

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  gpio_hold_dis(static_cast<gpio_num_t>(PINMAP[0]));
  esp_sleep_enable_timer_wakeup(SLEEPING_TIME_US);

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

// Configure and initialize TWDT with 10s timeout
  esp_task_wdt_deinit();
  const esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 10000,  // 10 seconds (elongated from default ~5s)
      .idle_core_mask = 0,
      .trigger_panic = true
  };
  esp_err_t err = esp_task_wdt_init(&twdt_config);
  if (err != ESP_OK) {
      Serial.printf("TWDT init failed: %s\n", esp_err_to_name(err));
  }

  //pinMode(22, OUTPUT);
  //digitalWrite(22, HIGH); //LED LOW

  display.begin();          // Initialize display with default SPI settings
  display.setRotation(1);   // Adjust rotation as needed (0-3)
  #ifndef WOKWI
    display.invertDisplay(true);  // ST7789 needs inversion, ILI9341 doesn't
  #endif
  display.fillScreen(BG_COLOR);
  #ifndef WOKWI
    display.initDMA();  // Wokwi doesn't support DMA
  #endif
  Serial.println("[DISPLAY] Initialized");
  
  //framebuffer.createSprite(display.width(), VIEWPORT_HEIGHT); //framebuffer.setSwapBytes(true);
  status_frame.createSprite(display.width(), STATUS_BAR_HEIGHT);
  #ifdef D1BIT
    status_frame.setColorDepth(8); 
  #endif
  Serial.println("[DISPLAY] Status frame created");
  
  program_frame.createSprite(display.width(), VIEWPORT_HEIGHT/*-STATUS_BAR_HEIGHT*/); //cause we gotta make it fullscreen now?
  #ifdef D1BIT
    program_frame.setColorDepth(8); 
  #endif
  Serial.println("[DISPLAY] Program frame created");
  Serial.printf("[DISPLAY] Viewport: %dx%d\n", display.width(), VIEWPORT_HEIGHT);
  
  // Configure LEDC timer with RTC clock for light sleep persistence
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_LOW_SPEED_MODE,
    .duty_resolution  = (ledc_timer_bit_t)PWM_RES,
    .timer_num        = (ledc_timer_t)PWM_TIMER,
    .freq_hz          = 1000,
    .clk_cfg          = LEDC_USE_RC_FAST_CLK  // RTC clock continues in light sleep
  };
  ledc_timer_config(&ledc_timer);
  
  // Configure LEDC channel
  ledc_channel_config_t ledc_channel = {
    .gpio_num       = PWM_PIN,
    .speed_mode     = LEDC_LOW_SPEED_MODE,
    .channel        = (ledc_channel_t)PWM_CH,
    .intr_type      = LEDC_INTR_DISABLE,
    .timer_sel      = (ledc_timer_t)PWM_TIMER,
    .duty           = 0,
    .hpoint         = 0,
    .sleep_mode     = LEDC_SLEEP_MODE_KEEP_ALIVE
  };
  ledc_channel_config(&ledc_channel);
  ledc_fade_func_install(0);  
  TimerHandle_t pwm_start_timer = xTimerCreate("PWM delay", pdMS_TO_TICKS(100), pdFALSE, 0, pwm_init);
  xTimerStart(pwm_start_timer, 0);
  //analogWriteResolution(PWM_PIN,9);
  //analogWrite(PWM_PIN, DISPLAY_DEFAULT_BRIGHTNESS);
  Serial.println("PWM initialized");
  
  pinMode(CHARGING_PIN,INPUT_PULLUP); //chrg indicator, low=charging
  pinMode(SOLAR_PIN,INPUT_PULLDOWN); //solar cell 3.1v max
  analogSetPinAttenuation(SOLAR_PIN, ADC_11db);
  pinMode(BATTERY_PIN,INPUT); //battery
  pinMode(0, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(CHARGING_PIN), chargeISR, CHANGE);
  TimerHandle_t boot_start_timer = xTimerCreate("bootloader service start delay", pdMS_TO_TICKS(1000), pdFALSE, 0, boot_init);
  xTimerStart(boot_start_timer, 0);
  Serial.println("GPIO initialized");
  
  matrix_state();
  matrix_reset();
  key_input_init();
  Serial.println("keyboard initialized");

  randomSeed(analogReadMilliVolts(SOLAR_PIN)*analogReadMilliVolts(SOLAR_PIN)); 
  text_event_queue = xQueueCreate(64, sizeof(TextEvent));
  network_command_queue = xQueueCreate(8, sizeof(network_commands));
  frame_command_queue = xQueueCreate(8, sizeof(FrameEvent));
  frame_done_sem = xSemaphoreCreateBinary();

  TimerHandle_t status_update_timer = xTimerCreate("status_update_timer", pdMS_TO_TICKS(250), pdTRUE, 0, [](TimerHandle_t xTimer) {
    FrameEvent evt = {STATUS_UPDATE, false, 0};
    xQueueSend(frame_command_queue, &evt, 0);
  });
  xTimerStart(status_update_timer, 0);

  //vTaskSuspendAll();
  xTaskCreate(input_daemon, "input_daemon", 4608, NULL, 3, &input_daemon_handle);//, SYSTEM_CORE);
  //xTaskCreatePinnedToCore(input_daemon, "input_daemon", 4608, NULL, 3, &input_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(serial_cx_daemon, "serial_cx_daemon", 3072, NULL, 1, &serial_cx_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore((VSYNC_ENABLED ? display_daemon_vsync : display_daemon), "display_daemon", 4096, NULL, 2, &display_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(power_daemon, "power_daemon", 2048, NULL, 2, &power_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(battery_service, "battery_service", 4096, NULL, 2, &battery_service_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(brightness_service, "brightness_service", 4096, NULL, 2, &brightness_service_handle, SYSTEM_CORE); //apparently core 0 doesnt play nice with adc tasks
  xTaskCreatePinnedToCore(connectivity_daemon, "network service", 4096, NULL, 1, &connectivity_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(device_status_daemon, "device_status_daemon", 3072, NULL, 1, NULL, SYSTEM_CORE);
  
  // Initialize BLE on boot
  BLEManager::get().init();
  BLEService::get().init();

  /*xTaskCreatePinnedToCore(APP_CALCULATOR, "app-0", 8192, NULL, 2, &app_0_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_ABACUS, "app-1", 8192, NULL, 2, &app_1_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_LLM, "app-2", 8192, NULL, 2, &app_2_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_DINO, "app-3", 8192, NULL, 1, &app_3_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_TERMINAL, "app-4", 4096, NULL, 1, &app_4_handle, PROGRAM_CORE);*/
  for (size_t i = 0; i < APP_COUNT; i++) {
    const auto &app = APP_REGISTRY[i];
    BaseType_t res = xTaskCreatePinnedToCore(app.function, app.name, app.config->stack_size, NULL, app.config->priority, &app_handles[i], PROGRAM_CORE);
    Serial.write("task: "); Serial.write(app.name); Serial.write(res); Serial.write('\n');
  }
 
  /*#define APP(name,stack,prio) \
    xTaskCreatePinnedToCore( APP_##name, \
                             "app-" #name, \
                             stack, NULL, prio, \
                             &app_handles[_##name], \
                             PROGRAM_CORE );
                            */
  //if (BT) xTaskCreate(BT_HANDLER, "just BLE?", 2048, NULL, 4, &bt_handle);
  //xTaskResumeAll(); 

  Serial.println("tasks initialized");
  Serial.printf("free heap: %u\n",esp_get_free_heap_size());
  Serial.println("======= done =======");
  //Serial.println("init successful");
}
void loop() {}


void input_daemon(void *parameters) {
  //1st 1000ms: provide raw keyboard data but not key events
  while(millis()<1000){
    matrix_scan(KEY_ARR, KEY_ARR_BOOL, &KEY_ARR_COUNT);
    ulTaskNotifyTake(pdTRUE, POLLING_TIME);
  }
  for(;;){
    bool block=false,forward_block=false;
    uint32_t now = millis();
    if (matrix_scan(KEY_ARR, KEY_ARR_BOOL, &KEY_ARR_COUNT)) {
      KEYBOARD_INACTIVE=false; 
      LAST_INPUT_TIME = now;
      //sw ext1 wakeup
      if(SLEEPING) {
        xTaskNotifyGive(power_daemon_handle); xTaskNotifyGive(display_daemon_handle);
      }
    }
    //if (now>1750)
    patch_keys(KEY_ARR_BOOL);
    key_input_update(KEY_ARR_BOOL);

    //what
    if (forward_block) {forward_block=false; continue;}
    KeyEvent event_key;
    while (key_input_pop(event_key)) {
      //handle overlay, hotkeys etc
      TextEvent event_text;
      block = superkey(event_key);

      if (!block) {
      //translate and enlist into text queue
      switch (INPUT_MODE) {
        case CLASSIC_INPUT:
          if(event_key.type==KEY_HOLD) {
              event_text.symbol = SYMBOL_MAP_ALT[event_key.id];
          } else event_text.symbol = SYMBOL_MAP[event_key.id];

          break;
        case T9X:
          if (event_key.type!=KEY_PRESS) continue;
          static int prev_id = -1;
          static uint8_t increment = 0;
          static unsigned long last_same_time = 0;
          static bool warp = false;
          if (prev_id == event_key.id && millis() - last_same_time <= 1000) {
  // Same key pressed again within 500 ms → cycle through letters
            if (increment==0&&!warp) {event_text.symbol = T9_MAP[event_key.id][increment];} else event_text.symbol = String("#")+T9_MAP[event_key.id][increment];//event_text.symbol = String("#") + T9_MAP[event_key.id][increment];
            increment++;
            last_same_time = millis();

            // wrap around if we exceed the available letters for that key
            if (increment > T9_MAP[event_key.id].length()) {
              increment = 0;
              warp=true;
            }

          } else {
            // new key or timeout
            event_text.symbol = T9_MAP[event_key.id][0];
            increment = 1; // next tap will pick the 2nd character
            last_same_time = millis();
            warp=false;
          }
          
          prev_id=event_key.id;
          break;
        case ABX:
          if (event_key.type==KEY_HOLD) {event_text.symbol = ABC_MAP_ALT[event_key.id];
          } else event_text.symbol = ABC_MAP[event_key.id];
          //if (event_key.type=KEY_HOLD) event_text.symbol = ABC_MAP[event_key.id][1];
          break;
        case GSX:
          event_text.symbol = GSX_MAP[event_key.id];
          break;
        case CHIP8:
          event_text.symbol = CHIP8_MAP[event_key.id]; 
          break;
        default:
          event_text.symbol = "";
      }
      //logic
      event_text.id = event_key.id;
      event_text.type = event_key.type;
      if (event_text.symbol != "") xQueueSend(text_event_queue, &event_text, 0);
      if (debug) {Serial.print(event_name[event_text.type]); Serial.print(" [ID:"); Serial.print(event_key.id); Serial.print("] >> "); Serial.println(event_text.symbol);}
      } else {if(debug) Serial.println("blocked");};
    } 

    if (millis()-LAST_INPUT_TIME>=KEYBOARD_TIMEOUT && !KEYBOARD_INACTIVE) {
      KEYBOARD_INACTIVE=true;
      Serial.println(">> keyboard inactive");
    } 
    ulTaskNotifyTake(pdTRUE, POLLING_TIME);
  }
}

void power_daemon (void *parameters) {
  for (;;){
    static bool first_time_sleeping = true;
    static int itself=KEYBOARD_TIMEOUT/4;
    if ((!KEYBOARD_INACTIVE && SLEEPING)||(SLEEPING&&WAKE_LOCK)){
      if (debug) Serial.println("[SLEEP] EXITING");
      //exit from sleep
      SLEEPING=false;
#ifndef NO_WIFI
      if (WIFI) WiFiManager::get().init();
#endif
      //setCpuFrequencyMhz(DEFAULT_CPU_FREQ);
      matrix_reset();
      POLLING_RATE=POLLING_RATE_DEFAULT;
      POLLING_TIME=1000/POLLING_RATE;
      REFRESH_RATE=REFRESH_RATE_DEFAULT;
      REFRESH_TIME = 1000/REFRESH_RATE;
      itself=KEYBOARD_TIMEOUT/4;
    } else if (((KEYBOARD_INACTIVE && !SLEEPING)&&!WAKE_LOCK)||SLEEP_OVERRIDE) {
      if (debug) Serial.println("[SLEEP] ENTERING");

      //go to sleep
      SLEEPING=true;
#ifndef NO_WIFI
      if (WIFI) WiFiManager::get().deinit();
#endif
      //setCpuFrequencyMhz(10);
      matrix_reset();
      POLLING_RATE=SLEEPING_REFRESH_RATE;
      POLLING_TIME=1000/SLEEPING_REFRESH_RATE;
      REFRESH_RATE=SLEEPING_REFRESH_RATE/2;
      REFRESH_TIME = 1000/SLEEPING_REFRESH_RATE/2;
      itself=POLLING_TIME;
      first_time_sleeping = true;
    }

    if (SLEEPING) {
      //esp_start_light
      if (debug) Serial.print("going to sleep.. ");
      if (first_time_sleeping) {
        //statusbar_update();
        update_statusbar();
        first_time_sleeping=false;
      }
      vTaskSuspend(input_daemon_handle);
      matrix_reset();
      set_gpio_wakeups();
      vTaskDelay(30);
      long time_at_sleep = millis();
      esp_light_sleep_start();
      long time_after_sleep = millis();
      esp_sleep_wakeup_cause_t wakeup_reason;      
      wakeup_reason = esp_sleep_get_wakeup_cause();
      if (debug) {
        switch (wakeup_reason) {
          case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("RTC_IO"); break;
          case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("RTC_CNTL (MATRIX_IO)"); print_ext1_wake_pins(); break;
          case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("TIMER"); break;
          case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("touch?"); break;
          case ESP_SLEEP_WAKEUP_ULP:      Serial.println("ULP"); break;
          default:                        Serial.printf("%d\n", wakeup_reason); break;
        };
        Serial.print(time_after_sleep-time_at_sleep); Serial.println("ms");
      }
      matrix_reset();
      
      // Reconfigure LEDC channel after waking from light sleep
      /*if (debug) Serial.println("[SLEEP] Reconfiguring LEDC after wake...");
      
      //pinMode(PWM_PIN, OUTPUT);
      if (debug) Serial.println("[SLEEP] GPIO mode set to OUTPUT");
      
      ledc_channel_config_t ledc_channel = {
        .gpio_num       = PWM_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (ledc_channel_t)PWM_CH,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = (ledc_timer_t)PWM_TIMER,
        .duty           = BRIGHTNESS,
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_KEEP_ALIVE
      };
      if (debug) Serial.println("[SLEEP] Calling ledc_channel_config...");
      esp_err_t err = ledc_channel_config(&ledc_channel);
      if (debug) {
        Serial.print("[SLEEP] ledc_channel_config result: ");
        Serial.println(esp_err_to_name(err));
      }
      
      if (debug) Serial.println("[SLEEP] LEDC reconfiguration complete");*/
      vTaskResume(input_daemon_handle);

      if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        KEYBOARD_INACTIVE=false;
        LAST_INPUT_TIME=millis();
        itself=KEYBOARD_TIMEOUT/8;
        xTaskNotifyGive(input_daemon_handle);
      }

    } 
    if (DEEP_SLEEP_REQUESTED) {
      deep_sleep();
    }

    ulTaskNotifyTake(pdTRUE, itself);
  }
}

void battery_service(void *parameters) {
  for(;;){
    static int last_voltage;
    //DAEMON_STRING="";
    //CHARGING=!digitalRead(CHARGING_PIN);
    VOLTAGE=analogReadMilliVolts(BATTERY_PIN)*2;
    if(VOLTAGE<10)VOLTAGE=last_voltage;
    PERCENTAGE=mV2PERCENTAGE(VOLTAGE/10);
    //DISWIFI_CONNECTED=((last_voltage<4195 || VOLTAGE<4195)&&!CHARGING);
    if (VOLTAGE<3300||was_low_battery){
      if (CHARGING) {was_low_battery=false;continue;}
      //verify
      int sum=0;
      for (size_t i = 0; i < 5; i++){sum+=analogReadMilliVolts(BATTERY_PIN)*2; }
      if(sum/5 < 3300||was_low_battery) {
        /*if(KEY_ARR_BOOL[0]==true) {
          PROG_STRING="POWER OVERRIDE";
          continue;
        }*/
        was_low_battery=true;
        //override pins
        static unsigned long last_flash = -99999;
        if (millis() - last_flash >= 30000){   
          //exit sleep
          if(SLEEPING) {
            WAKE_LOCK=true;
            LAST_INPUT_TIME=millis();
            //xTaskNotify()
          }
          flash_string("LOW BATTERY", 3, X_MIDDLE, 34+(108/2), false);
          POWER_OVERRIDE=KEY_ARR_BOOL[0];
          last_flash=millis();
        }
        if(POWER_OVERRIDE) {
          //DAEMON_STRING="POWER OVERRIDE";
          status("POWER OVERRIDE", 10, 30000);
          vTaskResume(display_daemon_handle);
          vTaskDelay(2000);
        } else {
          deep_sleep(true, (low_battery_wakeup_count < 2));
        }
      }
    }
    last_voltage=VOLTAGE;
    vTaskDelay(2500);

    //240mhz no radio 200/4085brg, 100fps clock, wakelock
    /*static uint32_t last=-200000;
    if (millis() - last >=120000) { //100s?
      last=millis();
      //Serial.print("BATTERY VOLTAGE: "); Serial.print(VOLTAGE); Serial.print("mV   UPTIME: "); Serial.print(millis()/1000); Serial.println("s");
      carrier="mV: " + String(VOLTAGE) + "      uptime: " + uptime() + "s\n";
      carrier3 += carrier; 
      status(carrier, 30, 130000);
      //carrier1
    }
    *///TEST////////////////////////////////////////////////////////////////////////////////////////////////////
    //10:41 charging
  }
}

void device_status_daemon(void *parameters) {
  // Reports battery status and terminal output to BLE companion app
  for (;;) {
    if (BLEManager::get().connected()) {
      // Send device status every 5 seconds
      static unsigned long last_status_send = 0;
      if (millis() - last_status_send >= 5000) {
        String status_str = CHARGING ? "charging" : "ready";
        BLEManager::get().sendStatus(status_str, PERCENTAGE, "");
        last_status_send = millis();
        
        if (debug) Serial.printf("[Status] Sent to BLE: %s, %d%%\n", status_str.c_str(), PERCENTAGE);
      }
      
      // Process serial output and send to terminal
      static String serial_buffer = "";
      while (Serial.available()) {
        char c = Serial.read();
        serial_buffer += c;
        
        if (c == '\n' || serial_buffer.length() >= 128) {
          BLEManager::get().sendTerminal(serial_buffer);
          serial_buffer = "";
        }
      }
    }
    
    vTaskDelay(500);  // Check every 500ms
  }
}


void connectivity_daemon(void *parameters) {
#ifndef NO_WIFI
  Ticker wifi_timer;
#endif
  Ticker ble_timer;
  for (;;) {
    network_commands event;
    while (xQueueReceive(network_command_queue, &event, 0) == pdTRUE) {
      switch (event) {
#ifndef NO_WIFI
        case wifi_init:
          // Don't init WiFi if BLE is connected
          if (BLEManager::get().connected()) {
            Serial.println("[daemon] BLE connected, skipping WiFi init");
            break;
          }
          wifi_timer.detach(); 
          WiFiManager::get().init();
          //Serial.println("[Daemon] WiFi Init - Timeout Cancelled");
          break;
        case wifi_deinit:
          wifi_timer.detach(); // Also clear if manual deinit is called
          WiFiManager::get().deinit();
          break;
        case wifi_scan:
          wifi_timer.detach(); 
          WiFiManager::get().scan();
          break;
        case wifi_inactive:
          Serial.println("[daemon] wifi inactive: starting timeout...");
          wifi_timer.once_ms(WIFI_TIMEOUT_MS, []() {
              Serial.println("[timer] deinitializing wifi...");
              WiFiManager::get().deinit();
          });
          break;
#endif
        case ble_init:
          ble_timer.detach();
          BLEManager::get().init();
          break;
        case ble_deinit:
          ble_timer.detach();
          BLEManager::get().deinit();
          break;
        case ble_inactive:
          Serial.println("[daemon] BLE inactive: starting timeout...");
          ble_timer.once_ms(WIFI_TIMEOUT_MS, []() {
              Serial.println("[timer] deinitializing BLE...");
              BLEManager::get().deinit();
          });
          break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void serial_cx_daemon (void *parameters) {
  for(;;){
    static String s_buffer="";
    while (Serial.available()) {
      char s_data = Serial.read();
      //Serial.print(s_data);
      if ((s_data == '\n' || s_data == '\r' )&& s_buffer.length() > 0) {
        Serial.println(process_command(s_buffer));
        s_buffer="";
      } else {
        s_buffer=s_buffer+s_data;
      }
    }
    vTaskDelay(200);
  }
}

void brightness_service(void *parameters) {
    const float ema_alpha = 0.09;     // smoothing strength; 0.15f
    float smoothed_mv = 0;

    // Calibration measured earlier: 9-bit
    // ~300 mV  → brightness 1
    // ~1500 mV → brightness 5
    // ~3000 mV → brightness 20
    const int mv_min = 200;
    const int mv_mid = 1500;
    const int mv_max = 3000; 

    const int pwm_min = 6; //8
    const int pwm_mid = 45; //50
    const int pwm_max = 120;//160;

    #define written_for  512; //
    #define current_bit_depth 4096

    unsigned long last_update = 0;
    vTaskDelay(500);

    for (;;) {
        if (!AUTO_BRIGHTNESS) {vTaskDelay(2000); continue;}
        //unsigned long now = millis();
        //if (now - last_update >= 50) {   // update every 50 ms (20 Hz)
          //  last_update = now;
            
            int a = analogReadMilliVolts(SOLAR_PIN);
            int b = analogReadMilliVolts(SOLAR_PIN);
            int c = analogReadMilliVolts(SOLAR_PIN);
                 //if (debug) Serial.print("raw: a: "); if (debug) {Serial.print(a); Serial.print("mv  b:");}; if (debug) {Serial.print(b); Serial.print("mv  c:");} if (debug) Serial.println(c);
            SOLAR = (a+b+c) / 3;
            //vTaskDelay(1000);continue;
            if (SOLAR<200) SOLAR=200;
            // EMA smoothing
            smoothed_mv = smoothed_mv == 0 ? SOLAR : (smoothed_mv * (1 - ema_alpha)) + (SOLAR * ema_alpha);
            // Clamp
            float mv_c = constrain(smoothed_mv, mv_min, mv_max);
            // Map to your curve
            float brightness;
            if (mv_c < mv_mid)
                brightness = map(mv_c, mv_min, mv_mid, pwm_min, pwm_mid);
            else
                brightness = map(mv_c, mv_mid, mv_max, pwm_mid, pwm_max);
            // Optional gamma correction to improve low-light resolution
            float gamma = 1.4;
            brightness = pow(brightness / pwm_max, gamma) * pwm_max;
            if (brightness<3) brightness=3;
            BRIGHTNESS=brightness;
            // Write to LEDC (0–4095)
            ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)PWM_CH, (uint32_t)brightness, 350, LEDC_FADE_NO_WAIT);
        //}

        vTaskDelay(350);   //target: 4-2Hz >> 2.86Hz // feed watchdog, cheap
    }
}
  //display.println("CITIZEN SLD-200NR                          helen v0.1"); 

void display_daemon(void *parameters) {
  for(;;){
    static float frames=0;
    static int frame_time_last=millis();
    static String middle_string;

    static uint32_t last_status_update = -250;
    frames++;
    //status bar
    if ((millis()-last_status_update)>250) {
      if(!fullscreen) {
        status_frame.setTextFont(1);
        status_frame.setTextSize(1);
        status_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        status_frame.fillRect(L_OFFSET,6,320,STATUS_BAR_HEIGHT, BG_COLOR);
        
        // Battery voltage
        status_frame.setCursor(L_OFFSET, T_OFFSET);
        //status_frame.print("test");
        if(CHARGING){
          status_frame.print("[charging]");
        } else {
          status_frame.print(PERCENTAGE); status_frame.print("% "); 
          if(!mute){status_frame.print("["); status_frame.print(float(VOLTAGE)/1000); status_frame.print("V]"); }
        }
        
        middle_string="";

        if (status()!="") middle_string = status();
        status_frame.setTextDatum(MC_DATUM);
        status_frame.drawString(middle_string,X_MIDDLE,12);
        // Uptime


        status_frame.setTextDatum(MR_DATUM);
        String t = String(SLEEPING ? "Zz " : "") 

        + ((boosting) ? "X " : "") 

#ifndef NO_WIFI
        + ((WiFiManager::get().getState()==WIFI_CONNECTED) ? "W " : "") 
        + ((WiFiManager::get().getState()==WIFI_ERROR) ? "W! " : "") 
        + ((WiFiManager::get().getState()==WIFI_STARTING) ? ".. " : "") 
        + ((WiFiManager::get().getState()==WIFI_UNKNOWN) ? "? " : "") 
#endif
        
        + ((BLEManager::get().getState()==BLE_CONNECTED) ? "B " : "")

        + ((BLEManager::get().getState()==BLE_ERROR) ? "B! " : "")
        
        + String(uptime()) + "s ";
        //String t =(SLEEPING ? "Zz " : "") + (mute ? "" : String(SOLAR) + "mV ");
        status_frame.drawString(t,320-R_OFFSET,12);
        //status bar
      } else status_frame.fillRect(0,0,DISPLAY_WIDTH,STATUS_BAR_HEIGHT,BG_COLOR);

      status_frame.pushSprite(0,32);    
    }   
    //framebuffer.pushImage(L_OFFSET, STATUS_BAR_HEIGHT, program_frame.width(), program_frame.height(),(uint16_t*)program_frame.getPointer()); // raw pixel copy
    //framebuffer.pushSprite(0,32); //flush

    program_frame.pushSprite(L_OFFSET,32+(fullscreen ? 0 : STATUS_BAR_HEIGHT));

    if(millis()-frame_time_last>=1000&&told_to_do_so) {
      FPS=frames*1000/(millis()-frame_time_last);
      //if (debug) {Serial.print("FPS: "); Serial.println(FPS);}
      frames=0;
      frame_time_last=millis();
      //if (told_to_do_so) {
        status("FPS: "+String(FPS, 2)+" F_h: "+getFreeHeap()/1000+"KB", 1, REFRESH_TIME+1000);
      //}
    }
    ulTaskNotifyTake(pdTRUE, REFRESH_TIME);
  }
}

void display_daemon_vsync(void *parameters) {
  xSemaphoreGive(frame_done_sem);
  Serial.println("[WARN] using vsync display daemon");
  for(;;){
    static float frames=0;
    static int frame_time_last=millis();
    static String middle_string;

    FrameEvent evt;
    if (xQueueReceive(frame_command_queue, &evt, portMAX_DELAY) == pdTRUE) {
      frames++;
      if (evt.type == STATUS_UPDATE) {
        
        //status bar
        render_status();

        if(millis()-frame_time_last>=1000&&told_to_do_so) {
          FPS=frames*1000/(millis()-frame_time_last);
          //if (debug) {Serial.print("FPS: "); Serial.println(FPS);}
          frames=0;
          frame_time_last=millis();
          //if (told_to_do_so) {
            status("FPS: "+String(FPS, 2)+" F_h: "+getFreeHeap()/1000+"KB", 1, REFRESH_TIME+1000);
          //}
        }
      } else if (evt.type == FRAME_READY) {
        // Draw program frame
        program_frame.pushSprite(L_OFFSET,32+(fullscreen ? 0 : STATUS_BAR_HEIGHT));

        if (WAIT_FOR_DMA) {
          display.dmaWait();
        }
        // Send frame done signal
        xSemaphoreGive(frame_done_sem);
      } else if (evt.type == CLEAR_DISPLAY) {
        display.fillScreen(BG_COLOR);
      }
    }
  }
}



