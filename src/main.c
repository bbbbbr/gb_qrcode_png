#include <gbdk/platform.h>
#include <stdint.h>

#include "common.h"
#include "input.h"

#include "img_2_qrcode.h"


void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0u); // RAM bank 0

    cpu_fast();
    set_default_palette();

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
            image_to_png_qrcode_url();
            waitpadticked_lowcpu(J_ANY);
            // return_to_drawing_mode();
            redraw_workarea();
            SHOW_SPRITES;
        }

        vsync();
    }
}


