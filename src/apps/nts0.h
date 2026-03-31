#include "Arduino.h"
#include "TFT_eSPI.h"

#include "app_config.h"

#include <definitions.h>
#include <global.h>
#include <key_input.h>

// App configuration
inline constexpr AppConfig appcfg_NTS = make_app_config([](AppConfig &c) {
  c.fullscreen = true;
  c.refresh_ms = 50;
  c.priority = 2;
  c.needs_network = true;
  c.stack_size = 1024;
});

//05
void APP_NTS(void *parameters) {
  for (;;) {
    if (FOCUSED_APP==_NTS) {
                //Serial.println("llm tick ");
      if(just_switched_apps||color_change) {
        just_switched_apps=false;
        color_change=false;
        
        // Draw 8-bit color test grid on 320x110 screen
        program_frame.fillSprite(TFT_BLACK);
        
        // Create color test pattern
        // Top half: RGB gradient (55px height)
        for (int y = 0; y < 55; y++) {
          for (int x = 0; x < 320; x++) {
            // Map x to red (0-31) and green (0-63)
            uint8_t r = (x * 32) / 320;  // 5-bit red
            uint8_t g = (x * 64) / 320;  // 6-bit green
            uint8_t b = (y * 32) / 55;   // 5-bit blue
            uint16_t color = (r << 11) | (g << 5) | b;
            program_frame.drawPixel(x, y, color);
          }
        }
        
        // Bottom half: Color bars (55px height)
        int barWidth = 320 / 8;
        uint16_t colors[8] = {
          0xF800,  // Red
          0xFFE0,  // Yellow
          0x07E0,  // Green
          0x07FF,  // Cyan
          0x001F,  // Blue
          0xF81F,  // Magenta
          0xFFFF,  // White
          0x0000   // Black
        };
        
        for (int i = 0; i < 8; i++) {
          program_frame.fillRect(i * barWidth, 55, barWidth, 55, colors[i]);
        }
        
        frame_ready();
      }
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        //if (SUPERKEY==key_ON){input=""; reset_next=true; continue; }// only process key presses
        //Serial.print("llm key event ");
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)) continue;
                //Serial.println("llm past key event check")
        
      }
    }
    vTaskDelay(REFRESH_TIME);
  }
}
