#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y) {
	// Russian peasant multiplication
	uint8_t z=0;
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


// Range should be 0 - 3
// The output will get placed in banks 1-4
static void gentable(uint8_t range) {

    // Generate tables split across 4 x 16k banks
    uint16_t range_start  = (range & 0x03u) << 6;
    uint16_t range_end    = range_start + (0x100u / 4);
    uint8_t  autobank_num = range + 1u;

    printf("#include <stdint.h>\n\n");
    printf("#pragma bank %hu // Autobanked\n\n", autobank_num);

    printf("// Reed Solomon Multiply LUT x:(%u -> %u)\n\n", range_start, range_end - 1u);
    printf("const uint8_t reed_solomon_mul_lut_%u[] = {", autobank_num);

    for (uint16_t x = range_start; x < range_end; x++) {
        for (uint16_t y = 0u; y < 256u; y++) {
            uint8_t res = reedSolomonMultiply( (uint8_t)x, (uint8_t)y);

            if ((y & 0x1Fu) == 0u) printf("\n    ");
            printf("0x%02hX, ", res);
        }
    }

    printf("\n};\n\n\n");
}


static void show_help(void) {
    printf("Error: incorrect arg count! Usage: reedsolomon_mmul_lut <table> (where table is 0-3)\n");
}


int main( int argc, char *argv[] )  {

    if( argc != 2 ) {
        show_help();
        return EXIT_FAILURE;
    }

    uint8_t table = (uint8_t)strtol(argv[1], NULL, 10);
    if ((table < 0) || (table > 3)) {
        show_help();
        return EXIT_FAILURE;
    }

    gentable(table);
}


