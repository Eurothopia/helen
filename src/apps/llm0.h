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
/*
curl https://api.mistral.ai/v1/chat/completions \
 -X POST \
 -H 'Authorization: Bearer ************************' \
 -H 'Content-Type: application/json' \
 -d '{
  "messages": [
    {
      "role": "user",
      "content": "ipsum eiusmod"
    }
  ],
  "model": "mistral-large-2411"
}'
*/

String get_jokeV2() {
  cpu_boost(5000);
  //esp_heap_caps_free();  // Force garbage collection (if available)
  //Serial.printf("Free heap before TLS: %u\n", esp_get_free_heap_size());
  client.setInsecure();              // or load cert if you have one
  //client.setBufferSizes(4096, 1024); // reduce TLS RAM use
  client.setNoDelay(true);           // Disable Nagle's algorithm
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
    client.setTimeout(30000); // Increased to 30s for long LLM responses

    if (!http.begin(client, llm_url)) {
        streamed_response = "<http begin failed>";
        streaming = false;
        return;
    }
    http.setTimeout(30000); // Set HTTP timeout to 30s

    // 1. Prepare Request JSON with stream=true
    StaticJsonDocument<2048> requestDoc; 
    requestDoc["model"] = "mistral-large-2411";
    requestDoc["max_tokens"] = 512;
    requestDoc["stream"] = true;  // Enable streaming
    JsonArray messages = requestDoc.createNestedArray("messages");
    //JsonObject historyUser = messages.createNestedObject();
    //historyUser["role"] = "user";
    //historyUser["content"] = "What is your name?";

    // Past Assistant Response
    JsonObject historyAssistant = messages.createNestedObject();
    historyAssistant["role"] = "assistant";
    historyAssistant["content"] = "do not use excessive markdown";

    // New User Prompt
    JsonObject newPrompt = messages.createNestedObject();
    newPrompt["role"] = "user";
    newPrompt["content"] = prompt; // e.g., "And what can you do?"

    String requestBody;
    serializeJson(requestDoc, requestBody);

    // 2. Send Request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + llm_api_key);
    int code = http.POST(requestBody);
    
    if (code != HTTP_CODE_OK) {
        streamed_response = "<http error: " + String(code) + ">";
        http.end();
        streaming = false;
        return;
    }

    // Streaming started
    streamed_response = "";
    streaming = true;
}

void process_stream() {

    WiFiClient& stream = http.getStream();
    static String buffer = "";
    static unsigned long last_data_time = millis();  // Track last data receipt
    
    // Read all available data
    while (stream.available()) {
        char c = stream.read();
        buffer += c;
        last_data_time = millis();  // Reset timer on data
        
        if (c == '\n') {
            // Process complete line
            if (buffer.startsWith("data: ")) {
                first_stream=false;

                String data = buffer.substring(6);  // Trim to handle newlines
                buffer = "";
                
                if (data == "[DONE]") {
                    Serial.println("llm: stream ended");
                    http.end();
                    streaming = false;
                    first_stream=true;
                    return;
                }
                
                // Parse JSON
                StaticJsonDocument<1024> doc;
                DeserializationError err = deserializeJson(doc, data);
                if (err) {
                    Serial.printf("JSON parse error: %s\n", err.c_str());
                    continue;  // Skip bad data
                }
                const char* content = doc["choices"][0]["delta"]["content"];
                if (content) {
                    streamed_response += content;
                    if(debug)Serial.printf("llm received: %s\n", content);  // Debug log
                }
            } else {
                buffer = "";  // Reset on non-data lines
            }
        }
    }
    
    // Fallback: End stream if no data for 10 seconds (stall detection)
    if (millis() - last_data_time > 10000) {
        Serial.println("stream stalled, ending manually");
        end_stream();
    }
}

//02
void APP_LLM(void *parameters) {
  static String input = "";
  static bool reset_display = false;
  static int scroll_offset = 0;
  static unsigned long last_update = 0;

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
        #define initial_px 12
        #define spacing_px 12
        /*program_frame.drawString("a b c d e", 308, 10);
        program_frame.drawString("f g h i j", 308, 22);
        program_frame.drawString("k l m n o", 308, 34);
        program_frame.drawString("p r s t u", 308, 46);
        program_frame.drawString("v # _   y", 308, 58);*/
        program_frame.drawString("a b c d e", 308, initial_px);
        program_frame.drawString("f g h i j", 308, initial_px+spacing_px*1);
        program_frame.drawString("k l m n o", 308, initial_px+spacing_px*2);
        program_frame.drawString("p r s t u", 308, initial_px+spacing_px*3);
        program_frame.drawString("v # _   y", 308, initial_px+spacing_px*4);
      } else {
        program_frame.drawString(" ---   abc  def", 308, 10);
        program_frame.drawString("ghi  jkl  mno", 308, 22);
        program_frame.drawString("pqrs  tuv wxyz", 308, 34);
        program_frame.drawString("      __       ", 308, 46);
      }

      // Process input events
      TextEvent event;
      int scroll_direction = 0;
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
            scroll_direction = 0;
            if (!streaming) {
              input = "";
              streamed_response = "";
              scroll_offset = 0;
              reset_display = true;
              display_changed = true;
            }
          } else if (sym == "ENTER") {
            scroll_direction = 0;
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
                scroll_offset = 0;
                display_changed = true;
                }
            }
            viewing_response=!viewing_response;
          } else if (sym == "LEFT") {
            scroll_direction = -1;
          } else if (sym == "RIGHT") {
            scroll_direction = 1;
          } else {
            scroll_direction = 0;
            if (!streaming) {
              input += sym;
              reset_display = true;
              display_changed = true;
            }
          }
      }

      // Smooth scrolling
      if (scroll_direction != 0 && viewing_response/*(streaming || streamed_response != "")*/) {
        scroll_offset += scroll_direction;
        if (scroll_offset < 0) scroll_offset = 0;
        display_changed = true;
      }

      // Update display only if changed
      //if (display_changed || (streaming && millis() - last_update > 100)) {  // Update streaming every 100ms
        program_frame.setViewport(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT);
        if (reset_display) {
          reset_display = false;
          program_frame.fillRect(0, 0, 320 - 70, VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
        }
        program_frame.setTextColor(FG_COLOR, BG_COLOR, true);
        program_frame.setCursor(0, 8);
        program_frame.setTextSize(1);

        if (viewing_response) {
          reset_display=true;
          // Display streamed response with scrolling
          int max_lines = (VIEWPORT_HEIGHT - STATUS_BAR_HEIGHT - 8) / 8;  // Rough line height
          String display_text = streamed_response;
          // Simple scrolling: show from scroll_offset line
          int start_pos = 0;
          for (int i = 0; i < scroll_offset && start_pos < display_text.length(); i++) {
            start_pos = display_text.indexOf('\n', start_pos) + 1;
            if (start_pos == 0) break;
          }
          String visible_text = display_text.substring(start_pos);
          program_frame.print(visible_text);
        } else {
          program_frame.print(input);
          if ((millis()%CURSOR_BLINK_TIME*2)>CURSOR_BLINK_TIME) program_frame.print("_");
          else program_frame.print(" ");
        }
        program_frame.resetViewport();
        frame_ready();
        last_update = millis();
      //}

      // Process stream
      if (streaming) {
        //Serial.print("S");
        process_stream();
        if (!streaming) display_changed = true;  // Force update when streaming ends
      }    
    frame_ready();
    if (VSYNC_ENABLED) xSemaphoreTake(frame_done_sem, portMAX_DELAY);

    vTaskDelay(REFRESH_TIME*2);  // slower refresh
    } else ulTaskNotifyTake(pdTRUE, REFRESH_TIME*10); // even slower refresh

   
  } //else vTaskDelay()
}