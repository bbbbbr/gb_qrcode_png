#include <gbdk/platform.h>
#include <gb/drawing.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <gbdk/emu_debug.h>

#include "qrcodegen.h"

// See qrcodegen.h for setting the QR code version/capacity

#define SCALE 1  // pixel size multiplier for output rendering

bool qr_generate(const char * embed_str, uint16_t len) NONBANKED {

    if (len > QR_MAX_PAYLOAD_BYTES) {
        // Optionally emit an error here that size was too large
        EMU_printf("Input size too large! %d > %d\n", (uint16_t)len, (uint16_t)QR_MAX_PAYLOAD_BYTES);
        return false;
    }

    uint8_t save_bank = CURRENT_BANK;
    SWITCH_ROM(BANK(qrcodegen));

    qrcodegen(embed_str, len);

    SWITCH_ROM(save_bank);

    return true;
}


void qr_render(void) NONBANKED {

    uint8_t save_bank = CURRENT_BANK;
    SWITCH_ROM(BANK(qrcodegen));

    #ifdef SCALE == 1
        for (uint8_t x = 0; x < (QR_FINAL_PIXEL_WIDTH); x++) {
            for (uint8_t y = 0; y < (QR_FINAL_PIXEL_HEIGHT); y++) {
                if (qr(x,y))                
                    color(WHITE,WHITE,SOLID);
                else
                    color(BLACK,BLACK,SOLID);

                plot_point(x, y);
            }
        }
    #else
        for (uint8_t x = 0; x < (QR_FINAL_PIXEL_WIDTH); x++) {
            for (uint8_t y = 0; y < (QR_FINAL_PIXEL_HEIGHT); y++) {
                if (qr(x,y))                
                    color(WHITE,WHITE,SOLID);
                else
                    color(BLACK,BLACK,SOLID);

                // If scaling isn't needed (i.e SCALE is 1) then this is faster:
                // plot_point(x, y);

                uint8_t x1 = x * SCALE;
                uint8_t y1 = y * SCALE;
                box(x1, y1, x1 + (SCALE - 1), y1 + (SCALE - 1), M_FILL);
            }
        }
    #endif

    SWITCH_ROM(save_bank);    
}
