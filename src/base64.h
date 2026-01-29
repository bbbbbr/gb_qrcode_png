#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>

uint16_t base64_encode_url(const uint8_t * p_src, char * p_dest, uint16_t src_len);

#endif // BASE64_H