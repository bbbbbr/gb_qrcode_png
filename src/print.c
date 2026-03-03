#include <gbdk/platform.h>
#include <stdint.h>
#include <gb/drawing.h>

#include "common.h"
#include "input.h"

#include "ui_main.h"
#include "save_and_undo.h"
#include "gbprinter.h"



#pragma bank 255  // Autobanked


bool printer_check_cancel(void) BANKED {
    UPDATE_KEYS();
    return (KEY_TICKED(J_B));
}


void print_drawing(void) BANKED {

    drawing_take_undo_snapshot();  // This clears out any Redo queue entries that might be present

    color(WHITE, BLACK, SOLID);
    gotogxy(5u,4u);

    // Printing may be more reliable at DMG link speed
    if (_cpu == CGB_TYPE) cpu_slow();

    bool printer_found = gbprinter_detect(PRINTER_DETECT_TIMEOUT) == PRN_STATUS_OK;
    if (printer_found) {
        // gbprinter_print_screen_apa(IMG_TILE_X_START, IMG_TILE_Y_START, IMG_TILE_X_END, IMG_TILE_Y_END);
        gbprinter_print_screen_rect(IMG_TILE_X_START, IMG_TILE_Y_START, IMG_WIDTH_TILES, IMG_HEIGHT_TILES, true);
        gprintf("Printing");
        gotogxy(5u,5u);        
        gprintf("Done");
    } else {
        gprintf("Printer");
        gotogxy(5u,5u);        
        gprintf("Not Found");
    }

    if (_cpu == CGB_TYPE) cpu_fast();

    waitpadup_lowcpu(J_ALL);
    waitpadticked_lowcpu(J_ANY);
    waitpadup_lowcpu(J_ALL);

    ui_redraw_full();
    drawing_restore_undo_snapshot(UNDO_RESTORE_WITHOUT_REDO_SNAPSHOT);  // Don't create a Redo snapshot since it would be of the QRCode overlay on the drawing image
}
