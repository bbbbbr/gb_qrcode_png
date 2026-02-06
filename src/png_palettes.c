#include <gbdk/platform.h>
#include <stdint.h>

#include "png_indexed.h"
#include "common.h"

#define PAL_1BPP_NUM_COLORS 2u
#define PAL_2BPP_NUM_COLORS 4u

#define PAL_RGB888_SZ       3u

// Note: DO NOT BANK! Needs to be accessible from PNG building code which is banked

// Mainly for testing
// const uint8_t img_8x8_4_colors_8bpp_encoded_pal[PAL_2BPP_NUM_COLORS * PNG_PAL_RGB888_SZ] = {
//     0x00u, 0xFFu, 0x00u,  // Green
//     0xFFu, 0xFFu, 0xFFu,  // White
//     0x00u, 0x00u, 0xFFu,  // Blue
//     0xFFu, 0x00u, 0x00u,  // Red
// };

const uint8_t pal_1bpp_white_black[PAL_1BPP_NUM_COLORS * PAL_RGB888_SZ] = {
    0xFFu, 0xFFu, 0xFFu,  // White
    0x00u, 0x00u, 0x00u,  // Black
};

const uint16_t pal_1bpp_white_black_sz = ARRAY_LEN(pal_1bpp_white_black);