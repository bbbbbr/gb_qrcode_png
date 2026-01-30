#ifndef COMMON_H
#define COMMON_H

#include "png_indexed.h"

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))

// Current QRCode sizing estimates
// - 1bpp Image Max = 1282 - 6  palette (@ 2 col) = 1276 Bytes * 8 pixels per byte = 10208 Pixels -> fits: 104 x 96[tiles:13x12] = 9984
// - 2bpp Image Max = 1282 - 12 palette (@ 4 col) = 1270 Bytes * 4 pixels per byte = 5080  Pixels -> fits: 72 x 64[tiles:9x8] = 4608

#define IMG_BPP        PNG_BPP_1
#define TILE_SZ_PX         8u

// Note: Optimized display capture/read -> png relies on image being aligned to tiles horizontally
#define IMG_WIDTH_TILES   12u
#define IMG_HEIGHT_TILES  12u

#define IMG_WIDTH_PX      (IMG_WIDTH_TILES  * TILE_SZ_PX)
#define IMG_HEIGHT_PX     (IMG_HEIGHT_TILES * TILE_SZ_PX)

#define IMG_X_START       ((DEVICE_SCREEN_PX_WIDTH  - IMG_WIDTH_PX) / 2u)
#define IMG_Y_START       ((DEVICE_SCREEN_PX_HEIGHT - IMG_HEIGHT_PX) / 2u)

#define IMG_TILE_X_START  (IMG_X_START / TILE_SZ_PX)
#define IMG_TILE_Y_START  (IMG_Y_START / TILE_SZ_PX)

#define IMG_X_END         ((IMG_X_START + IMG_WIDTH_PX)  - 1u)
#define IMG_Y_END         ((IMG_Y_START + IMG_HEIGHT_PX) - 1u)


#endif // COMMON_H