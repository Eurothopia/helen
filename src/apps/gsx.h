#include "Arduino.h"
#include "TFT_eSPI.h"

#include "app_config.h"

#include "misc/arasaka.h"

#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>

// App configuration
inline constexpr AppConfig appcfg_GSX = make_app_config([](AppConfig &c) {
  c.fullscreen = true;
  c.refresh_ms = 50;
  c.vsync = true;
  c.priority = 2;
});

//05
void APP_GSX(void *parameters) {
  for (;;) {
    if (FOCUSED_APP==_GSX) {
                //Serial.println("llm tick ");
      if(just_switched_apps||color_change) {
        Serial.println("gsx init");
        just_switched_apps=false;
        color_change=false;
        program_frame.fillSprite(BG_COLOR);
      }
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        //if (SUPERKEY==key_ON){input=""; reset_next=true; continue; }// only process key presses
        //Serial.print("llm key event ");
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)) continue;
                //Serial.println("llm past key event check")
        
      }
      //Serial.print("smasher ");
      arasaka_draw();

      // Send frame update event
      frame_ready();
    }
    vTaskDelay(REFRESH_TIME);
  }
}
