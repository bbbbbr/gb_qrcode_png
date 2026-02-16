#ifndef UI_MAIN_AREA_H
#define UI_MAIN_AREA_H

#include <speed_button.h>

#define UI_BOTTOM_BORDER_START 120u

// ===== TITLE BAR MENU =====

#define HELP_BUTTON_X_START 137u
#define HELP_BUTTON_Y_START 4u

#define HELP_BUTTON_WIDTH  19u
#define HELP_BUTTON_HEIGHT 16u

#define HELP_BUTTON_X_END  ((HELP_BUTTON_X_START) + HELP_BUTTON_WIDTH)
#define HELP_BUTTON_Y_END  ((HELP_BUTTON_Y_START) + (HELP_BUTTON_HEIGHT))

#define TITLE_BAR_MENU_Y_END (HELP_BUTTON_Y_END + 1u)

// ===== TOOLS MENU =====

#define TOOLS_MENU_ITEM_WIDTH  16u
#define TOOLS_MENU_ITEM_HEIGHT 16u

#define TOOLS_MENU_ITEM_COUNT   (DRAW_TOOL_COUNT) // Number of tool icons *vertically*

#define TOOLS_MENU_X_START 8u
#define TOOLS_MENU_Y_START 24u

#define TOOLS_MENU_WIDTH  (TOOLS_MENU_ITEM_WIDTH)
#define TOOLS_MENU_HEIGHT (TOOLS_MENU_ITEM_HEIGHT * TOOLS_MENU_ITEM_COUNT)

#define TOOLS_MENU_X_END  ((TOOLS_MENU_X_START) + TOOLS_MENU_WIDTH)
#define TOOLS_MENU_Y_END  ((TOOLS_MENU_Y_START) + (TOOLS_MENU_HEIGHT))


// ===== RIGHT MENU =====

#define RIGHT_MENU_ITEM_WIDTH  20u
#define RIGHT_MENU_ITEM_HEIGHT 16u

#define RIGHT_MENU_ITEM_COUNT   (RIGHT_MENU_COUNT) // Number of tool icons *vertically*

#define RIGHT_MENU_X_START 134u
#define RIGHT_MENU_Y_START 24u

#define RIGHT_MENU_DRAW_WIDTH  (RIGHT_MENU_ITEM_WIDTH)
#define RIGHT_MENU_HEIGHT (RIGHT_MENU_ITEM_HEIGHT * RIGHT_MENU_ITEM_COUNT)

#define RIGHT_MENU_X_END  ((RIGHT_MENU_X_START) + RIGHT_MENU_DRAW_WIDTH)
#define RIGHT_MENU_Y_END  ((RIGHT_MENU_Y_START) + (RIGHT_MENU_HEIGHT))

    // ===== SWAP COLOR BUTTON =====

    #define COLOR_CHANGE_BUTTON_WIDTH  16u
    #define COLOR_CHANGE_BUTTON_HEIGHT 16u

    #define COLOR_CHANGE_BUTTON_X_START (RIGHT_MENU_X_START + 2u)
    #define COLOR_CHANGE_BUTTON_Y_START (RIGHT_MENU_Y_START + (RIGHT_MENU_COLOR_SWAP * RIGHT_MENU_ITEM_HEIGHT))

    #define COLOR_MAIN_WIDTH   7u
    #define COLOR_MAIN_HEIGHT  7u
    #define COLOR_ALT_WIDTH    6u
    #define COLOR_ALT_HEIGHT   6u

    #define COLOR_MAIN_X_START COLOR_CHANGE_BUTTON_X_START + 2u
    #define COLOR_MAIN_Y_START COLOR_CHANGE_BUTTON_Y_START + 2u
    #define COLOR_ALT_X_START  COLOR_CHANGE_BUTTON_X_START + 6u
    #define COLOR_ALT_Y_START  COLOR_CHANGE_BUTTON_Y_START + 6u

    #define COLOR_MAIN_X_END   (COLOR_MAIN_X_START + COLOR_MAIN_WIDTH)
    #define COLOR_MAIN_Y_END   (COLOR_MAIN_Y_START + COLOR_MAIN_HEIGHT)
    #define COLOR_ALT_X_END   (COLOR_ALT_X_START   + COLOR_ALT_WIDTH)
    #define COLOR_ALT_Y_END   (COLOR_ALT_Y_START   + COLOR_ALT_HEIGHT)

    // Undo and Redo buttons are actually sprites to make it cheaper to turn on/off
    #define UNDO_BUTTON_SPR_X   ((RIGHT_MENU_X_START + 7u) + (uint8_t)DEVICE_SPRITE_PX_OFFSET_X)
    #define UNDO_BUTTON_SPR_Y   ((RIGHT_MENU_Y_START + (RIGHT_MENU_UNDO * RIGHT_MENU_ITEM_HEIGHT) + 4u) + DEVICE_SPRITE_PX_OFFSET_Y)

    #define REDO_BUTTON_SPR_X   ((RIGHT_MENU_X_START + 5u) + (uint8_t)DEVICE_SPRITE_PX_OFFSET_X)
    #define REDO_BUTTON_SPR_Y   ((RIGHT_MENU_Y_START + (RIGHT_MENU_REDO * RIGHT_MENU_ITEM_HEIGHT) + 4u) + DEVICE_SPRITE_PX_OFFSET_Y)

    // Speed Button (update handled in ui_main)
    #define CURSOR_SPEED_IND_ROW1_TILE_ID       253u // Index of tile in apa image mode
    #define CURSOR_SPEED_IND_ROW2_TILE_ID       273u
    #define CURSOR_SPEED_IND_ROW1_VRAM_ADDR     (_VRAM8000 + (CURSOR_SPEED_IND_ROW1_TILE_ID * TILE_SZ_BYTES)) // Index of tile in apa image mode
    #define CURSOR_SPEED_IND_ROW2_VRAM_ADDR     (_VRAM8000 + (CURSOR_SPEED_IND_ROW2_TILE_ID * TILE_SZ_BYTES)) // Index of tile in apa image mode

    // Width and height are per-speed mode indicator sub-image
    #define CURSOR_SPEED_IND_TILES_WIDTH         (speed_button_WIDTH / speed_button_TILE_W)
    #define CURSOR_SPEED_IND_TILES_HEIGHT        ((speed_button_HEIGHT / speed_button_TILE_H) / CURSOR_SPEED_MODE_COUNT)
    #define CURSOR_SPEED_IND_ROW_SZ_BYTES        (CURSOR_SPEED_IND_TILES_WIDTH * TILE_SZ_BYTES)
    #define CURSOR_SPEED_IND_MODE_SZ_BYTES       ((CURSOR_SPEED_IND_TILES_WIDTH * CURSOR_SPEED_IND_TILES_HEIGHT) * TILE_SZ_BYTES)


    // Draw Width indicator button
    #define DRAW_WIDTH_IND_SPR_X                ((RIGHT_MENU_X_START) + DEVICE_SPRITE_PX_OFFSET_X)
    #define DRAW_WIDTH_IND_SPR_Y                ((RIGHT_MENU_Y_START + ((RIGHT_MENU_DRAW_WIDTH - 1u) * RIGHT_MENU_ITEM_HEIGHT) + 11u) + DEVICE_SPRITE_PX_OFFSET_Y)

    #define DRAW_WIDTH_SPR_STEP_X               7u

// ===== (BOTTOM) FILE MENU =====

#define FILE_MENU_ITEM_WIDTH  16u
#define FILE_MENU_ITEM_HEIGHT 16u

#define FILE_MENU_ITEM_COUNT   (FILE_MENU_COUNT) // Number of tool icons *vertically*

#define FILE_MENU_X_START 24u
#define FILE_MENU_Y_START 124u

#define FILE_MENU_WIDTH  (FILE_MENU_ITEM_WIDTH * FILE_MENU_ITEM_COUNT)
#define FILE_MENU_HEIGHT (FILE_MENU_ITEM_HEIGHT)

#define FILE_MENU_X_END  ((FILE_MENU_X_START) + FILE_MENU_WIDTH)
#define FILE_MENU_Y_END  ((FILE_MENU_Y_START) + (FILE_MENU_HEIGHT))


    // File confirm check sprite
    #define FILE_CONFIRM_CHECK_SPR_X(slot_num)  ((((slot_num + FILE_MENU_START_OF_SAVE_SLOTS_OFFSET) * FILE_MENU_ITEM_WIDTH) + FILE_MENU_X_START) + 10u + DEVICE_SPRITE_PX_OFFSET_X)
    #define FILE_CONFIRM_CHECK_SPR_Y            ((FILE_MENU_Y_START) - 3u + DEVICE_SPRITE_PX_OFFSET_Y)


void ui_redraw_menus_all(void) NONBANKED;
void ui_handle_menu_area(uint8_t cursor_8u_x, uint8_t cursor_8u_y) BANKED;

void ui_menu_tools_draw_highlight(uint8_t tool_num, uint8_t draw_color) BANKED;
void ui_menu_file_draw_highlight(uint8_t num, uint8_t draw_color) BANKED;

void ui_undo_button_refresh(void) BANKED;
void ui_redo_button_refresh(void) BANKED;

void ui_cursor_speed_redraw_indicator(void) NONBANKED;
void ui_draw_width_redraw_indicator(void) BANKED;

void ui_file_confirm_check_update(bool force_redraw) BANKED;

#endif // UI_MAIN_AREA_H