#include <gbdk/platform.h>
#include <gb/drawing.h>

#include <gbdk/emu_debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "common.h"
#include "qrcodegen.h"

#pragma bank 255  // Autobanked


// See qrcodegen.h for setting the QR code version/capacity

#define SCALE 1  // pixel size multiplier for output rendering

// static void qr_render_scale_1_apa(void);
// static void qr_render_scale_1_apa_direct_access(void);
// static void qr_render_scale_N_apa(void);

static void qr_render_scale_1_1bpp_direct(void);


bool qr_generate(const char * embed_str, uint16_t len) BANKED {

    if (len > QR_MAX_PAYLOAD_BYTES) {
        // Optionally emit an error here that size was too large
        EMU_printf("Input size too large! %d > %d\n", (uint16_t)len, (uint16_t)QR_MAX_PAYLOAD_BYTES);
        return false;
    }

    // Bank switching is needed for non-direct buffer access renders,
    // which allows this source file to be auto-banked.
    //
    // If using any of the *_apa() rendering modes then REMOVE this file
    // from auto-banking (due to use of qr_get() which may be in another bank)
    //
    // uint8_t save_bank = CURRENT_BANK;
    // SWITCH_ROM(BANK(qrcodegen));

    EMU_PROFILE_BEGIN(" QRCode Gen prof start ");
    qrcodegen(embed_str, len);
    EMU_PROFILE_END(" QRCode Gen prof end: ");

    // SWITCH_ROM(save_bank);

    return true;
}


void qr_render(void) BANKED {

    EMU_PROFILE_BEGIN(" QRCode Render prof start ");
    // Bank switching is needed for non-direct buffer access renders,
    // which allows this source file to be auto-banked.
    //
    // If using any of the *_apa() rendering modes then REMOVE this file
    // from auto-banking (due to use of qr_get() which may be in another bank)
    //    
    // uint8_t save_bank = CURRENT_BANK;
    // SWITCH_ROM(BANK(qrcodegen));

    #if SCALE == 1
        // qr_render_scale_1_apa();
        // qr_render_scale_1_apa_direct_access();
        qr_render_scale_1_1bpp_direct();
    #else
        qr_render_scale_N_apa();
    #endif

    EMU_PROFILE_END(" QRCode Render prof end: ");
    // SWITCH_ROM(save_bank);
}



extern uint8_t QRCODE[];
extern const uint8_t qr_bitmask[];

// // Mirror bits
const uint8_t mirror_bits[256] = {
    0x00u,0x80u,0x40u,0xC0u,0x20u,0xA0u,0x60u,0xE0u,0x10u,0x90u,0x50u,0xD0u,0x30u,0xB0u,0x70u,0xF0u,
    0x08u,0x88u,0x48u,0xC8u,0x28u,0xA8u,0x68u,0xE8u,0x18u,0x98u,0x58u,0xD8u,0x38u,0xB8u,0x78u,0xF8u,
    0x04u,0x84u,0x44u,0xC4u,0x24u,0xA4u,0x64u,0xE4u,0x14u,0x94u,0x54u,0xD4u,0x34u,0xB4u,0x74u,0xF4u,
    0x0Cu,0x8Cu,0x4Cu,0xCCu,0x2Cu,0xACu,0x6Cu,0xECu,0x1Cu,0x9Cu,0x5Cu,0xDCu,0x3Cu,0xBCu,0x7Cu,0xFCu,
    0x02u,0x82u,0x42u,0xC2u,0x22u,0xA2u,0x62u,0xE2u,0x12u,0x92u,0x52u,0xD2u,0x32u,0xB2u,0x72u,0xF2u,
    0x0Au,0x8Au,0x4Au,0xCAu,0x2Au,0xAAu,0x6Au,0xEAu,0x1Au,0x9Au,0x5Au,0xDAu,0x3Au,0xBAu,0x7Au,0xFAu,
    0x06u,0x86u,0x46u,0xC6u,0x26u,0xA6u,0x66u,0xE6u,0x16u,0x96u,0x56u,0xD6u,0x36u,0xB6u,0x76u,0xF6u,
    0x0Eu,0x8Eu,0x4Eu,0xCEu,0x2Eu,0xAEu,0x6Eu,0xEEu,0x1Eu,0x9Eu,0x5Eu,0xDEu,0x3Eu,0xBEu,0x7Eu,0xFEu,
    0x01u,0x81u,0x41u,0xC1u,0x21u,0xA1u,0x61u,0xE1u,0x11u,0x91u,0x51u,0xD1u,0x31u,0xB1u,0x71u,0xF1u,
    0x09u,0x89u,0x49u,0xC9u,0x29u,0xA9u,0x69u,0xE9u,0x19u,0x99u,0x59u,0xD9u,0x39u,0xB9u,0x79u,0xF9u,
    0x05u,0x85u,0x45u,0xC5u,0x25u,0xA5u,0x65u,0xE5u,0x15u,0x95u,0x55u,0xD5u,0x35u,0xB5u,0x75u,0xF5u,
    0x0Du,0x8Du,0x4Du,0xCDu,0x2Du,0xADu,0x6Du,0xEDu,0x1Du,0x9Du,0x5Du,0xDDu,0x3Du,0xBDu,0x7Du,0xFDu,
    0x03u,0x83u,0x43u,0xC3u,0x23u,0xA3u,0x63u,0xE3u,0x13u,0x93u,0x53u,0xD3u,0x33u,0xB3u,0x73u,0xF3u,
    0x0Bu,0x8Bu,0x4Bu,0xCBu,0x2Bu,0xABu,0x6Bu,0xEBu,0x1Bu,0x9Bu,0x5Bu,0xDBu,0x3Bu,0xBBu,0x7Bu,0xFBu,
    0x07u,0x87u,0x47u,0xC7u,0x27u,0xA7u,0x67u,0xE7u,0x17u,0x97u,0x57u,0xD7u,0x37u,0xB7u,0x77u,0xF7u,
    0x0Fu,0x8Fu,0x4Fu,0xCFu,0x2Fu,0xAFu,0x6Fu,0xEFu,0x1Fu,0x9Fu,0x5Fu,0xDFu,0x3Fu,0xBFu,0x7Fu,0xFFu
};


#define QR_TILE_X_START  1u
#define QR_TILE_Y_START  0u

#define QR_WIDTH_TILES      ((QRSIZE + (TILE_SZ_PX - 1u)) / TILE_SZ_PX)
#define QR_HEIGHT_TILES     ((QRSIZE + (TILE_SZ_PX - 1u)) / TILE_SZ_PX)
//
// Profiling:
//   Needs horizontal tile mirroring:  _qr_render       398658
//   +Mirror bits +border fixup:       _qr_render       456126
//   +Right edge border fixups w/box:  _qr_render       581760
//   +Right edge border fixups w/mask: _qr_render       485816 <-- Current version
//
// Note: Read bytes directly from QRCode output into tiles
// - Image being aligned to tiles horizontally
// - Display tiles arranged for APA mode
//
static void qr_render_scale_1_1bpp_direct(void) {

    DISPLAY_OFF;
    const uint8_t * p_qr_src_buf = QRCODE;

    // Start at first tile in vram
    // APA mode layout is 20 tiles wide x 18 tiles tall, starting at 0x8100
    // Offset to the starting tile of where to draw the QRCode
    uint8_t * p_vram = APA_MODE_VRAM_START + (((QR_TILE_Y_START * DEVICE_SCREEN_WIDTH) + QR_TILE_X_START) * TILE_SZ_BYTES);

    // Clear VRAM, this render will only be writing very other byte due to 1bpp source format
    memset(APA_MODE_VRAM_START, 0u, APA_MODE_VRAM_SZ);

    // Calculate a tile mask to fix up stray pixels on the right edge of the QRcode when it's width isn't an even multiple of 8 (tile width)
    const uint8_t right_edge_tile_row_mask = ~((1u << (8u - (QRSIZE % 8u))) - 1u);

    // The -2 is to rewind but then step down to the next 2bpp row in the tile
    const uint16_t next_line_row_rewind = (QR_WIDTH_TILES * TILE_SZ_BYTES) - 2u;

    // +2 is to wrap to the first bpp row of the next tile from the 1bpp end of the last tile in the row
    // -1 tiles is for after the end of a tile row it will be pointing one tile into the rowstride area (and so 1 needs to be skipped)
    const uint16_t next_row_of_tiles    = (((DEVICE_SCREEN_WIDTH - QR_WIDTH_TILES) -1u) * TILE_SZ_BYTES) + 2u;

    for (uint8_t tile_y = 0; tile_y < QR_WIDTH_TILES; tile_y++) {

        // Step through a row of tiles (i.e. N tiles wide x 8 pixels tall)
        uint8_t tile_height;
        for (tile_height = 0; tile_height < TILE_SZ_PX; tile_height++) {

            // Steps across row N of each adjacent tile, picking off the first 1bpp byte
            uint8_t tile_x;
            for (tile_x = 0; tile_x < QR_WIDTH_TILES; tile_x++) {

                // Need to horizontally mirror tile bits to convert QRCode format into to GB Tile format
                *p_vram = mirror_bits[*p_qr_src_buf++];
                p_vram += TILE_SZ_BYTES;
            }
            // Fix up rightmost edge of QRCode when it's width isn't an even multiple of 8 (tile width size)
            // Alternative to the box() version below that's apparently much slower
            *(p_vram - TILE_SZ_BYTES) &= right_edge_tile_row_mask;

            // Go to start of next scanline within the 8 pixel tall tile row
            // except on the very last tile row (to make calc to next tile row easier)
            if (tile_height != (TILE_SZ_PX - 1u))
                p_vram -= next_line_row_rewind;
        }
        // Wrap around to start of next line in current row of tiles
        p_vram += next_row_of_tiles;
    }

    // Border box and some edge fixups on the right side
    //
    // Slower alternative to the right_edge_tile_row_mask version above
    // color(WHITE,WHITE,SOLID);
    // box(QRSIZE, 0, QRSIZE+2, QR_FINAL_PIXEL_HEIGHT, M_FILL);
    //
    // Lil' Hacky
    // Tile on screen to the right outside of the QRCode at size 31 which we know will be blank
    // and is in the common "middle" apa tileset in VRAM
    const uint8_t white_tile_index = 134u;
    // Now fix up the -1,-1 screen scroll wraparound border area tiles with the white tile
    fill_bkg_rect(DEVICE_SCREEN_BUFFER_WIDTH - 1u, 0u,  1u,  DEVICE_SCREEN_BUFFER_HEIGHT, white_tile_index);
    fill_bkg_rect(0u,  DEVICE_SCREEN_BUFFER_HEIGHT - 1u,  DEVICE_SCREEN_BUFFER_WIDTH,  1u, white_tile_index);

    DISPLAY_ON;
}

/*
// About 30% faster than qr_render_scale_1_apa which uses qr_get():
 // Initial                _qr_render_scale_1_apa_direct_access      12,216,762
//  +Border and +1 offset: _qr_render_scale_1_apa_direct_access      13,666,004
//  +Display Off/On        _qr_render_scale_1_apa_direct_access      11,237,754
static void qr_render_scale_1_apa_direct_access(void) {
    
    DISPLAY_OFF;

    // Border box and some edge fixups on the right and bottom
    color(WHITE,WHITE,SOLID);
    box(0u,0u,QR_FINAL_PIXEL_WIDTH - 1u, QR_FINAL_PIXEL_HEIGHT -1u, M_NOFILL);
    // line(QR_FINAL_PIXEL_WIDTH, 0, QR_FINAL_PIXEL_WIDTH, QR_FINAL_PIXEL_HEIGHT,  M_NOFILL);

    for (uint8_t x = 0u; x < (QRSIZE); x++) {
        for (uint8_t y = 0u; y < (QRSIZE); y++) {

            uint8_t pixel_is_black = QRCODE[y * QR_OUTPUT_ROW_SZ_BYTES + (x>>3)] & qr_bitmask[x];

            if (pixel_is_black)
                color(BLACK,BLACK,SOLID);
            else
                color(WHITE,WHITE,SOLID);

            plot_point(x+1, y+1);
        }
    }
    DISPLAY_ON;
}


static void qr_render_scale_1_apa(void) {
    for (uint8_t x = 0; x < (QR_FINAL_PIXEL_WIDTH); x++) {
        for (uint8_t y = 0; y < (QR_FINAL_PIXEL_HEIGHT); y++) {

            if (qr(x,y))                
                color(WHITE,WHITE,SOLID);
            else
                color(BLACK,BLACK,SOLID);

            plot_point(x, y);
        }
    }
}


static void qr_render_scale_N_apa(void) {
    for (uint8_t x = 0; x < (QR_FINAL_PIXEL_WIDTH); x++) {
        for (uint8_t y = 0; y < (QR_FINAL_PIXEL_HEIGHT); y++) {
            if (qr(x,y))                
                color(WHITE,WHITE,SOLID);
            else
                color(BLACK,BLACK,SOLID);

            uint8_t x1 = x * SCALE;
            uint8_t y1 = y * SCALE;
            box(x1, y1, x1 + (SCALE - 1), y1 + (SCALE - 1), M_FILL);
        }
    }
}
*/