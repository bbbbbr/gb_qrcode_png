#include <gbdk/platform.h>
#include <stdint.h>

#include "common.h"
#include "sprites.h"

#pragma bank 255  // Autobanked

app_state_t app_state;

void app_state_reset(void) BANKED {

    // Save and Undo related
    app_state.save_slot_current = DRAW_SAVE_SLOT_DEFAULT;

    app_state.undo_count        = DRAW_UNDO_COUNT_NONE;
    app_state.undo_slot_current = DRAW_UNDO_SLOT_DEFAULT;

    // UI related
    app_state.cursor_x = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH / 2);
    app_state.cursor_y = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_HEIGHT / 2);

    app_state.cursor_8u_cache_x = CURSOR_TO_8U_X();
    app_state.cursor_8u_cache_y = CURSOR_TO_8U_Y();

    app_state.cursor_speed_mode = CURSOR_SPEED_MODE_DEFAULT;
    app_state.cursor_teleport_zone = CURSOR_TELEPORT_DEFAULT;


    // Cursor UI teleport defaults
    app_state.cursor_draw_saved_x = app_state.cursor_x;
    app_state.cursor_draw_saved_y = app_state.cursor_y;

    app_state.cursor_menu_left_saved_x = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH / 10);
    app_state.cursor_menu_left_saved_y = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_HEIGHT / 2);

    app_state.cursor_menu_right_saved_x = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH - (DEVICE_SCREEN_PX_WIDTH / 10));
    app_state.cursor_menu_right_saved_y = CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_HEIGHT / 2);
    
    // Drawing / Tools
    app_state.draw_cursor_8u_last_x = CURSOR_POS_UNSET_8U;
    app_state.draw_cursor_8u_last_y = CURSOR_POS_UNSET_8U;

    app_state.drawing_tool = DRAW_TOOL_DEFAULT;
    app_state.draw_tool_using_b_button_action = false;
    app_state.tool_currently_drawing          = false;

    app_state.draw_color_main = DRAW_COLOR_MAIN_DEFAULT;
    app_state.draw_color_bg   = DRAW_COLOR_BG_DEFAULT;

    app_state.draw_width = DRAW_WIDTH_MODE_DEFAULT;
}


const palette_color_t QR_PAL_CGB[]  = {RGB(31, 31, 31), RGB(0,0,0),    RGB(0,0,0),    RGB(0,0,0)};
const palette_color_t DEF_PAL_CGB[] = {RGB(31, 31, 31), RGB(21,21,21),   RGB(10,10,10),   RGB(0,0,0)};

const palette_color_t SPR_PAL_MENU_CGB[] = {RGB(31, 31, 31), RGB(31, 31, 31), RGB(0,0,0),    RGB(0,0,0)};  // White, White, Black, Black
const palette_color_t SPR_PAL_DRAW_CGB[] = {RGB(31, 31, 31), RGB(31, 31, 31), RGB(10,10,10), RGB(0,0,0)};  // White, White, D-Gray, Black (Draw cursor is D-Gray + white)
// const palette_color_t SPR_PAL_DRAW_CGB[] = {RGB(31, 31, 31), RGB(21,21,21),   RGB(10,10,10), RGB(0,0,0)};  // Alt palette where Draw cursor is D-Gray + L-Gray)

const uint8_t QR_PAL_DMG  = DMG_PALETTE(DMG_WHITE, DMG_BLACK,     DMG_BLACK,     DMG_BLACK);
const uint8_t DEF_PAL_DMG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);

const uint8_t SPR_PAL_MENU_DMG = DMG_PALETTE(DMG_WHITE, DMG_WHITE,     DMG_BLACK,     DMG_BLACK);
const uint8_t SPR_PAL_DRAW_DMG = DMG_PALETTE(DMG_WHITE, DMG_WHITE,     DMG_DARK_GRAY, DMG_BLACK);
// const uint8_t SPR_PAL_DRAW_DMG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);

void set_pal_qrmode(void) BANKED {
    if (_cpu == CGB_TYPE)  set_bkg_palette(0u, 1u, QR_PAL_CGB);
    else                   BGP_REG = QR_PAL_DMG;
}


void set_pal_normal(void) BANKED {
    if (_cpu == CGB_TYPE)  set_bkg_palette(0u, 1u, DEF_PAL_CGB);
    else                   BGP_REG = DEF_PAL_DMG;
}


void update_cursor_style_to_draw(void) BANKED {
    // Palette
    if (_cpu == CGB_TYPE)  set_sprite_palette(0u, 1u, SPR_PAL_DRAW_CGB);
    else                   OBP1_REG = SPR_PAL_DRAW_DMG;

    // Which cursor type is active
    if (app_state.drawing_tool == DRAW_TOOL_ERASER)
        set_sprite_tile(SPRITE_ID_MOUSE_CURSOR, SPR_TYPE_CURSOR_ERASER);
    else
        set_sprite_tile(SPRITE_ID_MOUSE_CURSOR, SPR_TYPE_CURSOR_POINTER);
}


void update_cursor_style_to_menu(void) BANKED {
    // Palette
    if (_cpu == CGB_TYPE)  set_sprite_palette(0u, 1u, SPR_PAL_MENU_CGB);
    else                   OBP1_REG = SPR_PAL_MENU_DMG;

    // Which cursor type is active
    set_sprite_tile(SPRITE_ID_MOUSE_CURSOR, SPR_TYPE_CURSOR_POINTER);
}

