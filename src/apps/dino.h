#include "Arduino.h"
#include "TFT_eSPI.h"
#include "esp_system.h"

#include "app_config.h"

#include <definitions.h>
#include <global.h>
#include <excludable.h>
#include <key_input.h>

// App configuration
inline constexpr AppConfig appcfg_DINO = make_app_config([](AppConfig &c) {
  //c.fullscreen = true;
  c.refresh_ms = 30;
  c.vsync = true;
  c.stack_size = 6144;
});

//03
void APP_DINO(void *parameters) {
  const int SCREEN_W = 320;
  const int SCREEN_H = VIEWPORT_HEIGHT;
  const int GROUND_Y = SCREEN_H - 25;
  const int dinoFPS = 60;
  //const uint16_t FG_COLOR = TFT_WHITE;
  //const uint16_t BG_COLOR = TFT_BLACK;

  static bool initialized = false;
  static bool jumping = false;
  static bool ducking = false;
  static bool dead = false;
  static float dinoY, dinoVel;
  static int cactusX, cactusType;
  static int birdX, birdY, birdWing;
  static float groundOffset;
  static unsigned long score = 0;
  static unsigned long lastFrame = 0;
  static unsigned long frameCount = 0;

  TFT_eSprite dino(&display);
  TFT_eSprite cactus(&display);
  TFT_eSprite bird(&display);
  TFT_eSprite ground(&display);
// ---- Sprite creation helpers ----
auto drawDinoStanding = [&](TFT_eSprite &spr) {
    spr.fillSprite(TFT_TRANSPARENT);

    // Head
    spr.fillRect(12, 0, 12, 6, FG_COLOR);

    // Eye (scaled up)
    spr.fillRect(16, 2, 2, 2, BG_COLOR);

    // Torso
    spr.fillRect(6, 6, 18, 14, FG_COLOR);

    // Arms
    spr.fillRect(4, 10, 4, 6, FG_COLOR);
    spr.fillRect(20, 10, 4, 6, FG_COLOR);

    // Legs
    spr.fillRect(8, 20, 6, 4, FG_COLOR);
    spr.fillRect(16, 20, 6, 4, FG_COLOR);

    // Tail
    spr.fillRect(0, 18, 6, 4, FG_COLOR);
};

auto drawCactus = [&](TFT_eSprite &spr, int type) {
    spr.fillSprite(TFT_TRANSPARENT);
    switch (type) {
        case 0:
            // Small single cactus with arms
            spr.fillRect(8, 4, 8, 32, FG_COLOR);
            spr.fillRect(4, 12, 4, 12, FG_COLOR);   // left arm
            spr.fillRect(16, 8, 4, 14, FG_COLOR);  // right arm
            break;
        case 1:
            // Three-segment cactus
            spr.fillRect(4, 6, 8, 28, FG_COLOR);
            spr.fillRect(12, 4, 8, 32, FG_COLOR);
            spr.fillRect(20, 8, 8, 28, FG_COLOR);
            break;
    }
};

  auto drawBird = [&](TFT_eSprite &spr, bool wingUp) {
    spr.fillSprite(TFT_TRANSPARENT);
    spr.fillRect(3, 6, 10, 6, FG_COLOR);
    if (wingUp)
      spr.fillRect(0, 2, 8, 4, FG_COLOR);
    else
      spr.fillRect(0, 12, 8, 4, FG_COLOR);
    spr.fillRect(13, 8, 2, 2, BG_COLOR);
  };

  auto drawGround = [&](TFT_eSprite &spr) {
    spr.fillSprite(TFT_TRANSPARENT);
    for (int x = 0; x < SCREEN_W; x += 4)
      spr.drawFastHLine(x, 0, 2, FG_COLOR);
  };
  

  if (!initialized) {
    dino.createSprite(24, 24);
    cactus.createSprite(16, 24);
    bird.createSprite(20, 16);
    ground.createSprite(SCREEN_W, 4);
    drawDinoStanding(dino);
    drawCactus(cactus, 0);
    drawBird(bird, true);
    drawGround(ground);
    dinoY = GROUND_Y - 24;
    cactusX = SCREEN_W;
    birdX = SCREEN_W + 180;
    birdY = GROUND_Y - 65;
    cactusType = 0;
    birdWing = 0;
    groundOffset = 0;
    initialized = true;
  }

  for (;;) {
    // Suspend when app is out of focus
    if (FOCUSED_APP != _DINO) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
    if (color_change||just_switched_apps) {
      just_switched_apps=false;
      color_change=false;
      initialized=false;
      program_frame.resetViewport();
          dino.createSprite(24, 24);
    cactus.createSprite(16, 24);
    bird.createSprite(20, 16);
    ground.createSprite(SCREEN_W, 4);
    drawDinoStanding(dino);
    drawCactus(cactus, 0);
    drawBird(bird, true);
    drawGround(ground);
  
    initialized = true;
    }

    unsigned long now = millis();
    if (now - lastFrame < (1000 / dinoFPS)) {
      vTaskDelay(1);
      continue;
    }
    lastFrame = now;
    frameCount++;

    // --- INPUT via queue ---
    bool pressed = false;
    TextEvent event;
    while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
      if (event.type == KEY_PRESS) pressed = true;
    }

    // --- Logic ---
    if (!dead && !jumping && pressed) {
      jumping = true;
      dinoVel = -6.0;
    }

    if (jumping) {
      dinoY += dinoVel;
      dinoVel += 0.45;
      if (dinoY >= GROUND_Y - 24) {
        dinoY = GROUND_Y - 24;
        jumping = false;
      }
    }

    // Ground scroll
    groundOffset -= 3;
    if (groundOffset < -SCREEN_W) groundOffset = 0;

    // Move cactus
    cactusX -= 5+(score*0.01);
    if (cactusX < -16) {
      cactusX = SCREEN_W + random(50, 120);
      cactusType = random(0, 2);
      drawCactus(cactus, cactusType);
      //score++;
    }
    
    if (!dead) {
      static unsigned long last_score_update=0;
      if (millis()-last_score_update>=1000/6) {score++; last_score_update=millis();}
    }
    // Move bird
    if (score > 250) {
    birdX -= 6;
    if (birdX < -20) {
      birdX = SCREEN_W + random(200, 350);
      birdY = GROUND_Y - random(45, 85);
    }

    if (frameCount % 12 == 0) {
      birdWing = !birdWing;
      drawBird(bird, birdWing);
    }
    }

    // --- Collision detection ---
    if (!dead) {
      if (cactusX < 45 && cactusX + 12 > 25 && dinoY > GROUND_Y - 26)
        dead = true;
      if (birdX < 40 && birdX + 16 > 25 && dinoY < birdY + 12 && dinoY + 20 > birdY)
        dead = true;
    }

    // --- Drawing ---
    program_frame.fillSprite(BG_COLOR);

    // Ground
    ground.pushToSprite(&program_frame, (int)groundOffset, GROUND_Y, TFT_TRANSPARENT);
    ground.pushToSprite(&program_frame, (int)groundOffset + SCREEN_W, GROUND_Y, TFT_TRANSPARENT);

    // Entities
    cactus.pushToSprite(&program_frame, cactusX, GROUND_Y - 24, TFT_TRANSPARENT);
    bird.pushToSprite(&program_frame, birdX, birdY, TFT_TRANSPARENT);
    dino.pushToSprite(&program_frame, 25, (int)dinoY, TFT_TRANSPARENT);

    // Score
    program_frame.setTextColor(FG_COLOR, BG_COLOR);
    program_frame.setTextSize(1);
    program_frame.setTextFont(1);
    //program_frame.setTextDatum(MR_DATUM);
    //String sscore = "SCORE: ";
    //sscore += score.toString();
    //program_frame.drawString(sscore, 320-R_OFFSET, 10);
    program_frame.setCursor(SCREEN_W - 42 - R_OFFSET, 10);
    program_frame.printf("%05lu", score);

    // Game over
    if (dead) {
      program_frame.setTextSize(2);
      program_frame.setTextDatum(MC_DATUM);
      program_frame.setTextColor(FG_COLOR, BG_COLOR);
      program_frame.drawString("GAME OVER", SCREEN_W / 2, SCREEN_H / 2.5);
      if (pressed) {
        dead = false;
        score = 0;
        cactusX = SCREEN_W;
        birdX = SCREEN_W + 180;
        dinoY = GROUND_Y - 24;
        dinoVel = 0;
      }
    }

    // Send frame update event
    frame_ready();

    // Wait for display draw done
    if (VSYNC_ENABLED) xSemaphoreTake(frame_done_sem, portMAX_DELAY);

    xTaskNotifyGive(display_daemon_handle);
    //vTaskDelay(1000 / dinoFPS / portTICK_PERIOD_MS);
    vTaskDelay(REFRESH_TIME);
  }
}


