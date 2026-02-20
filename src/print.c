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
    static uint8_t keys = 0, old_keys;
    old_keys = keys; keys = joypad();
    return (((old_keys ^ keys) & J_B) & (keys & J_B));
}


void print_drawing(void) BANKED {

    drawing_take_undo_snapshot();  // This clears out any Redo queue entries that might be present

    color(WHITE, BLACK, SOLID);
    gotogxy(5u,4u);

    bool printer_found = gbprinter_detect(PRINTER_DETECT_TIMEOUT) == PRN_STATUS_OK;
    if (printer_found) {
        gbprinter_print_screen_apa(IMG_TILE_X_START, IMG_TILE_Y_START, IMG_TILE_X_END, IMG_TILE_Y_END);
        gprintf("Printing");
        gotogxy(5u,5u);        
        gprintf("Done");
    } else {
        gprintf("Printer");
        gotogxy(5u,5u);        
        gprintf("Not Found");
    }

    waitpadup();
    waitpadticked_lowcpu(J_ANY);
    waitpadup();
    UPDATE_KEYS();

    ui_redraw_full();
    drawing_restore_undo_snapshot(UNDO_RESTORE_WITHOUT_REDO_SNAPSHOT);  // Don't create a Redo snapshot since it would be of the QRCode overlay on the drawing image
}