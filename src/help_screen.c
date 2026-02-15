#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include "common.h"
#include "input.h"

#include "ui_main.h"
#include "save_and_undo.h"

#include <help_page.h>      // BG APA style image

#pragma bank 255  // Autobanked


// Draws the paint working area
void help_page_show(void) NONBANKED {

    HIDE_SPRITES;
    // DISPLAY_OFF;

    drawing_take_undo_snapshot();  // This clears out any Redo queue entries that might be present

    uint8_t save_bank = CURRENT_BANK;
    SWITCH_ROM(BANK(help_page));
    draw_image(help_page_tiles);
    SWITCH_ROM(save_bank);

    // DISPLAY_ON;

    waitpadticked_lowcpu(J_ANY);

    ui_redraw_after_qrcode();
    drawing_restore_undo_snapshot(UNDO_RESTORE_WITHOUT_REDO_SNAPSHOT);  // Don't create a Redo snapshot since it would be of the QRCode overlay on the drawing image

    SHOW_SPRITES;
}