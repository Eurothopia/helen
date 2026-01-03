#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"

#include "app_config.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>
#include <excludable.h>
//#define gen7seg false

// App configuration
inline constexpr AppConfig appcfg_ABACUS = make_app_config([](AppConfig &c) {
  c.fullscreen = true;
  c.refresh_ms = 40;
});

//01
void APP_ABACUS(void *parameters) {
  static String current_input = "";
  static String last_input="";
  static double operand1 = 0;
  static double operand2 = 0;
  static char current_op = 0;
  static bool added_op=false;
  static bool reset_next = false;
  static bool just_pressed_something=false;
  static bool render_update = false;
  for (;;) {
    if (FOCUSED_APP==_ABACUS) {
      if(just_switched_apps || color_change) {
        just_switched_apps=false;
        color_change=false;
        INPUT_MODE=CLASSIC_INPUT;

        program_frame.resetViewport();
        program_frame.fillSprite(BG_COLOR);
        program_frame.setTextColor(FG_COLOR, BG_COLOR);
        //program_frame.setFreeFont(SEG7FONT);
        program_frame.setTextSize(1);
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
        just_pressed_something=true;
        //carrier2+=sym;
        //Serial.print("mainframe: key: "); Serial.println(sym);
        // --- Handle numeric input ---
        if (sym >= "0" && sym <= "9") {
          if (reset_next) {
            current_input = "";
            reset_next = false;
            //carrier2=sym;
          }
           if (!added_op) {
            current_input += sym;
           } else {
            current_input = sym;
            added_op=false;
           }
          //if(current_op=0&&operand1) {current_input += sym;
          //} else current_input=sym;
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
            added_op=true;
            //current_input = "";
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
            //carrier2=String(result, 0);
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
            //current_input = String(result, 2); // 6 decimals //more correct
            result=sqrt(result);
          } else if(current_input.length() > 0 && !current_op) {
            operand1=current_input.toDouble();
            result=sqrt(operand1);
          }
          //carrier2=String(result, 0);
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
          //carrier2="";
        }
        // --- Optional: handle +/- ---
        else if (sym == "+/-") {
          if (current_input.length() > 0) {
            if (current_input.startsWith("-")) current_input.remove(0, 1);
            else current_input = "-" + current_input;
          }
        }
      }
      /*program_frame.setTextColor(FG_COLOR,BG_COLOR,true);
      program_frame.setCursor(0, 32);
      program_frame.setTextSize(temp);
      program_frame.print(carrier2);
      program_frame.print("                   ");*/
      //Serial.print("input-print:"); Serial.println(current_input);
      static String buf;
      if (just_pressed_something) render_update = true;
      if (render_update) {//if (last_input!=current_input||just_pressed_something){
        program_frame.fillSprite(BG_COLOR);
        frame_ready();
        vTaskDelay(25);
        
        buf=current_input;
        if (current_input=="") buf="0";
        if (buf.endsWith(".00")) buf.remove(buf.length() - 3);
        if(gen7seg) {
          int8_t decimal_index = buf.indexOf('.');
          int8_t tmp_index=0;
          if(decimal_index !=-1) {
            buf.remove(decimal_index,1);
            tmp_index=buf.length()-decimal_index;
          }
          if(buf.startsWith("-")) {
            buf.remove(0,1);
            program_frame.fillRect(((274+50)-(buf.length()+1)*36 - R_OFFSET), 50+60-1, 20, 7, FG_COLOR);
          }
          for (size_t i = 0; i < buf.length(); i++)
          {
            int digit = buf[i] - '0';
            N7S(digit, (274)-(buf.length()-i-1)*36-R_OFFSET, 15, 3, FG_COLOR, BG_COLOR, false);
            if(n7s_fix) N7S_AA(digit, (274)-(buf.length()-i-1)*36-R_OFFSET, 15, 3, FG_COLOR, BG_COLOR, false);
          }
          program_frame.fillRect(((274+24)-(tmp_index)*36 - R_OFFSET), 15+60-1, 4, 5, FG_COLOR);
        } else {
          program_frame.setTextDatum(MR_DATUM);
          program_frame.drawString(buf, DISPLAY_WIDTH-15-R_OFFSET, 15+30);
        }
        just_pressed_something=false;
        render_update=false;

        // Send frame update event
        frame_ready();
      }
  

    }
    vTaskDelay(REFRESH_TIME);
  }
}
