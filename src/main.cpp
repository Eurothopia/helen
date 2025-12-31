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
#include "Preferences.h"
#include "WiFi.h"
#include "esp_wifi.h"


#include <definitions.h>
#include <global.h>
#include <excludable.h>

#include <ns/battery_ns.h>
#include <ns/matrix_ns.h> //keymaps
#include <ns/secrets_ns.h>
#include <matrix_core.h>
#include <key_input.h>
#include <serial.h>
#include <daemons/networkd.h>

#include <apps/_xox0.h>

void bootISR() {
  status((digitalRead(0) ? "BOOTLOADER ON STANDBY" : "-1"), 20, 15000);
  Serial.print(status());
}
void pwm_init(TimerHandle_t ass) {ledcWrite(PWM_PIN,DISPLAY_DEFAULT_BRIGHTNESS);}
void boot_init(TimerHandle_t ass) {attachInterrupt(digitalPinToInterrupt(0), bootISR, CHANGE);}

void set_gpio_wakeups() {
  Serial.print("setting up gpio wakeup..");
      //OUT_HIGH: 9, 1, 2, 8, //////4!!INPUT_ONLY  
    //INPUT_PD: 5, 10, 6, 3, 7,   ON:0
  const int OUT_HIGH[] = {PINMAP[1],PINMAP[2],PINMAP[8],PINMAP[9]};    // pins you can drive high
  const int INPUT_WAKE[] = {PINMAP[0], PINMAP[5],PINMAP[6],PINMAP[7],PINMAP[10]};  // input-only or inputs for wakeup
  const int INPUT_ONLY_WAKE[] = {PINMAP[3]};

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
      wake_mask |= 1ULL << INPUT_WAKE[i];//esp_sleep_enable_ext1_wakeup(1ULL << INPUT_ONLY_WAKE[i], ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  //Serial.print("setting up gpio input_only..");

}

bool superkey(KeyEvent event_key) {
  bool block = false;
  static bool hold = false;
  static uint32_t s_hold_timeout = 0;
  switch (event_key.type) {
        case KEY_PRESS:
          //Serial.printf("key %d pressed\n", event_key.id);
          if (SUPERKEY==0) {
            if (debug) Serial.print("keyevent >> switching app");
            switch_app(static_cast<AppID>(SYMBOL_MAP[event_key.id].toInt()));block=true;//,forward_block=true;
          }
          if(hold) SUPERKEY=-1;
          break;
        case KEY_RELEASE_HOLD:
          if (SUPERKEY!=-1 && event_key.id == SUPERKEY) SUPERKEY=-1;
          break;
        case KEY_RELEASE:
          break;
        case KEY_HOLD:
          //Serial.printf("key %d held\n", event_key.id);
          //check for superkeys
          if (event_key.id>=0 && event_key.id<5) {
            SUPERKEY=event_key.id;
            //status("SUPERKEY: "+SYMBOL_MAP[event_key.id], 9, POLLING_TIME+POLLING_TIME/2);
            if (SUPERKEY==4){
              if (BG_COLOR==0xb3c2) {
                //Serial.print("attempt reversiu g color change");
                change_system_color(0xFFFF,0x0000);
                R_OFFSET=0;
              } else {change_system_color(0x0000,0xb3c2);R_OFFSET=10;}
              //BG_COLOR=0xb3c2; FG_COLOR=0x0000;
            } else if(SUPERKEY==3){
              BRIGHTNESS+=4;
              ledcWrite(PWM_PIN, BRIGHTNESS);
            }else if(SUPERKEY==2){
              BRIGHTNESS-=4;
              ledcWrite(PWM_PIN, BRIGHTNESS);
            } else if (SUPERKEY==1) {
              //Serial.print("hold time: "); Serial.println(event_key.hold_time);
              #define off_timeout 7
              if (event_key.hold_time>off_timeout*1000) {
                DEEP_SLEEP_REQUESTED=true;
                xTaskNotifyGive(power_daemon_handle);
                //Serial.println("entering deep sleep"); //
                //deep_sleep();
                //process_command("ds");
              }else if (event_key.hold_time>3000) {status("ENTER SLEEP ("+String(off_timeout-(event_key.hold_time/1000))+"s)?", 10, 1000);}
            }
            //delete event
            //event_text.delete=true;
            //xQueueSend(text_event_queue, &event_text, 0);
            block=true;
          }
          break;
        case KEY_DOUBLE_PRESS:
          //if (event_key.id>=0 && event_key.id<5) {
            SUPERKEY=event_key.id;
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
void display_daemon(void *parameters);
void power_daemon(void *parameters);
void battery_service(void *parameters);
void brightness_service(void *parameters);
void network_service(void *parameters);

void setup() {
  Serial.begin(115200);
  Serial.print("===========");
  esp_reset_reason_t reason = esp_reset_reason();Serial.printf("\nreset reason: %d\n===========\n", reason);
  if (reason==8 && was_low_battery==true) {Serial.println("waking up from low battery"); low_battery_wakeup_count++;}
  
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  gpio_hold_dis(static_cast<gpio_num_t>(PINMAP[0]));
  esp_sleep_enable_timer_wakeup(SLEEPING_TIME);
    
  //pinMode(22, OUTPUT);
  //digitalWrite(22, HIGH); //LED LOW

  display.begin();          // Initialize display with default SPI settings
  display.setRotation(1);   // Adjust rotation as needed (0-3)
  display.invertDisplay(true);
  display.fillScreen(BG_COLOR);
  display.initDMA();
  framebuffer.createSprite(display.width(), VIEWPORT_HEIGHT); //framebuffer.setSwapBytes(true);
  //program_frame.setColorDepth(1); 
  program_frame.createSprite(display.width(), VIEWPORT_HEIGHT-STATUS_BAR_HEIGHT);
  Serial.println("display(lib) initialized");
  
  pinMode(PWM_PIN, OUTPUT);
  ledcAttach(PWM_PIN, 1000, PWM_RES);
  ledcWrite(PWM_PIN, 0);
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
  TimerHandle_t boot_start_timer = xTimerCreate("bootloader service start delay", pdMS_TO_TICKS(1000), pdFALSE, 0, boot_init);
  xTimerStart(boot_start_timer, 0);
  Serial.println("GPIO initialized");

  matrix_reset();
  key_input_init();
  Serial.println("keyboard initialized");

  randomSeed(analogReadMilliVolts(SOLAR_PIN)*analogReadMilliVolts(SOLAR_PIN)); 
  text_event_queue = xQueueCreate(32, sizeof(TextEvent));
  wifi_command_queue = xQueueCreate(16, sizeof(commands));

  //vTaskSuspendAll();
  xTaskCreatePinnedToCore(input_daemon, "input_daemon", 4096, NULL, 3, &input_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(serial_cx_daemon, "serial_cx_daemon", 2048, NULL, 1, &serial_cx_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(display_daemon, "display_daemon", 4096, NULL, 3, &display_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(power_daemon, "power_daemon", 2048, NULL, 2, &power_daemon_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(battery_service, "battery_service", 4096, NULL, 2, &battery_service_handle, SYSTEM_CORE);
  xTaskCreatePinnedToCore(brightness_service, "brightness_service", 4096, NULL, 2, &brightness_service_handle, SYSTEM_CORE); //apparently core 0 doesnt play nice with adc tasks

  /*xTaskCreatePinnedToCore(APP_CALCULATOR, "app-0", 8192, NULL, 2, &app_0_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_ABACUS, "app-1", 8192, NULL, 2, &app_1_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_LLM, "app-2", 8192, NULL, 2, &app_2_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_DINO, "app-3", 8192, NULL, 1, &app_3_handle, PROGRAM_CORE);
  xTaskCreatePinnedToCore(APP_TERMINAL, "app-4", 4096, NULL, 1, &app_4_handle, PROGRAM_CORE);*/
  for (size_t i = 0; i < APP_COUNT; i++) {
    BaseType_t res = xTaskCreatePinnedToCore(applist_init[i].function, applist_init[i].name, applist_init[i].stack_size, NULL, applist_init[i].priority, &app_handles[i], PROGRAM_CORE);
    //Serial.print("task: "); Serial.println(applist_init[i].name);
    Serial.write("task: "); Serial.write(applist_init[i].name); Serial.write(res); Serial.write('\n');
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
  Serial.printf("free heap: %u\n", esp_get_free_heap_size());
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
    static int itself=KEYBOARD_TIMEOUT/4;
    if ((!KEYBOARD_INACTIVE && SLEEPING)||(SLEEPING&&WAKE_LOCK)){
      //exit from sleep
      SLEEPING=false;
      if (WIFI) WiFiManager::get().init();
      setCpuFrequencyMhz(DEFAULT_CPU_FREQ);
      matrix_reset();
      POLLING_RATE=POLLING_RATE_DEFAULT;
      POLLING_TIME=1000/POLLING_RATE;
      REFRESH_RATE=REFRESH_RATE_DEFAULT;
      REFRESH_TIME = 1000/REFRESH_RATE;
      itself=KEYBOARD_TIMEOUT/4;
    } else if (((KEYBOARD_INACTIVE && !SLEEPING)&&!WAKE_LOCK)||SLEEP_OVERRIDE) {
      //go to sleep
      SLEEPING=true;
      if (WIFI) WiFiManager::get().deinit();

      setCpuFrequencyMhz(10);
      matrix_reset();
      POLLING_RATE=SLEEPING_RATE;
      POLLING_TIME=SLEEPING_TIME;
      REFRESH_RATE=SLEEPING_RATE/2;
      REFRESH_TIME = SLEEPING_TIME/2;
      itself=POLLING_TIME;
    }

    if (SLEEPING) {
      //esp_start_light
      Serial.print("going to sleep.. ");
      //set_gpio_wakeups();
      esp_light_sleep_start();
      //matrix_reset();
      esp_sleep_wakeup_cause_t wakeup_reason;
      wakeup_reason = esp_sleep_get_wakeup_cause();

      switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP:      Serial.println("ULP"); break;
        default:                        Serial.printf("%d\n", wakeup_reason); break;
      }
      if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        xTaskNotifyGive(input_daemon_handle);
      }

    } 
    if (DEEP_SLEEP_REQUESTED) {
      deep_sleep();
    }

    ulTaskNotifyTake(pdTRUE, itself);
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

void battery_service(void *parameters) {
  for(;;){
    static int last_voltage;
    //DAEMON_STRING="";
    CHARGING=!digitalRead(CHARGING_PIN);
    VOLTAGE=analogReadMilliVolts(BATTERY_PIN)*2;
    if(VOLTAGE<10)VOLTAGE=last_voltage;
    PERCENTAGE=mV2PERCENTAGE(VOLTAGE/10);
    //DISCONNECTED=((last_voltage<4195 || VOLTAGE<4195)&&!CHARGING);
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

/*void brightness_service(void *parameters) {
  for (;;) {
    SOLAR = analogReadMilliVolts(SOLAR_PIN);
    //300 = 1    1500 = 5 3100=

    //ledcWrite(PWM_PIN, SOLAR);
    vTaskDelay(1000);
  }
}*/

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

    const int pwm_min = 8;
    const int pwm_mid = 50;
    const int pwm_max = 160;//20;

    #define written_for  512; //
    #define current_bit_depth 4096

    unsigned long last_update = 0;
    vTaskDelay(500);

    for (;;) {
        if (!AUTO_BRIGHTNESS) {vTaskDelay(2000); continue;}
        //unsigned long now = millis();
        //if (now - last_update >= 50) {   // update every 50 ms (20 Hz)
          //  last_update = now;
            if (debug) Serial.print("raw: a: ");
            int a = analogReadMilliVolts(SOLAR_PIN);
            if (debug) {Serial.print(a); Serial.print("mv  b:");}
            int b = analogReadMilliVolts(SOLAR_PIN);
            if (debug) {Serial.print(b); Serial.print("mv  c:");}
            int c = analogReadMilliVolts(SOLAR_PIN);
            if (debug) Serial.println(c);
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
            // Write to LEDC (0–511)
            //ledcWrite(PWM_PIN, (uint32_t)brightness);
            ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)brightness, 350, LEDC_FADE_NO_WAIT);
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
      frames++;
    //status bar
    if(!fullscreen) {
      framebuffer.setTextFont(1);
      framebuffer.setTextSize(1);
      framebuffer.setTextColor(FG_COLOR, BG_COLOR, true);
      framebuffer.fillRect(L_OFFSET,6,320,STATUS_BAR_HEIGHT, BG_COLOR);
      
      // Battery voltage
      framebuffer.setCursor(L_OFFSET, T_OFFSET);
      //framebuffer.print("test");
      if(CHARGING){
        framebuffer.print("[charging]");
      } else {
        framebuffer.print(PERCENTAGE); framebuffer.print("% "); 
        if(!mute){framebuffer.print("["); framebuffer.print(float(VOLTAGE)/1000); framebuffer.print("V]"); }
      }
      
      middle_string="";
      //if(PROG_STRING!="") middle_string=PROG_STRING;

      /*if (SLEEPING) {
        middle_string="SLEEPING";
        //framebuffer.drawString("SLEEPING",D_MIDDLE,12);
      }*/

      if (status()!="") middle_string = status();
      framebuffer.setTextDatum(MC_DATUM);
      framebuffer.drawString(middle_string,X_MIDDLE,12);
      // Uptime


      framebuffer.setTextDatum(MR_DATUM);
      String t = String(SLEEPING ? "Zz " : "") 
      + ((WiFiManager::get().getState()==CONNECTED) ? "W " : "") 
      + ((WiFiManager::get().getState()==ERROR) ? "W! " : "") 
      + ((WiFiManager::get().getState()==STARTING) ? ".. " : "") 
      + String(uptime()) + "s ";
      //String t =(SLEEPING ? "Zz " : "") + (mute ? "" : String(SOLAR) + "mV ");
      framebuffer.drawString(t,320-R_OFFSET,12);
      //status bar
    } else framebuffer.fillRect(0,0,DISPLAY_WIDTH,STATUS_BAR_HEIGHT,BG_COLOR);
    
    framebuffer.pushImage(L_OFFSET, STATUS_BAR_HEIGHT, program_frame.width(), program_frame.height(),(uint16_t*)program_frame.getPointer()); // raw pixel copy
    framebuffer.pushSprite(0,32); //flush

    if(millis()-frame_time_last>=1000&&told_to_do_so) {
      FPS=frames*1000/(millis()-frame_time_last);
      //if (debug) {Serial.print("FPS: "); Serial.println(FPS);}
      frames=0;
      frame_time_last=millis();
      //if (told_to_do_so) {
        status("FPS: "+String(FPS, 2), 1, REFRESH_TIME+1000);
      //}
    }
    ulTaskNotifyTake(pdTRUE, REFRESH_TIME);
  }
}


