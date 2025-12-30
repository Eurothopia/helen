#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "tinyexpr.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>

//00
void APP_CALCULATOR(void *parameters) {
  String input = "";
  String last_print="";
  String output="";
  bool reset_next = false;
  bool render_update = false;
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
        input+=sym;
        //delete
        if (sym=="#") {
          input.remove(input.length()-2);
          //reset_next=true;
        
        // --- Clear ON key ---
        } else if (sym == "ON") {
          input = "";
          reset_next = false;
          output="";
        } else if (sym == "=") {
          input = output;
          reset_next = false;
          output="";
        }
        // --- Optional: handle +/- ---
        else if (sym == "+/-") {
          if (input.length() > 0) {
            input.remove(input.length()-3);
            if (input.startsWith("-")) { input.remove(0, 1);
            } else input = "-" + input;
          }
        } else {
          output = expr_eval(input);
        }
      }
      if(last_print!=input) render_update=true;
      if (render_update) {
        Serial.println("calc input: " + input);
        render_update=false;
        program_frame.setCursor(0, 23);
        program_frame.setTextSize(temp);
        program_frame.print(input);
        program_frame.print("                   "); //wipe
        program_frame.setCursor(0, 55);
        program_frame.setTextSize(temp-1);
        program_frame.print(output);
        program_frame.print("                   "); //wipe
        last_print=input;
      }

    }
    vTaskDelay(REFRESH_TIME);
  }
}
