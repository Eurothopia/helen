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
  String input_visible = "";
  String last_print="";
  String output="";
  bool reset_next = false;
  bool render_update = false;


  int cursor = 0;      // insertion index (0..input.length)
  int viewOffset = 0;  // first visible char for scrolling
  bool show_cursor = true;
  const int viewWidth = 16; // chars that fit on screen (example)

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
        } else if (sym == "sqrt") {
          sym="sq(";
          for (size_t i = 0; i < sym.length(); i++)
          {insertChar(input, cursor, sym[i]);}
        } else {
          for (size_t i = 0; i < sym.length(); i++)
          {insertChar(input, cursor, sym[i]);}
        }

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
        input_visible = renderWithCursor(input, cursor).substring(viewOffset, viewOffset + viewWidth+1);
      } else input_visible = input.substring(viewOffset, viewOffset + viewWidth);

      if(last_print!=input_visible) render_update=true;
      if (render_update) {
        render_update=false;
        if (!debug) {

          program_frame.setCursor(0, 23);
          program_frame.setTextSize(temp);
          program_frame.print(input_visible);
          //program_frame.setTextDatum(TR_DATUM);
          //program_frame.drawString(input_visible,320-R_OFFSET,23); //wipe
          program_frame.print("                   "); //wipe
          program_frame.setCursor(0, 55);
          program_frame.setTextSize(temp-1);
          program_frame.print(output);
          //program_frame.setTextDatum(TR_DATUM);
          //program_frame.drawString(output,320-R_OFFSET,55); //wipe
          program_frame.print("                   "); //wipe
          last_print=input_visible;
        } else {
          program_frame.fillSprite(BG_COLOR);
          program_frame.setTextDatum(TR_DATUM);
          program_frame.setTextSize(1);
          program_frame.setFreeFont(MICRO13);
          String display_input = input_visible;
          if (display_input.endsWith("_")) display_input.remove(display_input.length()-1);
          program_frame.drawString("                     "+display_input,320-R_OFFSET-16,23); 
          program_frame.setFreeFont(MICRO8);
          program_frame.drawString("                     "+output,320-R_OFFSET-16,55);
          last_print=input_visible;
        }
        // Send frame update event
        frame_ready();
      } 
    }
    vTaskDelay(REFRESH_TIME);
  }
}
