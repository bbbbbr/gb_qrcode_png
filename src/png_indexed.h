#ifndef PNG_INDEXED_H
#define PNG_INDEXED_H

#include <stdint.h>
#include <stdbool.h>

typedef struct png_data_t {

    // Input vars
    uint8_t   width;
    uint8_t   height;
    uint8_t   in_bpp;
    uint8_t   out_bpp;

    const uint8_t * p_palette_data;
    const uint8_t * p_pixel_color_indexes;  // TODO: RENAME: rename p_pixelColorIndexes -> todo done?
    uint16_t        palette_data_byte_len;  // Max size is presumably 256 * 3

    // Computed vars
    uint16_t        zlib_pixel_rows_max_size;
    uint16_t        file_max_size;
    bool            calc_initialized;

    // This var gets set after Computed vars are returned and a buffer is allocated
    uint8_t       * p_png_out_buf;
    bool            buffers_initialized;
} png_data_t;


// Call this first to initialize, use the returned value to allocate a buffer to build the png inside of
uint16_t png_indexed_init(uint8_t width, uint8_t height, uint8_t out_bpp, uint16_t palette_data_byte_len) BANKED;

// Sets the working buffers (note lack of size checking)
void png_indexed_set_buffers(uint8_t * p_img_palette_data, uint8_t * p_img_pixel_color_indexes, uint8_t * p_png_out_buf) BANKED;

// Builds the png file data into the provided buffer
// png_indexed_init() and png_indexed_set_buffers() should be called first
uint16_t png_indexed_encode(void) BANKED;

#endif // PNG_INDEXED_H