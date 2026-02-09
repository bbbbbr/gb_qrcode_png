#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "input.h"

#include "sprites.h"
#include "draw.h"
#include "ui_main.h"
#include "ui_menu_area.h"

#include <ui_main_bg.h>      // BG APA style image
#include <ui_main_bg_cde.h>  // BG APA style image  // CDE alternate theme

#pragma bank 255  // Autobanked

static void ui_cursor_teleport_save_zone(uint8_t teleport_zone_to_save);
static inline void ui_cursor_update(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static inline bool ui_check_cursor_in_draw_area(void);
static inline void ui_cursor_teleport_update(bool cursor_in_drawing, uint16_t cursor_last_x, uint16_t cursor_last_y);
static void ui_process_input(bool cursor_in_drawing);


void ui_init(void) BANKED {
    HIDE_BKG;
    HIDE_SPRITES;

    // Load the sprite cursor tiles
    set_sprite_data(SPRITE_TILE_MOUSE_START, mouse_cursors_count, mouse_cursors);

    // == Enters drawing mode ==
    mode(M_DRAWING);

    ui_redraw_menus_all();

    DISPLAY_ON;
    SPRITES_8x8;

    SHOW_BKG;
    SHOW_SPRITES;
}


void ui_update(void) BANKED {

    // Split UI handling between drawing area and UI
    // drawing area has different movement speed options than main UI area
    ui_process_input( ui_check_cursor_in_draw_area() );

    uint8_t cursor_8u_x = app_state.cursor_8u_cache_x;
    uint8_t cursor_8u_y = app_state.cursor_8u_cache_y;

    ui_cursor_update(cursor_8u_x, cursor_8u_y);

    if ( ui_check_cursor_in_draw_area() ) {
        draw_update(cursor_8u_x, cursor_8u_y);
    } else {
        ui_handle_menu_area(cursor_8u_x, cursor_8u_y);

        // Restore default draw colors in case they changed during menu updates
        // TODO: optimize: optionally only do this when changing between menu and drawing area instead of once per frame
        drawing_set_to_main_colors();
    }

}


void ui_redraw_after_qrcode(void) BANKED {

    // Cancel any pending tool use
    draw_tools_cancel_and_reset();

    // Refresh the UI
    ui_redraw_menus_all();

    // Then restore the drawing on top of it
    drawing_restore_from_sram(SRAM_BANK_CUR_DRAWING_CACHE, DRAWING_SAVE_SLOT_MIN);
    drawing_set_to_main_colors();
}


void ui_cycle_cursor_speed(void) BANKED {

    app_state.cursor_speed_mode++;
    if (app_state.cursor_speed_mode > CURSOR_SPEED_MODE_MAX)
        app_state.cursor_speed_mode = CURSOR_SPEED_MODE_MIN;
}


static void ui_cursor_teleport_save_zone(uint8_t teleport_zone_to_save) {

    switch(teleport_zone_to_save) {
        case CURSOR_TELEPORT_DRAWING:
            app_state.cursor_draw_saved_x = app_state.cursor_x;
            app_state.cursor_draw_saved_y = app_state.cursor_y;
            break;

        case CURSOR_TELEPORT_MENUS:
            app_state.cursor_menus_saved_x = app_state.cursor_x;
            app_state.cursor_menus_saved_y = app_state.cursor_y;
            break;
    }
}    

void ui_cycle_cursor_teleport(void) BANKED {

    // Note: Three teleport zones feels too complicated right now, reducing down to two

    // Save cursor position of current state
    ui_cursor_teleport_save_zone(app_state.cursor_teleport_zone);

    // Step to the next state
    app_state.cursor_teleport_zone++;
    if (app_state.cursor_teleport_zone > CURSOR_TELEPORT_MAX)
        app_state.cursor_teleport_zone = CURSOR_TELEPORT_MIN;

    // Restore cursor position for new state
    switch(app_state.cursor_teleport_zone) {
        case CURSOR_TELEPORT_DRAWING:
            app_state.cursor_x = app_state.cursor_draw_saved_x;
            app_state.cursor_y = app_state.cursor_draw_saved_y;
            update_cursor_style_to_draw();
            break;

    // TODO: UI: Cursor: Maybe teleport to menus should always snap to center of menu area instead of remembering, it might be more annoying in practice
        case CURSOR_TELEPORT_MENUS:
            app_state.cursor_x = app_state.cursor_menus_saved_x;
            app_state.cursor_y = app_state.cursor_menus_saved_y;
            update_cursor_style_to_menu();
            break;
    }
}


// ===== Local functions =====


static void inline ui_cursor_update(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    // Move the cursor
    // TODO: Maybe fancier updates here later
    move_sprite(SPRITE_ID_MOUSE_CURSOR, cursor_8u_x + DEVICE_SPRITE_PX_OFFSET_X, cursor_8u_y + DEVICE_SPRITE_PX_OFFSET_Y);
}


static inline bool ui_check_cursor_in_draw_area(void) {

    uint8_t cursor_8u_x = app_state.cursor_8u_cache_x;
    uint8_t cursor_8u_y = app_state.cursor_8u_cache_y;

    // TODO: May need an exception here for line or circle drawing to allow cursor control past drawing area
    if ((cursor_8u_x >= IMG_X_START) && (cursor_8u_x <= IMG_X_END) &&
        (cursor_8u_y >= IMG_Y_START) && (cursor_8u_y <= IMG_Y_END)) {
        return true;
    }
    else return false;
}


static inline void ui_cursor_teleport_update(bool cursor_in_drawing, uint16_t cursor_last_x, uint16_t cursor_last_y) {

    // Check if cursor needs an auto-teleport update due to manually navigating between areas
    if ((cursor_in_drawing == false) && (app_state.cursor_teleport_zone == CURSOR_TELEPORT_DRAWING)) {
        // Save position in drawing and change state to menu area
        // Use ..last.. since the position to save is where it was BEFORE it changed zones
        app_state.cursor_draw_saved_x = cursor_last_x;
        app_state.cursor_draw_saved_y = cursor_last_y;
        app_state.cursor_teleport_zone = CURSOR_TELEPORT_MENUS;
        update_cursor_style_to_menu();
    }
    else if ((cursor_in_drawing == true) && (app_state.cursor_teleport_zone == CURSOR_TELEPORT_MENUS)) {
        // Save position in drawing and change state to menu area
        // Use ..last.. since the position to save is where it was BEFORE it changed zones
        app_state.cursor_menus_saved_x = cursor_last_x;
        app_state.cursor_menus_saved_y = cursor_last_y;
        app_state.cursor_teleport_zone = CURSOR_TELEPORT_DRAWING;
        update_cursor_style_to_draw();
    }

    // TODO: J_SELECT instead?
    // Check for request to teleport between menus/drawing
    if (KEY_TICKED(J_B) && (app_state.draw_tool_using_b_button_action == false))
        ui_cycle_cursor_teleport();
}


static void ui_process_input(bool cursor_in_drawing) {

    static uint16_t cursor_last_x, cursor_last_y;

    // First check for and apply any cursor teleport actions/updates
    ui_cursor_teleport_update(cursor_in_drawing, cursor_last_x, cursor_last_y);

    cursor_last_x = app_state.cursor_x;
    cursor_last_y = app_state.cursor_y;

    if (!cursor_in_drawing) {
        // For the main UI area there is only one speed of cursor movement (fast)
        if      (KEYS() & J_LEFT)  { if (app_state.cursor_x > CURSOR_SPEED_UI) app_state.cursor_x -= CURSOR_SPEED_UI; }
        else if (KEYS() & J_RIGHT) { if (app_state.cursor_x < (SCREEN_X_MAX_16U - CURSOR_SPEED_UI)) app_state.cursor_x += CURSOR_SPEED_UI; }

        if      (KEYS() & J_UP)   { if (app_state.cursor_y > CURSOR_SPEED_UI) app_state.cursor_y -= CURSOR_SPEED_UI; }
        else if (KEYS() & J_DOWN) { if (app_state.cursor_y < (SCREEN_Y_MAX_16U - CURSOR_SPEED_UI)) app_state.cursor_y += CURSOR_SPEED_UI; }
    }
    else {
        // Drawing area cursor handling
        uint8_t cursor_speed_mode = app_state.cursor_speed_mode;
        if (cursor_speed_mode == CURSOR_SPEED_MODE_PIXELSTEP) {

            if      (KEY_TICKED(J_LEFT))  { if (app_state.cursor_x > CURSOR_SPEED_PIXELSTEP) app_state.cursor_x -= CURSOR_SPEED_PIXELSTEP; }
            else if (KEY_TICKED(J_RIGHT)) { if (app_state.cursor_x < (SCREEN_X_MAX_16U - CURSOR_SPEED_PIXELSTEP)) app_state.cursor_x += CURSOR_SPEED_PIXELSTEP; }

            if      (KEY_TICKED(J_UP))   { if (app_state.cursor_y > CURSOR_SPEED_PIXELSTEP) app_state.cursor_y -= CURSOR_SPEED_PIXELSTEP; }
            else if (KEY_TICKED(J_DOWN)) { if (app_state.cursor_y < (SCREEN_Y_MAX_16U - CURSOR_SPEED_PIXELSTEP)) app_state.cursor_y += CURSOR_SPEED_PIXELSTEP; }
        }
        else if (cursor_speed_mode == CURSOR_SPEED_MODE_NORMAL) {

            if      (KEYS() & J_LEFT)  { if (app_state.cursor_x > CURSOR_SPEED_NORMAL) app_state.cursor_x -= CURSOR_SPEED_NORMAL; }
            else if (KEYS() & J_RIGHT) { if (app_state.cursor_x < (SCREEN_X_MAX_16U - CURSOR_SPEED_NORMAL)) app_state.cursor_x += CURSOR_SPEED_NORMAL; }

            if      (KEYS() & J_UP)   { if (app_state.cursor_y > CURSOR_SPEED_NORMAL) app_state.cursor_y -= CURSOR_SPEED_NORMAL; }
            else if (KEYS() & J_DOWN) { if (app_state.cursor_y < (SCREEN_Y_MAX_16U - CURSOR_SPEED_NORMAL)) app_state.cursor_y += CURSOR_SPEED_NORMAL; }
        }
        else { // implied: if (cursor_speed_mode == CURSOR_SPEED_MODE_FAST) {

            if      (KEYS() & J_LEFT)  { if (app_state.cursor_x > CURSOR_SPEED_FAST) app_state.cursor_x -= CURSOR_SPEED_FAST; }
            else if (KEYS() & J_RIGHT) { if (app_state.cursor_x < (SCREEN_X_MAX_16U - CURSOR_SPEED_FAST)) app_state.cursor_x += CURSOR_SPEED_FAST; }

            if      (KEYS() & J_UP)   { if (app_state.cursor_y > CURSOR_SPEED_FAST) app_state.cursor_y -= CURSOR_SPEED_FAST; }
            else if (KEYS() & J_DOWN) { if (app_state.cursor_y < (SCREEN_Y_MAX_16U - CURSOR_SPEED_FAST)) app_state.cursor_y += CURSOR_SPEED_FAST; }
        }
    }

    // Refresh the cached 8 bit cursor position
    app_state.cursor_8u_cache_x = CURSOR_TO_8U_X();
    app_state.cursor_8u_cache_y = CURSOR_TO_8U_Y();
}

