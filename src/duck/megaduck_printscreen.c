#include <gbdk/platform.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <duck/laptop_io.h>

#include "../save_and_undo.h"
#include "../common.h"

#include "megaduck_printer.h"
#include "megaduck_printscreen.h"

static void printscreen_prepare_tile_row(uint8_t row, uint8_t tile_bitplane_offset);

uint8_t * p_undo_tile_data;

// Currently unknown:
// - Single pass printer probably does not support variable image width
// - Double pass printer might, since it has explicit Carriage Return and Line Feed commands, but it's not verified
//
// So for the time being require full screen image width
//
// The Duck Printer mechanical Carriage Return + Line Feed process takes about
// 500 msec for the print head to travel back to the start of the line.
//
// After that there is about a 600 msec period before the printer head
// starts moving. The ASIC between the CPU and the printer may be
// buffering printer data during that time so it can stream it out
// with the right timing.
//
// So giving the printer ~1000 msec between tile rows is needed
// to avoid printing glitches. The duration was determined through
// trial and error.
//
// The same  ~1000 msec delay should be present after the last
// tile row is sent to allow the printer to finish processing
// that row before sending other commands. For example if this
// isn't done and a keyboard poll is sent immediately after
// the last tile row, then the peripheral controller seems to
// trigger a cpu reset.
//
bool duck_print_rect_from_undo(void) {

    bool return_status = true;
    p_undo_tile_data = undo_get_last_snapshot_addr();  // Gets incremented in printscreen_prepare_tile_row()

    // Check for printer connectivity
    uint8_t printer_type = duck_io_printer_query();
    // Fix up printer status == 3 to be printer type 1, sort of a hack
    if (printer_type == DUCK_IO_PRINTER_MAYBE_BUSY)  printer_type = DUCK_IO_PRINTER_TYPE_1_PASS;
    if (printer_type != DUCK_IO_PRINTER_TYPE_1_PASS) return false;

    // TODO: Do interrupts actually need to be turned off, or will it meet print timing with them on?

    // Turn off VBlank interrupt during printing to avoid disruptions when
    // streaming the tile row data
    uint8_t int_enables_saved = IE_REG;
    set_interrupts(IE_REG & ~VBL_IFLAG);

    // Starting with a blank row (like system rom does) avoids a glitch where
    // a tile is skipped somewhere in the very first row printed
    duck_print_blank_row();

    // Send the tile data row by row
    for (uint8_t map_row = 0; map_row < DEVICE_SCREEN_HEIGHT; map_row++) {
        printscreen_prepare_tile_row(map_row, BITPLANE_BOTH);
        return_status = duck_printer_send_tile_row_1pass();
        if (return_status == false) break;
    }

    // Print up to N blank rows to scroll the printed result up
    // past the printer tear off position
    if (return_status) {
        for (uint8_t blank_row=0u; blank_row < PRINT_NUM_BLANK_ROWS_AT_END; blank_row++) {
            duck_print_blank_row();
        }
    }

    // Restore VBlank interrupt
    set_interrupts(int_enables_saved);

    return return_status;
}


// Prints a blank tile row by filling the pattern data with all zeros
// and then printing a row
bool duck_print_blank_row(void) {

    // Fill print buffer with zero's
    uint8_t * p_buf = tile_row_buffer;
    for (uint8_t c = 0u; c < (DEVICE_SCREEN_WIDTH * BYTES_PER_PRINTER_TILE); c++) {
        *p_buf++ = 0x00u;
    }
    
    return duck_printer_send_tile_row_1pass();
}


// Prepares a tile row from the contents of screen vram
static void printscreen_prepare_tile_row(uint8_t row, uint8_t tile_bitplane_offset) {
    
    uint8_t tile_buffer[BYTES_PER_VRAM_TILE];
    uint8_t * p_row_buffer = tile_row_buffer;

    bool    use_win_data = (((row * TILE_HEIGHT) >= WY_REG) && ( LCDC_REG & LCDCF_WINON));
    uint8_t col = 0;

    if (!use_win_data) {
        // Add Scroll offset to closest tile (if rendering BG instead of Window)
        row += (SCY_REG / TILE_HEIGHT);
        col += (SCX_REG / TILE_WIDTH);
    }

    // Loop through tile columns for the current tile row
    for (uint8_t tile = 0u; tile < DEVICE_SCREEN_WIDTH; tile++) {

        // If tile is within the active drawing area copy it from the undo snapshot
        if ((row >= IMG_TILE_Y_START) && (row <= IMG_TILE_Y_END) &&
            (tile >= IMG_TILE_X_START) && (tile <= IMG_TILE_X_END)) {

                // Since the undo snapshot contains Only and All of
                // the tiles we want, in sequential order, they can
                // be iterated through serially instead of checking
                // the map and then looking them up based on that
                memcpy(tile_buffer, p_undo_tile_data, TILE_SZ_BYTES);
                p_undo_tile_data += TILE_SZ_BYTES;

            } else memset(tile_buffer, 0x00, TILE_SZ_BYTES);

        // Mirror, Rotate -90 degrees and reduce tile to 1bpp
        if (tile_bitplane_offset == BITPLANE_BOTH)
            duck_printer_convert_tile_dithered(p_row_buffer, tile_buffer);
        else
            duck_printer_convert_tile(p_row_buffer, tile_buffer + tile_bitplane_offset);
        p_row_buffer += BYTES_PER_PRINTER_TILE;
    }
}
