#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "tinyexpr.h"

#include "app_config.h"

#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>

#include <_lib/input_field.h>

// App configuration
inline constexpr AppConfig appcfg_CALCULATOR = make_app_config([](AppConfig &c) {
  c.fullscreen = false;   // keep sidebar visible
  c.refresh_ms = 40;
  c.vsync = false;
});

//00
void APP_CALCULATOR(void *parameters) {
  String input = "";
  String output = "";
  bool reset_next = false;
  bool render_update = false;

  int cursor = 0;       // insertion index (0..input.length)
  int viewOffset = 0;   // scroll offset from right
  bool show_cursor = true;
  const int viewWidth = 15; // approximate chars that fit on screen

  for (;;) {
    if (FOCUSED_APP==_CALCULATOR) {
      if(just_switched_apps || color_change) {
        just_switched_apps=false;
        color_change=false;
        INPUT_MODE=CLASSIC_INPUT;
        program_frame.setTextFont(1);
        program_frame.resetViewport();
        program_frame.fillSprite(BG_COLOR);
        program_frame.setTextColor(FG_COLOR,BG_COLOR,true);
        render_update=true;      
      }
      
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)) continue;
        String sym = event.symbol;

        //ignore following keys
        if (sym=="OFF") continue;
        else if (sym == "#") {          // Backspace
          backspace(input, cursor);
        } else if (sym == "LEFT") {       // Cursor left
          moveLeft(cursor);
        } else if (sym == "RIGHT") {      // Cursor right
          moveRight(cursor, input);
        } else if (sym == "DEL") {        // Forward delete
          deleteAtCursor(input, cursor);
        } else if (sym == "sqrt") {
          sym="sq(";
          for (size_t i = 0; i < sym.length(); i++)
          {insertChar(input, cursor, sym[i]);}
        } else {
          for (size_t i = 0; i < sym.length(); i++)
          {insertChar(input, cursor, sym[i]);}
        }

        updateViewRight(viewOffset, cursor, viewWidth, input.length());
        // --- Clear ON key ---
        if (sym == "ON") {
          input = "";
          reset_next = false;
          output="";
          viewOffset=0;
          cursor=0;
        } else if (sym == "=") {
          input = output;
          cursor = input.length();
          viewOffset = 0;
          reset_next = false;
          output="";
        } else {
          output = expr_eval(input);
          if (input.length()<1 || input==output) {
            output="";
          }
        } 
        render_update = true;
      }

      show_cursor = !(((millis()-LAST_INPUT_TIME) % (CURSOR_BLINK_TIME * 2)) > CURSOR_BLINK_TIME);
      
      // Get visible portion for right-aligned display
      int len = input.length();
      int startIdx = max(0, len - viewWidth - viewOffset);
      int endIdx = min(len, len - viewOffset);
      String input_visible = input.substring(startIdx, endIdx);
      
      static String last_print = "";
      static bool last_cursor = false;
      
      if (last_print != input_visible || last_cursor != show_cursor || render_update) {
        render_update = false;
        last_print = input_visible;
        last_cursor = show_cursor;
        
        program_frame.fillSprite(BG_COLOR);
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        
        if (!debug) {
          // Non-debug: simple right-aligned text
          program_frame.setTextDatum(TR_DATUM);
          program_frame.setTextSize(temp);
          program_frame.drawString(input_visible, 320-R_OFFSET, 23);
          
          program_frame.setTextDatum(BR_DATUM);
          program_frame.setTextSize(temp-1);
          program_frame.drawString(output, 320-R_OFFSET, 55);
        } else {
          // Debug: right-aligned with cursor line
          program_frame.setTextDatum(TR_DATUM);
          program_frame.setFreeFont(MICRO13);
          program_frame.drawString(input_visible, 320-R_OFFSET-16, 23);
          
          // Draw cursor line if visible and not at end
          if (show_cursor && cursor != input.length() && cursor >= startIdx && cursor <= endIdx) {
            int right_px = getCursorPixelRight(program_frame, cursor, startIdx, input_visible);
            int x = 320 - R_OFFSET - 16 - right_px;
            
            // Draw 3-pixel wide cursor line (anti-aliased edges)
            program_frame.drawLine(x-1, 22, x-1, 22+21, BG_COLOR);
            program_frame.drawLine(x, 22, x, 22+21, FG_COLOR);
            program_frame.drawLine(x+1, 22, x+1, 22+21, BG_COLOR);
          }
          
          program_frame.setTextDatum(BR_DATUM);
          program_frame.setFreeFont(MICRO8);
          program_frame.drawString(output, 320-R_OFFSET-16, 55+19);
        }
        
        frame_ready();
      }
    }
    vTaskDelay(REFRESH_TIME);
  }
}
