/* 
 * QR Code generator library (C)
 * 
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include <gbdk/platform.h>

#pragma bank 255  // Autobanked

BANKREF(qrcodegen)

// #define STANDALONE

#ifdef STANDALONE
    #include <string.h>

    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <stdio.h>

    #define debugBorder(a) do {} while(false)  // TODO
    #define INLINE inline
    #define QRVERSION 8
    #define QRECL qrcodegen_Ecc_MEDIUM
    #define QRSIZE (QRVERSION * 4 + 17)
    #define QRPAD 64
    uint8_t *qrcodegen(const char *text);
#endif

#ifndef STANDALONE
    #include <string.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <stdio.h>

    #include "qrcodegen.h"
    #define INLINE inline
    #define size_t uint16_t
#endif

#define assert(a)

#define abs(a) (a>=0?a:-a)


// ENUM TYPES
enum qrcodegen_Ecc {
	qrcodegen_Ecc_LOW = 0 ,  // The QR Code can tolerate about  7% erroneous codewords
	qrcodegen_Ecc_MEDIUM  ,  // The QR Code can tolerate about 15% erroneous codewords
	qrcodegen_Ecc_QUARTILE,  // The QR Code can tolerate about 25% erroneous codewords
	qrcodegen_Ecc_HIGH    ,  // The QR Code can tolerate about 30% erroneous codewords
};
// ECC IS SELECTED IN THE HEADER

enum qrcodegen_Mask {
	qrcodegen_Mask_0 = 0,
	qrcodegen_Mask_1,
	qrcodegen_Mask_2,
	qrcodegen_Mask_3,
	qrcodegen_Mask_4,
	qrcodegen_Mask_5,
	qrcodegen_Mask_6,
	qrcodegen_Mask_7,
};
// THIS VERSION SUPPORTS ONLY MASK 0
#define MASK qrcodegen_Mask_0

enum qrcodegen_Mode {
	qrcodegen_Mode_NUMERIC      = 0x1,
	qrcodegen_Mode_ALPHANUMERIC = 0x2,  // 0–9, A–Z (upper-case only), space, $, %, *, +, -, ., /, :   Note: No semicolon as needed for mime header
	qrcodegen_Mode_BYTE         = 0x4,
	qrcodegen_Mode_KANJI        = 0x8,
	qrcodegen_Mode_ECI          = 0x7,
};
// THIS VERSION SUPPORTS ONLY BYTE MODE
#define MODE qrcodegen_Mode_BYTE

// #define qrcodegen_BUFFER_SZ  (QRPAD * QRSIZE/8)
#define qrcodegen_BUFFER_SZ  (QR_OUTPUT_ROW_SZ_BYTES * QR_FINAL_PIXEL_HEIGHT)

uint8_t TMPBUFFER[qrcodegen_BUFFER_SZ];
uint8_t QRCODE[qrcodegen_BUFFER_SZ];

#define qrcodegen_REED_SOLOMON_DEGREE_MAX 30  // Based on the table ECC_CODEWORDS_PER_BLOCK_D

#define  SKIP1(a,...) __VA_ARGS__,a
#define  SKIP2(...)    SKIP1H(SKIP1(__VA_ARGS__))
#define  SKIP4(...)   SKIP2H(SKIP2(__VA_ARGS__))
#define  SKIP8(...)   SKIP4H(SKIP4(__VA_ARGS__))
#define  SKIP16(...)  SKIP8H(SKIP8(__VA_ARGS__))
#define  SKIP32(...) SKIP16H(SKIP16(__VA_ARGS__))

#define  SKIP1H(...)   SKIP1(__VA_ARGS__)
#define  SKIP2H(...)   SKIP2(__VA_ARGS__)
#define  SKIP4H(...)   SKIP4(__VA_ARGS__)
#define  SKIP8H(...)   SKIP8(__VA_ARGS__)
#define  SKIP16H(...)   SKIP16(__VA_ARGS__)


#define SELECT1_H(a,...)  (a)
#define SELECT1(...) SELECT1_H(__VA_ARGS__)
#define SELECT2(n, ...)  (n< 1?  SELECT1(__VA_ARGS__):  SELECT1(SKIP1(__VA_ARGS__)))
#define SELECT4(n, ...)  (n< 2?  SELECT2(n,__VA_ARGS__):  SELECT2((n- 2),  SKIP2(__VA_ARGS__)))
#define SELECT8(n, ...)  (n< 4?  SELECT4(n,__VA_ARGS__):  SELECT4((n- 4),  SKIP4(__VA_ARGS__)))
#define SELECT16(n, ...) (n< 8?  SELECT8(n,__VA_ARGS__):  SELECT8((n- 8),  SKIP8(__VA_ARGS__)))
#define SELECT32(n, ...) (n<16? SELECT16(n,__VA_ARGS__): SELECT16((n-16), SKIP16(__VA_ARGS__)))
#define SELECT64(n, ...) (n<32? SELECT32(n,__VA_ARGS__): SELECT32((n-32), SKIP32(__VA_ARGS__)))

#define SELECT(n, ...) SELECT64(n,__VA_ARGS__)

#define ECC_CODEWORDS_PER_BLOCK_D(e, v, ...) SELECT(e, \
    SELECT(v, -1,  7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30), \
    SELECT(v, -1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28), \
    SELECT(v, -1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30), \
    SELECT(v, -1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30))   

#define NUM_ERROR_CORRECTION_BLOCKS_D(e, v, ...) SELECT(e, \
    SELECT(v, -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25), \
    SELECT(v, -1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49), \
    SELECT(v, -1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68), \
    SELECT(v, -1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81))   




/*---- Basic QR Code information ----*/

const uint8_t qr_bitmask[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    };

INLINE bool getBit(int x, int i) {
	return ((x >> i) & 1) != 0;
}

INLINE bool getModule(const uint8_t qrcode[], uint8_t x, uint8_t y) {
	
    // return qrcode[y * (QRPAD>>3) + (x>>3)] & qr_bitmask[x];
	return qrcode[y * QR_OUTPUT_ROW_SZ_BYTES + (x>>3)] & qr_bitmask[x];
}

INLINE void setModule(uint8_t qrcode[], uint8_t x, uint8_t y, bool isBlack) {
    // uint8_t v =  qrcode[y * (QRPAD>>3) + (x>>3)];
    uint16_t index = y * QR_OUTPUT_ROW_SZ_BYTES + (x>>3);
    uint8_t v =  qrcode[index];
    v = (v & ~qr_bitmask[x]) | (((uint8_t)((uint8_t)0)-(!!isBlack)) & qr_bitmask[x]);
    // qrcode[y * (QRPAD>>3) + (x>>3)] = v;
    qrcode[index] = v;
}

static void setModuleStatic(uint8_t qrcode[], uint8_t x, uint8_t y, bool isBlack) { setModule(qrcode, x, y, isBlack); }
bool qr_get(uint8_t x, uint8_t y) { return getModule(QRCODE,x,y); }
static void setModuleBounded(uint8_t qrcode[], int x, int y, bool isBlack) {
	if (0 <= x && x < QRSIZE && 0 <= y && y < QRSIZE)
		setModule(qrcode, x, y, isBlack);
}


// INLINE uint8_t getModule8(const uint8_t qrcode[], uint8_t x, uint8_t y) { return qrcode[y * (QRPAD>>3) + (x>>3)]; }
// INLINE void setModule8(uint8_t qrcode[], uint8_t x, uint8_t y, uint8_t value) { qrcode[y * (QRPAD>>3) + (x>>3)] = value; }
INLINE uint8_t getModule8(const uint8_t qrcode[], uint8_t x, uint8_t y) { return qrcode[y * QR_OUTPUT_ROW_SZ_BYTES + (x>>3)]; }
INLINE void setModule8(uint8_t qrcode[], uint8_t x, uint8_t y, uint8_t value) { qrcode[y * QR_OUTPUT_ROW_SZ_BYTES + (x>>3)] = value; }
uint8_t qr_get8(uint8_t x, uint8_t y) { return getModule8(QRCODE,x,y); }













// Returns the number of data bytes that can be stored in a QR Code of the given version number, after
// all function modules are excluded. This includes remainder bits, so it might not be a multiple of 8.
// The result is in the range [208, 29648]. This could be implemented as a 40-entry lookup table.
INLINE int getNumRawDataModules(void) {
	int result = (16 * QRVERSION + 128) * QRVERSION + 64;
	if (QRVERSION >= 2) {
		result -= (25 * (QRVERSION / 7 + 2) - 10) * (QRVERSION / 7 + 2) - 55;
		if (QRVERSION >= 7)
			result -= 36;
	}
	return result / 8;
}

// Returns the number of 8-bit codewords that can be used for storing data (not ECC),
// for the given version number and error correction level. The result is in the range [9, 2956].
INLINE int getNumDataCodewords(void) {
	return getNumRawDataModules()
		- ECC_CODEWORDS_PER_BLOCK_D(QRECL,QRVERSION)
		* NUM_ERROR_CORRECTION_BLOCKS_D(QRECL,QRVERSION);;
}










/*---- Reed-Solomon ECC generator functions ----*/

// Returns the product of the two given field elements modulo GF(2^8/0x11D).
// All inputs are valid. This could be implemented as a 256*256 lookup table.
//
// Optimized version with lookup tables: 13,651,906 cycles -> 5,520,276 (time reduced by 60%)
//
// TODO: OPTIONAL: Skipping the initial bank save and doing direct register writes takes it down
//                 to 3,609,216, but should only be done if there will be no interrupts that switch banks.
//        #define RS_LUT_NOSAVE_BANK   
//
static SFR rs_y;
static SFR rs_savebank;
static uint8_t reedSolomonMultiply(uint8_t x_) NONBANKED {

    #ifndef RS_LUT_NOSAVE_BANK
        rs_savebank = CURRENT_BANK;
        SWITCH_ROM((x_ >> 6) + 1u); // Tables are located in banks 1-4
    #else
        rROMB0 = ((x_ >> 6) + 1u); // Tables are located in banks 1-4
    #endif

    uint8_t result = *(const uint8_t *)(((uint16_t)((x_ & 0x3Fu) | 0x40u) << 8) + rs_y);

    #ifndef RS_LUT_NOSAVE_BANK
        SWITCH_ROM(rs_savebank);
    #else
        rROMB0 = CURRENT_BANK;
    #endif

    return result;
}
/*
static uint8_t reedSolomonMultiply(uint8_t x_) {
	// Russian peasant multiplication
	uint8_t x=x_, y=rs_y, z=0;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x80) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x40) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x20) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x10) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x08) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x04) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x02) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x01) z ^= x;
	return z;
}
*/

static uint8_t reedSolomonMultiply02(uint8_t x_) {
	// Russian peasant multiplication
	uint8_t x=x_, y=0x02, z=0;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x80) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x40) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x20) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x10) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x08) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x04) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x02) z ^= x;
    if (z & 0x80) { z = ((z << 1) ^ 0x1D); } else { z = (z << 1); }
    if (y & 0x01) z ^= x;
	return z;
}

#define RSDegree ECC_CODEWORDS_PER_BLOCK_D(QRECL,QRVERSION)

/////// SLOWISH
static uint8_t rsdiv[qrcodegen_REED_SOLOMON_DEGREE_MAX];
static uint8_t rsremainder[qrcodegen_REED_SOLOMON_DEGREE_MAX];
static const uint8_t *rsdata;
static SFR dataLen;
static void reedSolomonComputeRemainder(const uint8_t data_[], uint8_t dataLen_) {

    rsdata = data_;
    dataLen = dataLen_;
    memset(rsremainder,0,RSDegree);

    for (uint8_t i = 0; i < dataLen; i++) {  // Polynomial division
		rs_y = (*rsdata++) ^ rsremainder[0];

        for (uint8_t j = 0; j < RSDegree-1; j++)
            rsremainder[j] =  reedSolomonMultiply(rsdiv[j])^rsremainder[j+1];

		rsremainder[RSDegree - 1] = reedSolomonMultiply(rsdiv[RSDegree - 1]);

	}
}

/////// FAST
static void reedSolomonComputeDivisor(void) {

    memset(rsdiv,0,RSDegree);

	rsdiv[RSDegree - 1] = 1;  // Start off with the monomial x^0
	
	rs_y = 1;
	for (uint8_t i = 0; i < RSDegree; i++) {
		for (uint8_t j = 0; j < RSDegree; j++) {
			rsdiv[j] = reedSolomonMultiply(rsdiv[j]);
			if (j + 1 < RSDegree)
				rsdiv[j] ^= rsdiv[j + 1];
		}
		rs_y = reedSolomonMultiply02(rs_y);
	}
}





// Appends error correction bytes to each block of the given data array, then interleaves
// bytes from the blocks and stores them in the result array. data[0 : dataLen] contains
// the input data. data[dataLen : rawCodewords] is used as a temporary work area and will
// be clobbered by this function. The final answer is stored in result[0 : rawCodewords].
static void addEccAndInterleave(uint8_t data[], uint8_t result[]) {

    
	int numBlocks = NUM_ERROR_CORRECTION_BLOCKS_D(QRECL,QRVERSION);
	int blockEccLen = ECC_CODEWORDS_PER_BLOCK_D(QRECL,QRVERSION);
	int rawCodewords = getNumRawDataModules();
	int dataLen = getNumDataCodewords();
	int numShortBlocks = numBlocks - rawCodewords % numBlocks;
	int shortBlockDataLen = rawCodewords / numBlocks - blockEccLen;
	
	reedSolomonComputeDivisor();
	const uint8_t *dat = data;
	for (int i = 0; i < numBlocks; i++) {
		int datLen = shortBlockDataLen + (i < numShortBlocks ? 0 : 1);
        reedSolomonComputeRemainder(dat, datLen);
        for (int j = 0, k = i; j < datLen; j++, k += numBlocks) {  // Copy data
			if (j == shortBlockDataLen)
				k -= numShortBlocks;
			result[k] = dat[j];
		}
		for (int j = 0, k = dataLen + i; j < blockEccLen; j++, k += numBlocks)  // Copy ECC
			result[k] = rsremainder[j];
		dat += datLen;
	}
}



/*---- Drawing function modules ----*/


// Calculates and stores an ascending list of positions of alignment patterns
// for this version number, returning the length of the list (in the range [0,7]).
// Each position is in the range [0,177), and are used on both the x and y axes.
// This could be implemented as lookup table of 40 variable-length lists of unsigned bytes.
static int getAlignmentPatternPositions(uint8_t result[7]) {
	if (QRVERSION == 1)
		return 0;
	int step = (QRVERSION == 32) ? 26 :
		(QRVERSION*4 + (QRVERSION / 7 + 2)*2 + 1) / ((QRVERSION / 7 + 2)*2 - 2) * 2;
	for (int i = (QRVERSION / 7 + 2) - 1, pos = QRVERSION * 4 + 10; i >= 1; i--, pos -= step)
		result[i] = (uint8_t)pos;
	result[0] = 6;
	return (QRVERSION / 7 + 2);
}


// Sets every pixel in the range [left : left + width] * [top : top + height] to black.
static void fillRectangle(int left, int top, int width, int height, uint8_t qrcode[]) {
	for (int dy = 0; dy < height; dy++) {
		for (int dx = 0; dx < width; dx++)
			setModuleStatic(qrcode, left + dx, top + dy, true);
	}
}


// Clears the given QR Code grid with white modules for the given
// version's size, then marks every function module as black.
static void initializeFunctionModules(int version, uint8_t qrcode[]) {
	// Initialize QR Code
	memset(qrcode, 0, (size_t)qrcodegen_BUFFER_SZ * sizeof(qrcode[0]));
	
	// Fill horizontal and vertical timing patterns
	fillRectangle(6, 0, 1, QRSIZE, qrcode);
	fillRectangle(0, 6, QRSIZE, 1, qrcode);
	
	// Fill 3 finder patterns (all corners except bottom right) and format bits
	fillRectangle(0, 0, 9, 9, qrcode);
	fillRectangle(QRSIZE - 8, 0, 8, 9, qrcode);
	fillRectangle(0, QRSIZE - 8, 9, 8, qrcode);
	
	// Fill numerous alignment patterns
	uint8_t alignPatPos[7];
	int numAlign = getAlignmentPatternPositions(alignPatPos);
	for (int i = 0; i < numAlign; i++) {
		for (int j = 0; j < numAlign; j++) {
			// Don't draw on the three finder corners
			if (!((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0)))
				fillRectangle(alignPatPos[i] - 2, alignPatPos[j] - 2, 5, 5, qrcode);
		}
	}
	
	// Fill version blocks
	if (version >= 7) {
		fillRectangle(QRSIZE - 11, 0, 3, 6, qrcode);
		fillRectangle(0, QRSIZE - 11, 6, 3, qrcode);
	}
}


// Draws white function modules and possibly some black modules onto the given QR Code, without changing
// non-function modules. This does not draw the format bits. This requires all function modules to be previously
// marked black (namely by initializeFunctionModules()), because this may skip redrawing black function modules.
static void drawWhiteFunctionModules(void) {
	// Draw horizontal and vertical timing patterns
	int i;
    for (i = 7; i < QRSIZE - 7; i += 2) {
		setModuleStatic(QRCODE, 6, i, false);
		setModuleStatic(QRCODE, i, 6, false);
	}
	
	// Draw 3 finder patterns (all corners except bottom right; overwrites some timing modules)
	for (int dy = -4; dy <= 4; dy++) {
		for (int dx = -4; dx <= 4; dx++) {
			int dist = abs(dx);
			if (abs(dy) > dist)
				dist = abs(dy);
			if (dist == 2 || dist == 4) {
				setModuleBounded(QRCODE, 3 + dx, 3 + dy, false);
				setModuleBounded(QRCODE, QRSIZE - 4 + dx, 3 + dy, false);
				setModuleBounded(QRCODE, 3 + dx, QRSIZE - 4 + dy, false);
			}
		}
	}
	
	// Draw numerous alignment patterns
	uint8_t alignPatPos[7];
	int numAlign = getAlignmentPatternPositions(alignPatPos);
	for (i = 0; i < numAlign; i++) {
		for (int j = 0; j < numAlign; j++) {
			if ((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0))
				continue;  // Don't draw on the three finder corners
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++)
					setModuleStatic(QRCODE, alignPatPos[i] + dx, alignPatPos[j] + dy, dx == 0 && dy == 0);
			}
		}
	}
	
	// Draw version blocks
	if (QRVERSION >= 7) {
		// Calculate error correction code and pack bits
		int rem = QRVERSION;  // version is uint6, in the range [7, 40]
		for (i = 0; i < 12; i++)
			rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
		long bits = (long)QRVERSION << 12 | rem;  // uint18
		assert(bits >> 18 == 0);
		
		// Draw two copies
		for (i = 0; i < 6; i++) {
			for (int j = 0; j < 3; j++) {
				int k = QRSIZE - 11 + j;
				setModuleStatic(QRCODE, k, i, (bits & 1) != 0);
				setModuleStatic(QRCODE, i, k, (bits & 1) != 0);
				bits >>= 1;
			}
		}
	}
}



static void drawFormatBitsCopy0(uint16_t bits) {
    
    uint8_t b = bits&0xFF;
    setModuleStatic(QRCODE, 8, 0, b&1); b>>=1;
    setModuleStatic(QRCODE, 8, 1, b&1); b>>=1;
    setModuleStatic(QRCODE, 8, 2, b&1); b>>=1;
    setModuleStatic(QRCODE, 8, 3, b&1); b>>=1;
    setModuleStatic(QRCODE, 8, 4, b&1); b>>=1;
    setModuleStatic(QRCODE, 8, 5, b&1); b>>=1;

	setModuleStatic(QRCODE, 8, 7, b&1); b>>=1;
	setModuleStatic(QRCODE, 8, 8, b&1);
    
    b = bits>>8;
	setModuleStatic(QRCODE, 7, 8, b&1); b>>=1;

	setModuleStatic(QRCODE, 14 -  9, 8, b&1); b>>=1;
	setModuleStatic(QRCODE, 14 - 10, 8, b&1); b>>=1;
	setModuleStatic(QRCODE, 14 - 11, 8, b&1); b>>=1;
	setModuleStatic(QRCODE, 14 - 12, 8, b&1); b>>=1;
	setModuleStatic(QRCODE, 14 - 13, 8, b&1); b>>=1;
	setModuleStatic(QRCODE, 14 - 14, 8, b&1);
}

static void drawFormatBitsCopy1(uint16_t bits) {
    
    uint8_t b = bits&0xFF;
    uint8_t i;
	for (i = 0; i < 8; i++) {
		setModuleStatic(QRCODE, QRSIZE - 1 - i, 8, b&1); b>>=1;
    }
    b = bits>>8;
	for (i = 8; i < 15; i++) {
		setModuleStatic(QRCODE, 8, QRSIZE - 15 + i, b&1); b>>=1;
    }
	setModuleStatic(QRCODE, 8, QRSIZE - 8, true);  // Always black

}
static void drawFormatBits(void) {
    
	static const int table[] = {1, 0, 3, 2};
	int data = table[QRECL] << 3 | (int)MASK;  // errCorrLvl is uint2, mask is uint3
	int rem = data;
	for (uint8_t i = 0; i < 10; i++)
		rem = (rem << 1) ^ ((rem >> 9) * 0x537);
	uint16_t bits = (data << 10 | rem) ^ 0x5412;  // uint15
    
	drawFormatBitsCopy0(bits);
	drawFormatBitsCopy1(bits);
}




/*---- Drawing data modules and masking ----*/
static uint16_t dc_i;

static void drawCodewordsLR(uint8_t x) {
    uint8_t y=0;
    while (y<QRSIZE) {
        if (!getModule(QRCODE, x, y)) {
            setModule(QRCODE, x, y, getBit(TMPBUFFER[dc_i >> 3], 7 - (dc_i & 7)));
            dc_i++;
        }
        if (!getModule(QRCODE, x-1, y)) {
            setModule(QRCODE, x-1, y, getBit(TMPBUFFER[dc_i >> 3], 7 - (dc_i & 7)));
            dc_i++;
        }
        y++;
    }        
}

static void drawCodewordsRL(uint8_t x) {
    uint8_t y=QRSIZE;
    while (y) {
        y--;
        if (!getModule(QRCODE, x, y)) {
            setModule(QRCODE, x, y, getBit(TMPBUFFER[dc_i >> 3], 7 - (dc_i & 7)));
            dc_i++;
        }
        if (!getModule(QRCODE, x-1, y)) {
            setModule(QRCODE, x-1, y, getBit(TMPBUFFER[dc_i >> 3], 7 - (dc_i & 7)));
            dc_i++;
        }
    }
}

static void drawCodewords(void) {
    
    dc_i=0;
    
    uint8_t x=QRSIZE-1;

    drawCodewordsRL(x);
    x-=2;
    
    while(x>7) {

        drawCodewordsLR(x);
        x-=2;
        drawCodewordsRL(x);
        x-=2;
    }
    x = 5;

    drawCodewordsLR(x);
    x-=2;

    drawCodewordsRL(x);
    x-=2;

    drawCodewordsLR(x);
}

static void applyMask0(void) {
    uint8_t invert;
	for (uint8_t y = 0; y < QRSIZE; y++) {
        invert = ((y&1)?0xAA:~0xAA);
		for (uint8_t x = 0; x < QRSIZE; x+=8) {
            uint8_t tmp = invert & ~getModule8(TMPBUFFER, x, y);
            
			setModule8(QRCODE, x, y,  getModule8(QRCODE, x, y) ^ tmp);
		}
	}
}




/*---- Segment handling ----*/

INLINE int numCharCountBits(void) { return SELECT(((QRVERSION + 7) / 17), 8, 16, 16); }




////////////////////////////////////////////////////////////////////////
//


// Appends the given number of low-order bits of the given value to the given byte-based
// bit buffer, increasing the bit length. Requires 0 <= numBits <= 16 and val < 2^numBits.
INLINE void appendBitsToBuffer(unsigned int val, int numBits, uint8_t buffer[], int *bitLen) {
	assert(0 <= numBits && numBits <= 16 && (unsigned long)val >> numBits == 0);
	for (int i = numBits - 1; i >= 0; i--, (*bitLen)++)
		buffer[*bitLen >> 3] |= ((val >> i) & 1) << (7 - (*bitLen & 7));
}



/*---- Low-level QR Code encoding functions ----*/



////////////////////////////////////////////////////////////////////////
//

// uint8_t *qrcodegen(const char *text) {
uint8_t *qrcodegen(const char *text, uint16_t len) {
    
    
    // uint8_t len = 0;
    // while (text[len]!=0) len++;
	
	const uint8_t *data = (const uint8_t *)text;


	// Concatenate all segments to create the data bit string
	memset(QRCODE, 0, (size_t)qrcodegen_BUFFER_SZ * sizeof(QRCODE[0]));
	int bitLen = 0;
    appendBitsToBuffer((unsigned int)MODE, 4, QRCODE, &bitLen);
    appendBitsToBuffer((unsigned int)len, numCharCountBits(), QRCODE, &bitLen);
    for (int j = 0; j < len*8; j++) {
        int bit = (data[j >> 3] >> (7 - (j & 7))) & 1;
        appendBitsToBuffer((unsigned int)bit, 1, QRCODE, &bitLen);
    }
	
	// Add terminator and pad up to a byte if applicable
	appendBitsToBuffer(0, 4, QRCODE, &bitLen);

	int dataCapacityBits = getNumDataCodewords() * 8;
	
	// Pad with alternating bytes until data capacity is reached
	for (uint8_t padByte = 0xEC; bitLen < dataCapacityBits; padByte ^= 0xEC ^ 0x11)
		appendBitsToBuffer(padByte, 8, QRCODE, &bitLen);
	
	// Draw function and data codeword modules
    // debugBorder(BWhite);
	addEccAndInterleave(QRCODE, TMPBUFFER);
    // debugBorder(BLightBlue);
	initializeFunctionModules(QRVERSION, QRCODE);
    // debugBorder(BLightGreen); //***
	drawCodewords();
    // debugBorder(BLightRed);
	drawWhiteFunctionModules();
    // debugBorder(BLightYellow);
	initializeFunctionModules(QRVERSION, TMPBUFFER);
    // debugBorder(BDarkRed); 
    applyMask0();
    // debugBorder(BDarkYellow);
	drawFormatBits();
    // debugBorder(BBlack);
        
    return QRCODE;
}

bool qr(uint8_t x, uint8_t y) {
    if (!x) return 1;
    if (!y) return 1;
    if (x>QRSIZE) return 1;
    if (y>QRSIZE) return 1;
    return !qr_get(x-1,y-1);
}

