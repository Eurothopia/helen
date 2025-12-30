#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "tinyexpr.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>

//00
void APP_CALCULATOR(void *parameters) {
  static String current_input = "";
  static String last_print="";
  static double operand1 = 0;
  static double operand2 = 0;
  static char current_op = 0;
  static bool reset_next = false;
  static bool render_update = false;
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
        if (event.type != KEY_PRESS) continue; // only process key presses
        String sym = event.symbol;
        //uint8_t key = event.symbol;
        //String sym = SYMBOLMAP[key];
        //if (reset_next) carrier2="";'
        if (sym=="OFF") continue;
        carrier2+=sym;
        //Serial.print("mainframe: key: "); Serial.println(sym);
        // --- Handle numeric input ---
        if (sym=="#") {
          current_input.remove(current_input.length()-1);
          //reset_next=true;
        } else if (sym >= "0" && sym <= "9") {
          if (reset_next) {
            current_input = "";
            reset_next = false;
          carrier2=sym;
          }
          current_input += sym;
        }
        // --- Decimal point ---
        else if (sym == ".") {
            if (current_input.indexOf('.') == -1) {
                if (current_input.length() == 0) current_input = "0";
                current_input += ".";
            }
        }
        // --- Operations ---
        else if (sym == "+" || sym == "-" || sym == "x" || sym == "/" || sym == "%") {
          if (current_input.length() > 0) {
            operand1 = current_input.toDouble();
            current_op = sym[0];
            current_input = "";
          }
        }
        // --- Equals key ---
        else if (sym == "=") {
          if (current_input.length() > 0 && current_op) {
            operand2 = current_input.toDouble();
            double result = 0;
            switch (current_op) {
              case '+': result = operand1 + operand2; break;
              case '-': result = operand1 - operand2; break;
              case 'x': result = operand1 * operand2; break;
              case '%': result = fmod(operand1, operand2); break;
              case '/': result = (operand2 != 0) ? operand1 / operand2 : 0; break;
            }
            //current_input = String(result, 6); // 6 decimals //more correct
            carrier2=String(result, 2);
            current_op = 0;
            
            current_input=result;
            reset_next = true;
          }
        }
        else if (sym == "sqrt") {
          double result = 0;
          if (current_input.length() > 0 && current_op) {
            operand2 = current_input.toDouble();
            switch (current_op) {
              case '+': result = operand1 + operand2; break;
              case '-': result = operand1 - operand2; break;
              case 'x': result = operand1 * operand2; break;
              case '%': result = fmod(operand1, operand2); break;
              case '/': result = (operand2 != 0) ? operand1 / operand2 : 0; break;
            }
            //current_input = String(result, 6); // 6 decimals //more correct
            result=sqrt(result);
          } else if(current_input.length() > 0 && !current_op) {
            operand1=current_input.toDouble();
            result=sqrt(operand1);
          }
          carrier2=String(result, 2);
          current_input=result;
          current_op = 0;
          reset_next = true;
        }
        // --- Clear ON key ---
        else if (sym == "ON") {
          current_input = "";
          operand1 = operand2 = 0;
          current_op = 0;
          reset_next = false;
          carrier2="";
        }
        // --- Optional: handle +/- ---
        else if (sym == "+/-") {
          if (current_input.length() > 0) {
            if (current_input.startsWith("-")) current_input.remove(0, 1);
            else current_input = "-" + current_input;
          }
        }
      }
      if(last_print!=carrier2) render_update=true;
      if (render_update) {
      render_update=false;
      program_frame.setCursor(0, 32);
      program_frame.setTextSize(temp);
      program_frame.print(carrier2);
      program_frame.print("                   ");
      last_print=carrier2;
      }

    }
    vTaskDelay(REFRESH_TIME);
  }
}
