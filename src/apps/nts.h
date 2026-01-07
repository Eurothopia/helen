#include "Arduino.h"
#include "TFT_eSPI.h"
#include "HTTPClient.h"

#include "app_config.h"

#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>
#include <drivers/A2DPd.h>

// App configuration
inline constexpr AppConfig appcfg_NTS = make_app_config([](AppConfig &c) {
  c.fullscreen = true;
  c.refresh_ms = 50;
  c.priority = 2;
  c.needs_network = true;
});

// Background audio control
static TaskHandle_t nts_audio_task = nullptr;
static volatile bool nts_audio_should_run = false;
static volatile int nts_station = 1; // 1 or 2
static bool nts_started = false;     // start only after first focus to save heap

// NTS stream URLs (AAC/MP3); actual decode not implemented yet
static const char *NTS1_URL = "https://stream-relay-geo.ntslive.net/stream";
static const char *NTS2_URL = "https://stream-relay-geo.ntslive.net/stream2";

void nts_audio_loop(void *param) {
  A2DPManager &a2dp = A2DPManager::get();
  for (;;) {
    if (!nts_audio_should_run) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!a2dp.ready()) {
      a2dp.init();
    }

    // TODO: fetch and decode NTS stream (AAC/MP3 -> PCM16 stereo) then feed a2dp.write()
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

inline void nts_start_audio_if_needed() {
  if (!nts_started) {
    nts_started = true;
    nts_audio_should_run = true;
    if (nts_audio_task == nullptr) {
      xTaskCreatePinnedToCore(nts_audio_loop, "nts_audio", 4096, nullptr, 3, &nts_audio_task, SYSTEM_CORE);
    }
  } else {
    nts_audio_should_run = true;
  }
}

inline void nts_set_station(int station) {
  nts_station = (station == 2) ? 2 : 1;
  nts_start_audio_if_needed();
}

//05
void APP_NTS(void *parameters) {
  bool render_update = false;
  int last_station = nts_station;
  for (;;) {
    if (FOCUSED_APP==_NTS) {
      if (!nts_started) {
        nts_set_station(nts_station);
      }
      if(just_switched_apps||color_change) {
        just_switched_apps=false;
        color_change=false;
        render_update=true;
        program_frame.fillSprite(BG_COLOR);
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        program_frame.setTextDatum(MC_DATUM);
        program_frame.setTextFont(4);
      }
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD)) continue;
        if (event.symbol == "1") {
          nts_set_station(1);
          render_update = true;
        } else if (event.symbol == "2") {
          nts_set_station(2);
          render_update = true;
        }
      }

      if (render_update || last_station != nts_station) {
        last_station = nts_station;
        render_update=false;
        program_frame.fillSprite(BG_COLOR);
        program_frame.drawString(String("NTS ") + (nts_station == 2 ? "2" : "1"), DISPLAY_WIDTH/2, (VIEWPORT_HEIGHT/2));
        frame_ready();
      }
    }
    vTaskDelay(REFRESH_TIME);
  }
}
