#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef HARDWARE_GB_303

// Unused 	DIN 	SCK 	CHIP SELECT 	BANK
// GB-303 EEPROM/ADC Register bits:
// At: 0x2000
// 7: x
// 6: x
// 5: SPI out bit
// 4: SCK
// 3: CS/Chip Select
// 2: -+
// 1:  - Bank (0-7)
// 0: -+

#define POT_ANALOG_CHAN_NONE  0b00000000
#define POT_ANALOG_CHAN_1  0b00010000
#define POT_ANALOG_CHAN_2  0b00011000
#define POT_ANALOG_CHAN_3  0b00001000
#define SPI_READ_CONTINUED 0x00u


#define SPI_OUT_BIT_HI         0x20u  // 0b00100000u
#define SPI_SCK_HI             0x10u  // 0b00010000u
#define CS_LOW_FOR_ADC         0x08u  // 0b00001000u
#define CS_HI_FOR_ACQUISITION  0x00u  // 0b00000000u

#define HIGHEST_BIT 0x80u
#define LOWEST_BIT  0x01u

static uint8_t read_pot(uint8_t pot_channel);
static uint8_t spi_adc_xfer(uint8_t out_byte);

static uint8_t read_pot_asm(uint8_t pot_channel) __naked;
static uint8_t spi_adc_xfer_asm(uint8_t out_byte) __naked;

volatile uint8_t * pot_reg      = (uint8_t *)0x2000u;
volatile uint8_t * pot_read_reg = (uint8_t *)0xA000u;

uint8_t pot1 = 0x00u;
uint8_t pot2 = 0x00u;
uint8_t pot3 = 0x00u;

uint8_t pot1_avg = 0x00u;
uint8_t pot2_avg = 0x00u;
uint8_t pot3_avg = 0x00u;

int16_t pot1_delta = 0x00u;
int16_t pot2_delta = 0x00u;
int16_t pot3_delta = 0x00u;

static bool first_read = true;



// TODO: implement
// gb_303_check_adc_present


//adcbootcheck:
uint8_t gb_303_startup_check(void) {

//  ld     hl,$2000		; CS high, so CS low for ADC
//  ld     (hl),$08
//  nop
    *pot_reg = CS_LOW_FOR_ADC;
    __asm__("nop");

//  ld     c,%00000000
//  call   spicomb    		; Ignore
    spi_adc_xfer(POT_ANALOG_CHAN_NONE); // Discard result

//  ld     c,%00000000
//  call   spicomb		; If last 4 bits are 0 (as datasheet), assume ADC is there and working
 // Last 4 bits indicate whether ADC is present and functional. Apparently == 0 is success
  return  ((spi_adc_xfer(POT_ANALOG_CHAN_NONE) & 0x0Fu) == 0x00u);

//  ld     a,d
//  and    $0F
//  ld     a,$FF
//  jr     z,+
//  xor    a
//+:
//  ld     (HWOK_ADC),a
//  ret
}


// TODO: add nops for double speed mode? (or just generally since prob not timing sensitive)

void gb_303_read_potentiometers(void) {

    uint8_t save_bank = CURRENT_BANK;    
    uint8_t last;

 // TODO: commenting out channel 1 makes channel 3 stop reading
    // pot1 = read_pot(POT_ANALOG_CHAN_1); // TODO:FIXME: This is slightly buggy: only succcessfully reads pots 2 and 3
    // pot3 = read_pot(POT_ANALOG_CHAN_3);
    // pot2 = read_pot(POT_ANALOG_CHAN_2);

    pot1 = read_pot_asm(POT_ANALOG_CHAN_1); // TODO:FIXME: This is slightly buggy: only succcessfully reads pots 2 and 3
    pot2 = read_pot_asm(POT_ANALOG_CHAN_3);
    pot3 = read_pot_asm(POT_ANALOG_CHAN_2);


    if (first_read) {
        pot1_avg = pot1;
        pot2_avg = pot2;
        pot3_avg = pot3;
        first_read = false;
    }

    last = pot1_avg;
    pot1_avg = (uint8_t)( ((uint16_t)pot1_avg + (uint16_t)pot1 + (uint16_t)pot1) / 3u);
    pot1_delta = pot1_avg - last;

    last = pot2_avg;
    pot2_avg = (uint8_t)( ((uint16_t)pot2_avg + (uint16_t)pot2 + (uint16_t)pot2) / 3u);
    pot2_delta = pot2_avg - last;

    last = pot3_avg;
    pot3_avg = (uint8_t)( ((uint16_t)pot3_avg + (uint16_t)pot3 + (uint16_t)pot3) / 3u);
    pot3_delta = pot3_avg - last;

    SWITCH_ROM(save_bank);
}


static uint8_t read_pot(uint8_t pot_channel) {

    // Trigger an ADC Acquisition
    *pot_reg = CS_LOW_FOR_ADC;
    __asm__("nop");
    __asm__("nop");

    *pot_reg = CS_HI_FOR_ACQUISITION;
    __asm__("nop\n nop\n nop\n nop\n");

    *pot_reg = CS_LOW_FOR_ADC;
    __asm__("nop");

    // Read and return the result
    return ((spi_adc_xfer(pot_channel) << 4) | (spi_adc_xfer(SPI_READ_CONTINUED) >> 4) );
}


static uint8_t spi_adc_xfer(uint8_t out_byte) {

    uint8_t xfer;
    uint8_t result = 0x00u;

    for (uint8_t bit = 0u; bit < 8u; bit++) {

        // Outgoing data: prepare to clock out the current highest bit
        if (out_byte & HIGHEST_BIT) xfer = CS_LOW_FOR_ADC;
        else                        xfer = CS_LOW_FOR_ADC | SPI_OUT_BIT_HI;
        // Queue bit for next loop
        out_byte <<= 1;

        // Put the bit out
        *pot_reg = xfer;
        __asm__("nop");

        // SPI Clock High
        xfer |= SPI_SCK_HI;
        *pot_reg = xfer;
        __asm__("nop");

        // Read the returned bit
        result <<= 1;
        result |= ((*pot_read_reg ^ LOWEST_BIT) & LOWEST_BIT);

        // SPI Clock Low
        xfer &= (SPI_OUT_BIT_HI | CS_LOW_FOR_ADC);
        *pot_reg = xfer;
        __asm__("nop");
    }
    return result;
}

// Expects value in pot channel in A
static uint8_t read_pot_asm(uint8_t pot_channel) __naked {
    pot_channel;
    __asm \

    push   hl
    push   bc
    push   de

    ld     hl,#0x2000       ; CS high, so CS low for ADC
    ld     (hl),#0x08
    nop

    ld     hl,#0x2000       ; CS low, so CS high for ADC - Acquisition...
    ld     (hl),#0x00
    nop
    nop
    nop
    nop

    ld     hl,#0x2000       ; CS high, so CS low for ADC
    ld     (hl),#0x08
    nop

    ; Passing channel to read in A
    call   _spi_adc_xfer_asm
    ld     a,d
    and    #0x0F
    push   af

    ; Passing channel to read in A
    ld     a,#0x00
    call   _spi_adc_xfer_asm        ; Get remaining bits
    and    #0xF0
    ld     d,a
    pop    af
    or     d
    swap   a

    pop    de
    pop    bc
    pop    hl

    ret

    __endasm;
}


// Expects out_byte in C
static uint8_t spi_adc_xfer_asm(uint8_t out_byte) __naked {

    out_byte;
    __asm \

    ld     c, a

    // ;Talk to ADC (don't forget CS assertion)
    // ;Out in C, In in D
    ld     d, #0
    ld     b, #8

$1:
    rl     c
    ld     a,#0x08
    jr     nc, $2
    ld     a,#0x28

$2:                              ; 00D0 1000
    ld     (#0x2000),a      ; Data out
    nop

    or     #0x10            ; 00D1 1000
    ld     (#0x2000),a      ; SCK high
    nop

    push   af
    sla    d
    ld     a,(#0xA000)      ; Read SO
    xor    #1
    and    #1
    or     d
    ld     d,a
    pop    af

    and    #0x28                    ; 00D0 1000
    ld     (#0x2000),a      ; SCK low
    nop

    dec    b
    jr     nz,$1

    // Return in A
    ld     a, d

    ret

    __endasm;
}

#endif // HARDWARE_GB_303