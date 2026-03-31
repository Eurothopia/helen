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
  c.fullscreen = true;
  c.refresh_ms = 33;  // ~30 FPS
  c.vsync = true;
  c.stack_size = 6144;
});

// ============================================================================
// UNICORN DASH - Ported to Helen Framework
// Original: Arduboy (128×64)
// Target: Helen (320×114, rendered at 256×128 with 2× upscaling)
// ============================================================================

// Game state enumeration
enum GameState {
    StateIntro,
    StatePlay,
    StatePause,
    StateOver,
    StateCredits
};

// Object and particle structures
struct Position { int x; int y; };
struct Velocity { int x; int y; };
struct Size { unsigned int width; unsigned int height; };
struct Rect { int x; int y; int w; int h; };

struct GameObject {
    Position pos;
    Velocity vel;
    Rect box;
    unsigned int frame;
    unsigned int type;
    bool action;
};

struct Star {
    Position pos;
    unsigned int frame;
};

struct Particle {
    Position pos;
    unsigned int life;
    unsigned int lifeCount;
};

// Rendering constants for original game logic (128×64)
static constexpr int LOGIC_W = 128;
static constexpr int LOGIC_H = 64;
static constexpr int GROUND_Y = LOGIC_H - 30;

// Obstacle and bonus spawn timing
static constexpr int OBSTACLE_DELAY_MAX = 512;
static constexpr int OBSTACLE_DELAY_MIN = 96;
static constexpr int BONUS_DELAY_MAX = 1024;
static constexpr int BONUS_DELAY_MIN = 512;

// ============================================================================
// SPRITE DATA - Original Arduboy bitmaps
// ============================================================================

// Logo sprites
const byte spriteLogoA[] PROGMEM = {
    0x3e, 0x63, 0xc9, 0x9d, 0x39, 0x7b, 0xf2, 0xf6, 0xf4, 0xe4, 0xc8, 0xd8, 0x90, 0xb0, 0xa0, 0x20, 
    0x60, 0xc0, 0x80, 0x80, 0xc0, 0x60, 0x20, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x20, 0x60, 0x40, 
    0x40, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1, 0x7, 0xc, 0x19, 0x73, 0xc7, 0x9f, 0x3f, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xfd, 0xfd, 0xfc, 0xff, 0xff, 0xff, 0x9f, 0xf, 0xf, 0xf, 0xf, 
    0x1f, 0x1f, 0x1e, 0x3c, 0x3d, 0x7d, 0xf9, 0xfb, 0xf2, 0xe6, 0xcc, 0x98, 0x30, 0x60, 0xc0, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0x1f, 0xc3, 0xf8, 
    0xff, 0xff, 0xf3, 0x5, 0x3, 0x45, 0x8b, 0x17, 0xaf, 0x5f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xfe, 0xfe, 0xfc, 0xfc, 0xf8, 0xf8, 0xf0, 0xe0, 0xc0, 0x1, 0x3, 0xf, 0x1f, 0x7f, 0xff, 0xfe, 
    0xf8, 0xf3, 0xc6, 0x1c, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xf0, 0x1f, 0xc0, 0xfe, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xfc, 0xfe, 0xff, 0x7f, 0x3f, 0x9f, 0xf, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xf0, 0x80, 0x00, 
    0x1, 0xf, 0xff, 0xff, 0xff, 0xfc, 0x1, 0xff, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x7, 0x70, 0xfe, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x8f, 0xe3, 0xf9, 0xfc, 0xfe, 0x3f, 0x87, 
    0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0x1f, 0x00, 0x80, 0xf0, 0xff, 0xff, 0xff, 0x1f, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0xc0, 0x41, 0x47, 
    0x4c, 0x49, 0x6b, 0x2b, 0xab, 0xab, 0xbb, 0x99, 0xdd, 0xcc, 0xee, 0xe7, 0xf7, 0xf3, 0xf9, 0xfc, 
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0x9f, 
    0xc7, 0xf0, 0xf8, 0xfe, 0x7f, 0x3f, 0x8f, 0xe7, 0x31, 0x1c, 0x7, 0x00, 0x00, 0x00, 0x00, 0x7, 
    0xc, 0x19, 0x13, 0x37, 0x27, 0x6f, 0xcf, 0x9f, 0xbf, 0xbf, 0x3f, 0x7f, 0x7f, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0xbf, 0xbf, 0xbf, 0x9f, 
    0xcf, 0x67, 0x27, 0x33, 0x19, 0xc, 0x6, 0x3, 0x1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1, 0x1, 0x3, 0x2, 0x2, 
    0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x3, 0x1, 0x1, 0x1, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const byte spriteLogoB[] PROGMEM = {
    0xf, 0x10, 0x10, 0x1f, 0x00, 0x1f, 0x1, 0x1, 0x1e, 0x00, 0x1d, 0x00, 0x1f, 0x11, 0x11, 0x11, 
    0x00, 0xdf, 0x51, 0x51, 0xdf, 0x00, 0xdf, 0x49, 0x49, 0xd6, 0x00, 0xdf, 0x41, 0x41, 0x5e, 0x00, 
    0xc0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7, 0x4, 0x4, 0x3, 0x00, 0x7, 0x2, 0x2, 0x7, 0x00, 0x5, 0x5, 
    0x5, 0x7, 0x00, 0x7, 0x1, 0x1, 0x7
};

// Background sprites
const byte spriteBackgroundA[] PROGMEM = {
    0xc0, 0xf0, 0xf8, 0xfc, 0xfc, 0xfe, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xf8, 0xf0, 0xc0, 0x00, 0xc0, 
    0xe0, 0xf0, 0xf0, 0xf0, 0xf0, 0xe0, 0xc0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 
    0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
};

const byte spriteBackgroundB[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x4, 0x2, 0x2, 0x2, 0x4, 0x18, 0x20, 
    0x10, 0x10, 0x10, 0x20, 0x18, 0x4, 0x2, 0x2, 0x2, 0x4, 0x18, 0x20, 0x10, 0x10, 0x10, 0x20, 
    0x8a, 0x55, 0xa2, 0x54, 0x8a, 0x55, 0xa2, 0x54, 0x8a, 0x55, 0xa2, 0x54, 0x8a, 0x55, 0xa2, 0x54, 
    0x8a, 0x55, 0xa2, 0x54, 0x8a, 0x55, 0xa2, 0x54,
};

const byte spriteMoon[] PROGMEM = {
    0xe0, 0xf8, 0xfc, 0xfe, 0xe, 0x3, 0x1, 0x1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x7, 0x1f, 0x3f, 0x7f, 0x7f, 0xfc, 0xf8, 0xf8, 0xf0, 0xf0, 0xf0, 0x70, 0x78, 0x38, 0x1c, 0x7,
};

// Star animation frames
const byte spriteStar_0[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x8, 0x00, 0x00, 0x00 };
const byte spriteStar_1[] PROGMEM = { 0x00, 0x00, 0x00, 0x8, 0x1c, 0x8, 0x00, 0x00 };
const byte spriteStar_2[] PROGMEM = { 0x00, 0x00, 0x00, 0x8, 0x14, 0x8, 0x00, 0x00 };
const byte spriteStar_3[] PROGMEM = { 0x00, 0x00, 0x8, 0x8, 0x36, 0x8, 0x8, 0x00 };
const byte spriteStar_4[] PROGMEM = { 0x00, 0x00, 0x8, 0x00, 0x2a, 0x00, 0x8, 0x00 };
const byte *animationFramesStar[] = { spriteStar_0, spriteStar_1, spriteStar_2, spriteStar_3, spriteStar_4 };

// Unicorn animation frames
const byte spriteUnicorn_0[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x18, 0x8, 0x7c, 0x1c, 0x1c, 0x1e, 0x7f, 0x2, 0x3, 0x3, 0x00, 0x00, 0x00,
};
const byte spriteUnicorn_1[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 
    0x00, 0x20, 0x30, 0x10, 0x38, 0x78, 0x38, 0x38, 0x38, 0x7c, 0x3e, 0x5, 0x6, 0x6, 0x00, 0x00,
};
const byte spriteUnicorn_2[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0xc0, 0xe0, 0x50, 0x68, 0x64, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x2, 0x3, 0x1, 0xf, 0x3, 0x3, 0x3, 0xf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const byte spriteUnicornMask_0[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0xa0, 0xd0, 0x20, 0x00, 0x00, 
    0x00, 0x10, 0x28, 0x24, 0x74, 0x82, 0x62, 0x22, 0x61, 0x80, 0x7d, 0x4, 0x4, 0x3, 0x00, 0x00,
};
const byte spriteUnicornMask_1[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0xa0, 0x40, 0x00, 
    0x20, 0x50, 0x48, 0x28, 0x44, 0x84, 0x44, 0x44, 0x44, 0x82, 0x41, 0x3a, 0x9, 0x9, 0x6, 0x00,
};
const byte spriteUnicornMask_2[] PROGMEM = {
    0x00, 0x00, 0x00, 0x80, 0x80, 0x40, 0x40, 0x40, 0x20, 0x10, 0xa8, 0x94, 0x9a, 0x64, 0x00, 0x00, 
    0x00, 0x2, 0x5, 0x4, 0xe, 0x10, 0xc, 0x4, 0xc, 0x10, 0xf, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const byte *animationFramesUnicorn[] = { spriteUnicorn_0, spriteUnicorn_1, spriteUnicorn_2 };
const byte *animationFramesUnicornMask[] = { spriteUnicornMask_0, spriteUnicornMask_1, spriteUnicornMask_2 };

// Star and ghost sprites (obstacles)
const byte spriteStar[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x4, 0x6c, 0x7c, 0x3f, 0x37, 0x7c, 0x6c, 0x4, 0x00, 0x00, 0x00, 0x00,
};
const byte spriteStarMask[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x4, 0x6a, 0x92, 0x83, 0x40, 0x48, 0x83, 0x92, 0x6a, 0x4, 0x00, 0x00, 0x00,
};
const byte spriteGhost[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x1e, 0x33, 0x3f, 0x3f, 0x33, 0x3f, 0x3e, 0x18, 0x10, 0x00, 0x00, 0x00,
};
const byte spriteGhostMask[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1e, 0x21, 0x4c, 0x40, 0x40, 0x4c, 0x40, 0x41, 0x26, 0x28, 0x18, 0x00, 0x00,
};
const byte *spritesObject[] = { spriteGhost, spriteStar };
const byte *spritesObjectMask[] = { spriteGhostMask, spriteStarMask };

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Draw a 1-bit Arduboy-style bitmap
void drawBitmap(TFT_eSprite &spr, int x, int y, const byte *bitmap, int w, int h, uint16_t color) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int byteIdx = row * ((w + 7) / 8) + col / 8;
            int bitIdx = 7 - (col % 8);
            bool pixel = (pgm_read_byte(bitmap + byteIdx) >> bitIdx) & 1;
            if (pixel) {
                spr.drawPixel(x + col, y + row, color);
            }
        }
    }
}

// Calculate string length for manual text rendering
int intLength(int i) {
    int j = 0;
    for (; i; i /= 10) j++;
    return (j == 0) ? 1 : j;
}

// ============================================================================
// MAIN APP
// ============================================================================

void APP_DINO(void *parameters) {
    // Static game state variables (persist across loop iterations)
    static bool initialized = false;
    static GameState state = StateIntro;
    static unsigned int counterState = 0;
    static unsigned int counterBackgroundA = 0;
    static unsigned int counterBackgroundB = 0;
    static long score = 0;
    static long scoreHI = 0;
    static int scoreBonusDuration = 0;
    static unsigned long lastFrame = 0;
    static unsigned int frameCount = 0;

    // Game objects
    static GameObject unicorn;
    static GameObject objects[3];
    static Star stars[9];
    static Particle particles[24];

    // Initialization
    if (!initialized) {

        // Initialize stars
        stars[0] = {{2, 13}, 1};
        stars[1] = {{23, 3}, 4};
        stars[2] = {{27, 24}, 2};
        stars[3] = {{42, 7}, 0};
        stars[4] = {{59, 16}, 4};
        stars[5] = {{77, 8}, 1};
        stars[6] = {{92, 21}, 0};
        stars[7] = {{109, 9}, 3};
        stars[8] = {{116, 17}, 0};

        // Initialize unicorn
        unicorn = {{48, GROUND_Y}, {0, 0}, {0, 0, 7, 8}, 0, 7, true};

        // Initialize obstacles and bonuses
        objects[0] = {{-16, GROUND_Y}, {-1, 0}, {0, 0, 8, 8}, 0, 0, true};
        objects[1] = {{-16, GROUND_Y}, {-1, 0}, {0, 0, 8, 8}, 0, 0, true};
        objects[2] = {{-16, 22}, {-1, 0}, {0, 0, 8, 8}, 0, 1, true};

        // Position objects off-screen with random delays
        for (byte i = 0; i < 3; i++) {
            if (objects[i].type == 0) {
                objects[i].pos.x = LOGIC_W + random(OBSTACLE_DELAY_MIN, OBSTACLE_DELAY_MAX);
            } else {
                objects[i].pos.x = LOGIC_W + random(BONUS_DELAY_MIN, BONUS_DELAY_MAX);
            }
        }

        // Reset particles
        for (int i = 0; i < 24; i++) {
            particles[i] = {{0, 0}, 0, 0};
        }

        state = StateIntro;
        score = 0;
        scoreHI = 0;
        counterState = 0;
        initialized = true;
    }

    // ========================================================================
    // MAIN GAME LOOP
    // ========================================================================
    for (;;) {
        // Suspend when app is out of focus (Helen framework pattern)
        if (FOCUSED_APP != _DINO) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        // Handle color/theme changes or app switch (Helen framework pattern)
        if (color_change || just_switched_apps) {
            just_switched_apps = false;
            color_change = false;
            program_frame.resetViewport();
        }

        // Frame timing (30 FPS)
        unsigned long now = millis();
        if (now - lastFrame < 33) {
            vTaskDelay(1);
            continue;
        }
        lastFrame = now;
        frameCount++;

        // Process input events via Helen's text_event_queue
        bool btnA = false;
        bool btnB = false;
        bool btnLeft = false;
        bool btnRight = false;
        bool btnDown = false;

        TextEvent event;
        while (xQueueReceive(text_event_queue, &event, 0) == pdTRUE) {
            if (event.type == KEY_PRESS || event.type == KEY_HOLD) {
                // Map Helen keypad to Arduboy buttons
                // Assuming Helen number keys: 1=A, 2=B, 4=Left, 6=Right, 5=Down (adjust as needed)
                if (event.symbol == "1" || event.symbol == "7") btnA = true;
                if (event.symbol == "2" || event.symbol == "8") btnB = true;
                if (event.symbol == "4") btnLeft = true;
                if (event.symbol == "6") btnRight = true;
                if (event.symbol == "5") btnDown = true;
            }
        }

        // Clear frame and set viewport for centered 128×64 rendering
        program_frame.fillSprite(BG_COLOR);
        int offsetX = (DISPLAY_WIDTH - LOGIC_W) / 2;
        int offsetY = (VIEWPORT_HEIGHT - LOGIC_H) / 2;
        if (offsetY < 0) offsetY = 0;
        program_frame.setViewport(offsetX, offsetY, LOGIC_W, LOGIC_H, false);

        // ====================================================================
        // STATE MACHINE
        // ====================================================================
        switch (state) {
            case StateIntro: {
                counterState++;

                // Draw logo (centered approximation)
                drawBitmap(program_frame, 39, 0, spriteLogoA, 50, 58, FG_COLOR);
                drawBitmap(program_frame, 4, 16, spriteLogoB, 36, 11, FG_COLOR);

                // Draw high score using built-in font
                program_frame.setTextSize(1);
                program_frame.setTextColor(FG_COLOR, BG_COLOR);
                program_frame.setCursor(44, 60);
                for (byte i = (8 - intLength(scoreHI)); i > 0; i--) {
                    program_frame.print("0");
                }
                program_frame.print(scoreHI);

                // A button = credits, B button = play
                if (btnA) {
                    state = StateCredits;
                    counterState = 0;
                } else if (btnB) {
                    state = StatePlay;
                    counterState = 0;
                    score = 0;
                    scoreBonusDuration = 0;
                    // Reset unicorn and objects
                    unicorn = {{48, GROUND_Y}, {0, 0}, {0, 0, 7, 8}, 0, 7, true};
                    for (byte i = 0; i < 3; i++) {
                        if (objects[i].type == 0) {
                            objects[i].pos.x = LOGIC_W + random(OBSTACLE_DELAY_MIN, OBSTACLE_DELAY_MAX);
                        } else {
                            objects[i].pos.x = LOGIC_W + random(BONUS_DELAY_MIN, BONUS_DELAY_MAX);
                        }
                    }
                    for (int i = 0; i < 24; i++) {
                        particles[i] = {{0, 0}, 0, 0};
                    }
                }
                break;
            }

            case StateCredits: {
                if (frameCount % 3 == 0 && counterState <= 100) {
                    counterState += 1;
                }

                program_frame.setTextSize(1);
                program_frame.setTextColor(FG_COLOR, BG_COLOR);
                program_frame.setCursor(43, LOGIC_H - counterState);
                program_frame.print("CREDITS");
                program_frame.setCursor(37, LOGIC_H * 2 - counterState);
                program_frame.print("a game by");
                program_frame.setCursor(19, LOGIC_H * 2 + 10 - counterState);
                program_frame.print("KIRILL KOROLKOV");

                if (btnB) {
                    state = StateIntro;
                    counterState = 0;
                }
                break;
            }

            case StatePause: {
                // Draw paused message
                program_frame.setTextSize(1);
                program_frame.setTextColor(FG_COLOR, BG_COLOR);
                program_frame.setCursor(49, LOGIC_H / 2 - 4);
                program_frame.print("PAUSE");

                // Draw score
                program_frame.setCursor(87, 4);
                for (byte i = (8 - intLength(score)); i > 0; i--) {
                    program_frame.print("0");
                }
                program_frame.print(score);

                if (btnA) {
                    state = StatePlay;
                } else if (btnDown) {
                    state = StateIntro;
                    counterState = 0;
                }
                break;
            }

            case StateOver: {
                // Draw game over message
                program_frame.setTextSize(1);
                program_frame.setTextColor(FG_COLOR, BG_COLOR);
                program_frame.setCursor(37, 18);
                program_frame.print("GAME OVER");

                // Draw ghosts (obstacles)
                for (byte i = 0; i < 3; i++) {
                    if (objects[i].type == 0) {
                        drawBitmap(program_frame, objects[i].pos.x, objects[i].pos.y - 15, 
                                        spritesObjectMask[objects[i].type], 16, 16, BG_COLOR);
                        drawBitmap(program_frame, objects[i].pos.x, objects[i].pos.y - 15, 
                                        spritesObject[objects[i].type], 16, 16, FG_COLOR);
                    }
                }

                // Draw unicorn
                int unicornFrame = 0;
                drawBitmap(program_frame, unicorn.pos.x, unicorn.pos.y - 15, 
                                animationFramesUnicornMask[unicornFrame], 16, 16, BG_COLOR);
                drawBitmap(program_frame, unicorn.pos.x, unicorn.pos.y - 15, 
                                animationFramesUnicorn[unicornFrame], 16, 16, FG_COLOR);

                // Draw score
                program_frame.setCursor(87, 4);
                for (byte i = (8 - intLength(score)); i > 0; i--) {
                    program_frame.print("0");
                }
                program_frame.print(score);

                if (btnA) {
                    if (score > scoreHI) {
                        scoreHI = score;
                    }
                    state = StateIntro;
                    counterState = 0;
                }
                break;
            }

            case StatePlay: {
                counterState++;

                // ============================================================
                // INPUT HANDLING
                // ============================================================
                if (btnLeft && unicorn.action) {
                    unicorn.pos.x -= 1;
                }
                if (btnRight && unicorn.action) {
                    unicorn.pos.x += 1;
                }
                if (btnA) {
                    state = StatePause;
                    break;
                }
                if (btnB && unicorn.action) {
                    unicorn.vel.y = -3;
                    counterState = 0;
                }

                // ============================================================
                // UNICORN PHYSICS
                // ============================================================
                if (counterState % 10 == 0) {
                    unicorn.vel.y += 1;
                }

                unicorn.pos.y = min(unicorn.pos.y + unicorn.vel.y, GROUND_Y);
                unicorn.pos.x = min(unicorn.pos.x + unicorn.vel.x, LOGIC_W - 12);
                unicorn.pos.x = max(unicorn.pos.x + unicorn.vel.x, -4);

                unicorn.box.x = unicorn.pos.x + 4;
                unicorn.box.y = unicorn.pos.y - 9;

                if (unicorn.pos.y >= GROUND_Y) {
                    unicorn.action = true;
                    unicorn.vel.y = 0;
                } else {
                    unicorn.action = false;
                }

                // ============================================================
                // BACKGROUND LAYER 1 - MOON AND STARS
                // ============================================================
                drawBitmap(program_frame, 12, 4, spriteMoon, 16, 16, FG_COLOR);

                for (byte i = 0; i < 9; i++) {
                    if (frameCount % 8 == 0) {
                        stars[i].frame = ((stars[i].frame + 1) > 4) ? 0 : stars[i].frame + 1;
                    }
                    drawBitmap(program_frame, stars[i].pos.x, stars[i].pos.y, 
                                    animationFramesStar[stars[i].frame], 8, 8, FG_COLOR);
                }

                // ============================================================
                // BACKGROUND LAYER 2 - MOUNTAIN SILHOUETTES
                // ============================================================
                if (frameCount % 2 == 0) {
                    counterBackgroundA = ((counterBackgroundA + 1) >= 24) ? 0 : counterBackgroundA + 1;
                }
                for (byte i = 0; i <= 6; i++) {
                    drawBitmap(program_frame, (i * 24) - counterBackgroundA, 28, 
                                    spriteBackgroundA, 24, 36, FG_COLOR);
                }

                // ============================================================
                // BACKGROUND LAYER 3 - GROUND PATTERN
                // ============================================================
                counterBackgroundB = ((counterBackgroundB + 1) >= 24) ? 0 : counterBackgroundB + 1;
                for (byte i = 0; i <= 6; i++) {
                    drawBitmap(program_frame, (i * 24) - counterBackgroundB, 41, 
                                    spriteBackgroundB, 24, 24, BG_COLOR);
                }

                // ============================================================
                // OBJECTS UPDATE
                // ============================================================
                for (byte i = 0; i < 3; i++) {
                    objects[i].pos.x += objects[i].vel.x;
                    objects[i].box.x = objects[i].pos.x + 4;
                    objects[i].box.y = objects[i].pos.y - 8;

                    if (objects[i].pos.x >= -8) {
                        Rect boxA = {unicorn.box.x, unicorn.box.y, unicorn.box.w, unicorn.box.h};
                        Rect boxB = {objects[i].box.x, objects[i].box.y, objects[i].box.w, objects[i].box.h};

                        // Collision detection (using TFT_eSPI's collide equivalent)
                        bool collision = !(boxA.x + boxA.w <= boxB.x || boxB.x + boxB.w <= boxA.x ||
                                         boxA.y + boxA.h <= boxB.y || boxB.y + boxB.h <= boxA.y);

                        if (collision) {
                            if (objects[i].type == 0) {
                                state = StateOver;
                            } else if (objects[i].type == 1) {
                                // Collect bonus
                                objects[i].pos.x = LOGIC_W + random(BONUS_DELAY_MIN, BONUS_DELAY_MAX);
                                if (scoreBonusDuration <= 0) {
                                    for (int p = 0; p < 24; p++) {
                                        particles[p] = {{0, 0}, 0, 0};
                                    }
                                }
                                scoreBonusDuration += 50;
                            }
                        }

                        // Render object
                        drawBitmap(program_frame, objects[i].pos.x, objects[i].pos.y - 15, 
                                        spritesObjectMask[objects[i].type], 16, 16, BG_COLOR);
                        drawBitmap(program_frame, objects[i].pos.x, objects[i].pos.y - 15, 
                                        spritesObject[objects[i].type], 16, 16, FG_COLOR);
                    } else {
                        // Reset object
                        if (objects[i].type == 0) {
                            objects[i].pos.x = LOGIC_W + random(OBSTACLE_DELAY_MIN, OBSTACLE_DELAY_MAX);
                            for (byte j = 0; j < 3; j++) {
                                if (j != i && objects[j].type == 0 && objects[j].pos.x >= LOGIC_W) {
                                    if ((rand() % 4) > 2) {
                                        objects[i].pos.x = objects[j].pos.x + 10;
                                        objects[i].pos.y = GROUND_Y;
                                    } else {
                                        objects[i].pos.x = objects[j].pos.x + random(OBSTACLE_DELAY_MIN, OBSTACLE_DELAY_MAX);
                                        objects[i].pos.y = GROUND_Y - random(0, 12);
                                    }
                                }
                            }
                        } else {
                            objects[i].pos.x = LOGIC_W + random(BONUS_DELAY_MIN, BONUS_DELAY_MAX);
                        }
                    }
                }

                // ============================================================
                // UNICORN ANIMATION AND RENDERING
                // ============================================================
                if (unicorn.action) {
                    if (frameCount % 6 == 0) {
                        unicorn.frame = ((unicorn.frame + 1) > 2) ? 0 : unicorn.frame + 1;
                    }
                } else {
                    unicorn.frame = 0;
                }

                drawBitmap(program_frame, unicorn.pos.x, unicorn.pos.y - 15, 
                                animationFramesUnicornMask[unicorn.frame], 16, 16, BG_COLOR);
                drawBitmap(program_frame, unicorn.pos.x, unicorn.pos.y - 15, 
                                animationFramesUnicorn[unicorn.frame], 16, 16, FG_COLOR);

                // ============================================================
                // PARTICLES (STAR TRAIL)
                // ============================================================
                if (scoreBonusDuration > 0) {
                    for (byte i = 0; i < 24; i++) {
                        particles[i].pos.x -= 1;

                        if (particles[i].lifeCount > particles[i].life) {
                            particles[i].life = 8 + rand() % 32;
                            particles[i].lifeCount = 0;
                            particles[i].pos.y = unicorn.pos.y - 3 + ((rand() % 2 > 0) ? (rand() % 4 * -1) : rand() % 4);
                            particles[i].pos.x = unicorn.pos.x + 4;
                        } else {
                            particles[i].lifeCount += 1;
                        }

                        // Draw particle
                        program_frame.drawPixel(particles[i].pos.x, particles[i].pos.y - 1, BG_COLOR);
                        program_frame.drawPixel(particles[i].pos.x, particles[i].pos.y, FG_COLOR);
                    }
                }

                // ============================================================
                // SCORE UPDATE
                // ============================================================
                if (frameCount % 16 == 0) {
                    scoreBonusDuration = max(0, (scoreBonusDuration - 1));
                    if (scoreBonusDuration > 0) {
                        score += 10;
                    }
                    score = min(99999999L, (score + 1));
                }

                // Draw score
                program_frame.setTextSize(1);
                program_frame.setTextColor(FG_COLOR, BG_COLOR);
                if (score > scoreHI) {
                    program_frame.setCursor(77, 4);
                    program_frame.print("HI");
                    program_frame.setCursor(87, 4);
                } else {
                    program_frame.setCursor(87, 4);
                }
                for (byte i = (8 - intLength(score)); i > 0; i--) {
                    program_frame.print("0");
                }
                program_frame.print(score);

                break;
            }
        }

        // Reset viewport to full screen for status bar rendering
        program_frame.resetViewport();

        // Send frame update event (Helen framework pattern)
        frame_ready();

        // Wait for display draw done (Helen vsync pattern)
        if (VSYNC_ENABLED) xSemaphoreTake(frame_done_sem, portMAX_DELAY);

        vTaskDelay(REFRESH_TIME / portTICK_PERIOD_MS);
    }
}
