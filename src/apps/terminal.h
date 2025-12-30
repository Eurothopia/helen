#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>
#include <serial.h>

//04
void APP_TERMINAL(void *parameters) {
  static String input = "", output="";
  static bool reset_next = false;
  for (;;) {
    if (FOCUSED_APP==_TERMINAL) {
      if(just_switched_apps||color_change) {
        just_switched_apps=false;
        color_change=false;
        if(INPUT_MODE!=T9X) INPUT_MODE=ABX;
        //program_frame.setCursor(0, 32);
        Serial.print("run ui terminal setup");
        program_frame.setTextFont(1);
        program_frame.fillSprite(BG_COLOR);
        program_frame.resetViewport();
        program_frame.setTextSize(1); 
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        //program_frame.fillSprite(BG_COLOR);
        if(INPUT_MODE==ABX){
        program_frame.setTextDatum(MR_DATUM);
        program_frame.drawString("a b c d e", 308, 10);
        program_frame.drawString("f g h i j", 308, 22);
        program_frame.drawString("k l m n o", 308, 34);
        program_frame.drawString("p r s t u", 308, 46);
        program_frame.drawString("v # _   y", 308, 58);
        } else {
        program_frame.drawString(" ---   abc  def", 308, 10);
        program_frame.drawString("ghi  jkl  mno", 308, 22);
        program_frame.drawString("pqrs  tuv wxyz", 308, 34);
        program_frame.drawString("      __       ", 308, 46);
        }
        //program_frame.setViewport(0,0,320-70,VIEWPORT_HEIGHT-STATUS_BAR_HEIGHT);
        program_frame.setCursor(0, 32);
        program_frame.setTextSize(1);
      }
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        //if (SUPERKEY==key_ON){input=""; reset_next=true; continue; }// only process key presses
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)&& INPUT_MODE!=T9X) continue;
        String sym = event.symbol;
        //uint8_t key = event.symbol;
        //String sym = SYMBOLMAP[key];
        //if (reset_next) carrier2="";
        if (sym=="#"){
          input.remove(input.length()-1);
          reset_next=true;
        } else if (sym=="ENTER") {
          output=process_command(input);
          input="";
          reset_next=true;  
        } else if (sym[0]=='#'){
          input.remove(input.length()-1);
          //reset_next=true;
          input+=sym[1];
        } else if (sym=="CLEAR") {
            input=""; reset_next=true;
        } else input+=sym;
        
        //Serial.print("mainframe: key: "); Serial.println(sym);
        // --- Handle numeric input ---
        
      }
      program_frame.setViewport(0,0,320-70,VIEWPORT_HEIGHT-STATUS_BAR_HEIGHT);
      if(reset_next) {
        reset_next=false;
        program_frame.fillRect(0,0,320-70,VIEWPORT_HEIGHT-STATUS_BAR_HEIGHT,BG_COLOR);
      }
      //program_frame.setTextColor(FG_COLOR,BG_COLOR,true);
      program_frame.setCursor(0, 8);
      program_frame.setTextSize(temp-2);
      //program_frame.print("v0.1@helen - ");
      program_frame.print("HELEN> ");
      program_frame.println(input);
      program_frame.println("                            ");
      //program_frame.setCursor(0,20);
      program_frame.println(output);
      program_frame.resetViewport();

      //program_frame.print("                   ");



    }
    vTaskDelay(REFRESH_TIME);
  }
}

