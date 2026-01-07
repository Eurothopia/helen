//beta pixel based scrolling
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"
#include "HTTPClient.h"
#include "WiFiClient.h"
#include "ArduinoJson.h"

#include "app_config.h"

//#define USE_BEARSSL  // Enable BearSSL backend for WiFiClientSecure
#include "WiFiClientSecure.h"  // Keep this include
// ... other includes unchanged


#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>

//#include <drivers/cpud.h>

#include <ns/secrets_ns.h>

bool streaming = false;
HTTPClient http;
String streamed_response = "";    
bool first_stream=true;

// App configuration
inline constexpr AppConfig appcfg_LLM = make_app_config([](AppConfig &c) {
  c.fullscreen = false;
  c.refresh_ms = 80;
  c.vsync = false;
  c.stack_size = 8192;
  c.priority = 2;
  c.needs_network = true;
});

bool viewing_response=false;


WiFiClientSecure client;
StaticJsonDocument<4096> doc;
const String url = "https://v2.jokeapi.dev/joke/Programming";

//const char*
//const String llm_url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";
const String llm_url = "https://api.mistral.ai/v1/chat/completions";

String get_jokeV2() {
  cpu_boost(5000);
  client.setInsecure();
  client.setNoDelay(true);
  client.setTimeout(5000);   

  HTTPClient http;
  http.useHTTP10(true);
  http.setReuse(true);
  http.setUserAgent("");

  if (!http.begin(client, url)) {
    return "<http begin failed>";
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return "<http error>";
  }

  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) { return "<parse error>"; }

  const char* type = doc["type"] | "";
  if (!strcmp(type, "single")) return String(doc["joke"] | "");

  String s;
  s.reserve(96);
  s += (doc["setup"] | "");
  s += "  ";
  s += (doc["delivery"] | "");
  return s;

}

void end_stream() {
    http.end();
    streaming = false;
    first_stream = true;
}

void start_stream(String prompt) {
    cpu_boost(10000);
    client.setInsecure();
    client.setNoDelay(true);
    client.setTimeout(30000);

    if (!http.begin(client, llm_url)) {
        streamed_response = "<http begin failed>";
        streaming = false;
        return;
    }
    http.setTimeout(30000);

    StaticJsonDocument<2048> requestDoc; 
    requestDoc["model"] = "mistral-large-2411";
    requestDoc["max_tokens"] = 512;
    requestDoc["stream"] = true;
    JsonArray messages = requestDoc.createNestedArray("messages");

    JsonObject historyAssistant = messages.createNestedObject();
    historyAssistant["role"] = "assistant";
    historyAssistant["content"] = "do not use excessive markdown";

    JsonObject newPrompt = messages.createNestedObject();
    newPrompt["role"] = "user";
    newPrompt["content"] = prompt;

    String requestBody;
    serializeJson(requestDoc, requestBody);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + llm_api_key);
    int code = http.POST(requestBody);
    
    if (code != HTTP_CODE_OK) {
        streamed_response = "<http error: " + String(code) + ">";
        http.end();
        streaming = false;
        return;
    }

    streamed_response = "";
    streaming = true;
}

void process_stream() {
    WiFiClient& stream = http.getStream();
    static String buffer = "";
    static unsigned long last_data_time = millis();
    
    while (stream.available()) {
        char c = stream.read();
        buffer += c;
        last_data_time = millis();
        
        if (c == '\n') {
            if (buffer.startsWith("data: ")) {
                first_stream=false;

                String data = buffer.substring(6);
                buffer = "";
                
                if (data == "[DONE]") {
                    Serial.println("llm: stream ended");
                    http.end();
                    streaming = false;
                    first_stream=true;
                    return;
                }
                
                StaticJsonDocument<1024> doc;
                DeserializationError err = deserializeJson(doc, data);
                if (err) {
                    Serial.printf("JSON parse error: %s\n", err.c_str());
                    continue;
                }
                const char* content = doc["choices"][0]["delta"]["content"];
                if (content) {
                    streamed_response += content;
                    if(debug)Serial.printf("llm received: %s\n", content);
                }
            } else {
                buffer = "";
            }
        }
    }
    
    if (millis() - last_data_time > 10000) {
        Serial.println("stream stalled, ending manually");
        end_stream();
    }
}

// Helper: Calculate total content height in pixels
int calculate_content_height(const String& text) {
    int lines = 1;
    for (char c : text) {
        if (c == '\n') lines++;
    }
    return lines * 8;  // 8 pixels per line at text size 1
}

//02
void APP_LLM(void *parameters) {
  static String input = "";
  static bool reset_display = false;
  static int scroll_pixel_offset = 0;  // Pixel-based scroll offset
  static int target_scroll_offset = 0;  // Target for smooth interpolation
  static float scroll_velocity = 0.0f;  // Smooth scrolling velocity
  static unsigned long last_update = 0;
  static int scroll_direction = 0;

  for (;;) {
    if (FOCUSED_APP == _LLM) {
      bool display_changed = false;

      if (just_switched_apps || color_change) {
        just_switched_apps = false;
        color_change = false;
        if (INPUT_MODE != T9X) INPUT_MODE = ABX;
        program_frame.setTextFont(1);
        program_frame.resetViewport();
        program_frame.setTextSize(1);
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        program_frame.fillSprite(BG_COLOR);
        program_frame.setCursor(0, 32);
        program_frame.setTextSize(1);
        reset_display = true;
        display_changed = true;
      }

      // Sidebar keyboard
      if (INPUT_MODE == ABX) {
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

      // Process input events
      TextEvent event;
      while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
        if (!(event.type == KEY_RELEASE || event.type == KEY_HOLD) && INPUT_MODE != T9X) continue;
        String sym = event.symbol;

          if (sym == "#") {
            if (!streaming) {
              input.remove(input.length() - 1);
              reset_display = true;
              display_changed = true;
            }
          } else if (sym.startsWith("#")) {
            if (!streaming) {
              input.remove(input.length() - 1);
              input += sym[1];
              reset_display = true;
              display_changed = true;
            }
          } else if (sym == "CLEAR") {
            if (!streaming) {
              input = "";
              streamed_response = "";
              scroll_pixel_offset = 0;
              target_scroll_offset = 0;
              scroll_velocity = 0;
              reset_display = true;
              display_changed = true;
            }
          } else if (sym == "ENTER") {
            if (viewing_response) {
                end_stream();
            } else {
                if (!streaming) {
                program_frame.fillRect(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
                program_frame.setCursor(0, 8);
                program_frame.print("loading..");
                frame_ready();
                streamed_response = "";
                start_stream(input);
                input = "";
                scroll_pixel_offset = 0;
                target_scroll_offset = 0;
                scroll_velocity = 0;
                display_changed = true;
                }
            }
            viewing_response=!viewing_response;
          } else if (sym == "LEFT") {
            scroll_direction = -1;

          } else if (sym == "RIGHT") {
            scroll_direction = 1;

          } else {
            if (!streaming && !viewing_response) {
              input += sym;
              reset_display = true;
              display_changed = true;
            }
          }
      }

      // Smooth pixel-based scrolling
      if (viewing_response) {
        // Update target based on scroll direction
        if (scroll_direction != 0) {
          target_scroll_offset += scroll_direction * 3;  // 3 pixels per frame (adjustable speed)
          
          // Clamp target to valid range
          int content_height = calculate_content_height(streamed_response);
          int viewport_height = VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT - 8;
          int max_scroll = max(0, content_height - viewport_height);
          target_scroll_offset = constrain(target_scroll_offset, 0, max_scroll);
        }
      }
      
      // Smooth interpolation (always runs to complete animation)
      if (viewing_response) {
        float diff = target_scroll_offset - scroll_pixel_offset;
        if (abs(diff) > 0.5f) {
          scroll_velocity = diff * 0.2f;  // 20% interpolation (adjust for smoothness)
          scroll_pixel_offset += (int)scroll_velocity;
          display_changed = true;
        } else if (scroll_pixel_offset != target_scroll_offset) {
          scroll_pixel_offset = target_scroll_offset;  // Snap to target
          display_changed = true;
        }
      }

      // Update display
      program_frame.setViewport(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT);
      if (reset_display) {
        reset_display = false;
        program_frame.fillRect(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
      }
      program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
      program_frame.setTextSize(1);

      if (viewing_response) {
        // Render text with pixel-perfect scroll offset
        program_frame.setCursor(0, 8 - scroll_pixel_offset);  // Negative offset shifts text up
        program_frame.print(streamed_response);
      } else {
        program_frame.setCursor(0, 8);
        program_frame.print(input);
        if ((millis()%CURSOR_BLINK_TIME*2)>CURSOR_BLINK_TIME) program_frame.print("_");
        else program_frame.print(" ");
      }
      program_frame.resetViewport();
      frame_ready();
      last_update = millis();

      // Process stream
      if (streaming) {
        process_stream();
        if (!streaming) display_changed = true;
        
        // Auto-scroll to bottom during streaming
        if (viewing_response) {
          int content_height = calculate_content_height(streamed_response);
          int viewport_height = VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT - 8;
          target_scroll_offset = max(0, content_height - viewport_height);
        }
      }    
    
    if (VSYNC_ENABLED) xSemaphoreTake(frame_done_sem, portMAX_DELAY);
    vTaskDelay(REFRESH_TIME*2);  // slower refresh
    } else ulTaskNotifyTake(pdTRUE, REFRESH_TIME*10); // even slower refresh
  }
}