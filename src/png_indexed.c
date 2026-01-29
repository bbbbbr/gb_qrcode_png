#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>  // For memcopy()

#include <stdio.h>

#include <gbdk/emu_debug.h>

#include "common.h"
#include "png_indexed.h"

#pragma bank 255  // Autobanked

// == Uncompressed Indexed PNG Export ==


// The PNG uncompressed indexed color file structure in this implementation:
//
// Only 1x of each PNG chunk type is used
//
//  - Signature (8 bytes)
//  - IHDR chunk
//  - PLTE chunk [Palette data]
//  - IDAT chunk [Indexed Pixel dat]
//    - IDAT payload length (4 bytes)
//    - IDAT chunk type     (4 bytes)
//    - IDAT Payload
//      - Zlib header (2 bytes)
//      - Deflate chunks [One per pixel row]
//        - Final/Non-final indicator (1 byte)
//        - Deflate Length            (2 bytes)
//        - Deflate Length xor FF     (2 bytes)
//          - PNG single Scanline of Row data
//            - Row start filter[0 for none] (1 bytes)
//            - Indexed Pixel Row Data       (width's worth of bytes)
//      - Zlib Adler checksum (4 bytes)
//    - IDAT CRC-32 of chunk type and payload (4 bytes)
//  - IEND chunk


#define PNG_PAL_RGB888_SZ                   3u
#define PNG_PAL_RGBA8888_SZ                 4u

#define PNG_BIT_DEPTH_8                     8u
#define PNG_COLOR_TYPE_INDEXED              3u
#define PNG_COMPRESSION_METHOD_DEFLATE_NONE 0u
#define PNG_FILTER_METHOD_NONE              0u
#define PNG_INTERLACING_NONE                0u

#define PNG_CHUNK_LENGTH_SZ                 4u
#define PNG_CHUNK_TYPE_SZ                   4u
#define PNG_CHUNK_CHECKSUM_SZ               4u
#define PNG_CHUNK_OVERHEAD                  (PNG_CHUNK_LENGTH_SZ + PNG_CHUNK_TYPE_SZ + PNG_CHUNK_CHECKSUM_SZ)
#define PNG_SIGNATURE_SZ                    8u
#define PNG_IHDR_SZ                         13u
#define PNG_IEND_SZ                         0u

#define PNG_IDAT_BYTE_SZ                    8192u

#define PNG_ROW_FILTER_TYPE_SZ              1u
#define PNG_ROW_FILTER_TYPE_NONE            0u

// #define PNG_EXPORT_SUPPORTED_MAX_WIDTH      65535u

#define ZLIB_HEADER_SZ                      2u    // CMF, FLG
#define ZLIB_FOOTER_SZ                      4u    // 4 byte Adler checksum
#define ZLIB_HEADER_CMF                     0x78u // CMF: Compression Method: No Compression
#define ZLIB_HEADER_FLG                     0x01u // FLG: Flags: FCHECK[4..0], No DICT[5], FLEVEL 0[7..6] (fastest)

#define DEFLATE_HEADER_FINAL_NO             0u    // BTYPE Uncompressed, not final
#define DEFLATE_HEADER_FINAL_YES            1u    // BTYPE Uncompressed, final
#define DEFLATE_HEADER_SZ                   5u    // 1 byte Is Final block, 2 bytes Length, 2 bytes

#define NO_DATA_COPY                        NULL

const uint8_t png_signature[] = {0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au};

static const uint32_t crc32Table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



static png_data_t png;

static uint32_t zlib_adler_a;
static uint32_t zlib_adler_b;



static uint16_t calc_zlib_pixel_data_size(uint8_t width, uint8_t height, const uint8_t bpp);
static uint16_t calc_idat_payload_start_offset(uint8_t palette_data_byte_len);
static uint16_t calc_max_file_size(uint8_t palette_data_byte_len, uint16_t zlibPixelRows_maxSize);

static uint8_t * write_u16_le(uint8_t * outBuffer, uint16_t value);
static uint8_t * write_u16_be(uint8_t * outBuffer, uint16_t value);
static uint8_t * write_u32_be(uint8_t * outBuffer, uint32_t value);

static void adler_reset(void);
static void adler_crc_update(uint8_t * buffer, uint16_t buffer_sz);

static uint32_t crc32(const uint8_t * p_buffer, uint16_t buffer_sz);
static uint8_t * png_write_chunk(uint8_t * p_out_buf, const char * type, const uint8_t * p_payload, const uint16_t payload_sz);

static uint16_t prepare_pixel_data(void);


// TODO: IMPLEMENT: conversion to base64
// function uint8ToBase64(arr) {
//     return btoa(Array(arr.length)
//         .fill("")
//         .map((_, i) => String.fromCharCode(arr[i]))
//         .join(""));
// }




// Expects:
// - palette_data_byte_len: size of palette data array in RGB888 format (so, 4 colors = 4 * 3 = 12)
// - bpp:                Must be 1, 2, 4 or 8
// - width & height:     8 bit only for now
uint16_t png_indexed_init(uint8_t width, uint8_t height, uint8_t out_bpp, uint16_t palette_data_byte_len) BANKED {

    // Clamp to max colors allowed by bpp
    png.width   = width;
    png.height  = height;
    png.out_bpp = out_bpp;

    uint16_t bpp_palette_len_max = (1 << out_bpp) * PNG_PAL_RGB888_SZ;
    if (palette_data_byte_len > bpp_palette_len_max) palette_data_byte_len = bpp_palette_len_max;
    png.palette_data_byte_len = palette_data_byte_len;

    // Calc max sizes
    png.zlib_pixel_rows_max_size = calc_zlib_pixel_data_size(width, height, out_bpp);
    png.file_max_size            = calc_max_file_size(png.palette_data_byte_len, png.zlib_pixel_rows_max_size);

    png.calc_initialized = true;

    return png.file_max_size;
}

// Expects:
// - p_img_palette_data:         Palette buffer in RGB888 format (3 component bytes per color entry), 256 colors max
// - p_img_pixel_color_indexes:  Source pixel data, currently 1 byte per pixel // TODO: N pixels per byte determined by 8 / bpp
// - p_png_out_buf:              Buffer for writing the PNG file data into
void png_indexed_set_buffers(uint8_t * p_img_palette_data, uint8_t * p_img_pixel_color_indexes, uint8_t * p_png_out_buf) BANKED {

    png.p_palette_data        = p_img_palette_data;
    png.p_pixel_color_indexes = p_img_pixel_color_indexes;
    png.p_png_out_buf         = p_png_out_buf;

    // Ensure no buffers are NULL
    if (p_img_palette_data && p_img_pixel_color_indexes && p_png_out_buf)
        png.buffers_initialized = true;
}




// Pre-calc the max size for zlib encapsulated pixel data
static uint16_t calc_zlib_pixel_data_size(uint8_t width, uint8_t height, const uint8_t bpp) {
    // For Indexed Pixel data, encode each scanline row as a separate
    // zlib chunk which makes it easier to pack it all together.
    // Tuck the PNG row data into that
    const uint8_t pixels_per_byte = 8 / bpp;

    const uint16_t deflate_chunk_sz  = PNG_ROW_FILTER_TYPE_SZ + ((width + (pixels_per_byte - 1)) / pixels_per_byte);  // Needs to be rounded up in case it's not an even multiple of pixels_per_byte
    const uint16_t zlib_row_chunk_sz = DEFLATE_HEADER_SZ + deflate_chunk_sz;
    const uint16_t zlib_total_size   = ZLIB_HEADER_SZ + (zlib_row_chunk_sz * height) + ZLIB_FOOTER_SZ;

    // DEBUG: printf("fmax_dfz=%u\n"
       // "zck=%u, "
       // "zts=%u\n\n",
       // (uint16_t)deflate_chunk_sz,
       // (uint16_t)zlib_row_chunk_sz,
       // (uint16_t)zlib_total_size);

    return zlib_total_size;
}


// Pre-calc the max size for zlib encapsulated pixel data
static uint16_t calc_max_file_size(uint8_t palette_data_byte_len, uint16_t zlibPixelRows_maxSize) {

    return (PNG_SIGNATURE_SZ
            + PNG_IHDR_SZ           + PNG_CHUNK_OVERHEAD
            + palette_data_byte_len + PNG_CHUNK_OVERHEAD // PLTE
            + zlibPixelRows_maxSize + PNG_CHUNK_OVERHEAD // IDAT
            + PNG_IEND_SZ           + PNG_CHUNK_OVERHEAD);
}

static uint16_t calc_idat_payload_start_offset(uint8_t palette_data_byte_len) {

    return (PNG_SIGNATURE_SZ
            + PNG_IHDR_SZ           + PNG_CHUNK_OVERHEAD
            + palette_data_byte_len + PNG_CHUNK_OVERHEAD // PLTE
            // Plus the start of the IDAT chunk, while excluding the payload and checksum
            + PNG_CHUNK_LENGTH_SZ + PNG_CHUNK_TYPE_SZ 
            );
}

// TODO: STRUCTURE: move to png_indexed_utils function?
static uint8_t * write_u16_le(uint8_t * outBuffer, uint16_t value) {
    // Write out least significant bytes first
    *outBuffer++ = (uint8_t)value;
    *outBuffer++ = (uint8_t)(value >> 8);

    return outBuffer;
}

static uint8_t * write_u16_be(uint8_t * outBuffer, uint16_t value) {

    // Write out most significant bytes first
    *outBuffer++ = (uint8_t)(value >> 8);
    *outBuffer++ = (uint8_t)value;
    
    return outBuffer;
}

static uint8_t * write_u32_be(uint8_t * outBuffer, uint32_t value) {
    
    // Write out most significant bytes first
    *outBuffer++ = (uint8_t)(value >> 24);
    *outBuffer++ = (uint8_t)(value >> 16);
    *outBuffer++ = (uint8_t)(value >> 8);
    *outBuffer++ = (uint8_t)value;

    return outBuffer;
}

// Look into Alternate delayed modulo version?
static void adler_reset(void) {
    zlib_adler_a = 1u, zlib_adler_b = 0u;
}

// TODO: OPTIMIZE: 32 bit modulo is going to be brutal (optimizations for this?)
static void adler_crc_update(uint8_t * buffer, uint16_t buffer_sz) {

    // DEBUG: printf("ssz->%u:,", buffer_sz);
    for (uint16_t i = 0u; i < buffer_sz; i++ ) {
         zlib_adler_a = (zlib_adler_a + buffer[i]) % 65521u;
         zlib_adler_b = (zlib_adler_b + zlib_adler_a) % 65521u;
         // DEBUG: printf("%hu,", (uint8_t)buffer[i]);
    }
    // DEBUG: printf("==\n");
}



// Source - https://stackoverflow.com/a
// Posted by Denys Séguret, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-01, License - CC BY-SA 3.0


// TODO: OPTIMIZE?
uint32_t crc32(const uint8_t * p_buffer, uint16_t buffer_sz) {

    uint32_t crc =  0xFFFFFFFF;

    for (uint16_t i = 0u; i < buffer_sz; i++ ) {
        crc = (crc >> 8) ^ crc32Table[(crc ^ p_buffer[i]) & 0xFFu];
    }

    return (crc ^ 0xFFFFFFFF);
}


static uint8_t * png_write_chunk(uint8_t * p_out_buf, const char * type, const uint8_t * p_payload, const uint16_t payload_sz) {    

    // Write the Length of Payload (does not include length, type, checksum)
    p_out_buf = write_u32_be(p_out_buf, payload_sz);
    // Data covered by CRC starts immediately after length field
    const uint8_t * p_crc_start = p_out_buf;

    // Write the type
    memcpy(p_out_buf, type, PNG_CHUNK_TYPE_SZ);
    p_out_buf += PNG_CHUNK_TYPE_SZ;

    // Write the payload (if not NULL to request no copy)
    if (p_payload != NO_DATA_COPY) memcpy(p_out_buf, p_payload, payload_sz);
    p_out_buf += payload_sz;

    // Write the checksum of type + payload (omitting length field)
    uint32_t checksum = crc32(p_crc_start, p_out_buf - p_crc_start);
    p_out_buf = write_u32_be(p_out_buf, checksum);

    return p_out_buf;
}




// TODO: OPTIMIZE: Stop encapsulating each row as separate zlib chunk to reduce size overhead?... But will that result in a slower adler check? -> probably not with current adler calc
// TODO: OPTIMIZE: separate functions for 1bpp/2bpp/etc -> IN PARTICULAR ONES THAT TAKE 1BPP / 2BPP DATA AS SOURCE TO REDUCE SIZE (i.e. take gb bpp data directly)
// TODO: FEATURE: Require source bpp to match out bpp? (may be needed to reduce memory required by 1-8x) 
// TODO: FEATURE: Accept data in gb tile format? (1 or 2bpp, repack the bytes into the output buffer)    - WAIT UNTIL PORTED CODE IS WORKING FIRST
//
//  NOTE: Currently assumes incoming BPP is 8
static uint16_t prepare_pixel_data(void) {

    // zlib/Deflate Adler checksum is only on the
    // Size of block in little endian and its 1's complement (4 bytes)
    adler_reset();

    // To reduce memory the zlib encapsulated image data is written
    // directly intothe expected location of the IDAT chunk data field
    // in the pre-allocated PNG output buffer

    // TODO: collapse this into a single line
    const uint16_t  png_z_lib_data_start    = calc_idat_payload_start_offset(png.palette_data_byte_len);
    uint8_t *       p_zlib_pixel_rows       = png.p_png_out_buf +  png_z_lib_data_start;
    const uint8_t * p_zlib_pixel_rows_start = p_zlib_pixel_rows;

    const uint8_t * p_color_indexes      = png.p_pixel_color_indexes;

    // TODO: OPTIMIZE: Local vars (or globals) could be a little faster than their global struct access
    const uint8_t bpp    = png.out_bpp;
    const uint8_t width  = png.width;
    const uint8_t height = png.height;

    // For Indexed Pixel data, encode each scanline row as a separate
    // zlib chunk which makes it easier to pack it all together.
    // Tuck the PNG row data into that
    const uint8_t bpp_max_index_value_and_mask  = ((1 << bpp) - 1);
    const uint8_t pixels_per_byte = 8 / bpp;


    const uint16_t deflate_chunk_sz  = PNG_ROW_FILTER_TYPE_SZ + ((width + (pixels_per_byte - 1)) / pixels_per_byte);  // Needs to be rounded up in case it's not an even multiple of pixels_per_byte

    // DEBUG: printf("fmax_sz=%u\n"
        // "ds=%x, "
        // "pds=%x\n",
        // (uint16_t)png.file_max_size,
        // (uint16_t)png_z_lib_data_start,
        // (uint16_t)p_zlib_pixel_rows);

    // DEBUG: printf("dfz=%u\n",
       // (uint16_t)deflate_chunk_sz);


    // TEST OUTPUT
    const uint8_t bpp_pal_sz = 1u << bpp;

    // Write zlib header bytes
    *p_zlib_pixel_rows++ = ZLIB_HEADER_CMF;
    *p_zlib_pixel_rows++ = ZLIB_HEADER_FLG;

    // Write out the scanline pixel index rows
    for (uint8_t y = 0u; y < height; y++) {

        // TODO: OPTIMIZE: discard deflate block per pixel row and put everything in a single block since output it will always be less than block limit (65,535 bytes)
        // Deflate Header
        *p_zlib_pixel_rows++ = (y == (height - 1)) ? DEFLATE_HEADER_FINAL_YES :  DEFLATE_HEADER_FINAL_NO;
        // 
        p_zlib_pixel_rows = write_u16_le(p_zlib_pixel_rows, deflate_chunk_sz);
        p_zlib_pixel_rows = write_u16_le(p_zlib_pixel_rows, deflate_chunk_sz ^ 0xFFFFu);

        // PNG Row filter header + row data
        uint8_t * p_zlib_adler_start = p_zlib_pixel_rows;
        *p_zlib_pixel_rows++ = PNG_ROW_FILTER_TYPE_NONE;

        // uint16_t last_x, // TODO: unused in C version?
        uint8_t bitpacked = 0u;

        uint8_t pixels_remaining_to_pack = pixels_per_byte;
        for (uint8_t x = 0u; x < width; x++) {
            // Spec:
            // Pixels are always packed into scanlines with no wasted bits between pixels.
            // Pixels smaller than a byte never cross byte boundaries; they are packed into bytes
            // **with the leftmost pixel in the high-order bits of a byte**, the rightmost in the low-order bits.

            // TEST OUTPUT: Make an 8x8 checkboard like arrangement of the bpp sized palette
            // uint8_t next_pixel_index = (((y / 8u) % bpp_pal_sz) + (x / 8u)) % bpp_pal_sz;

            // Read next pixel color index and clamp (via mask, dropping bits) the pixel to max bpp allowed value
            uint8_t next_pixel_index = *p_color_indexes++ & bpp_max_index_value_and_mask;
            bitpacked |= next_pixel_index;

            // Write out packed bits if this is the last bit in the byte
            if (--pixels_remaining_to_pack == 0u) {
                *p_zlib_pixel_rows++ = bitpacked;
                // Don't reset the pixels to pack counter if this is the end of the pixel row
                if (x != (width - 1u))
                    pixels_remaining_to_pack = pixels_per_byte;
            }
            // Rotate bits upward (for 8bpp this is don't care since it's all replaced next iteration)
            bitpacked = (bitpacked << bpp) & 0xFF;

        }

        // If there trailing bits that need to be flushed then write them out
        // (i.e, image width isn't a multiple of 8 and so a row won't end on full bit)
        // Doesn't apply for 8bpp since that always writes a full byte per pixel
        if ((bpp != 8) && (pixels_remaining_to_pack != 0u)) {
            // Spec:
            // Scanlines always begin on byte boundaries.
            // When pixels have fewer than 8 bits and the scanline width
            // is not evenly divisible by the number of pixels per byte, the low-order bits in the last byte of each scanline are wasted.
            // The contents of these wasted bits are unspecified.

            // Upshift so that unused bits are in low order bits
            bitpacked = (bitpacked << (pixels_remaining_to_pack * bpp));
            *p_zlib_pixel_rows++ = bitpacked & 0xFF;
        }

        // DEBUG: printf("a_rng=%x /sz=%x\n",
        // (uint16_t)p_zlib_adler_start,
        // (uint16_t)(p_zlib_pixel_rows - p_zlib_adler_start));

        adler_crc_update(p_zlib_adler_start, (p_zlib_pixel_rows - p_zlib_adler_start)); // Length calc is not +1 since pointer is already at byte after end of adler range

        // printf("ad a=%x,b=%x\n",
        //        (uint16_t)zlib_adler_a,
        //        (uint16_t)zlib_adler_b);
    }
    // Write zlib Adler crc
    p_zlib_pixel_rows = write_u16_be(p_zlib_pixel_rows, zlib_adler_b);
    p_zlib_pixel_rows = write_u16_be(p_zlib_pixel_rows, zlib_adler_a);


    // DEBUG: printf("zfinsz=%u\n",
       // (uint16_t)(p_zlib_pixel_rows - p_zlib_pixel_rows_start));

    // Return resulting size (may be smaller than max size if compression is used) // Note: no compression yet :)    
    return (p_zlib_pixel_rows - p_zlib_pixel_rows_start);
}


uint16_t png_indexed_encode(void) BANKED {

    if (!png.calc_initialized && !png.buffers_initialized)
        return 0;

    EMU_PROFILE_BEGIN(" PNG prof start ");

    // TODO: error handling

        // Check array pointers for NULL
        // if ((!paletteData) || (!colorIndexes))

        // if (width > PNG_EXPORT_SUPPORTED_MAX_WIDTH)
        //     throw new Error("Image is wider than maximum supported width of " + PNG_EXPORT_SUPPORTED_MAX_WIDTH);

        // if ((bpp !== 1) && (bpp !== 2) && (bpp !== 4) && (bpp !== 8))
        //     throw new Error("Error: Bits-per-pixel (bpp) must be 1, 2, 4 or 8. Input was " + bpp);

        // if (totalPaletteColors > (1 << bpp))
        // Warn that total colors exceeds limit

    // == Process PNG Data ==
    const uint16_t zlib_packed_size = prepare_pixel_data();


    // == Now build the PNG output ==
    uint8_t  * p_pngbuf = png.p_png_out_buf;

    // PNG Signature
    // for (uint8_t sig_idx = 0u; sig_idx < ARRAY_LEN(png_signature); sig_idx++) {
    //     *p_pngbuf++ = png_signature[sig_idx];
    // }
    memcpy(p_pngbuf, png_signature, ARRAY_LEN(png_signature));
    p_pngbuf += ARRAY_LEN(png_signature);

    // PNG IHDR
    // Skip past IHDR Length and Type
    uint8_t * p_pngbuf_ihdr_start = p_pngbuf;
    p_pngbuf += PNG_CHUNK_LENGTH_SZ + PNG_CHUNK_TYPE_SZ;
    // Write the data directly
    p_pngbuf   = write_u32_be(p_pngbuf, png.width);
    p_pngbuf   = write_u32_be(p_pngbuf, png.height);
    *p_pngbuf++ = png.out_bpp;
    *p_pngbuf++ = PNG_COLOR_TYPE_INDEXED;
    *p_pngbuf++ = PNG_COMPRESSION_METHOD_DEFLATE_NONE;
    *p_pngbuf++ = PNG_FILTER_METHOD_NONE;
    *p_pngbuf++ = PNG_INTERLACING_NONE;
    // And then commit the chunk with no data copy
    p_pngbuf = png_write_chunk(p_pngbuf_ihdr_start, "IHDR", NO_DATA_COPY, PNG_IHDR_SZ);

    // PNG Indexed Color Palette
    p_pngbuf = png_write_chunk(p_pngbuf, "PLTE", png.p_palette_data, png.palette_data_byte_len);  // Expects palette data in RGB888 format

    // PNG Indexed Color Pixel Data (in 8192 byte IDAT Chunks)
    // p_pngbuf = png_write_chunk(p_pngbuf, "IDAT", zlibPixelRows, zlibPixelRows_sz);
    // Don't actually write the pixel data since it's already been assembled in place
    // DEBUG: printf("\npchk=%x, %u\n", (uint16_t)p_pngbuf, (uint16_t)zlib_packed_size);
    p_pngbuf = png_write_chunk(p_pngbuf, "IDAT", NO_DATA_COPY, zlib_packed_size);
    // DEBUG:  printf("\npchk=%x\n", (uint16_t)p_pngbuf);

    // PNG End of data
    p_pngbuf = png_write_chunk(p_pngbuf, "IEND", NO_DATA_COPY, 0);

    EMU_PROFILE_END(" PNG prof end: ");

    const uint16_t pngfile_final_size = p_pngbuf - png.p_png_out_buf;
    return pngfile_final_size;  // Return size of completed PNG image, NULL for error
}
