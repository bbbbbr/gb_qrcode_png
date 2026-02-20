#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "input.h"

#include "draw.h"
#include "ui_main.h"
#include "ui_menu_area.h"
#include "sprites.h"

// Image data
#include <ui_main_bg.h>      // BG APA style image
#include <ui_main_bg_cde.h>  // BG APA style image  // CDE alternate theme
#include <sprites_img.h>     // Cursor and other sprites
#include <speed_button.h>

#pragma bank 255  // Autobanked

static int16_t  cursor_accel_limit;
static uint8_t  cursor_accel_factor;
static int16_t  cursor_accel_increment;
static bool     cursor_continuous_move;

static void ui_draw_width_handle_input(void);
static void ui_cursor_speed_handle_input(void);
static void ui_cursor_teleport_save_zone(uint8_t teleport_zone_to_save);
static inline void ui_cursor_update(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static inline bool ui_check_cursor_in_draw_area(void);
static inline void ui_clamp_cursor_to_draw_area(void);
static inline void ui_cursor_teleport_update(bool cursor_in_drawing, uint16_t cursor_last_x, uint16_t cursor_last_y);
static void ui_process_input(bool cursor_in_drawing);


void ui_init(void) NONBANKED {
    HIDE_BKG;
    HIDE_SPRITES;

    // Load the sprite cursor tiles
    uint8_t save_bank = CURRENT_BANK;
    SWITCH_ROM(BANK(sprites_img));
    set_sprite_data(SPRITE_TILE_MOUSE_START, SPRITE_CURSOR_COUNT,      SPRITE_CURSOR_TILE_DATA_START);
    set_sprite_data(SPRITE_TILE_UNDO_BUTTON, SPRITE_UNDO_BUTTON_COUNT, SPRITE_UNDO_TILE_DATA_START);    
    // Redo sprite uses undo sprite
    set_sprite_data(SPRITE_TILE_DRAW_WIDTH_IND, SPRITE_DRAW_WIDTH_IND_COUNT, SPRITE_DRAW_WIDTH_IND_TILE_DATA_START);
    set_sprite_data(SPRITE_TILE_CONFIRM_CHECK,  SPRITE_CONFIRM_CHECK_COUNT, SPRITE_CONFIRM_CHECK_TILE_DATA_START);
    SWITCH_ROM(save_bank);

    set_sprite_tile(SPRITE_ID_UNDO_BUTTON, SPRITE_TILE_UNDO_BUTTON);
    set_sprite_tile(SPRITE_ID_REDO_BUTTON, SPRITE_TILE_REDO_BUTTON);
    set_sprite_prop(SPRITE_ID_REDO_BUTTON, S_FLIPX); // Redo sprite is the undo flipped horizontally
    set_sprite_tile(SPRITE_ID_DRAW_WIDTH_IND, SPRITE_TILE_DRAW_WIDTH_IND);
    set_sprite_tile(SPRITE_ID_CONFIRM_CHECK, SPRITE_TILE_CONFIRM_CHECK);
    // Undo and Redo not enabled by default, will get hidden on initial menu setup

    ui_cursor_speed_update_settings();

    // == Enters drawing mode ==
    mode(M_DRAWING);

    ui_redraw_menus_all();

    DISPLAY_ON;
    SPRITES_8x8;

    SHOW_BKG;
    SHOW_SPRITES;
}


void ui_update(void) BANKED {

    ui_confirm_check_update(UI_CONFIRM_NORMAL_UPDATE);

    if (KEY_PRESSED(UI_SHORTCUT_BUTTON)) {
        ui_cursor_speed_handle_input();
        ui_draw_width_handle_input();
    }
    else {
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
}


void ui_redraw_full(void) BANKED {

    // Refresh the UI
    ui_redraw_menus_all();

    // Then restore the drawing on top of it
    drawing_set_to_main_colors();
}


void ui_draw_width_cycle(void) BANKED {

    app_state.draw_width++;
    if (app_state.draw_width > DRAW_WIDTH_MODE_MAX)
        app_state.draw_width = DRAW_WIDTH_MODE_MIN;

    ui_draw_width_redraw_indicator();
}


static void ui_draw_width_handle_input(void) {

    if (KEY_TICKED(DRAW_WIDTH_UP_BUTTON)) {        
        if (app_state.draw_width < DRAW_WIDTH_MODE_MAX)
            app_state.draw_width++;
            ui_draw_width_redraw_indicator();
    }
    else if (KEY_TICKED(DRAW_WIDTH_DOWN_BUTTON)) {        
        if (app_state.draw_width > DRAW_WIDTH_MODE_MIN)
            app_state.draw_width--;
            ui_draw_width_redraw_indicator();
    }
}


void ui_cursor_speed_update_settings(void) BANKED {

    cursor_continuous_move = true;
    // Set acceleration factor and max speed
    switch (app_state.cursor_speed_mode) {
        // Pixel step mode only moves on first press
        case CURSOR_SPEED_MODE_PIXELSTEP: cursor_accel_limit = CURSOR_SPEED_PIXELSTEP;
                                          cursor_accel_factor = CURSOR_ACCEL_FACTOR_FULL;
                                          cursor_continuous_move = false;
                                          break;

        // All other modes move as long as button pressed
        case CURSOR_SPEED_MODE_SLOW:      cursor_accel_limit = CURSOR_SPEED_SLOW;
                                          cursor_accel_factor = CURSOR_ACCEL_FACTOR_SLOW;
                                          break;
        default:
        case CURSOR_SPEED_MODE_MEDIUM:    cursor_accel_limit = CURSOR_SPEED_NORMAL;
                                          cursor_accel_factor = CURSOR_ACCEL_FACTOR_NORMAL;
                                          break;

        case CURSOR_SPEED_MODE_FAST:      cursor_accel_limit = CURSOR_SPEED_FAST;
                                          cursor_accel_factor = CURSOR_ACCEL_FACTOR_FAST;
                                          break;
    }

    cursor_accel_increment = cursor_accel_limit / cursor_accel_factor;
}

// Used for clicking speed menu button
void ui_cursor_cycle_speed(void) BANKED {
    if (app_state.cursor_speed_mode == CURSOR_SPEED_MODE_MAX)
        app_state.cursor_speed_mode = CURSOR_SPEED_MODE_MIN;
    else
        app_state.cursor_speed_mode++;

    ui_cursor_speed_update_settings();
    ui_cursor_speed_redraw_indicator();
}


static void ui_cursor_speed_handle_input(void) {

    if (KEY_TICKED(SPEED_UP_BUTTON)) {        
        if (app_state.cursor_speed_mode < CURSOR_SPEED_MODE_MAX) {
            app_state.cursor_speed_mode++;
            ui_cursor_speed_redraw_indicator();
            ui_cursor_speed_update_settings();
        }
    }
    else if (KEY_TICKED(SPEED_DOWN_BUTTON)) {        
        if (app_state.cursor_speed_mode > CURSOR_SPEED_MODE_MIN) {
            app_state.cursor_speed_mode--;
            ui_cursor_speed_redraw_indicator();
            ui_cursor_speed_update_settings();
        }
    }
}


static void ui_cursor_teleport_save_zone(uint8_t teleport_zone_to_save) {

    switch(teleport_zone_to_save) {
        case CURSOR_TELEPORT_DRAWING:
            app_state.cursor_draw_saved_x = app_state.cursor_x;
            app_state.cursor_draw_saved_y = app_state.cursor_y;
            break;

        // Changed behavior to not save menu positions (commented out below),
        // so now cursor always snaps to centers of the side menus

        // case CURSOR_TELEPORT_MENU_LEFT:
        //     app_state.cursor_menu_left_saved_x = app_state.cursor_x;
        //     app_state.cursor_menu_left_saved_y = app_state.cursor_y;
        //     break;

        // case CURSOR_TELEPORT_MENU_RIGHT:
        //     app_state.cursor_menu_right_saved_x = app_state.cursor_x;
        //     app_state.cursor_menu_right_saved_y = app_state.cursor_y;
        //     break;
    }
}    

void ui_cursor_cycle_teleport(void) BANKED {

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

        case CURSOR_TELEPORT_MENU_LEFT:
            app_state.cursor_x = app_state.cursor_menu_left_saved_x;
            app_state.cursor_y = app_state.cursor_menu_left_saved_y;
            update_cursor_style_to_menu();
            break;

        case CURSOR_TELEPORT_MENU_RIGHT:
            app_state.cursor_x = app_state.cursor_menu_right_saved_x;
            app_state.cursor_y = app_state.cursor_menu_right_saved_y;
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


static inline void ui_clamp_cursor_to_draw_area(void) {

    // TODO: May need an exception here for line or circle drawing to allow cursor control past drawing area
    if (app_state.cursor_x < CURSOR_8U_TO_16U(IMG_X_START)) app_state.cursor_x = CURSOR_8U_TO_16U(IMG_X_START);
    if (app_state.cursor_x > CURSOR_8U_TO_16U(IMG_X_END))   app_state.cursor_x = CURSOR_8U_TO_16U(IMG_X_END);
    if (app_state.cursor_y < CURSOR_8U_TO_16U(IMG_Y_START)) app_state.cursor_y = CURSOR_8U_TO_16U(IMG_Y_START);
    if (app_state.cursor_y > CURSOR_8U_TO_16U(IMG_Y_END))   app_state.cursor_y = CURSOR_8U_TO_16U(IMG_Y_END);
}


static inline void ui_cursor_teleport_update(bool cursor_in_drawing, uint16_t cursor_last_x, uint16_t cursor_last_y) {
    cursor_last_x;  // Warning quell
    cursor_last_y;  // Warning quell

    // Check if cursor needs an auto-teleport update due to manually navigating between areas
    if ((cursor_in_drawing == false) && (app_state.cursor_teleport_zone == CURSOR_TELEPORT_DRAWING)) {
        // When moving the cursor out of the drawing area and into the menu, don't save
        // the last drawing cursor position since it will just be right on the edge.
        // Instead when teleporting back it will pop to the last teleported-out drawing
        // position which feels more intuitive.

        // Save position in drawing and change state to menu area
        // Use ..last.. since the position to save is where it was BEFORE it changed zones
        // app_state.cursor_draw_saved_x = cursor_last_x;
        // app_state.cursor_draw_saved_y = cursor_last_y;

        // When manually moving out of drawing to a menu, always set the zone to Max
        // so that the next press will return to drawing. 
        // WARNING: If saving zone position is re-enabled then the version below should be used isntead
        app_state.cursor_teleport_zone = CURSOR_TELEPORT_MAX;
        //
        // if (cursor_last_x < CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH / 2u))
        //     app_state.cursor_teleport_zone = CURSOR_TELEPORT_MENU_LEFT;
        // else
        //     app_state.cursor_teleport_zone = CURSOR_TELEPORT_MENU_RIGHT;
        update_cursor_style_to_menu();
    }
    else if ((cursor_in_drawing == true) && (app_state.cursor_teleport_zone != CURSOR_TELEPORT_DRAWING)) {

        // As noted in ui_cursor_teleport_save_zone(), no longer saving cursor position
        // in menu zones

        // Save position in drawing and change state to menu area
        // Use ..last.. since the position to save is where it was BEFORE it changed zones
        //  if (cursor_last_x < CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH / 2u))
        //     app_state.cursor_menu_left_saved_x = cursor_last_x;
        //     app_state.cursor_menu_left_saved_y = cursor_last_y;
        // } else {
        //     app_state.cursor_menu_right_saved_x = cursor_last_x;
        //     app_state.cursor_menu_right_saved_y = cursor_last_y;
        // }
        app_state.cursor_teleport_zone = CURSOR_TELEPORT_DRAWING;
        update_cursor_style_to_draw();
    }

    // Check for request to teleport between menus/drawing
    //
    // The trigger (KEY_RELEASED() or KEY_TICKED()) needs to match the cancel trigger
    // used by the tools, otherwise it will get false triggers when they cancel
    if (KEY_RELEASED(UI_CURSOR_SPEED_BUTTON) && (app_state.draw_tool_using_b_button_action == false)) {
        // Only teleport if not over the threshold to fast mode
        // EMU_printf("B Len = %hu\n", (uint8_t)KEY_REPEAT_COUNT_LAST);
        if (KEY_REPEAT_COUNT_LAST <= UI_CURSOR_TELLEPORT_THRESHOLD)
            ui_cursor_cycle_teleport();
    }
}


static void ui_process_input(bool cursor_in_drawing) {

    static uint16_t cursor_last_x, cursor_last_y;
    static bool ui_cursor_fast_mode = false;

    // Used for Teleport threshold gating
    UPDATE_KEY_REPEAT(UI_CURSOR_SPEED_BUTTON);

    // First check for and apply any cursor teleport actions/updates
    ui_cursor_teleport_update(cursor_in_drawing, cursor_last_x, cursor_last_y);

    cursor_last_x = app_state.cursor_x;
    cursor_last_y = app_state.cursor_y;

    if ((!cursor_in_drawing) || KEY_PRESSED(UI_CURSOR_SPEED_BUTTON)) {
        // For the main UI area there is only one speed of cursor movement (fast, with no inertia)
        if      (KEYS() & J_LEFT)  { if (app_state.cursor_x > CURSOR_SPEED_UI_X) app_state.cursor_x -= CURSOR_SPEED_UI_X; }
        else if (KEYS() & J_RIGHT) { if (app_state.cursor_x < (SCREEN_X_MAX_16U - CURSOR_SPEED_UI_X)) app_state.cursor_x += CURSOR_SPEED_UI_X; }

        if      (KEYS() & J_UP)   { if (app_state.cursor_y > CURSOR_SPEED_UI_Y) app_state.cursor_y -= CURSOR_SPEED_UI_Y; }
        else if (KEYS() & J_DOWN) { if (app_state.cursor_y < (SCREEN_Y_MAX_16U - CURSOR_SPEED_UI_Y)) app_state.cursor_y += CURSOR_SPEED_UI_Y; }
    }
    else {
        // Drawing area cursor handling has inertia and an acceleration factor

        if (cursor_continuous_move || KEY_TICKED(J_DPAD)) {

            // Ramp speed up gradually instead of all at once, clamp to maximum speed
            if (KEY_PRESSED(J_LEFT) && (app_state.cursor_accel_x > -cursor_accel_limit)) {
                app_state.cursor_accel_x -= cursor_accel_increment;
            }
            else if (KEY_PRESSED(J_RIGHT) && (app_state.cursor_accel_x < cursor_accel_limit)) {
                app_state.cursor_accel_x += cursor_accel_increment;
            }

            if (KEY_PRESSED(J_UP) && (app_state.cursor_accel_y > -cursor_accel_limit)) {
                app_state.cursor_accel_y -= cursor_accel_increment;
            }
            else if (KEY_PRESSED(J_DOWN) && (app_state.cursor_accel_y < cursor_accel_limit)) {
                app_state.cursor_accel_y += cursor_accel_increment;
            }
        }
        if (!KEY_PRESSED(J_ANY)) {
            // Reset inertia if all buttons released (D-Pad or otherwise), brings cursor to immediate stop
            app_state.cursor_accel_y = 0;
            app_state.cursor_accel_x = 0;
        }

        // Apply inertia to cursor position
        app_state.cursor_y += app_state.cursor_accel_y;
        app_state.cursor_x += app_state.cursor_accel_x;

        if (cursor_continuous_move) {
            // Clamp down to remove drift
            if ((app_state.cursor_accel_x > -8) & (app_state.cursor_accel_x < 8)) app_state.cursor_accel_x = 0;
            if ((app_state.cursor_accel_y > -8) & (app_state.cursor_accel_y < 8)) app_state.cursor_accel_y = 0;

            if (app_state.cursor_accel_x != 0) app_state.cursor_accel_x -= app_state.cursor_accel_x / cursor_accel_factor;
            if (app_state.cursor_accel_y != 0) app_state.cursor_accel_y -= app_state.cursor_accel_y / cursor_accel_factor;
        } else {
            // No inertia in pixel step mode
            app_state.cursor_accel_y = 0;
            app_state.cursor_accel_x = 0;
        }

    }

    // Clamp cursor to drawing area if a tool is actively drawing
    if (app_state.tool_currently_drawing) {
        ui_clamp_cursor_to_draw_area();
    }

    // Refresh the cached 8 bit cursor position
    app_state.cursor_8u_cache_x = CURSOR_TO_8U_X();
    app_state.cursor_8u_cache_y = CURSOR_TO_8U_Y();
}

