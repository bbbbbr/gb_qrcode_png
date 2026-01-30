#include <gbdk/platform.h>
#include <stdint.h>

#include <stdio.h>

#include "common.h"
#include "img_2_qrcode.h"


void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0u); // RAM bank 0

    cpu_fast();
    set_default_palette();

    // printf("Starting\n");
    // EMU_printf("\nStarting\n");

    vsync();
    SHOW_BKG;

    image_to_png_qrcode_url();

    // Loop forever
    while(1) {

        // Main loop processing goes here
        // Done processing, yield CPU and wait for start of next frame
        vsync();
    }
}


