#include <gbdk/platform.h>
#include <gb/sgb.h>
#include <stdint.h>

#include "platform_cart_type.h"
#include "common.h"
#include "input.h"

#include "sgb_mouse_on_gb.h"
#include "sgb_border.h"

#include "sgb_border_img.h"


// Command code + data to set BGP / OBP GB Pals on the SNES/SGB
// "Transmit color data for SGB palette 0, color 0-3, and for SGB palette 1, color 1-3 (without separate color 0)."
const uint8_t sgb_cmd_gb_screen_area_palette[] = {
    (SGB_PAL_01 << 3) | 1,
    0xFF, 0xFF, // Light Blue  0xFFFFFF -> 0xFFFF
    0xB5, 0x56, // Med Grey    0xAFAFAF -> B556  [ rgb555(21,21,21) ]
    0x4A, 0x29, // Dark Grey   0x505050 -> 4A29  [ rgb555(10,10,10) ]
    0x00, 0x00  // Black
};


void sgb_check_and_init(void) {

    DISPLAY_ON;

    // Wait 4 frames
    // For SGB on PAL SNES this delay is required on startup, otherwise borders don't show up
    for (uint8_t i = 4; i != 0; i--) vsync();

    sgb_found = sgb_check();
    if (sgb_found) {

        // The display must be ON before calling set_sgb_border()
        uint8_t save_bank = CURRENT_BANK;
        PLAT_SWITCH_ROM(BANK(sgb_border_img));
        set_sgb_border(sgb_border_img_tiles, sizeof(sgb_border_img_tiles), sgb_border_img_map, sizeof(sgb_border_img_map), sgb_border_img_palettes, sizeof(sgb_border_img_palettes));
        PLAT_SWITCH_ROM(save_bank);

        // Set the palettes used by the game boy screen area (OBP/BGP)
        // set_bkg_data(0, (uint8_t)(sizeof(sgb_gb_screen_area_palette) >> 4), sgb_gb_screen_area_palette);
        sgb_transfer(sgb_cmd_gb_screen_area_palette);


        // Apparently mouse setup needs to be called AFTER border is set
        //
        // init joypads and install mouse hook and handler
        joypad_init(4, &joypads);
        sgb_mouse_install();
    }
}
