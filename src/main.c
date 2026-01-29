#include <gbdk/platform.h>
#include <gbdk/incbin.h>
#include <gbdk/emu_debug.h>
#include <stdint.h>

#include <stdio.h>

#include "common.h"
#include "png_indexed.h"
#include "base64.h"

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

const uint8_t * SRAM_base  = 0xA000u;
const uint8_t * SRAM_upper = 0xB000u;


static void image_to_png_qrcode_url(void);


static void image_to_png_qrcode_url(void) {

    SWITCH_RAM(0u); // RAM bank 0

    // Output buffers in Cart SRAM, no need to allocate them
    uint8_t * p_png_buf        = SRAM_base;
    uint8_t * p_base64_png_buf = SRAM_upper;

    // ===== Conversion to PNG =====
    uint16_t png_buf_sz = png_indexed_init(IMG_8X8_4_COLORS_8BPP_ENCODED_WIDTH,
                                           IMG_8X8_4_COLORS_8BPP_ENCODED_HEIGHT, 
                                           // OUT_BPP_8,     // Current build works, to match  test_8x8_indexed_nocomp_2bpp-encoded.png use 8BPP
                                           OUT_BPP_2,        // Output passes pngcheck and imports to GIMP ok
                                           ARRAY_LEN(img_8x8_4_colors_8bpp_encoded_pal));

    png_indexed_set_buffers(img_8x8_4_colors_8bpp_encoded_pal,
                            img_8x8_4_colors_8bpp_encoded_map,
                            p_png_buf);

    uint16_t png_file_output_sz = png_indexed_encode();
    EMU_printf("PNG out sz=%u\n", png_file_output_sz);


    // ===== PNG encoding to Base64 URL =====
    uint16_t b64_enc_len = base64_encode_to_url(p_base64_png_buf, p_png_buf, png_file_output_sz);

    EMU_printf("B64 out sz=%u\n", (uint16_t)b64_enc_len);
    EMU_printf("B64 out:%s\n", p_base64_png_buf);

}


void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0u); // RAM bank 0

    cpu_fast();
    set_default_palette();

    printf("Starting\n");
    EMU_printf("\nStarting\n");

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


