#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include "common.h"
#include "input.h"

#include "ui_main.h"

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#pragma bank 255  // Autobanked

#define DRAWING_ROW_OF_TILES_SZ   (IMG_WIDTH_TILES * TILE_SZ_BYTES)
#define SCREEN_ROW_SZ             (DEVICE_SCREEN_WIDTH * TILE_SZ_BYTES)
#define DRAWING_SAVE_SLOT_SIZE    (IMG_WIDTH_TILES * IMG_HEIGHT_TILES * TILE_SZ_BYTES)
#define DRAWING_VRAM_START        (APA_MODE_VRAM_START + (((IMG_TILE_Y_START * DEVICE_SCREEN_WIDTH) + IMG_TILE_X_START) * TILE_SZ_BYTES))

#define TOOL_ERASER_SIZE  4u

static uint8_t get_radius(uint8_t cursor_8u_x, uint8_t cursor_8u_y);

static void draw_tool_pencil(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_line(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_rect(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_circle(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_eraser(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_floodfill(uint8_t cursor_8u_x, uint8_t cursor_8u_y);

static bool flood_queue_push(int8_t x1, int8_t x2, int8_t y1, int8_t y2);
static bool flood_check_fillable(uint8_t x, uint8_t y);

static uint8_t tool_start_x, tool_start_y;
static bool    tool_currently_drawing = false;
static bool    tool_fillstyle = M_NOFILL;

// For Flood-fill
static int8_t * p_flood_queue = (int8_t *)SRAM_UPPER_B000; // Flood-fill Queue temp buffer is in SRAM shared with other uses
static uint16_t flood_queue_count = 0u;
#define FLOOD_QUEUE_ENTRY_SIZE 4u  // Four bytes per flood-fill queue entry
#define FILL_OUT_OF_MEMORY false


// TODO: REORG: split sram load and save out to to new file: file_loadsave.c
void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED {

    SWITCH_RAM(sram_bank);
    DISPLAY_OFF;
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAWING_SAVE_SLOT_SIZE * save_slot));
    uint8_t * p_vram_drawing   = (uint8_t *)(DRAWING_VRAM_START);

    for (uint8_t tile_row = 0u; tile_row < IMG_HEIGHT_TILES; tile_row++) {
        vmemcpy(p_sram_save_slot, p_vram_drawing, DRAWING_ROW_OF_TILES_SZ); // Copy all tile patterns
        p_sram_save_slot += DRAWING_ROW_OF_TILES_SZ;
        p_vram_drawing   += SCREEN_ROW_SZ;
    }
    DISPLAY_ON;
}

void drawing_restore_from_sram(uint8_t sram_bank, uint8_t save_slot) BANKED {

    SWITCH_RAM(sram_bank);
    DISPLAY_OFF;
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAWING_SAVE_SLOT_SIZE * save_slot));
    uint8_t * p_vram_drawing   = (uint8_t *)(DRAWING_VRAM_START);

    for (uint8_t tile_row = 0u; tile_row < IMG_HEIGHT_TILES; tile_row++) {
        vmemcpy(p_vram_drawing, p_sram_save_slot, DRAWING_ROW_OF_TILES_SZ); // Copy all tile patterns
        p_sram_save_slot += DRAWING_ROW_OF_TILES_SZ;
        p_vram_drawing   += SCREEN_ROW_SZ;
    }
    DISPLAY_ON;

}


// // TODO: For testing, not final Controls UI
// static void test_load_save(void) {
//
//     switch (GET_KEYS_TICKED(~J_SELECT)) {
//         case J_UP:   if (app_state.save_slot_current > DRAWING_SAVE_SLOT_MIN) app_state.save_slot_current--;
//             break;
//         case J_DOWN: if (app_state.save_slot_current < DRAWING_SAVE_SLOT_MAX) app_state.save_slot_current++;
//             break;
//         case J_A:    drawing_restore_from_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
//             break;
//         case J_B:    drawing_save_to_sram(SRAM_BANK_DRAWING_SAVES, app_state.save_slot_current);
//             break;
//     }
// }



// Foreground drawing colors (border and fill the same)
void drawing_set_to_main_colors(void) BANKED {
    color(app_state.draw_color_main, app_state.draw_color_main, SOLID);
}


// Background drawing colors (border and fill the same)
void drawing_set_to_alt_colors(void) BANKED {
    // For pixel drawing
    color(app_state.draw_color_bg, app_state.draw_color_bg, SOLID);
}


void drawing_clear(void) BANKED {

    // Fill active image area in white
    drawing_set_to_alt_colors();
    box(IMG_X_START, IMG_Y_START, IMG_X_END, IMG_Y_END, M_FILL);

    drawing_set_to_main_colors();
}

void draw_init(void) BANKED {

    // Set default brush For pixel drawing
    drawing_set_to_main_colors();
}


// Expects UPDATE_KEYS() to have been called before each invocation
void draw_update(uint8_t cursor_8u_x, uint8_t cursor_8u_y) BANKED {

    if (KEY_TICKED(J_SELECT)) ui_cycle_cursor_speed();  // TODO: Possibly buggy right now?

    switch (app_state.drawing_tool) {
        case DRAW_TOOL_PENCIL: draw_tool_pencil(cursor_8u_x,cursor_8u_y);
            break;
        case DRAW_TOOL_LINE: draw_tool_line(cursor_8u_x,cursor_8u_y);
            break;
        case DRAW_TOOL_ERASER: draw_tool_eraser(cursor_8u_x,cursor_8u_y);
            break;
        case DRAW_TOOL_RECT:  draw_tool_rect(cursor_8u_x,cursor_8u_y);
            break;
        case DRAW_TOOL_CIRCLE: draw_tool_circle(cursor_8u_x,cursor_8u_y);
            break;
        case DRAW_TOOL_FLOODFILL: draw_tool_floodfill(cursor_8u_x,cursor_8u_y);
            break;
    }

    app_state.draw_cursor_8u_last_x = cursor_8u_x;
    app_state.draw_cursor_8u_last_y = cursor_8u_y;
}


// Clear any pending tool behavior and state
// May be called when switching tools/etc
void draw_tools_cancel_and_reset(void) BANKED {  // TODO

    // Clear any reservation on the B button
    app_state.draw_tool_using_b_button_action = false;

    tool_currently_drawing = false;

    drawing_set_to_main_colors();

    // switch (app_state.drawing_tool) {
    //     case DRAW_TOOL_PENCIL: // Nothing to reset for pencil
    //         break;
    //     case DRAW_TOOL_LINE: tool_currently_drawing = false; // Undraw any pending lines
    //         break;
    //     case DRAW_TOOL_ERASER:
    //         break;
    //     case DRAW_TOOL_RECT:
    //         break;
    //     case DRAW_TOOL_CIRCLE:
    //         break;
    //     case DRAW_TOOL_FLOODFILL:
    //         break;
    // }
}


static void draw_tool_pencil(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (KEY_PRESSED(DRAW_MAIN_BUTTON)) {
        plot_point(cursor_8u_x, cursor_8u_y);
    }
}


static void draw_tool_line(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (tool_currently_drawing == false) {

        // Start drawing a line
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
            // Draw the first line (1 pixel) XOR style so it can be undrawn
            color(BLACK,WHITE,XOR);
            line(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y);

            // Set line starting point
            app_state.draw_tool_using_b_button_action = true;
            tool_currently_drawing = true;
        }

    } else {
        // Line drawing is active, currently previewing position

        // Un-draw the line from the last frame (XOR)
        // But only if the cursor moved, so that it remains visible otherwise
        bool new_draw_position = false;
        if ((cursor_8u_x != app_state.draw_cursor_8u_last_x) || (cursor_8u_y !=app_state.draw_cursor_8u_last_y)) {
            color(BLACK,WHITE,XOR);
            line(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
            new_draw_position = true;
        }

        // If finalizing the line is requested, draw it normally
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            // Finalize the line
            drawing_set_to_main_colors();
            line(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y);

            // Finalizing the line doesn't end line drawing, instead
            // it begins a new line at the current position
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
        }
        else {
            // Otherwise the Line is still being actively drawn

            // B Button cancels drawing the current line
            if (KEY_TICKED(DRAW_CANCEL_BUTTON)) {

                // Cancel all line drawing
                tool_currently_drawing = false;
                app_state.draw_tool_using_b_button_action = false;

                // If it's the same position as before then need to undraw the line to cancel
                if (!new_draw_position) {
                    color(BLACK,WHITE,XOR);
                    line(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
                }
            }
            else {
                // Otherwise, if moved, update the line preview to the new position
                if (new_draw_position) {
                    // Again, XOR draw so it can be un-drawn later
                    color(BLACK,WHITE,XOR);
                    line(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y);
                }
            }
        }
    }
}


static void draw_tool_rect(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (tool_currently_drawing == false) {

        // Start drawing a rect
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
            // Draw the first rect(1 pixel) XOR style so it can be undrawn
            color(BLACK,WHITE,XOR);
            box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);

            // Set rect starting point
            app_state.draw_tool_using_b_button_action = true;
            tool_currently_drawing = true;
        }

    } else {
        // rect drawing is active, currently previewing position

        // Un-draw the rect from the last frame (XOR)
        // But only if the cursor moved, so that it remains visible otherwise
        bool new_draw_position = false;
        if ((cursor_8u_x != app_state.draw_cursor_8u_last_x) || (cursor_8u_y !=app_state.draw_cursor_8u_last_y)) {
            color(BLACK,WHITE,XOR);
            box(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y, tool_fillstyle);
            new_draw_position = true;
        }

        // If finalizing the rect is requested, draw it normally
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            // Finalize the rect
            drawing_set_to_main_colors();
            box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);

            app_state.draw_tool_using_b_button_action = false;
            tool_currently_drawing = false;
        }
        else {
            // Otherwise still being actively drawn

            // B Button cancels drawing the current one
            if (KEY_TICKED(DRAW_CANCEL_BUTTON)) {

                // Cancel all drawing
                tool_currently_drawing = false;
                app_state.draw_tool_using_b_button_action = false;

                // If it's the same position as before then need to undraw to cancel
                if (!new_draw_position) {
                    color(BLACK,WHITE,XOR);
                    box(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y, tool_fillstyle);
                }
            }
            else {
                // If moved, update the preview to the new position
                if (new_draw_position) {
                    // Again, XOR draw so it can be un-drawn later
                    color(BLACK,WHITE,XOR);
                    box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);
                }
            }
        }
    }
}


// TODO: could do a 96x96 LUT to get actual line distance
// Quick but inaccurate line length
static uint8_t get_radius(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {
    uint8_t x_dist, y_dist;
    if (tool_start_x > cursor_8u_x) x_dist = (tool_start_x - cursor_8u_x);
    else                            x_dist = (cursor_8u_x - tool_start_x);

    if (tool_start_y > cursor_8u_y) y_dist = (tool_start_y - cursor_8u_y);
    else                            y_dist = (cursor_8u_y - tool_start_y);

    // Return whichever is longer
    uint8_t result = (x_dist > y_dist) ? x_dist : y_dist;
    // Circle drawing doesn't handle radius of zero well
    if (result == 0) result++;

    // Clamp distance to not exceed drawing area
    // There is gating at the start of line drawing to make sure that this shouldn't
    // result in a radius 0, which would crash the circle drawing
    if (result > (tool_start_x - IMG_X_START)) result = (tool_start_x - IMG_X_START);
    if (result > (IMG_X_END - tool_start_x))   result = (IMG_X_END - tool_start_x);
    if (result > (tool_start_y - IMG_Y_START)) result = (tool_start_y - IMG_Y_START);
    if (result > (IMG_Y_END - tool_start_y))   result = (IMG_Y_END - tool_start_y);

    // EMU_printf("xd=%hu, yd=%hu, len=%hu\n", (uint8_t)x_dist, (uint8_t)y_dist, (uint8_t)result);
    return result;
}


static void draw_tool_circle(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (tool_currently_drawing == false) {

        // Start drawing
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {

            // Block starting a circle on any edge of the drawing area
            // since radius of 1+ would spill into the UI area
            if ((cursor_8u_x != IMG_X_START) && (cursor_8u_x != IMG_X_END) &&
                (cursor_8u_y != IMG_Y_START) && (cursor_8u_y != IMG_Y_END)) {

                tool_start_x = cursor_8u_x;
                tool_start_y = cursor_8u_y;
                // Draw the first XOR style so it can be undrawn
                color(BLACK,WHITE,XOR);
                circle(tool_start_x, tool_start_y, get_radius(cursor_8u_x, cursor_8u_y), tool_fillstyle);

                // Set starting point
                app_state.draw_tool_using_b_button_action = true;
                tool_currently_drawing = true;
            }
        }

    } else {
        // Drawing is active, currently previewing position

        // Un-draw from the last frame (XOR)
        // But only if the cursor moved, so that it remains visible otherwise
        bool new_draw_position = false;
        if ((cursor_8u_x != app_state.draw_cursor_8u_last_x) || (cursor_8u_y !=app_state.draw_cursor_8u_last_y)) {
            color(BLACK,WHITE,XOR);
            circle(tool_start_x, tool_start_y, get_radius(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y), tool_fillstyle);
            new_draw_position = true;
        }

        // If finalizing is requested, draw it normally
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            // Finalize
            drawing_set_to_main_colors();
            circle(tool_start_x, tool_start_y, get_radius(cursor_8u_x, cursor_8u_y), tool_fillstyle);

            app_state.draw_tool_using_b_button_action = false;
            tool_currently_drawing = false;
        }
        else {
            // Otherwise still being actively drawn

            // B Button cancels drawing the current one
            if (KEY_TICKED(DRAW_CANCEL_BUTTON)) {

                // Cancel all drawing
                tool_currently_drawing = false;
                app_state.draw_tool_using_b_button_action = false;

                // If it's the same position as before then need to undraw to cancel
                if (!new_draw_position) {
                    color(BLACK,WHITE,XOR);
                    circle(tool_start_x, tool_start_y, get_radius(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y), tool_fillstyle);
                }
            }
            else {
                // If moved, update the preview to the new position
                if (new_draw_position) {
                    // Again, XOR draw so it can be un-drawn later
                    color(BLACK,WHITE,XOR);
                    circle(tool_start_x, tool_start_y, get_radius(cursor_8u_x, cursor_8u_y), tool_fillstyle);
                }
            }
        }
    }
}


static void draw_tool_eraser(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (KEY_PRESSED(DRAW_MAIN_BUTTON)) {
        uint8_t end_x = cursor_8u_x + (TOOL_ERASER_SIZE - 1u);
        uint8_t end_y = cursor_8u_y + (TOOL_ERASER_SIZE - 1u);

        if (end_x > IMG_X_END) end_x = IMG_X_END;
        if (end_y > IMG_Y_END) end_y = IMG_Y_END;

        drawing_set_to_alt_colors();
        // Work around a bug in GBDK box() where it draws an I-Bar shape instead of line when x1==x2
        if (cursor_8u_x == end_x)
            line(cursor_8u_x, cursor_8u_y, end_x, end_y);
        else
            box(cursor_8u_x, cursor_8u_y, end_x, end_y, M_FILL);
    }
}


// uint16_t queue_fill_max;  // For debug measurement
static bool flood_queue_push(int8_t x1, int8_t x2, int8_t y1, int8_t y2) {

   // Bail if over memory limit
    if (flood_queue_count >= 0x1000u) {
        return FILL_OUT_OF_MEMORY;
    }

    p_flood_queue[flood_queue_count++] = x1;
    p_flood_queue[flood_queue_count++] = x2;
    p_flood_queue[flood_queue_count++] = y1;
    p_flood_queue[flood_queue_count++] = y2;

    //
    // if (flood_queue_count > queue_fill_max) queue_fill_max = flood_queue_count;
    return true;
}


static bool flood_check_fillable(uint8_t x, uint8_t y) {

    // EMU_printf(" Check: %hu, %hu\n", (uint8_t)x, (uint8_t)y);
    if ((x >= IMG_X_START) && (x <= IMG_X_END) &&
        (y >= IMG_Y_START) && (y <= IMG_Y_END)) {
        if (getpix(x, y) == app_state.draw_color_bg) return true;
    }
    return false;
}

// A lot of the time this spends is waiting for safe VRAM access (get/set pixel).
// Could turn the screen off to be faster, but it's a lot more fun to watch.
// Could also copy VRAM to SRAM and work off that, but it's ok enough as is.
//
// "Combined-scan-and-fill span filler" per Wikipedia
// Heckbert, Paul S (1990). "IV.10: A Seed Fill Algorithm"
static void draw_tool_floodfill(uint8_t x, uint8_t y) {

    if (KEY_PRESSED(DRAW_MAIN_BUTTON)) {
        drawing_set_to_main_colors();

        // EMU_printf("Start: %hu, %hu\n", (uint8_t)x, (uint8_t)y);

        // Fill queue temp buffer is in SRAM
        SWITCH_RAM(SRAM_BANK_CALC_BUFFER);

        if (flood_check_fillable(x,y) == false) return;
        flood_queue_count = 0u;
        // queue_fill_max = 0u;

        flood_queue_push(x, x, y,      1);
        flood_queue_push(x, x, y - 1, -1);

        while (flood_queue_count >= FLOOD_QUEUE_ENTRY_SIZE) {

            // Pop an entry from the queue
            uint8_t dy = p_flood_queue[--flood_queue_count]; // Last entry in is first out since it was last in (queue is LIFO)
            uint8_t y  = p_flood_queue[--flood_queue_count];
            uint8_t x2 = p_flood_queue[--flood_queue_count];
            uint8_t x1 = p_flood_queue[--flood_queue_count];

            uint8_t x = x1;
            if (flood_check_fillable(x, y)) {
                if (flood_check_fillable(x - 1, y)) {
                    plot_point(x - 1, y);
                    x = x - 1;
                }
                if (x < x1) if (flood_queue_push(x, x1 - 1, y - dy, -dy) == FILL_OUT_OF_MEMORY) return;
            }

            while (x1 <= x2) {
                uint8_t st_x = x1;
                while (flood_check_fillable(x1, y)) {
                    plot_point(x1, y);
                    x1 = x1 + 1;
                }
                if (x1     >  x) if (flood_queue_push(x, x1 - 1, y + dy, dy) == FILL_OUT_OF_MEMORY) return;
                if (x1 - 1 > x2) if (flood_queue_push(x2 + 1, x1 - 1, y - dy, -dy) == FILL_OUT_OF_MEMORY) return;
                x1 = x1 + 1;
                while ((x1 <= x2) && (flood_check_fillable(x1, y) == false)) {
                    x1 = x1 + 1;
                }
                x = x1;
            }
        }
       // EMU_printf("Fill Queue Max Depth = %u\n", (uint16_t)queue_fill_max);
    }
}

