#include <gbdk/platform.h>
#include <stdint.h>
#include <string.h>  // For memcopy()

#include <gbdk/emu_debug.h>

#include "common.h"
#include "base64.h"


#pragma bank 255  // Autobanked


#define PADDING_CHAR '='

// Select which base64 variant to use
// #define URL_ENCODE

#ifdef URL_ENCODE
    #define B64_ENC_62  '-'
    #define B64_ENC_63  '_'
#else
    #define B64_ENC_62  '+'
    #define B64_ENC_63  '/'
#endif

const uint8_t base64_digit_lut[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', B64_ENC_62, B64_ENC_63
};

const uint8_t url_base64_bindata_prefix[]   = "data:application/octet-stream;base64,";
const uint8_t url_base64_pngimage_prefix[]  = "data:image/png;base64,";
const uint8_t url_base64_pngimage_prefix_sz = ARRAY_LEN(url_base64_pngimage_prefix) - 1; // -1 to strip null terminator

#define PAD_2_CHARS 2u
#define PAD_1_CHARS 1u


static uint16_t base64_encode_url_format(uint8_t * p_dest, const uint8_t * p_src, uint16_t src_len);


uint16_t base64_encode_to_url(uint8_t * p_dest, const uint8_t * p_src, uint16_t src_len) BANKED {

    // Convert to Base64 (offset past prefix)
    uint16_t out_len = base64_encode_url_format(p_dest + url_base64_pngimage_prefix_sz, p_src, src_len);

    // EMU_printf(".1 B64 sz=%u\n", (uint16_t)out_len);

    // Add url prefix to the buffer and length
    memcpy(p_dest, url_base64_pngimage_prefix, url_base64_pngimage_prefix_sz);
    out_len += url_base64_pngimage_prefix_sz;

    // EMU_printf(".2 B64 sz=%u\n", (uint16_t)out_len);

    // Append string null terminator
    p_dest[out_len] = '\0';
    out_len++;

    // EMU_printf(".2 B64 sz=%u\n", (uint16_t)out_len);

    return out_len;
}


// TODO: Fixed, easier to read, but slower -> PROFILE: 2E14
//
// TODO: OPTIMIZE: Not much luck trying to optimize the C implementation (they all get slower).
//                 Maybe for a fast version it'll have to be hand written
//
// Encodes URL style base64
static uint16_t base64_encode_url_format(uint8_t * p_dest, const uint8_t * p_src, uint16_t src_len) {

    EMU_PROFILE_BEGIN(" B64 prof start ");

    static uint16_t len_in;
    static uint8_t b1, b2, b3;

    uint8_t padding_char_count = 0u;
    uint8_t * p_dest_start = p_dest;
    len_in = src_len;

    while (len_in) {

        b1 = *p_src++;
        b2 = *p_src++;
        b3 = *p_src++;

        len_in -= 3u;
        if (len_in == (uint16_t)-2) {
            len_in = 0;
            // 2 padding bytes
            b2 = 0u;
            b3 = 0u;
            padding_char_count = PAD_2_CHARS;
        }
        else if (len_in == (uint16_t)-1) {
            len_in = 0;
            // 2 padding bytes
            b3 = 0u;
            padding_char_count = PAD_1_CHARS;
        }

        *p_dest++ = base64_digit_lut[b1 >> 2];                       // Byte0[7..2]        
        *p_dest++ = base64_digit_lut[((b1 & 0x03u) << 4) | b2 >> 4]; // Byte0[1..0] with Byte1[7..4]        
        *p_dest++ = base64_digit_lut[((b2 & 0x0Fu) << 2) | b3 >> 6]; // Byte1[3..0] with Byte2[7..6]
        *p_dest++ = base64_digit_lut[b3 & 0x3Fu];                    // Byte2[5..0]
    }

    // Fix up padding chars at end if needed
    switch (padding_char_count) {        
        case PAD_2_CHARS: *(p_dest - 2u) = PADDING_CHAR; // Overwrite 3rd (of 4) previously encoded bytes with padding
             // Intentional fall through        
        case PAD_1_CHARS: *(p_dest - 1u) = PADDING_CHAR; // Overwrite 4th (of 4) previously encoded bytes with padding
    }

    EMU_PROFILE_END(" B64 prof end: ");

    // Return length
    return (p_dest - p_dest_start);
}


