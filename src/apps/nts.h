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
});

//05
void APP_NTS(void *parameters) {
  for (;;) {
    if (FOCUSED_APP==_NTS) {
                //Serial.println("llm tick ");
      if(just_switched_apps||color_change) {
        just_switched_apps=false;
        color_change=false;
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
