#include <gbdk/platform.h>
#include <gbdk/incbin.h>
#include <stdint.h>

#include <stdio.h>

#include "common.h"
#include "png_indexed.h"

#define OUT_BPP_1  1u
#define OUT_BPP_2  2u
#define OUT_BPP_4  4u
#define OUT_BPP_8  8u
// #define RGB888_SZ  3u  // 3 bytes

#define IMG_8X8_4_COLORS_8BPP_ENCODED_WIDTH  8u
#define IMG_8X8_4_COLORS_8BPP_ENCODED_HEIGHT 8u

const uint8_t img_8x8_4_colors_8bpp_encoded_pal[] = {
    0x00u, 0xFFu, 0x00u,  // Green
    0xFFu, 0xFFu, 0xFFu,  // White
    0x00u, 0x00u, 0xFFu,  // Blue
    0xFFu, 0x00u, 0x00u,  // Red
};
const uint8_t img_8x8_4_colors_8bpp_encoded_map[] = {
     // 0x78u, 0x01u, 
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x02u, 0x03u, 0x00u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
     /* 0x01u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x03u,
     // 0x09u, 0xD8u, 0x00u, 0x45u, 
};



void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0u); // RAM bank 0

    cpu_fast();
    set_default_palette();

    printf("Starting\n");

    vsync();
    SHOW_BKG;

    uint16_t png_buf_sz = png_indexed_init(IMG_8X8_4_COLORS_8BPP_ENCODED_WIDTH,
                                           IMG_8X8_4_COLORS_8BPP_ENCODED_HEIGHT, 
                                           // OUT_BPP_8,     // Current build works, to match  test_8x8_indexed_nocomp_2bpp-encoded.png use 8BPP
                                           OUT_BPP_2,        // Output passes pngcheck and imports to GIMP ok
                                           ARRAY_LEN(img_8x8_4_colors_8bpp_encoded_pal));

    // Put the output buffer in Cart SRAM, no need to allocate it
    // png_out_buf = malloc(png_buf_sz);
    uint8_t * p_png_out_buf = (uint8_t *)0xA000u;

    png_indexed_set_buffers(img_8x8_4_colors_8bpp_encoded_pal,
                            img_8x8_4_colors_8bpp_encoded_map,
                            p_png_out_buf);

    uint16_t png_file_output_sz = png_indexed_encode();

    printf("Done\nOut Size=%u", png_file_output_sz);

    // Loop forever
    while(1) {

        // Main loop processing goes here
        // Done processing, yield CPU and wait for start of next frame
        vsync();
    }
}


