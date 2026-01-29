#include <stdint.h>

#include "common.h"


const uint8_t base64_digit_lut[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_',
};

#define PADDING_CHAR '='


// TODO: explicit design for sram banks: src_bank, dest_bank and src-bank-read/cache/dest-bank-write pattern

// TODO: redo this cleaner

static uint16_t len_in;


// TODO: OPTIMIZE: Not much luck trying to optimize the C implementation (they all get slower).
//                 Maybe for a fast version it'll have to be hand written
//
// Encodes URL style base64
uint16_t base64_encode_url(const uint8_t * p_src, char * p_dest, uint16_t src_len) {

    uint8_t * p_dest_start = p_dest;
    uint8_t pack;
    len_in = src_len;

    while (len_in) {

        // Byte0[7..2]
        *p_dest++ = base64_digit_lut[*p_src >> 2];

        // Byte0[1..0] with Byte1[7..4]
        pack = (*p_src++ & 0x03u) << 4;
        *p_dest++ = base64_digit_lut[pack | *p_src >> 4];

        // Byte1[3..0] with Byte2[7..6]
        pack = (*p_src++ & 0x0Fu) << 2;
        *p_dest++ = base64_digit_lut[pack | *p_src >> 6];

        // Byte2[5..0]
        *p_dest++ = base64_digit_lut[*p_src++ & 0x3Fu];

        len_in -= 3u;

        // Kind of naughty: intentionally overrun the source buffer
        // if it's not an even multiple of 3, and then apply the padding
        // encoding for any excess source bytes
        if (len_in == (uint16_t)-2) {
            // 2 padding bytes, and remove 4 bits from second encoded byte
            *p_dest-- = PADDING_CHAR;
            *p_dest-- = PADDING_CHAR;
            *p_dest  &= 0x30u;
            p_dest   += 2u;
            break;
        }
        else if (len_in == (uint16_t)-1) {
            // 1 padding bytes, and remove 2 bits from third encoded byte
            *p_dest-- = PADDING_CHAR;
            *p_dest  &= 0x3Au;
            p_dest++;
            break;
        }
    }

    // Return length
    return (p_dest - p_dest_start);
}


