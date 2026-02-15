#include <gbdk/platform.h>
#include <stdint.h>

#include "common.h"
#include "input.h"

#include "img_2_qrcode.h"
#include "draw.h"
#include "ui_main.h"
#include "save_and_undo.h"

void make_and_show_qrcode(void);

void make_and_show_qrcode(void) {

    drawing_take_undo_snapshot();  // This means generating a QRCode clears out any Redo queue entries that might be present
    image_to_png_qrcode_url();
    set_pal_qrmode();

        // Much more efficient than making a white border by shifting the tile-aligned QRCode output
        scroll_bkg(0,-1);

        // Wait for the user to press a button before clearing QRCode
        waitpadticked_lowcpu(J_ANY);
        waitpadup();

        scroll_bkg(0,1);  // Restore default scroll

    set_pal_normal();
    ui_redraw_after_qrcode();
    drawing_restore_undo_snapshot(UNDO_RESTORE_WITHOUT_REDO_SNAPSHOT);  // Don't create a Redo snapshot since it would be of the QRCode overlay on the drawing image

    SHOW_SPRITES;
}


void main(void)
{
    HIDE_SPRITES;
    ENABLE_RAM;
    SWITCH_RAM(SRAM_BANK_CALC_BUFFER); // RAM bank 0

    if (_cpu == CGB_TYPE) {
        cpu_fast();
        set_pal_normal();
        update_cursor_style_to_draw();
    }

    UPDATE_KEYS();

    app_state_reset();
    ui_init();
    draw_init();

    help_page_show();

    SHOW_SPRITES;

    // Loop forever
    while(1) {

        UPDATE_KEYS();
        ui_update();

        if (KEYS() & J_START) {
            make_and_show_qrcode();
        }

        vsync();
    }
}


