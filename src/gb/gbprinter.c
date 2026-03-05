#include <gbdk/platform.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "../save_and_undo.h"
#include "../common.h"

#include "gbprinter.h"

#pragma bank 255  // Autobanked

#define REINIT_SEIKO

#define RET_ERR_IF_FAIL(CMD)  if ((error = CMD) & PRN_STATUS_MASK_ERRORS_AND_SUM) return error

#define START_TRANSFER 0x81
#if (CGB_FAST_TRANSFER==1)
    // 0b10000011 - start, CGB double speed, internal clock
    #define START_TRANSFER_FAST 0x83
#else
    // 0b10000001 - start, internal clock
    #define START_TRANSFER_FAST 0x81
#endif
#define PRN_BUSY_TIMEOUT        PRN_SECONDS(2)
#define PRN_COMPLETION_TIMEOUT  PRN_SECONDS(20)
#define PRN_SEIKO_RESET_TIMEOUT 10
#define PRN_FULL_TIMEOUT        PRN_SECONDS(2)

#define PRN_FINAL_MARGIN        0x03
#define PRN_STATUS_MASK_ERRORS_AND_SUM (PRN_STATUS_MASK_ERRORS | PRN_STATUS_SUM)

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

uint8_t printer_completion = 0;
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
        if (error & PRN_STATUS_MASK_ERRORS_AND_SUM) break;
        vsync();
    }
    return error;
}

uint8_t gbprinter_detect(uint8_t delay) BANKED {
    printer_init();
    return printer_wait(delay, PRN_STATUS_MASK_ANY, PRN_STATUS_OK);
}


#define TILE_BANK_0 _VRAM8800
#define TILE_BANK_1 _VRAM8000

#define TILE_BYTES_SZ              (16u)
#define APA_TILE_SRC_TOGGLE_TILE_Y (72u / 8u)
#define APA_TILE_NUM_UPPER_START   (128u)

// Prints the requested tile region of the screen in APA mode, tiles outside the screen are printed WHITE
// uint8_t gbprinter_print_screen_rect_from_undo(uint8_t sx, uint8_t sy, uint8_t sw, uint8_t sh, uint8_t centered) BANKED {
uint8_t gbprinter_print_screen_rect_from_undo(void) BANKED {
    static uint8_t error;

    // Printing from an undo snapshot means the size of the image is fixed
    const uint8_t sx = IMG_TILE_X_START;
    const uint8_t sy = IMG_TILE_Y_START;
    const uint8_t sw = IMG_WIDTH_TILES;
    const uint8_t sh = IMG_HEIGHT_TILES;
    const uint8_t centered = true;

    uint8_t * p_undo_tile_data = undo_get_last_snapshot_addr();

    // call printer progress: zero progress
    printer_completion = 0; // call_far(&printer_progress_handler);

    uint8_t tile_data[0x10], rows = ((sh & 0x01) ? (sh + 1) : sh), pkt_count = 0, x_ofs = (centered) ? ((PRN_TILE_WIDTH - sw) >> 1) : 0;

    printer_tile_num = 0;

    // Return early if the area to print is zero
    // if ((sw == 0u) || (sh == 0u)) return PRN_STATUS_OK;

    for (uint8_t y = 0; y != rows; y++) {
        // uint8_t * map_addr = get_bkg_xy_addr(sx, y + sy);
        for (uint8_t x = 0; x != PRN_TILE_WIDTH; x++) {
            if ((x >= x_ofs) && (x < (x_ofs + sw)) && (y < sh))  {

                // uint8_t tile = get_vram_byte(map_addr++);
                // uint8_t * source = (((y + sy) >= APA_TILE_SRC_TOGGLE_TILE_Y) && (tile < APA_TILE_NUM_UPPER_START)) ? _VRAM9000 : _VRAM8000;
                // vmemcpy(tile_data, source + ((uint16_t)tile << 4), sizeof(tile_data));

                // Since the undo snapshot contains Only and All of
                // the tiles we want, in sequential order, they can
                // be iterated through serially instead of checking
                // the map and then looking them up based on that
                memcpy(tile_data, p_undo_tile_data, sizeof(tile_data));
                p_undo_tile_data += TILE_BYTES_SZ;

            } else memset(tile_data, 0x00, sizeof(tile_data));

            // Send tile to printer
            // On first tile, a packet header is emitted beforehand
            // On last tile (40 tiles, 2 screen tile rows) it completes the packet (and the call returns true)
            if (printer_print_tile(tile_data)) {
                pkt_count++;
                if (printer_check_cancel()) {
                    PRINTER_SEND_COMMAND(PRN_PKT_CANCEL);
                    return PRN_STATUS_CANCELLED;
                }
            }

            // Once 18 tile rows have been printed (9 x 2 per print packet)
            // Send the EOF and then Print commands
            if (pkt_count == 9) {
                pkt_count = 0;
                RET_ERR_IF_FAIL(PRINTER_SEND_COMMAND(PRN_PKT_EOF));
                // Some emulators don't set PRN_STATUS_FULL bit until PRN_PKT_START, so this may fail on them.
                // It also seems optional since some OEM games skip this check and call print immediately.
                RET_ERR_IF_FAIL(printer_wait(PRN_FULL_TIMEOUT, PRN_STATUS_FULL, PRN_STATUS_FULL));

                gbprinter_set_print_params((y == (rows - 1)) ? PRN_FINAL_MARGIN : PRN_NO_MARGINS, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
                RET_ERR_IF_FAIL(PRINTER_SEND_COMMAND(PRN_PKT_START));
                // query printer status
                RET_ERR_IF_FAIL(printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY));
                RET_ERR_IF_FAIL(printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0));
#ifdef REINIT_SEIKO
                // reinit printer (required by Seiko?)
                if (y != (rows - 1)) {
                    PRINTER_SEND_COMMAND(PRN_PKT_INIT);
                    RET_ERR_IF_FAIL(printer_wait(PRN_SEIKO_RESET_TIMEOUT, PRN_STATUS_MASK_ANY, PRN_STATUS_OK));
                }
#endif
                // call printer progress callback
                uint8_t current_progress = (((uint16_t)y * PRN_MAX_PROGRESS) / rows);
                if (printer_completion != current_progress) {
                    printer_completion = current_progress; //, call_far(&printer_progress_handler);
                }
            }
        }
    }

    // End of printing, finalize any data that hasn't been printed
    if (pkt_count) {
        RET_ERR_IF_FAIL(PRINTER_SEND_COMMAND(PRN_PKT_EOF));
        // See earlier comment about PRN_STATUS_FULL
        RET_ERR_IF_FAIL(printer_wait(PRN_FULL_TIMEOUT, PRN_STATUS_FULL, PRN_STATUS_FULL));

        // setup printing if required
        gbprinter_set_print_params(PRN_FINAL_MARGIN, PRN_PALETTE_NORMAL, PRN_EXPOSURE_DARK);
        RET_ERR_IF_FAIL(PRINTER_SEND_COMMAND(PRN_PKT_START));
        // query printer status
        RET_ERR_IF_FAIL(printer_wait(PRN_BUSY_TIMEOUT, PRN_STATUS_BUSY, PRN_STATUS_BUSY));
        RET_ERR_IF_FAIL(printer_wait(PRN_COMPLETION_TIMEOUT, PRN_STATUS_BUSY, 0));

        // indicate 100% completion
        printer_completion = PRN_MAX_PROGRESS; //, call_far(&printer_progress_handler);
    }
    return PRINTER_SEND_COMMAND(PRN_PKT_STATUS);
}
