#ifndef COMMON_H
#define COMMON_H

#include "png_indexed.h"

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))

#define ALL_MENUS_BG_COLOR          (LTGREY)

#define TOOLS_MENU_BG_COLOR         (ALL_MENUS_BG_COLOR)
#define TOOLS_MENU_HIGHLIGHT_COLOR  (DKGREY)

#define FILE_MENU_BG_COLOR         (ALL_MENUS_BG_COLOR)
#define FILE_MENU_HIGHLIGHT_COLOR  (DKGREY)

#define DRAWING_SAVE_SLOT_MIN   0u
#define DRAWING_SAVE_SLOT_MAX   2u
#define DRAWING_SAVE_SLOT_COUNT ((DRAW_SAVE_SLOT_MAX - DRAW_SAVE_SLOT_MIN) + 1u)

#define SRAM_BANK_0   0u
#define SRAM_BANK_1   1u
#define SRAM_BANK_2   2u
#define SRAM_BANK_3   3u

#define SRAM_BANK_CALC_BUFFER        (SRAM_BANK_0)  // Keep [PNG / Base64] Calc buffer in first SRAM bank so that image apps can detect it as PNG format
#define SRAM_BANK_DRAWING_SAVES      (SRAM_BANK_1)
#define SRAM_BANK_CUR_DRAWING_CACHE  (SRAM_BANK_2)

// Current QRCode sizing estimates
// - 1bpp Image Max = 1282 - 6  palette (@ 2 col) = 1276 Bytes * 8 pixels per byte = 10208 Pixels -> fits: 104 x 96[tiles:13x12] = 9984
// - 2bpp Image Max = 1282 - 12 palette (@ 4 col) = 1270 Bytes * 4 pixels per byte = 5080  Pixels -> fits: 72 x 64[tiles:9x8] = 4608

#define IMG_BPP        PNG_BPP_1
#define TILE_SZ_PX         8u
#define TILE_SZ_BYTES  16u

// Note: Optimized display capture/read -> png relies on image being aligned to tiles horizontally
#define IMG_WIDTH_TILES   12u
#define IMG_HEIGHT_TILES  12u

#define IMG_WIDTH_PX      (IMG_WIDTH_TILES  * TILE_SZ_PX)
#define IMG_HEIGHT_PX     (IMG_HEIGHT_TILES * TILE_SZ_PX)

#define IMG_X_START       ((DEVICE_SCREEN_PX_WIDTH  - IMG_WIDTH_PX) / 2u)
#define IMG_Y_START       ((DEVICE_SCREEN_PX_HEIGHT - IMG_HEIGHT_PX) / 2u)

#define IMG_TILE_X_START  (IMG_X_START / TILE_SZ_PX)
#define IMG_TILE_Y_START  (IMG_Y_START / TILE_SZ_PX)

#define IMG_X_END         ((IMG_X_START + IMG_WIDTH_PX)  - 1u)
#define IMG_Y_END         ((IMG_Y_START + IMG_HEIGHT_PX) - 1u)

// SRAM used for working buffers
#define SRAM_BASE_A000  0xA000u
#define SRAM_UPPER_B000 0xB000u

#define APA_MODE_VRAM_START (_VRAM8000 + 0x100u)  // APA Mode starts at 0x8100, I guess leaving a couple tiles for sprites and such
#define APA_MODE_VRAM_SZ    ((_SCRN0 - _VRAM8000) - 0x100u)


#define SCREEN_X_MIN_16U  (0u)
#define SCREEN_Y_MIN_16U  (0u)
#define SCREEN_X_MAX_16U  (CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_WIDTH - 1u))
#define SCREEN_Y_MAX_16U  (CURSOR_8U_TO_16U(DEVICE_SCREEN_PX_HEIGHT - 1u))

#define SCREEN_X_MIN_8U  (0u)
#define SCREEN_Y_MIN_8U  (0u)
#define SCREEN_X_MAX_8U  (DEVICE_SCREEN_PX_WIDTH - 1u)
#define SCREEN_Y_MAX_8U  (DEVICE_SCREEN_PX_HEIGHT - 1u)


#define SPRITE_MOUSE_CURSOR 0u
#define CURSOR_POS_UNSET_8U    0xFFu

#define CURSOR_8U_TO_16U(value) ((uint16_t)value << 8)
#define CURSOR_TO_8U_X() ((uint8_t)(app_state.cursor_x >> 8))
#define CURSOR_TO_8U_Y() ((uint8_t)(app_state.cursor_y >> 8))

#define CURSOR_SPEED_PIXELSTEP  (CURSOR_8U_TO_16U(1u))
#define CURSOR_SPEED_FAST       (CURSOR_8U_TO_16U(1u))
#define CURSOR_SPEED_NORMAL     (CURSOR_8U_TO_16U(1u) / 2u) 
#define CURSOR_SPEED_UI         (CURSOR_8U_TO_16U(1u) + (CURSOR_8U_TO_16U(1u) / 2u))


enum {
    CURSOR_SPEED_MODE_MIN,

    CURSOR_SPEED_MODE_PIXELSTEP = CURSOR_SPEED_MODE_MIN,
    CURSOR_SPEED_MODE_NORMAL,
    CURSOR_SPEED_MODE_FAST,

    CURSOR_SPEED_MODE_MAX = CURSOR_SPEED_MODE_FAST,    
    CURSOR_SPEED_MODE_DEFAULT = CURSOR_SPEED_MODE_NORMAL,
};

enum {
    CURSOR_TELEPORT_MIN,

    CURSOR_TELEPORT_DRAWING = CURSOR_TELEPORT_MIN,
    CURSOR_TELEPORT_MENUS,
    CURSOR_TELEPORT_MAX = CURSOR_TELEPORT_MENUS,

    CURSOR_TELEPORT_DEFAULT = CURSOR_TELEPORT_DRAWING,
};


enum {
    DRAW_TOOL_MIN,

    DRAW_TOOL_PENCIL = DRAW_TOOL_MIN,
    DRAW_TOOL_LINE,
    DRAW_TOOL_ERASER,
    DRAW_TOOL_RECT,
    DRAW_TOOL_CIRCLE,
    DRAW_TOOL_FLOODFILL,
    // DRAW_TOOL_PAINT,

    DRAW_TOOL_MAX   = DRAW_TOOL_FLOODFILL,
    DRAW_TOOL_COUNT = (DRAW_TOOL_MAX + 1u),

    DRAW_TOOL_DEFAULT = DRAW_TOOL_PENCIL,
};

enum {
    FILE_MENU_MIN,

    FILE_MENU_LOAD = FILE_MENU_MIN,
    FILE_MENU_SAVE_SLOT_0,
    FILE_MENU_SAVE_SLOT_1,
    FILE_MENU_SAVE_SLOT_2,
    FILE_MENU_SAVE,

    FILE_MENU_MAX   = FILE_MENU_SAVE,
    FILE_MENU_COUNT = (FILE_MENU_MAX + 1u),

    FILE_MENU_START_OF_SAVE_SLOTS_OFFSET = (FILE_MENU_SAVE_SLOT_0 - FILE_MENU_LOAD)
};



typedef struct app_state_t {

    // == Saves ==
    uint8_t  save_slot_current;

    // == Cursor ==
    uint16_t cursor_x;
    uint16_t cursor_y;

    uint8_t cursor_8u_cache_x;
    uint8_t cursor_8u_cache_y;

    uint16_t cursor_draw_saved_x;
    uint16_t cursor_draw_saved_y;
    uint16_t cursor_menus_saved_x;
    uint16_t cursor_menus_saved_y;

    uint8_t  cursor_speed_mode;
    uint8_t  cursor_teleport_zone;

    // == Drawing ==
    uint8_t drawing_tool;

    uint8_t draw_cursor_8u_last_x;  // TODO: This is a drawing var, not a cursor var
    uint8_t draw_cursor_8u_last_y;

    bool     buttons_up_pending; // Helps mask some button presses


} app_state_t;

// TODO: Make this part of global status struct
extern app_state_t app_state;

void app_state_reset(void) BANKED;

void set_pal_qrmode(void) BANKED;
void set_pal_normal(void) BANKED;

void set_pal_spr_draw(void) BANKED;
void set_pal_spr_menu(void) BANKED;



#endif // COMMON_H