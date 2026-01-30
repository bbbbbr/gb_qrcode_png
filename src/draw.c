#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>
#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "input.h"

#pragma bank 255  // Autobanked


#define SPRITE_MOUSE_CURSOR 0u
#define CURSOR_POS_UNSET    0xFFu

static const unsigned char mouse_cursors[] = {
  // Arrow 1
  0xFE, 0xFE, 0xFC, 0x84, 0xF8, 0x98, 0xF8, 0xA8,
  0xFC, 0xB4, 0xCE, 0xCA, 0x87, 0x85, 0x03, 0x03,
  // Arrow 2
  0x80, 0x80, 0xC0, 0xC0, 0xE0, 0xA0, 0xF0, 0x90,
  0xF8, 0x88, 0xF0, 0xB0, 0xD8, 0xD8, 0x08, 0x08,
  // Arrow 3
  0x80, 0x80, 0xC0, 0xC0, 0xE0, 0xA0, 0xF0, 0x90,
  0xF8, 0x88, 0xFC, 0x84, 0xF8, 0x98, 0xE0, 0xE0,
  // Hand
  0x10, 0x10, 0x38, 0x28, 0x38, 0x28, 0x7E, 0x6E,
  0xFE, 0xA2, 0xFE, 0x82, 0x7E, 0x42, 0x3E, 0x3E
};

void draw_init(void) BANKED {
    HIDE_BKG;
    HIDE_SPRITES;

    set_sprite_data(0u, 2u, mouse_cursors);
    set_sprite_tile(SPRITE_MOUSE_CURSOR, 1u);
    move_sprite(0, 160u / 2, 144u / 2);

    // == Enters drawing mode ==

    // Fill screen with black
    mode(M_DRAWING);
    color(BLACK, BLACK, SOLID);
    box(0u, 0u, DEVICE_SCREEN_PX_WIDTH - 1u, DEVICE_SCREEN_PX_HEIGHT - 1u, M_FILL);

    // Fill active image area in white
    color(WHITE, WHITE, SOLID);
    box(IMG_X_START, IMG_Y_START, IMG_X_END, IMG_Y_END, M_FILL);

    // For pixel drawing
    color(BLACK,WHITE,SOLID);


    EMU_printf("Display: %hux%hu\n", (uint8_t)IMG_WIDTH_PX, (uint8_t)IMG_HEIGHT_PX);

    DISPLAY_ON;
    SPRITES_8x8;

    SHOW_BKG;
    SHOW_SPRITES;
}


// Expects UPDATE_KEYS() to have been called before each invocation
void draw_update(void) BANKED {

    static uint8_t cursor_x = DEVICE_SCREEN_PX_WIDTH / 2;
    static uint8_t cursor_y = DEVICE_SCREEN_PX_HEIGHT / 2;
    static bool    buttons_up_pending = false;

    if      (KEYS() & J_LEFT)  cursor_x--;
    else if (KEYS() & J_RIGHT) cursor_x++;

    if      (KEYS() & J_UP)   cursor_y--;
    else if (KEYS() & J_DOWN) cursor_y++;

    // Move the cursor
    move_sprite(SPRITE_MOUSE_CURSOR, cursor_x + DEVICE_SPRITE_PX_OFFSET_X, cursor_y + DEVICE_SPRITE_PX_OFFSET_Y);

    switch (KEYS() & (J_A | J_B)) {
        case (J_A | J_B):
            // Fill active image area in white
            color(WHITE, WHITE, SOLID);
            box(IMG_X_START, IMG_Y_START, IMG_X_END, IMG_Y_END, M_FILL);
            // // For pixel drawing
            color(BLACK, WHITE, SOLID);
            // Wait for both buttons up before drawing again to avoid leaving a dot after clearing screen
            buttons_up_pending = true;
            break;

        case J_A: if (!buttons_up_pending) plot_point(cursor_x, cursor_y);
            break;

        case J_B: break;

        default: // no buttons pressed
            buttons_up_pending = false;
            break;
    }
}
