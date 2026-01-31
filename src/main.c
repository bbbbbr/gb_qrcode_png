#include <gbdk/platform.h>
#include <stdint.h>

#include "common.h"
#include "input.h"

#include "img_2_qrcode.h"
#include "draw.h"


void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0u); // RAM bank 0

    if (_cpu == CGB_TYPE) {
        cpu_fast();
        set_default_palette();
    }

    UPDATE_KEYS();

    // printf("Starting\n");
    // EMU_printf("\nStarting\n");

    draw_init();

    // Loop forever
    while(1) {

        UPDATE_KEYS();
        draw_update();

        if (KEYS() & J_START) {
            // TODO: Save the drawing (VRAM) to cart SRAM and then restore it after QRcode is done
            drawing_save_to_sram(SRAM_BANK_1);
            image_to_png_qrcode_url();

            // Wait for the user to press a button before clearing QRCode
            waitpadticked_lowcpu(J_ANY);
            waitpadup();

            drawing_restore_from_sram(SRAM_BANK_1);
            drawing_restore_default_colors();
            // redraw_workarea();
            SHOW_SPRITES;
        }

        vsync();
    }
}


