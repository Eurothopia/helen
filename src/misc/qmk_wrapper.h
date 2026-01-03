#include "Arduino.h"
#include <global.h>

#define OLED_W 32
#define OLED_H 128

void p_write(const char *src, size_t size) {
    int x_offset=(DISPLAY_WIDTH-(OLED_H*a_scale))/2;
    int y_offset=(VIEWPORT_HEIGHT-STATUS_BAR_HEIGHT-(OLED_W*a_scale)+20)/2;
    const uint8_t *buf = (const uint8_t *)src;

    for (int y = 0; y < OLED_H; y++) {
        for (int x = 0; x < OLED_W; x++) {

            int byteIndex = (y / 8) * OLED_W + x;
            uint8_t byte = buf[byteIndex];
            bool bit = byte & (1 << (y & 7));

            // --- 90° CLOCKWISE ROTATION ---
            int rx = OLED_H - 1 - y;
            int ry = x;
            
            
            //program_frame.drawPixel(rx, ry, bit ? FG_COLOR : BG_COLOR);
            program_frame.drawRect(rx*a_scale+x_offset,ry*a_scale+y_offset, a_scale, a_scale, bit ? FG_COLOR : BG_COLOR);
        }
    }
}


uint16_t timer_read() { return (uint16_t)millis(); }
uint16_t timer_elapsed(uint16_t start) { return (uint16_t)(millis() - start); }