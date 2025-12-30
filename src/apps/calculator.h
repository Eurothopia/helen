#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "tinyexpr.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>

#include <_lib/input_field.h>

//00
void APP_CALCULATOR(void *parameters) {
  String input = "";
  String input_visible = "";
  String last_print="";
  String output="";
  bool reset_next = false;
  bool render_update = false;


  int cursor = 0;      // insertion index (0..input.length)
  int viewOffset = 0;  // first visible char for scrolling
  bool show_cursor = true;
  const int viewWidth = 17; // chars that fit on screen (example)

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
        //provisional raw reading
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)) continue;
        //if (event.type != KEY_RELEASE && event.type != KEY_RELEASE_HOLD) continue; // only process key presses
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
        } else insertChar(input, cursor, sym[0]);

        updateView(viewOffset, cursor, viewWidth);
              // --- Clear ON key ---
        if (sym == "ON") {
          input = "";
          reset_next = false;
          output="";
          viewOffset=0;
          cursor=0;
        } else if (sym == "=") {
          input = output;
          reset_next = false;
          output="";
        }
        // --- Optional: handle +/- ---
        /*else if (sym == "+/-") {
          if (input.length() > 0) {
            input.remove(input.length()-3);
            if (input.startsWith("-")) { input.remove(0, 1);
            } else input = "-" + input;
          }
        }*/ else {
          output = expr_eval(input);
          if (input.length()<1 || input==output) {
            output="";
          }

        } 
        //LAST_INPUT_TIME
      }

      show_cursor = (((millis()%CURSOR_BLINK_TIME*2)>CURSOR_BLINK_TIME)||millis()-LAST_INPUT_TIME<CURSOR_BLINK_TIME);
      if (show_cursor){
        input_visible = renderWithCursor(input, cursor).substring(viewOffset, viewOffset + viewWidth);
      } else input_visible = input.substring(viewOffset, viewOffset + viewWidth);

      if(last_print!=input_visible) render_update=true;
      if (render_update) {
        

        render_update=false;
        program_frame.setCursor(0, 23);
        program_frame.setTextSize(temp);
        program_frame.print(input_visible);
        program_frame.print("                   "); //wipe
        program_frame.setCursor(0, 55);
        program_frame.setTextSize(temp-1);
        program_frame.print(output);
        program_frame.print("                   "); //wipe
        last_print=input_visible;
      }

    }
    vTaskDelay(REFRESH_TIME);
  }
}
