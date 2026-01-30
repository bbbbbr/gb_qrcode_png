#ifndef _QRCODEGEN_H
#define _QRCODEGEN_H

BANKREF_EXTERN(qrcodegen)

// ========== Configurable Settings ==========

// See: https://www.qrcode.com/en/about/version.html
// Also, see "Byte sz" column table further below for selecting QRVERSION
// Only configured for "Byte" mode operation
//
#define QRVERSION 3                   // <---- Configurable to change size and capacity <----
// #define QRVERSION 4


// Manually set this based on the "Byte sz" column that matches:
// - The "Error Correction" Low setting
// - And the "Version" which matches QRVERSION above
//
#define QR_MAX_PAYLOAD_BYTES 53u      // <---- Change this based on the table below and QRVERSION above <----
// #define QR_MAX_PAYLOAD_BYTES 78u  // Size for Version "4"
//
// For example these are the defaults for Version "3"
// | Version   |                             |   Byte sz     |
// | --> 3 <-- | 29 x 29   | LOW  | 127 | 77 | --> 53 <--    |
//
// Anecdotally the trailing string terminator may be excluded from
// the total byte count


#define QRECL qrcodegen_Ecc_LOW


// ========== Below are Non-Configurable Calculations ==========
// #define QRPAD 32

// This calculates the width and height in pixels ("modules")
// of the QR Code image PRIOR to borders
#define QRSIZE (QRVERSION * 4 + 17)

// QR Output row size in bytes should be:
//   pixel width / 8, rounded up to nearest whole value of 8
#define PIXELS_PER_BYTE  8u // 1bpp, so 8 horizontal pixels per byte
#define QR_OUTPUT_ROW_SZ_BYTES  ((QRSIZE + (PIXELS_PER_BYTE - 1u)) / PIXELS_PER_BYTE)

// The output QRCode is stored internally as a 1bpp image with
// each row composed of bytes, where a byte stores 8 sequential horizontal pixels.
//
// So the size of the image buffer is~ ROUND_UP(WIDTH / 8) * HEIGHT

// This is the Width and Height of the QR code image prior to any user scaling
// 1 pixel of border on each side, hence +2
#define QR_BORDER_WIDTH 1u
#define QR_FINAL_PIXEL_WIDTH  (QRSIZE + (QR_BORDER_WIDTH * 2u))
#define QR_FINAL_PIXEL_HEIGHT (QRSIZE + (QR_BORDER_WIDTH * 2u))



// USING_MODULE(qrcodegen, PAGE_D);


// uint8_t *qrcodegen(const char *text);
uint8_t *qrcodegen(const char *text, uint16_t len);
bool qr(uint8_t x, uint8_t y);

bool qr_get(uint8_t x, uint8_t y);
uint8_t qr_get8(uint8_t x, uint8_t y);

#endif // _QRCODEGEN_H

/*
| Version | Size      | Error Correction | Num sz | Alpha sz | Byte sz |
|---------|-----------|------------------|--------|----------|---------|
| 1       | 21 x 21   | LOW              | 41     | 25       | 17      |
| 1       | 21 x 21   | MEDIUM           | 34     | 20       | 14      |
| 1       | 21 x 21   | QUARTILE         | 27     | 16       | 11      |
| 1       | 21 x 21   | HIGH             | 17     | 10       | 7       |
| 2       | 25 x 25   | LOW              | 77     | 47       | 32      |
| 2       | 25 x 25   | MEDIUM           | 63     | 38       | 26      |
| 2       | 25 x 25   | QUARTILE         | 48     | 29       | 20      |
| 2       | 25 x 25   | HIGH             | 34     | 20       | 14      |
| 3       | 29 x 29   | LOW              | 127    | 77       | 53      |
| 3       | 29 x 29   | MEDIUM           | 101    | 61       | 42      |
| 3       | 29 x 29   | QUARTILE         | 77     | 47       | 32      |
| 3       | 29 x 29   | HIGH             | 58     | 35       | 24      |
| 4       | 33 x 33   | LOW              | 187    | 114      | 78      |
| 4       | 33 x 33   | MEDIUM           | 149    | 90       | 62      |
| 4       | 33 x 33   | QUARTILE         | 111    | 67       | 46      |
| 4       | 33 x 33   | HIGH             | 82     | 50       | 34      |
| 5       | 37 x 37   | LOW              | 255    | 154      | 106     |
| 5       | 37 x 37   | MEDIUM           | 202    | 122      | 84      |
| 5       | 37 x 37   | QUARTILE         | 144    | 87       | 60      |
| 5       | 37 x 37   | HIGH             | 106    | 64       | 44      |
| 6       | 41 x 41   | LOW              | 322    | 195      | 134     |
| 6       | 41 x 41   | MEDIUM           | 255    | 154      | 106     |
| 6       | 41 x 41   | QUARTILE         | 178    | 108      | 74      |
| 6       | 41 x 41   | HIGH             | 139    | 84       | 58      |
| 7       | 45 x 45   | LOW              | 370    | 224      | 154     |
| 7       | 45 x 45   | MEDIUM           | 293    | 178      | 122     |
| 7       | 45 x 45   | QUARTILE         | 207    | 125      | 86      |
| 7       | 45 x 45   | HIGH             | 154    | 93       | 64      |
| 8       | 49 x 49   | LOW              | 461    | 279      | 192     |
| 8       | 49 x 49   | MEDIUM           | 365    | 221      | 152     |
| 8       | 49 x 49   | QUARTILE         | 259    | 157      | 108     |
| 8       | 49 x 49   | HIGH             | 202    | 122      | 84      |
| 9       | 53 x 53   | LOW              | 552    | 335      | 230     |
| 9       | 53 x 53   | MEDIUM           | 432    | 262      | 180     |
| 9       | 53 x 53   | QUARTILE         | 312    | 189      | 130     |
| 9       | 53 x 53   | HIGH             | 235    | 143      | 98      |
| 10      | 57 x 57   | LOW              | 652    | 395      | 271     |
| 10      | 57 x 57   | MEDIUM           | 513    | 311      | 213     |
| 10      | 57 x 57   | QUARTILE         | 364    | 221      | 151     |
| 10      | 57 x 57   | HIGH             | 288    | 174      | 119     |
| 11      | 61 x 61   | LOW              | 772    | 468      | 321     |
| 11      | 61 x 61   | MEDIUM           | 604    | 366      | 251     |
| 11      | 61 x 61   | QUARTILE         | 427    | 259      | 177     |
| 11      | 61 x 61   | HIGH             | 331    | 200      | 137     |
| 12      | 65 x 65   | LOW              | 883    | 535      | 367     |
| 12      | 65 x 65   | MEDIUM           | 691    | 419      | 287     |
| 12      | 65 x 65   | QUARTILE         | 489    | 296      | 203     |
| 12      | 65 x 65   | HIGH             | 374    | 227      | 155     |
| 13      | 69 x 69   | LOW              | 1022   | 619      | 425     |
| 13      | 69 x 69   | MEDIUM           | 796    | 483      | 331     |
| 13      | 69 x 69   | QUARTILE         | 580    | 352      | 241     |
| 13      | 69 x 69   | HIGH             | 427    | 259      | 177     |
| 14      | 73 x 73   | LOW              | 1101   | 667      | 458     |
| 14      | 73 x 73   | MEDIUM           | 871    | 528      | 362     |
| 14      | 73 x 73   | QUARTILE         | 621    | 376      | 258     |
| 14      | 73 x 73   | HIGH             | 468    | 283      | 194     |
| 15      | 77 x 77   | LOW              | 1250   | 758      | 520     |
| 15      | 77 x 77   | MEDIUM           | 991    | 600      | 412     |
| 15      | 77 x 77   | QUARTILE         | 703    | 426      | 292     |
| 15      | 77 x 77   | HIGH             | 530    | 321      | 220     |
| 16      | 81 x 81   | LOW              | 1408   | 854      | 586     |
| 16      | 81 x 81   | MEDIUM           | 1082   | 656      | 450     |
| 16      | 81 x 81   | QUARTILE         | 775    | 470      | 322     |
| 16      | 81 x 81   | HIGH             | 602    | 365      | 250     |
| 17      | 85 x 85   | LOW              | 1548   | 938      | 644     |
| 17      | 85 x 85   | MEDIUM           | 1212   | 734      | 504     |
| 17      | 85 x 85   | QUARTILE         | 876    | 531      | 364     |
| 17      | 85 x 85   | HIGH             | 674    | 408      | 280     |
| 18      | 89 x 89   | LOW              | 1725   | 1046     | 718     |
| 18      | 89 x 89   | MEDIUM           | 1346   | 816      | 560     |
| 18      | 89 x 89   | QUARTILE         | 948    | 574      | 394     |
| 18      | 89 x 89   | HIGH             | 746    | 452      | 310     |
| 19      | 93 x 93   | LOW              | 1903   | 1153     | 792     |
| 19      | 93 x 93   | MEDIUM           | 1500   | 909      | 624     |
| 19      | 93 x 93   | QUARTILE         | 1063   | 644      | 442     |
| 19      | 93 x 93   | HIGH             | 813    | 493      | 338     |
| 20      | 97 x 97   | LOW              | 2061   | 1249     | 858     |
| 20      | 97 x 97   | MEDIUM           | 1600   | 970      | 666     |
| 20      | 97 x 97   | QUARTILE         | 1159   | 702      | 482     |
| 20      | 97 x 97   | HIGH             | 919    | 557      | 382     |
| 21      | 101 x 101 | LOW              | 2232   | 1352     | 929     |
| 21      | 101 x 101 | MEDIUM           | 1708   | 1035     | 711     |
| 21      | 101 x 101 | QUARTILE         | 1224   | 742      | 509     |
| 21      | 101 x 101 | HIGH             | 969    | 587      | 403     |
| 22      | 105 x 105 | LOW              | 2409   | 1460     | 1003    |
| 22      | 105 x 105 | MEDIUM           | 1872   | 1134     | 779     |
| 22      | 105 x 105 | QUARTILE         | 1358   | 823      | 565     |
| 22      | 105 x 105 | HIGH             | 1056   | 640      | 439     |
| 23      | 109 x 109 | LOW              | 2620   | 1588     | 1091    |
| 23      | 109 x 109 | MEDIUM           | 2059   | 1248     | 857     |
| 23      | 109 x 109 | QUARTILE         | 1468   | 890      | 611     |
| 23      | 109 x 109 | HIGH             | 1108   | 672      | 461     |
| 24      | 113 x 113 | LOW              | 2812   | 1704     | 1171    |
| 24      | 113 x 113 | MEDIUM           | 2188   | 1326     | 911     |
| 24      | 113 x 113 | QUARTILE         | 1588   | 963      | 661     |
| 24      | 113 x 113 | HIGH             | 1228   | 744      | 511     |
| 25      | 117 x 117 | LOW              | 3057   | 1853     | 1273    |
| 25      | 117 x 117 | MEDIUM           | 2395   | 1451     | 997     |
| 25      | 117 x 117 | QUARTILE         | 1718   | 1041     | 715     |
| 25      | 117 x 117 | HIGH             | 1286   | 779      | 535     |
| 26      | 121 x 121 | LOW              | 3283   | 1990     | 1367    |
| 26      | 121 x 121 | MEDIUM           | 2544   | 1542     | 1059    |
| 26      | 121 x 121 | QUARTILE         | 1804   | 1094     | 751     |
| 26      | 121 x 121 | HIGH             | 1425   | 864      | 593     |
| 27      | 125 x 125 | LOW              | 3517   | 2132     | 1465    |
| 27      | 125 x 125 | MEDIUM           | 2701   | 1637     | 1125    |
| 27      | 125 x 125 | QUARTILE         | 1933   | 1172     | 805     |
| 27      | 125 x 125 | HIGH             | 1501   | 910      | 625     |
| 28      | 129 x 129 | LOW              | 3669   | 2223     | 1528    |
| 28      | 129 x 129 | MEDIUM           | 2857   | 1732     | 1190    |
| 28      | 129 x 129 | QUARTILE         | 2085   | 1263     | 868     |
| 28      | 129 x 129 | HIGH             | 1581   | 958      | 658     |
| 29      | 133 x 133 | LOW              | 3909   | 2369     | 1628    |
| 29      | 133 x 133 | MEDIUM           | 3035   | 1839     | 1264    |
| 29      | 133 x 133 | QUARTILE         | 2181   | 1322     | 908     |
| 29      | 133 x 133 | HIGH             | 1677   | 1016     | 698     |
| 30      | 137 x 137 | LOW              | 4158   | 2520     | 1732    |
| 30      | 137 x 137 | MEDIUM           | 3289   | 1994     | 1370    |
| 30      | 137 x 137 | QUARTILE         | 2358   | 1429     | 982     |
| 30      | 137 x 137 | HIGH             | 1782   | 1080     | 742     |
| 31      | 141 x 141 | LOW              | 4417   | 2677     | 1840    |
| 31      | 141 x 141 | MEDIUM           | 3486   | 2113     | 1452    |
| 31      | 141 x 141 | QUARTILE         | 2473   | 1499     | 1030    |
| 31      | 141 x 141 | HIGH             | 1897   | 1150     | 790     |
*/