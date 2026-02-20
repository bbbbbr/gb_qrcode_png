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
#include "save_and_undo.h"
#include "help_screen.h"
#include "print.h"

#include <ui_main_bg.h>      // BG APA style image
#include <ui_main_bg_cde.h>  // BG APA style image  // CDE alternate theme

#pragma bank 255  // Autobanked

const uint8_t menu_tools[DRAW_TOOL_COUNT] = {
    DRAW_TOOL_PENCIL,
    DRAW_TOOL_LINE,
    DRAW_TOOL_ERASER,
    DRAW_TOOL_RECT,
    DRAW_TOOL_CIRCLE,
    DRAW_TOOL_FLOODFILL,
};

static void ui_menu_tools(uint8_t cursor_8u_y);
static void ui_menu_file(uint8_t cursor_8u_x);
static void ui_menu_right(uint8_t cursor_8u_y);
static void ui_swap_active_color(void);

static void ui_perform_undo(void);
static void ui_perform_redo(void);

static void ui_file_confirm_check_show(void);
static void ui_print_confirm_check_show(void);
static void ui_print_confirm_check_clear(void);

static uint8_t ui_confirm_check_x = 0u;
static uint8_t ui_confirm_check_y = 0u;

// Draws the paint working area
void ui_redraw_menus_all(void) NONBANKED {

    DISPLAY_OFF;

    uint8_t save_bank = CURRENT_BANK;

    // Alternate CDE theme from holding SELECT on startup
    if (app_state.solaris_cde_ui_theme) {
        SWITCH_ROM(BANK(ui_main_bg_cde));
        draw_image(ui_main_bg_cde_tiles);
        SWITCH_ROM(save_bank);
    } else {
        SWITCH_ROM(BANK(ui_main_bg));
        draw_image(ui_main_bg_tiles);
        SWITCH_ROM(save_bank);
    }

    // Redraw various menus and their state
    ui_menu_tools_draw_highlight(app_state.drawing_tool, TOOLS_MENU_HIGHLIGHT_COLOR);
    ui_menu_file_draw_highlight(app_state.save_slot_current, FILE_MENU_HIGHLIGHT_COLOR);

    ui_undo_button_refresh();
    ui_redo_button_refresh();

    ui_cursor_speed_redraw_indicator();
    ui_draw_width_redraw_indicator();
    ui_confirm_check_update(UI_CONFIRM_FORCE_REDRAW);

    DISPLAY_ON;

    // EMU_printf("Display: %hux%hu\n", (uint8_t)IMG_WIDTH_PX, (uint8_t)IMG_HEIGHT_PX);
}

void ui_handle_menu_area(uint8_t cursor_8u_x, uint8_t cursor_8u_y) BANKED {

    // TODO: OPTIMIZE: instead of dispatching on every cursor move, only dispatch to menus on button press?

    if (cursor_8u_y < TITLE_BAR_MENU_Y_END) {
        if ((cursor_8u_x >= HELP_BUTTON_X_START) && (cursor_8u_x < HELP_BUTTON_X_END) &&
            (cursor_8u_y >= HELP_BUTTON_Y_START) && (cursor_8u_y < HELP_BUTTON_Y_END)) {
            if (KEY_TICKED(UI_ACTION_BUTTON)) help_page_show();
        }
    } // Partition the screen left/right
    else if (cursor_8u_x <= TOOLS_MENU_X_END) {
        // Tools Menu
        if ((cursor_8u_x >= TOOLS_MENU_X_START) && (cursor_8u_x < TOOLS_MENU_X_END) &&
            (cursor_8u_y >= TOOLS_MENU_Y_START) && (cursor_8u_y < TOOLS_MENU_Y_END)) {
            ui_menu_tools(cursor_8u_y);
        }
    } else if (cursor_8u_x >= RIGHT_MENU_X_START) {
        // File Menu
        if ((cursor_8u_x >= RIGHT_MENU_X_START) && (cursor_8u_x < RIGHT_MENU_X_END) &&
             (cursor_8u_y >= RIGHT_MENU_Y_START) && (cursor_8u_y < RIGHT_MENU_Y_END)) {
            ui_menu_right(cursor_8u_y);
        }
    }
    // Bottom menu
    else if (cursor_8u_y > UI_BOTTOM_BORDER_START) {
        if ((cursor_8u_x >= FILE_MENU_X_START) && (cursor_8u_x < FILE_MENU_X_END) &&
            (cursor_8u_y >= FILE_MENU_Y_START) && (cursor_8u_y < FILE_MENU_Y_END)) {
            ui_menu_file(cursor_8u_x);
        }
    } // Top title bar area

}


// ===== Tools Menu =====


static void ui_menu_tools(uint8_t cursor_8u_y) {

    // TODO: optional: highlight button under cursor as it moves
    // TODO: optional: animate button press somehow

    // A button used to press buttons
    if (KEY_TICKED(UI_ACTION_BUTTON)) {
        // Clear any pending tool actions
        draw_tools_cancel_and_reset();

        // Tool icons are uniform in size, so divide position by size to get it
        uint8_t new_tool = (cursor_8u_y - TOOLS_MENU_Y_START) / TOOLS_MENU_ITEM_HEIGHT;

        // Remove highlight from previous tool
        ui_menu_tools_draw_highlight(app_state.drawing_tool, TOOLS_MENU_BG_COLOR);
        // Highlight new tool
        ui_menu_tools_draw_highlight(new_tool, TOOLS_MENU_HIGHLIGHT_COLOR);
        app_state.drawing_tool = new_tool;
    }

}

// Highlight active tool, for now just draw a rectangle around it
//
// TODO: optional: dashed outline for tool highlighting (could be done via 1x sprite mirrored)
void ui_menu_tools_draw_highlight(uint8_t tool_num, uint8_t draw_color) BANKED {

    uint8_t x1 =  TOOLS_MENU_X_START;
    uint8_t y1 = (tool_num * TOOLS_MENU_ITEM_HEIGHT) + TOOLS_MENU_Y_START;

    color(draw_color, WHITE, SOLID);
    box(x1, y1, x1 + (TOOLS_MENU_ITEM_WIDTH - 1u), y1 + (TOOLS_MENU_ITEM_HEIGHT - 1u), M_NOFILL);
}


// ===== File Menu =====


static void ui_menu_file(uint8_t cursor_8u_x) {

    // TODO: optional: highlight button under cursor as it moves
    // TODO: optional: animate button press somehow

    // A button used to press buttons
    if (KEY_TICKED(UI_ACTION_BUTTON)) {
        // Clear any pending tool actions
        draw_tools_cancel_and_reset();

        // Tool icons are uniform in size, so divide position by size to get it
        uint8_t file_menu_item = (cursor_8u_x - FILE_MENU_X_START) / FILE_MENU_ITEM_WIDTH;

        switch (file_menu_item) {
            case FILE_MENU_LOAD:
            case FILE_MENU_LOAD2:  // Right half of 2 unit wide button
                // Take undo snapshot first, in case user changes their mind
                drawing_take_undo_snapshot();
                drawing_restore_from_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
                ui_file_confirm_check_show();
                break;

            case FILE_MENU_SAVE_SLOT_0:  // Fall through
            case FILE_MENU_SAVE_SLOT_1:  // Fall through
            case FILE_MENU_SAVE_SLOT_2:
                    // Calculate actual save slot number
                    uint8_t new_save_slot = file_menu_item - FILE_MENU_SAVE_SLOT_0;
                    // Remove highlight from previous save
                    ui_menu_file_draw_highlight(app_state.save_slot_current, FILE_MENU_BG_COLOR);
                    // Highlight new save
                    ui_menu_file_draw_highlight(new_save_slot, FILE_MENU_HIGHLIGHT_COLOR);
                    app_state.save_slot_current = new_save_slot;
                    break;

            case FILE_MENU_SAVE:
            case FILE_MENU_SAVE2:  // Right half of 2 unit wide button
                    drawing_save_to_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
                    ui_file_confirm_check_show();
                    break;
        }
    }
}


// Highlight active save slot, for now just draw a rectangle around it
//
// TODO: optional: dashed outline for tool highlighting (could be done via 1x sprite mirrored)
void ui_menu_file_draw_highlight(uint8_t num, uint8_t draw_color) BANKED {

    // Menu highlights start after Load button which is first/top
    num += FILE_MENU_START_OF_SAVE_SLOTS_OFFSET;

    uint8_t x1 = (num * FILE_MENU_ITEM_WIDTH) + (FILE_MENU_X_START);
    uint8_t y1 = FILE_MENU_Y_START;

    color(draw_color, WHITE, SOLID);
    box(x1, y1, x1 + (FILE_MENU_ITEM_WIDTH -1u), y1 + (FILE_MENU_ITEM_HEIGHT - 1u), M_NOFILL);
}


// ===== Right Menu ====

static void ui_menu_right(uint8_t cursor_8u_y) {

    // TODO: optional: highlight button under cursor as it moves
    // TODO: optional: animate button press somehow

    // A button used to press buttons
    if (KEY_TICKED(UI_ACTION_BUTTON)) {
        // Tool icons are uniform in size, so divide position by size to get it
        uint8_t file_menu_item = (cursor_8u_y - RIGHT_MENU_Y_START) / RIGHT_MENU_ITEM_HEIGHT;

        switch (file_menu_item) {
            case RIGHT_MENU_REDO:       ui_perform_redo();
                                        break;

            case RIGHT_MENU_UNDO:       ui_perform_undo();
                                        break;

            case RIGHT_MENU_COLOR_SWAP: ui_swap_active_color();
                                        break;

            case RIGHT_MENU_DRAW_WIDTH_IND: ui_draw_width_cycle();
                                        break;

            case RIGHT_MENU_SPEED:      ui_cursor_cycle_speed();
                                        break;

            case RIGHT_MENU_CLEAR:      drawing_clear();
                                        break;

            case RIGHT_MENU_PRINT:      ui_print_confirm_check_show();
                                        print_drawing();
                                        ui_print_confirm_check_clear();
                                        break;
        }
    }
}


static void ui_swap_active_color(void) {
    // Switch the active color
    if (app_state.draw_color_main == BLACK) {
        app_state.draw_color_main = WHITE;
        app_state.draw_color_bg   = BLACK;
    }
    else {
        app_state.draw_color_main = BLACK;
        app_state.draw_color_bg   = WHITE;
    }

    // Redraw the icon to match current state
    color(app_state.draw_color_bg, app_state.draw_color_bg, SOLID);
    box(COLOR_ALT_X_START,  COLOR_ALT_Y_START,  COLOR_ALT_X_END,  COLOR_ALT_Y_END,  M_FILL);

    color(app_state.draw_color_main, app_state.draw_color_main, SOLID);
    box(COLOR_MAIN_X_START, COLOR_MAIN_Y_START, COLOR_MAIN_X_END, COLOR_MAIN_Y_END, M_FILL);
}


void ui_undo_button_refresh(void) BANKED {
    if (app_state.undo_count > DRAW_UNDO_COUNT_NONE) {
        // Enabled
        move_sprite(SPRITE_ID_UNDO_BUTTON, UNDO_BUTTON_SPR_X, UNDO_BUTTON_SPR_Y);
    } else {
        // Disabled
        hide_sprite(SPRITE_ID_UNDO_BUTTON);
    }
}


void ui_redo_button_refresh(void) BANKED {
    if (app_state.redo_count > DRAW_REDO_COUNT_NONE) {
        // Enabled
        move_sprite(SPRITE_ID_REDO_BUTTON, REDO_BUTTON_SPR_X, REDO_BUTTON_SPR_Y);
    } else {
        // Disabled
        hide_sprite(SPRITE_ID_REDO_BUTTON);
    }
}


// Copy tiles from ROM into vram to update the indicated cursor speed
void ui_cursor_speed_redraw_indicator(void) NONBANKED {

    uint8_t save_bank = CURRENT_BANK;
    SWITCH_ROM(BANK(speed_button));

    const uint8_t * p_tile_src = speed_button_tiles + (app_state.cursor_speed_mode * CURSOR_SPEED_IND_MODE_SZ_BYTES);

    // Copy two rows of tiles
    vmemcpy((uint8_t*)CURSOR_SPEED_IND_ROW1_VRAM_ADDR, (uint8_t *)p_tile_src, CURSOR_SPEED_IND_ROW_SZ_BYTES);

    p_tile_src += CURSOR_SPEED_IND_ROW_SZ_BYTES;
    vmemcpy((uint8_t*)CURSOR_SPEED_IND_ROW2_VRAM_ADDR, (uint8_t *)p_tile_src, CURSOR_SPEED_IND_ROW_SZ_BYTES);

    SWITCH_ROM(save_bank);
}


static void ui_perform_undo(void) {
    drawing_restore_undo_snapshot(UNDO_RESTORE_REDO_SNAPSHOT_YES);
}


static void ui_perform_redo(void) {
    drawing_restore_redo_snapshot();
}


void ui_draw_width_redraw_indicator(void) BANKED {
    uint8_t spr_x = DRAW_WIDTH_IND_SPR_X + (app_state.draw_width * DRAW_WIDTH_SPR_STEP_X);
    move_sprite(SPRITE_ID_DRAW_WIDTH_IND, spr_x, DRAW_WIDTH_IND_SPR_Y);
}


static void ui_file_confirm_check_show(void) {
    ui_confirm_check_x = FILE_CONFIRM_CHECK_SPR_X(app_state.save_slot_current);
    ui_confirm_check_y = FILE_CONFIRM_CHECK_SPR_Y;
    app_state.ui_confirm_check_counter = UI_CONFIRM_CHECK_COUNT_ENABLE;
}

static void ui_print_confirm_check_show(void) {
    ui_confirm_check_x = PRINT_CONFIRM_CHECK_SPR_X;
    ui_confirm_check_y = PRINT_CONFIRM_CHECK_SPR_Y;
    app_state.ui_confirm_check_counter = UI_CONFIRM_CHECK_COUNT_ENABLE;
    ui_confirm_check_update(UI_CONFIRM_NORMAL_UPDATE);
}

static void ui_print_confirm_check_clear(void) {
    app_state.ui_confirm_check_counter = UI_CONFIRM_CHECK_COUNT_NEXT_OFF;
}


void ui_confirm_check_update(bool force_redraw) BANKED {

    if (app_state.ui_confirm_check_counter) {

        // Update display if first pass after enabling or a forced redraw such as after help screen or QR Code
        if ((force_redraw) || (app_state.ui_confirm_check_counter == UI_CONFIRM_CHECK_COUNT_ENABLE)) {
            // Enabled
            move_sprite(SPRITE_ID_CONFIRM_CHECK, ui_confirm_check_x, ui_confirm_check_y);
        }

        app_state.ui_confirm_check_counter--;
        // Hide if done
        if (app_state.ui_confirm_check_counter == UI_CONFIRM_CHECK_COUNT_OFF) {
            hide_sprite(SPRITE_ID_CONFIRM_CHECK);
        }
    } else if (force_redraw) {
        // Disabled
        hide_sprite(SPRITE_ID_CONFIRM_CHECK);
    }
}
