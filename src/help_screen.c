#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include "platform_cart_type.h"
#include "common.h"
#include "input.h"

#include "ui_main.h"
#include "save_and_undo.h"

#include <help_page.h>      // BG APA style image

#pragma bank 255  // Autobanked

static bool is_startup_help = true;

// Draws the paint working area
void help_page_show(void) NONBANKED {

    HIDE_SPRITES;
    // DISPLAY_OFF;

    drawing_take_undo_snapshot();  // This clears out any Redo queue entries that might be present

    uint8_t save_bank = CURRENT_BANK;
    PLAT_SWITCH_ROM(BANK(help_page));
    draw_image(help_page_tiles);
    PLAT_SWITCH_ROM(save_bank);

    // DISPLAY_ON;

    waitpadticked_lowcpu(J_ANY);

    if (is_startup_help) {
        // Alternate Solaris CDE theme by pressing select on first help screen
        if (KEY_PRESSED(J_SELECT)) app_state.solaris_cde_ui_theme = true;
        is_startup_help = false;
    }

    ui_redraw_full();
    drawing_restore_undo_snapshot(UNDO_RESTORE_WITHOUT_REDO_SNAPSHOT);  // Don't create a Redo snapshot since it would be of the QRCode overlay on the drawing image

    // Can't use gbdk lib waitpadup() when SGB mouse hook is running since
    // it seems to OR in mouse data that is always high, causing a hang
    waitpadup_lowcpu(J_ANY);

    SHOW_SPRITES;
}