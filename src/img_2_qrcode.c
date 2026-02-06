#include <gbdk/platform.h>
#include <gbdk/incbin.h>
#include <gbdk/emu_debug.h>
#include <gb/drawing.h>
#include <stdint.h>

// #include <stdio.h>

#pragma bank 255  // Autobanked

#include "common.h"

#include "png_indexed.h"
#include "png_palettes.h"
#include "base64.h"
#include "qr_wrapper.h"



// ===== START PNG TEST IMAGE =====
#define IMG_8X8_4_COLORS_8BPP_ENCODED_WIDTH  8u
#define IMG_8X8_4_COLORS_8BPP_ENCODED_HEIGHT 8u


// Reference 8x8 Test PNG image
//
// const uint8_t img_8x8_4_colors_8bpp_encoded_map[] = {
//      // 0x78u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x02u, 0x03u, 0x00u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x00u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u,
//      /* 0x01u, 0x09u, 0x00u, 0xF6u, 0xFFu, 0x00u, */  0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x01u, 0x03u,
//      // 0x09u, 0xD8u, 0x00u, 0x45u,
// };

// ===== END PNG TEST IMAGE =====


// Note: Optimized display capture/read -> png relies:
// - Image being aligned to tiles horizontally
// - Display tiles arranged for APA mode
static uint16_t copy_1bpp_image_from_vram(uint8_t * p_out_buf) {

    DISPLAY_OFF;
    const uint8_t * p_out_buf_start = p_out_buf;

    // Start at first tile in vram
    // APA mode layout is 20 tiles wide x 18 tiles tall, starting at 0x8100
    uint8_t *   p_vram = APA_MODE_VRAM_START + (((IMG_TILE_Y_START * DEVICE_SCREEN_WIDTH) + IMG_TILE_X_START) * TILE_SZ_BYTES);

    // The -2 is to rewind but then step down to the next 2bpp row in the tile
    const uint16_t next_line_row_rewind = (IMG_WIDTH_TILES * TILE_SZ_BYTES) - 2u;

    // +2 is to wrap to the first bpp row of the next tile from the 1bpp end of the last tile in the row
    // -1 tiles is for after the end of a tile row it will be pointing one tile into the rowstride area (and so 1 needs to be skipped)
    const uint16_t next_row_of_tiles    = (((DEVICE_SCREEN_WIDTH - IMG_WIDTH_TILES) -1u) * TILE_SZ_BYTES) + 2u;


    for (uint8_t tile_y = 0; tile_y < IMG_WIDTH_TILES; tile_y++) {

        // Step through a row of tiles (i.e. N tiles wide x 8 pixels tall)
        uint8_t tile_height;
        for (tile_height = 0; tile_height < TILE_SZ_PX; tile_height++) {

            // Steps across row N of each adjacent tile, picking off the first 1bpp byte
            uint8_t tile_x;
            for (tile_x = 0; tile_x < IMG_WIDTH_TILES; tile_x++) {
                *p_out_buf++ = *p_vram;
                p_vram += TILE_SZ_BYTES;
            }
            // Go to start of next scanline within the 8 pixel tall tile row
            // except on the very last tile row (to make calc to next tile row easier)
            if (tile_height != (TILE_SZ_PX - 1u))
                p_vram -= next_line_row_rewind;
        }
        // Wrap around to start of next line in current row of tiles
        p_vram += next_row_of_tiles;
    }

    DISPLAY_ON;

    uint16_t len = p_out_buf - p_out_buf_start;
    EMU_printf(" Image: len=%u, end=%x\n", (uint16_t)len, (uint16_t)p_out_buf);

    return len;
}


void image_to_png_qrcode_url(void) BANKED {

    SWITCH_RAM(SRAM_BANK_CALC_BUFFER);

    // Output buffers in Cart SRAM, no need to allocate them
    uint8_t * p_img_1bpp_buf       = (uint8_t *)SRAM_UPPER_B000; // Gets overwritten by Base64 encoded image
    uint8_t * p_png_buf            = (uint8_t *)SRAM_BASE_A000;
    uint8_t * p_base64_png_url_str = (uint8_t *)SRAM_UPPER_B000;

    // ===== Prepare image buffer =====
    EMU_printf("Generating Image\n");
    // Read image from screen apa graphics
    // The img_1bpp_sz result is not used right now, it's assumed it matches ((width * height) packed at 1bpp) 
    uint16_t img_1bpp_sz = copy_1bpp_image_from_vram(p_img_1bpp_buf);


    // ===== Conversion to PNG =====
    // printf("Generating PNG\n");
    EMU_printf("Generating PNG\n");
    // uint16_t png_buf_sz = png_indexed_init(IMG_8X8_4_COLORS_8BPP_ENCODED_WIDTH,
    //                                        IMG_8X8_4_COLORS_8BPP_ENCODED_HEIGHT,
    //                                        // PNG_BPP_8,     // Current build works, to match  test_8x8_indexed_nocomp_2bpp-encoded.png use 8BPP
    //                                        PNG_BPP_2,        // Output passes pngcheck and imports to GIMP ok
    //                                        ARRAY_LEN(img_8x8_4_colors_8bpp_encoded_pal));
    uint16_t png_buf_sz = png_indexed_init(IMG_WIDTH_PX, IMG_HEIGHT_PX, SRC_BPP_1, PNG_BPP_1, pal_1bpp_white_black_sz);
    png_indexed_set_buffers(pal_1bpp_white_black, p_img_1bpp_buf, p_png_buf);

    uint16_t png_file_output_sz = png_indexed_encode();
    EMU_printf("PNG out sz=%u\n", png_file_output_sz);


    // ===== PNG encoding to Base64 URL =====
    // printf("Base 64 Encode PNG\n");
    uint16_t b64_enc_len = base64_encode_to_url(p_base64_png_url_str, p_png_buf, png_file_output_sz);

    EMU_printf("B64 out sz=%u\n", (uint16_t)b64_enc_len);
    EMU_printf("B64 out:%s\n", p_base64_png_url_str);


    // TODO: move buffer to SRAM? Possibly overwrite previously generated PNG file output

    // ===== PNG encoding to Base64 URL =====
    // QR Code is generated in byte mode because:
    // - Ported C implementation doesn't support other modes
    // - Alphanumeric mode character set doesn't include all chars needed for base64 encoded strings and mime header chars (;)
    //
    gotogxy(5u,4u);
    gprintf("Generating");
    gotogxy(5u,5u);
    gprintf("QR Code");

    EMU_printf("Generating QR Code\n");
    if (qr_generate(p_base64_png_url_str, b64_enc_len) ) {
        EMU_printf("Rendering QR Code\n");
        qr_render();
    } else {
        EMU_printf("QR Code gen Error\n");
    }
    HIDE_SPRITES;
}
