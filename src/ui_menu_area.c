#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "input.h"

#include "draw.h"
#include "ui_main.h"
#include "ui_menu_area.h"

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
static void ui_menu_file(uint8_t cursor_8u_y);
static void ui_swap_active_color(void);

// Draws the paint working area
void ui_redraw_menus_all(void) NONBANKED {

    DISPLAY_OFF;

    uint8_t save_bank = CURRENT_BANK;

    // Alternate CDE theme by holding SELECT
    if (KEY_PRESSED(J_SELECT)) {
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

    DISPLAY_ON;

    // EMU_printf("Display: %hux%hu\n", (uint8_t)IMG_WIDTH_PX, (uint8_t)IMG_HEIGHT_PX);
}

void ui_handle_menu_area(uint8_t cursor_8u_x, uint8_t cursor_8u_y) BANKED {

    // TODO: If needed: make a more abstract button handler (array of rect areas + function pointers)

    // TODO: OPTIMIZE: instead of dispatching on every cursor move, only dispatch to menus on button press?

    // TODO: Could partition the screen into left/Right to reduce the number of collision box tests

    // TODO: Sort of a hack, but cancel any pending tool operations if a button is pressed in the menu area
    if (KEY_TICKED(UI_ACTION_BUTTON)) {
        draw_tools_cancel_and_reset();
    }

    // Tools Menu
    if ((cursor_8u_x >= TOOLS_MENU_X_START) && (cursor_8u_x < TOOLS_MENU_X_END) &&
        (cursor_8u_y >= TOOLS_MENU_Y_START) && (cursor_8u_y < TOOLS_MENU_Y_END)) {
        ui_menu_tools(cursor_8u_y);
    }
    // File Menu
    else if ((cursor_8u_x >= FILE_MENU_X_START) && (cursor_8u_x < FILE_MENU_X_END) &&
             (cursor_8u_y >= FILE_MENU_Y_START) && (cursor_8u_y < FILE_MENU_Y_END)) {
        ui_menu_file(cursor_8u_y);
    }
    else if ((cursor_8u_x >= CLEAR_BUTTON_X_START) && (cursor_8u_x < CLEAR_BUTTON_X_END) &&
             (cursor_8u_y >= CLEAR_BUTTON_Y_START) && (cursor_8u_y < CLEAR_BUTTON_Y_END)) {
        if (KEY_TICKED(UI_ACTION_BUTTON)) {
            drawing_clear();
        }
    }
    else if ((cursor_8u_x >= CLEAR_BUTTON_X_START) && (cursor_8u_x < CLEAR_BUTTON_X_END) &&
             (cursor_8u_y >= CLEAR_BUTTON_Y_START) && (cursor_8u_y < CLEAR_BUTTON_Y_END)) {
        if (KEY_TICKED(UI_ACTION_BUTTON)) {
            drawing_clear();
        }
    }
    else if ((cursor_8u_x >= COLOR_CHANGE_BUTTON_X_START) && (cursor_8u_x < COLOR_CHANGE_BUTTON_X_END) &&
             (cursor_8u_y >= COLOR_CHANGE_BUTTON_Y_START) && (cursor_8u_y < COLOR_CHANGE_BUTTON_Y_END)) {
        if (KEY_TICKED(UI_ACTION_BUTTON)) {
            ui_swap_active_color();
        }
    }
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


static void ui_menu_file(uint8_t cursor_8u_y) {

    // TODO: optional: highlight button under cursor as it moves
    // TODO: optional: animate button press somehow

    // A button used to press buttons
    if (KEY_TICKED(UI_ACTION_BUTTON)) {
        // Tool icons are uniform in size, so divide position by size to get it
        uint8_t file_menu_item = (cursor_8u_y - FILE_MENU_Y_START) / FILE_MENU_ITEM_HEIGHT;

        switch (file_menu_item) {
            case FILE_MENU_LOAD:
                drawing_restore_from_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
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
                    drawing_save_to_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
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
    
    uint8_t x1 =  FILE_MENU_X_START;
    uint8_t y1 = (num * FILE_MENU_ITEM_HEIGHT) + (FILE_MENU_Y_START);

    color(draw_color, WHITE, SOLID);
    box(x1, y1, x1 + (FILE_MENU_ITEM_WIDTH -1u), y1 + (FILE_MENU_ITEM_HEIGHT - 1u), M_NOFILL);
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

