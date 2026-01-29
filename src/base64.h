#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>

#define BASE64_IN_LEN  3u  // Base64 works in 3 byte units
#define BASE64_OUT_LEN 4u  // And writes out 4 byte units

#define B64_CALC_OUT_SZ(len) (((len + (BASE64_IN_LEN - 1u)) / BASE64_IN_LEN) * BASE64_OUT_LEN)

uint16_t base64_encode_to_url(uint8_t * p_dest, const uint8_t * p_src, uint16_t src_len) BANKED;
static uint16_t base64_encode_url_format(uint8_t * p_dest, const uint8_t * p_src, uint16_t src_len);

#endif // BASE64_H