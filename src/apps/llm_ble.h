#pragma once
// LLM app with BLE proxy support - llm1.h
// This version uses BLE connection to companion app instead of direct HTTP

#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "Ticker.h"

#include "app_config.h"
#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>
#include <drivers/ble_service.h>

// App configuration - uses BLE, not WiFi
inline constexpr AppConfig appcfg_LLM_BLE = make_app_config([](AppConfig &c) {
  c.fullscreen = false;
  c.refresh_ms = 80;
  c.vsync = false;
  c.stack_size = 8192;
  c.priority = 2;
  c.needs_network = false;  // Uses BLE, not WiFi
});

void APP_LLM_BLE(void *parameters) {
  static String input = "";
  static String streamed_response = "";
  static bool reset_display = false;
  static int scroll_offset = 0;
  static unsigned long last_update = 0;
  static int sym_id_continuous=-1;
  static bool viewing_response = false;
  static bool waiting_for_response = false;

  static Ticker marker_reset;
  
  for (;;) {
    if (FOCUSED_APP == _LLM) {
      bool display_changed = false;

      if (just_switched_apps || color_change) {
        just_switched_apps = false;
        color_change = false;
        
        // Request BLE connection if not connected
        if (!BLEService::get().hasConnection()) {
          network_commands cmd = ble_init;
          xQueueSend(network_command_queue, &cmd, 0);
        }
        
        if (INPUT_MODE != T9X) INPUT_MODE = ABX;
        program_frame.setTextFont(1);
        program_frame.resetViewport();
        program_frame.setTextSize(1);
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        program_frame.fillSprite(BG_COLOR);
        program_frame.setCursor(0, 32);
        program_frame.setTextSize(1);
        reset_display = true;
        display_changed = true;
      }

      // Draw keyboard
      if (INPUT_MODE == ABX) {
        program_frame.resetViewport();
        draw_keyboard(sym_id_continuous);
      } else {
        program_frame.drawString(" ---   abc  def", 308, 10);
        program_frame.drawString("ghi  jkl  mno", 308, 22);
        program_frame.drawString("pqrs  tuv wxyz", 308, 34);
        program_frame.drawString("      __       ", 308, 46);
      }

      // Process input events
      TextEvent event;
      int scroll_direction = 0;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        
        if (event.type==KEY_PRESS) {
          sym_id_continuous=event.id;
        } else if (event.type==KEY_RELEASE || event.type==KEY_RELEASE_HOLD) {
          sym_id_continuous=-1;
        }
        
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD) && INPUT_MODE != T9X) continue;
        String sym = event.symbol;

        if (sym == "#") {
          if (!waiting_for_response) {
            input.remove(input.length() - 1);
            reset_display = true;
            display_changed = true;
          }
        } else if (sym.startsWith("#")) {
          if (!waiting_for_response) {
            input.remove(input.length() - 1);
            input += sym[1];
            reset_display = true;
            display_changed = true;
          }
        } else if (sym == "CLEAR") {
          scroll_direction = 0;
          if (!waiting_for_response) {
            input = "";
            streamed_response = "";
            scroll_offset = 0;
            reset_display = true;
            display_changed = true;
          }
        } else if (sym == "ENTER") {
          scroll_direction = 0;
          
          if (viewing_response) {
            // Exit response view
            viewing_response = false;
            waiting_for_response = false;
            reset_display = true;
          } else {
            // Send message via BLE
            if (!waiting_for_response && input.length() > 0) {
              if (!BLEService::get().hasConnection()) {
                status("NO BLE CONNECTION", 10, 2000);
              } else {
                draw_keyboard();
                program_frame.fillRect(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
                program_frame.setCursor(0, 8);
                program_frame.print("Sending via BLE...");
                frame_ready();
                
                // Send user message to app via BLE
                BLEService::get().sendUserMessage(input);
                
                streamed_response = "";
                scroll_offset = 0;
                waiting_for_response = true;
                viewing_response = true;
                input = "";
                display_changed = true;
              }
            }
          }
        } else if (sym == "LEFT") {
          scroll_direction = -1;
        } else if (sym == "RIGHT") {
          scroll_direction = 1;
        } else {
          scroll_direction = 0;
          if (!waiting_for_response) {
            input += sym;
            reset_display = true;
            display_changed = true;
          }
        }
      }

      // Smooth scrolling
      if (scroll_direction != 0 && viewing_response) {
        scroll_offset += scroll_direction;
        if (scroll_offset < 0) scroll_offset = 0;
        display_changed = true;
      }

      // Check for incoming messages from BLE
      String ble_message;
      if (BLEService::get().getNextMessage(ble_message)) {
        streamed_response = ble_message;
        waiting_for_response = false;
        display_changed = true;
        if (debug) Serial.printf("[LLM BLE] Received response: %s\n", ble_message.c_str());
      }

      // Update display
      program_frame.setViewport(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT);
      if (reset_display) {
        reset_display = false;
        program_frame.fillRect(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
      }
      program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
      program_frame.setCursor(0, 8);
      program_frame.setTextSize(1);

      if (viewing_response) {
        reset_display = true;
        
        // Show connection status
        if (!BLEService::get().hasConnection()) {
          program_frame.print("[No BLE connection]\n");
        } else if (waiting_for_response) {
          program_frame.print("[Waiting for response...]\n");
        }
        
        // Display response with scrolling
        int max_lines = (VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT - 8) / 8;
        String display_text = streamed_response;
        int start_pos = 0;
        for (int i = 0; i < scroll_offset && start_pos < display_text.length(); i++) {
          start_pos = display_text.indexOf('\n', start_pos) + 1;
          if (start_pos == 0) break;
        }
        String visible_text = display_text.substring(start_pos);
        program_frame.print(visible_text);
      } else {
        // Show input with cursor
        program_frame.print(input);
        if((millis() % (CURSOR_BLINK_TIME * 2)) > CURSOR_BLINK_TIME) program_frame.print("_");
        else program_frame.print(" ");
        
        // Show BLE status
        program_frame.setCursor(0, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT - 10);
        if (BLEService::get().hasConnection()) {
          program_frame.print("[BLE OK]");
        } else {
          program_frame.print("[BLE disconnected]");
        }
      }
      
      program_frame.resetViewport();
      frame_ready();
      last_update = millis();

      frame_ready();
      if (VSYNC_ENABLED) xSemaphoreTake(frame_done_sem, portMAX_DELAY);

      vTaskDelay(REFRESH_TIME*2);
    } else {
      // Not focused - check less frequently
      ulTaskNotifyTake(pdTRUE, REFRESH_TIME*10);
    }
  }
}
