#include <gbdk/platform.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "gbprinter.h"

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#pragma bank 255  // Autobanked

#define REINIT_SEIKO

#define START_TRANSFER          0x81
#define PRN_BUSY_TIMEOUT        PRN_SECONDS(2)
#define PRN_COMPLETION_TIMEOUT  PRN_SECONDS(20)
#define PRN_SEIKO_RESET_TIMEOUT 10

#define PRN_FINAL_MARGIN        0x03

static const uint8_t PRN_PKT_INIT[]    = { PRN_LE(PRN_MAGIC), PRN_LE(PRN_CMD_INIT),   PRN_LE(0), PRN_LE(0x01), PRN_LE(0) };
static const uint8_t PRN_PKT_STATUS[]  = { PRN_LE(PRN_MAGIC), PRN_LE(PRN_CMD_STATUS), PRN_LE(0), PRN_LE(0x0F), PRN_LE(0) };
static const uint8_t PRN_PKT_EOF[]     = { PRN_LE(PRN_MAGIC), PRN_LE(PRN_CMD_DATA),   PRN_LE(0), PRN_LE(0x04), PRN_LE(0) };
static const uint8_t PRN_PKT_CANCEL[]  = { PRN_LE(PRN_MAGIC), PRN_LE(PRN_CMD_BREAK),  PRN_LE(0), PRN_LE(0x01), PRN_LE(0) };

start_print_pkt_t PRN_PKT_START = {
    .magic = PRN_MAGIC, .command = PRN_CMD_PRINT, .length = 4,
    .print = TRUE, .margins = 0, .palette = PRN_PALETTE_NORMAL, .exposure = PRN_EXPOSURE_DARK,
    .crc = 0, .trail = 0
};

static uint16_t printer_status;
static uint8_t printer_tile_num;

static uint8_t printer_send_receive(uint8_t b) {
    SB_REG = b;
    SC_REG = 0x81;
    while (SC_REG & 0x80);
    return SB_REG;
}

static uint8_t printer_send_byte(uint8_t b) {
    return (uint8_t)(printer_status = ((printer_status << 8) | printer_send_receive(b)));
}

static uint8_t printer_send_command(const uint8_t *command, uint8_t length) {
    uint8_t index = 0;
    while (index++ < length) printer_send_byte(*command++);
    return ((uint8_t)(printer_status >> 8) == PRN_MAGIC_DETECT) ? (uint8_t)printer_status : PRN_STATUS_MASK_ERRORS;
}
#define PRINTER_SEND_COMMAND(CMD) printer_send_command((const uint8_t *)&(CMD), sizeof(CMD))

static uint8_t printer_print_tile(const uint8_t *tiledata) {
    static const uint8_t PRINT_TILE[] = { 0x88,0x33,0x04,0x00,0x80,0x02 };
    static uint16_t printer_CRC;
    if (printer_tile_num == 0) {
        const uint8_t * data = PRINT_TILE;
        for (uint8_t i = sizeof(PRINT_TILE); i != 0; i--) printer_send_receive(*data++);
        printer_CRC = 0x04 + 0x80 + 0x02;
    }
    for(uint8_t i = 0x10; i != 0; i--, tiledata++) {
        printer_CRC += *tiledata;
        printer_send_receive(*tiledata);
    }
    if (++printer_tile_num == 40) {
        printer_send_receive((uint8_t)printer_CRC);
        printer_send_receive((uint8_t)(printer_CRC >> 8));
        printer_send_receive(0x00);
        printer_send_receive(0x00);
        printer_CRC = printer_tile_num = 0;
        return TRUE;
    }
    return FALSE;
}

inline void printer_init(void) {
    printer_tile_num = 0;
    PRINTER_SEND_COMMAND(PRN_PKT_INIT);
}

extern bool printer_check_cancel(void) BANKED;

static uint8_t printer_wait(uint16_t timeout, uint8_t mask, uint8_t value) {
    uint8_t error;
    while (((error = PRINTER_SEND_COMMAND(PRN_PKT_STATUS)) & mask) != value) {
        if (printer_check_cancel()) {
            PRINTER_SEND_COMMAND(PRN_PKT_CANCEL);
            return PRN_STATUS_CANCELLED;
        }
        if (timeout-- == 0) return PRN_STATUS_MASK_ERRORS;
        if (error & PRN_STATUS_MASK_ERRORS) break;
        vsync();
    }
    return error;
}

uint8_t gbprinter_detect(uint8_t delay) BANKED {
    printer_init();
    return printer_wait(delay, PRN_STATUS_MASK_ANY, PRN_STATUS_OK);
}

/*
uint8_t gbprinter_print_image(const uint8_t * image_map, const uint8_t * image, int8_t pos_x, uint8_t width, uint8_t height) BANKED {
    static const uint8_t * img;
    static uint8_t error;

    uint8_t tile_data[16], rows = (((height + 1) >> 1) << 1), pkt_count = 0;

    if ((rows >> 1) == 0) return PRN_STATUS_OK;

    img = image;

    printer_tile_num = 0;

    for (uint8_t y = 0; y != rows; y++) {
        for (int16_t x = 0; x != PRN_TILE_WIDTH; x++) {
            // overlay the picture tile if in range
            if ((y < height) && (x >= pos_x) && (x < (pos_x + width))) {
                uint8_t tile = image_map[(y * width) + (x - pos_x)];
                memcpy(tile_data, img + ((uint16_t)tile << 4), sizeof(tile_data));
            } else {
                memset(tile_data, 0, sizeof(tile_data));
            }
            // print the resulting tile
            if (printer_print_tile(tile_data)) {
                pkt_count++;
                if (printer_check_cancel()) {
                    PRINTER_SEND_COMMAND(PRN_PKT_CANCEL);
                    return PRN_STATUS_CANCELLED;
                }
            }
            if (pkt_count == 9) {
                pkt_count = 0;
                PRINTER_SEND_COMMAND(PRN_PKT_EOF);
                // setup margin if last packet
                gbprinter_set_print_params((y == (rows - 1)) ? PRN_FINAL_MARGIN : PRN_NO_MARGINS, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
                PRINTER_SEND_COMMAND(PRN_PKT_START);
                // query printer status
                if ((error = printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY)) & PRN_STATUS_MASK_ERRORS) return error;
                if ((error = printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0)) & PRN_STATUS_MASK_ERRORS) return error;
#ifdef REINIT_SEIKO
                // reinit printer (required by Seiko?)
                if (y != (rows - 1)) {
                    PRINTER_SEND_COMMAND(PRN_PKT_INIT);
                    if (error = printer_wait(PRN_SEIKO_RESET_TIMEOUT, PRN_STATUS_MASK_ANY, PRN_STATUS_OK)) return error;
                }
#endif
            }
        }
    }
    if (pkt_count) {
        PRINTER_SEND_COMMAND(PRN_PKT_EOF);
        // setup printing if required
        gbprinter_set_print_params(PRN_FINAL_MARGIN, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
        PRINTER_SEND_COMMAND(PRN_PKT_START);
        // query printer status
        if ((error = printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY)) & PRN_STATUS_MASK_ERRORS) return error;
        if ((error = printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0)) & PRN_STATUS_MASK_ERRORS) return error;
    }
    return PRINTER_SEND_COMMAND(PRN_PKT_STATUS);
}
*/


#define TILE_BYTES_SZ              (16u)
#define APA_TILE_SRC_TOGGLE_TILE_Y (72u / 8u)
#define APA_TILE_NUM_UPPER_START   (128u)

// Prints the requested tile region of the screen in APA mode, tiles outside the screen are printed WHITE
// Display should be turned off before calling
uint8_t gbprinter_print_screen_apa(uint8_t tile_x_start,uint8_t tile_y_start,uint8_t tile_x_end,uint8_t tile_y_end) BANKED {
    static const uint8_t * img;
    static uint8_t error;

    uint8_t tile_data[16], pkt_count = 0;

    printer_tile_num = 0;

    // Print size is entire GB screen size
    for (uint16_t tile_y = 0; tile_y < DEVICE_SCREEN_HEIGHT; tile_y++) {
        for (uint16_t tile_x = 0; tile_x < PRN_TILE_WIDTH; tile_x++) {

            // If this is a tile within the active print area then print it
            // Otherwise print an empty tile
            if ((tile_x >= tile_x_start) && (tile_x <= tile_x_end) &&
                (tile_y >= tile_y_start) && (tile_y <= tile_y_end)) 
            {
                // Get tile_id from hardware map
                uint16_t tile_id = get_vram_byte(_SCRN0 + (tile_y * DEVICE_SCREEN_BUFFER_WIDTH) + tile_x);
                uint8_t * p_vram = _VRAM8000;

                // Mid-way through the screen APA mode switches from low tile index values
                // sourced from 0x8000 to 0x9000
                if ((tile_y >= APA_TILE_SRC_TOGGLE_TILE_Y) && (tile_id < APA_TILE_NUM_UPPER_START))
                    p_vram = _VRAM9000;

                // Calculate offset into tile data based on tile id                
                p_vram += (tile_id * TILE_BYTES_SZ);
                // EMU_printf("%u,%u tile_id=%hu, vram=%x\n", (uint16_t)tile_x, (uint16_t)tile_y, (uint8_t)tile_id, p_vram);
                vmemcpy(tile_data, p_vram, sizeof(tile_data));
            } else {
                // EMU_printf("%u,%u [BORDER]\n", (uint16_t)tile_x, (uint16_t)tile_y);
                vmemset(tile_data, 0, sizeof(tile_data));
            }

            // print the resulting tile
            if (printer_print_tile(tile_data)) {
                pkt_count++;
                if (printer_check_cancel()) {
                    PRINTER_SEND_COMMAND(PRN_PKT_CANCEL);
                    return PRN_STATUS_CANCELLED;
                }
            }
            if (pkt_count == 9) {
                pkt_count = 0;
                PRINTER_SEND_COMMAND(PRN_PKT_EOF);
                // setup margin if last packet
                gbprinter_set_print_params((tile_y == (DEVICE_SCREEN_HEIGHT - 1)) ? PRN_FINAL_MARGIN : PRN_NO_MARGINS, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
                PRINTER_SEND_COMMAND(PRN_PKT_START);
                // query printer status
                if ((error = printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY)) & PRN_STATUS_MASK_ERRORS) return error;
                if ((error = printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0)) & PRN_STATUS_MASK_ERRORS) return error;
#ifdef REINIT_SEIKO
                // reinit printer (required by Seiko?)
                if (tile_y != (DEVICE_SCREEN_HEIGHT - 1)) {
                    PRINTER_SEND_COMMAND(PRN_PKT_INIT);
                    if (error = printer_wait(PRN_SEIKO_RESET_TIMEOUT, PRN_STATUS_MASK_ANY, PRN_STATUS_OK)) return error;
                }
#endif
            }
        }
    }
    if (pkt_count) {
        PRINTER_SEND_COMMAND(PRN_PKT_EOF);
        // setup printing if required
        gbprinter_set_print_params(PRN_FINAL_MARGIN, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
        PRINTER_SEND_COMMAND(PRN_PKT_START);
        // query printer status
        if ((error = printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY)) & PRN_STATUS_MASK_ERRORS) return error;
        if ((error = printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0)) & PRN_STATUS_MASK_ERRORS) return error;
    }
    return PRINTER_SEND_COMMAND(PRN_PKT_STATUS);
}
