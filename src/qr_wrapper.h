#include <gbdk/platform.h>
#include <stdint.h>

#ifndef _QR_WRAPPER_H
#define _QR_WRAPPER_H

bool qr_generate(const char * embed_str, uint16_t len);
void qr_render(void);

#endif // _QR_WRAPPER_H