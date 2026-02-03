#include <gbdk/platform.h>
#include <stdint.h>

#include "common.h"
#include "input.h"

#include "img_2_qrcode.h"
#include "draw.h"

const palette_color_t QR_PAL_CGB[]  = {RGB(31, 31, 31), RGB(0,0,0),    RGB(0,0,0),    RGB(0,0,0)};
const palette_color_t DEF_PAL_CGB[] = {RGB(31, 31, 31), RGB(21,21,21),   RGB(10,10,10),   RGB(0,0,0)};

const uint8_t QR_PAL_DMG  = DMG_PALETTE(DMG_WHITE, DMG_BLACK,     DMG_BLACK,     DMG_BLACK);
const uint8_t DEF_PAL_DMG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);

void make_and_show_qrcode(void);


void make_and_show_qrcode(void) {

    drawing_save_to_sram(SRAM_BANK_CUR_DRAWING_CACHE, DRAWING_SAVE_SLOT_MIN);


    image_to_png_qrcode_url();

        if (_cpu == CGB_TYPE)  set_bkg_palette(0u, 1u, QR_PAL_CGB);
        else                   BGP_REG = QR_PAL_DMG;

        // Much more efficient than making a white border by shifting the tile-aligned QRCode output
        scroll_bkg(-1,-1);

        // Wait for the user to press a button before clearing QRCode
        waitpadticked_lowcpu(J_ANY);
        waitpadup();

        scroll_bkg(1,1);

    if (_cpu == CGB_TYPE)  set_bkg_palette(0u, 1u, DEF_PAL_CGB);
    else                   BGP_REG = DEF_PAL_DMG;

    redraw_workarea();
    drawing_restore_from_sram(SRAM_BANK_CUR_DRAWING_CACHE, DRAWING_SAVE_SLOT_MIN);
    drawing_restore_default_colors();

    // redraw_workarea();
    SHOW_SPRITES;
}


void main(void)
{
    ENABLE_RAM;
    SWITCH_RAM(SRAM_BANK_CALC_BUFFER); // RAM bank 0

    if (_cpu == CGB_TYPE) {
        cpu_fast();
        set_default_palette();
    }

    UPDATE_KEYS();

    // printf("Starting\n");
    // EMU_printf("\nStarting\n");

    draw_init();

    // Loop forever
    while(1) {

        UPDATE_KEYS();
        draw_update();

        if (KEYS() & J_START) {
            make_and_show_qrcode();
        }

        vsync();
    }
}


