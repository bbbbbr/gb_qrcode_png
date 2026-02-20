#include <gbdk/platform.h>
#include <stdint.h>

#include <gb/drawing.h>

#include "common.h"
#include "input.h"

#include "save_and_undo.h"
#include "ui_main.h"

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#pragma bank 255  // Autobanked

enum {
    DRAW_ACTION_IDLE,
    DRAW_ACTION_FINALIZE,
    DRAW_ACTION_CANCEL,
    DRAW_ACTION_NEW_DRAW_POSITION
};

#define TOOL_ERASER_SIZE  4u

static uint8_t get_radius(uint8_t cursor_8u_x, uint8_t cursor_8u_y);

// Width variations for tools
static void draw_tool_pencil_width_2(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_pencil_width_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_line_width_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_rect_width_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_rect_circle_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y);

static void draw_tool_pencil(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_line(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_rect(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_circle(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_eraser(uint8_t cursor_8u_x, uint8_t cursor_8u_y);
static void draw_tool_floodfill(uint8_t cursor_8u_x, uint8_t cursor_8u_y);

// Should now only be (indirectly) called by ui_redraw_full()
// Can be remove if QRCode is changed from always-available START button press to a menu icon (that can't be accessed when tools are actively drawing)
static void draw_tool_line_finalize_last_preview(void);
static void draw_tool_rect_finalize_last_preview(void);
static void draw_tool_circle_finalize_last_preview(void);

static bool flood_queue_push(int8_t x1, int8_t x2, int8_t y1, int8_t y2);
static bool flood_check_fillable(uint8_t x, uint8_t y);

static uint8_t  tool_start_x, tool_start_y;
static bool     tool_fillstyle = M_NOFILL;
static bool     tool_undo_snapshot_taken = false;

// For Flood-fill
static int8_t * p_flood_queue = (int8_t *)SRAM_UPPER_B000; // Flood-fill Queue temp buffer is in SRAM shared with other uses
static uint16_t flood_queue_count = 0u;
#define FLOOD_QUEUE_ENTRY_SIZE 4u  // Four bytes per flood-fill queue entry
#define FILL_OUT_OF_MEMORY false



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

    drawing_take_undo_snapshot();

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
void draw_tools_cancel_and_reset(void) BANKED {

    if (app_state.tool_currently_drawing) {
        // For a couple tools that show previews there needs to be cleanup.
        // This is important since otherwise it complicates the undo approach of not saving
        // before tool preview starts and instead only saving right-before-finalizing (to avoid
        // using up and discarding undo states if the tool is canceled via B button/etc)

        switch (app_state.drawing_tool) {
            case DRAW_TOOL_LINE: draw_tool_line_finalize_last_preview();
                                 break;
            case DRAW_TOOL_RECT: draw_tool_rect_finalize_last_preview();
                                 break;
            case DRAW_TOOL_CIRCLE: draw_tool_circle_finalize_last_preview();
                                   break;
        }
    }

    // Clear any reservation on the B button
    app_state.draw_tool_using_b_button_action = false;
    app_state.tool_currently_drawing = false;
    tool_undo_snapshot_taken = false;

    drawing_set_to_main_colors();
}


static void draw_tool_pencil_width_2(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {
    // 3x3 cross shape
    uint8_t min_x = cursor_8u_x - 1u;
    uint8_t min_y = cursor_8u_y - 1u;
    uint8_t max_x = cursor_8u_x + 1u;
    uint8_t max_y = cursor_8u_y + 1u;

    // Draw area clipping
    if (min_x < IMG_X_START) min_x = IMG_X_START;
    if (min_y < IMG_Y_START) min_y = IMG_Y_START;
    if (max_x > IMG_X_END)   max_x = IMG_X_END;
    if (max_y > IMG_Y_END)   max_y = IMG_Y_END;

    line(min_x, cursor_8u_y, max_x, cursor_8u_y);
    line(cursor_8u_x, min_y, cursor_8u_x, max_y);
}


static void draw_tool_pencil_width_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {
    // ~4 pixel wide filled circle shape
    // 3x3 cross shape
    uint8_t min_x = cursor_8u_x - 1u;
    uint8_t min_y = cursor_8u_y - 1u;
    uint8_t max_x = cursor_8u_x;
    uint8_t max_y = cursor_8u_y;

    // Draw area clipping
    if (min_x < IMG_X_START) min_x++;
    if (min_y < IMG_Y_START) min_y++;

    // Narrow Top line
    line(min_x, min_y, max_x, min_y);

    // Wider Middle line
    if (min_x > IMG_X_START) min_x--;
    if (max_x < IMG_X_END)   max_x++;
    line(min_x, max_y, max_x, max_y);

    // Wider Lower line
    if (max_y < IMG_Y_END)   max_y++;
    line(min_x, max_y, max_x, max_y);

    // Narrow Bottom line
    min_x = cursor_8u_x - 1u;
    max_x = cursor_8u_x;
    if (min_x < IMG_X_START) min_x++;
    if (max_y < IMG_Y_END)   max_y++;
    line(min_x, max_y, max_x, max_y);
}


static void draw_tool_pencil(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    // Start drawing
    if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
        // Take a undo snapshot only at the start of a drawing segment
        if (tool_undo_snapshot_taken == false) {
            drawing_take_undo_snapshot();
            tool_undo_snapshot_taken = true;
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
        }
        app_state.tool_currently_drawing = true;
    }
    else if (KEY_RELEASED(DRAW_MAIN_BUTTON)) {
        // End Drawing
        tool_undo_snapshot_taken = false;
        app_state.tool_currently_drawing = false;
    }

    // Draw if active
    if (app_state.tool_currently_drawing) {

        // If cursor speed button pressed, it moves faster than 1 pixel
        // so draw another set of pencil marks.
        // Line draw isn't a good match right now
        // Pencil is currently the only tool that really has this speed issue
        if (KEY_PRESSED(UI_CURSOR_SPEED_BUTTON)) {
            uint8_t midpoint_x = (tool_start_x + cursor_8u_x) / 2u;
            uint8_t midpoint_y = (tool_start_y + cursor_8u_y) / 2u;
            if (app_state.draw_width == DRAW_WIDTH_MODE_1)
                plot_point(midpoint_x, midpoint_y);
            else if (app_state.draw_width == DRAW_WIDTH_MODE_2)
                draw_tool_pencil_width_2(midpoint_x, midpoint_y);
            else // Implied: DRAW_WIDTH_MODE_3
                draw_tool_pencil_width_3(midpoint_x, midpoint_y);

            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
        }

        if (app_state.draw_width == DRAW_WIDTH_MODE_1)
            plot_point(cursor_8u_x, cursor_8u_y);
        else if (app_state.draw_width == DRAW_WIDTH_MODE_2)
            draw_tool_pencil_width_2(cursor_8u_x, cursor_8u_y);
        else // Implied: DRAW_WIDTH_MODE_3
            draw_tool_pencil_width_3(cursor_8u_x, cursor_8u_y);
    }
}


static void draw_tool_line_width_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    uint8_t start_x = tool_start_x;
    uint8_t start_y = tool_start_y;
    uint8_t end_x   = cursor_8u_x;
    uint8_t end_y   = cursor_8u_y;

    // Draw area clipping (limit to START -1 to accommodate for left/up shifting of second line
    if (start_x < (IMG_X_START + 1u)) start_x = IMG_X_START + 1u;
    if (start_y < (IMG_Y_START + 1u)) start_y = IMG_Y_START + 1u;
    if (end_x   < (IMG_X_START + 1u)) end_x   = IMG_X_START + 1u;
    if (end_y   < (IMG_Y_START + 1u)) end_y   = IMG_Y_START + 1u;

    if (app_state.draw_width == DRAW_WIDTH_MODE_3) {
        // Draw area clipping (limit to START -1 to accommodate for left/up shifting of second line
        if (start_x > (IMG_X_END - 1u)) start_x = IMG_X_END - 1u;
        if (start_y > (IMG_Y_END - 1u)) start_y = IMG_Y_END - 1u;
        if (end_x   > (IMG_X_END - 1u)) end_x   = IMG_X_END - 1u;
        if (end_y   > (IMG_Y_END - 1u)) end_y   = IMG_Y_END - 1u;
    }

    uint8_t dist_x = (start_x > end_x) ? start_x - end_x : end_x - start_x;
    uint8_t dist_y = (start_y > end_y) ? start_y - end_y : end_y - start_y;

    // First line
    line(start_x, start_y, end_x, end_y);

    // Second line is shifted by 1 pixel, direction depends on line slope
    if (start_x == end_x) {
        start_x--; end_x--; // Vertical line
    }
    else if (start_y == end_y) {
        start_y--; end_y--; // Horizontal line
    }
    else if (dist_x > dist_y) {
        start_y--; end_y--; // Line has more horizontal spread than vertical
    }
    else { // Implied: else if (dist_x > dist_y) || (dist_x == dist_y)
        start_x--; end_x--; // Line has more vertical spread than vertical
    }

    line(start_x, start_y, end_x, end_y);

    // Third line if applicable
    if (app_state.draw_width == DRAW_WIDTH_MODE_3) {

        // The += 2 below instead of ++ is to compensate for the earlier -1
        // to get onto the opposite side of the midpoint line

        // Second line is shifted by 1 pixel, direction depends on line slope
        if (start_x == end_x) {
            start_x+=2; end_x+=2; // Vertical line
        }
        else if (start_y == end_y) {
            start_y+=2; end_y+=2; // Horizontal line
        }
        else if (dist_x > dist_y) {
            start_y+=2; end_y+=2; // Line has more horizontal spread than vertical
        }
        else { // Implied: else if (dist_x > dist_y) || (dist_x == dist_y)
            start_x+=2; end_x+=2; // Line has more vertical spread than vertical
        }

        line(start_x, start_y, end_x, end_y);
    }
}


static void draw_tool_line_finalize_last_preview(void) {
    // Undraw last preview
    // Don't draw (for preview) lines that start and end on the same pixel
    if ((tool_start_x != app_state.draw_cursor_8u_last_x) || (tool_start_y != app_state.draw_cursor_8u_last_y)) {
        color(BLACK,WHITE,XOR);
        line(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
    }

    // Take undo snapshot only if not already takenif needed (see main line draw for details)
    if (tool_undo_snapshot_taken == false)
        drawing_take_undo_snapshot();

    // Draw finalized version
    drawing_set_to_main_colors();
    if (app_state.draw_width == DRAW_WIDTH_MODE_1)
        line(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
    else
        draw_tool_line_width_2_and_3(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
}


static void draw_tool_line(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (app_state.tool_currently_drawing == false) {

        // Start drawing a line
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            tool_undo_snapshot_taken = false;
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;

            // Don't XOR draw the first line preview now since it would be 1 pixel long.
            // 1 pixel line drawing gets suppressed in the rest of the preview drawing as well

            // Set line starting point
            app_state.draw_tool_using_b_button_action = true;
            app_state.tool_currently_drawing = true;
        }

    } else {
        // Line drawing is active, currently previewing position
        bool    new_cursor_pos     = ((cursor_8u_x != app_state.draw_cursor_8u_last_x) ||
                                      (cursor_8u_y != app_state.draw_cursor_8u_last_y));

        uint8_t current_action = DRAW_ACTION_IDLE;
        if      (KEY_TICKED(DRAW_MAIN_BUTTON))   current_action = DRAW_ACTION_FINALIZE;
        else if (KEY_RELEASED(DRAW_CANCEL_BUTTON)) current_action = DRAW_ACTION_CANCEL;
        else if (new_cursor_pos)                 current_action = DRAW_ACTION_NEW_DRAW_POSITION;

        // Un-draw from the last frame (XOR)
        //
        // But only if the cursor moved (so it remains visible), it's being canceled, or being finalized (to take a clean undo snapshot)
        if (current_action != DRAW_ACTION_IDLE) {
            // Don't draw (for preview) lines that start and end on the same pixel
            if ((tool_start_x != app_state.draw_cursor_8u_last_x) || (tool_start_y != app_state.draw_cursor_8u_last_y)) {
                color(BLACK,WHITE,XOR);
                line(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);

                // EMU_printf(" Line UNDRAW xor: tx=%hu, ty=%hu | cx=%hu, cy=%hu | lx=%hu, ly=%hu\n",
                //             (uint8_t)tool_start_x, (uint8_t)tool_start_y, (uint8_t)cursor_8u_x, (uint8_t)cursor_8u_y,
                //             (uint8_t)app_state.draw_cursor_8u_last_x, (uint8_t)app_state.draw_cursor_8u_last_y);
            }
        }

        // If finalizing is requested, draw it normally
        if (current_action == DRAW_ACTION_FINALIZE) {
            // Finalize

            // Note: Unlike Rect and Circle, connected multi-line segment drawing presents two approaches to Undo
            //       1. Take a snapshot at the start and waste it if the user cancels a single line segment.
            //          Multiple started-and-canceled segments in a row waste all the undo slots
            //       2. Take a snapshot for each line segment, which uses excessive undo slots is less non-intuitive
            //       3. The middle ground is to Defer the first snapshot until after the first line segment is about
            //          to be finalized. After that don't take anymore snapshots until line drawing is completed
            //
            // Approach taken is number #3
            // So only take a snapshot before committing the first line segment
            if (tool_undo_snapshot_taken == false) {
                drawing_take_undo_snapshot();
                tool_undo_snapshot_taken = true;
            }

            drawing_set_to_main_colors();
            if (app_state.draw_width == DRAW_WIDTH_MODE_1)
                line(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y);
            else
                draw_tool_line_width_2_and_3(cursor_8u_x, cursor_8u_y);


            // Finalizing doesn't end line drawing, instead
            // it begins a new line at the current position
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
        }
        else if (current_action == DRAW_ACTION_CANCEL) {

            // Cancel all drawing
            app_state.tool_currently_drawing = false;
            app_state.draw_tool_using_b_button_action = false;
            tool_undo_snapshot_taken = false;
        }
        else if (current_action == DRAW_ACTION_NEW_DRAW_POSITION) {
            // If moved, update the preview to the new position, XOR draw so it can be un-drawn later
            // Don't draw (for preview) lines that start and end on the same pixel
            if ((tool_start_x != cursor_8u_x) || (tool_start_y != cursor_8u_y)) {
                color(BLACK,WHITE,XOR);
                line(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y);
            }
        }
    }
}


static void draw_tool_rect_width_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    uint8_t start_x = tool_start_x;
    uint8_t start_y = tool_start_y;
    uint8_t end_x   = cursor_8u_x;
    uint8_t end_y   = cursor_8u_y;

    if (app_state.draw_width == DRAW_WIDTH_MODE_3) {
        // Draw area clipping to accommodate drawing a rect outside all the way around the primary location
        if (start_x < (IMG_X_START + 1u)) start_x = IMG_X_START + 1u;
        if (start_y < (IMG_Y_START + 1u)) start_y = IMG_Y_START + 1u;
        if (end_x   < (IMG_X_START + 1u)) end_x   = IMG_X_START + 1u;
        if (end_y   < (IMG_Y_START + 1u)) end_y   = IMG_Y_START + 1u;

        if (start_x > (IMG_X_END - 1u)) start_x = IMG_X_END - 1u;
        if (start_y > (IMG_Y_END - 1u)) start_y = IMG_Y_END - 1u;
        if (end_x   > (IMG_X_END - 1u)) end_x   = IMG_X_END - 1u;
        if (end_y   > (IMG_Y_END - 1u)) end_y   = IMG_Y_END - 1u;
    }

    // First rect
    box(start_x, start_y, end_x, end_y, tool_fillstyle);

    // Second one is shifted by 1 pixel all the way around to be inside the primary one
    if (start_x == end_x) {
        // no X change since this is basically a vertical line
    } else {
        if (start_x < end_x) { start_x++; end_x--; }
        else                 { start_x--; end_x++; }
    }

    if (start_y == end_y) {
        // no y change since this is basically a horizontal line
    } else {
        if (start_y < end_y) { start_y++; end_y--; }
        else                 { start_y--; end_y++; }
    }

    box(start_x, start_y, end_x, end_y, tool_fillstyle);

    // Third rect if applicable
    if (app_state.draw_width == DRAW_WIDTH_MODE_3) {

        // The += 2 below instead of ++ is to compensate for the earlier -1
        // to shrink inside the main rect

        if (start_x == end_x) {
            // no X change since this is basically a vertical line
        } else {
            if (start_x < end_x) { start_x -= 2u; end_x += 2u; }
            else                 { start_x += 2u; end_x -= 2u; }
        }

        if (start_y == end_y) {
            // no y change since this is basically a horizontal line
        } else {
            if (start_y < end_y) { start_y -= 2u; end_y += 2u; }
            else                 { start_y += 2u; end_y -= 2u; }
        }

        box(start_x, start_y, end_x, end_y, tool_fillstyle);
    }
}


// Used for when a tool is canceled and there is an active preview pending
static void draw_tool_rect_finalize_last_preview(void) {
    // Undraw last preview
    color(BLACK,WHITE,XOR);
    box(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y, tool_fillstyle);

    // Take undo snapshot
    drawing_take_undo_snapshot();

    // Draw finalized version
    drawing_set_to_main_colors();
    if (app_state.draw_width == DRAW_WIDTH_MODE_1)
        box(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y, tool_fillstyle);
    else
        draw_tool_rect_width_2_and_3(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
}


static void draw_tool_rect(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (app_state.tool_currently_drawing == false) {

        // Start drawing a rect
        if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
            tool_start_x = cursor_8u_x;
            tool_start_y = cursor_8u_y;
            // Draw the first rect(1 pixel) XOR style so it can be undrawn
            color(BLACK,WHITE,XOR);
            box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);

            // Set rect starting point
            app_state.draw_tool_using_b_button_action = true;
            app_state.tool_currently_drawing = true;
        }

    } else {
        // rect drawing is active, currently previewing position

        uint8_t current_action = DRAW_ACTION_IDLE;
        if      (KEY_TICKED(DRAW_MAIN_BUTTON))   current_action = DRAW_ACTION_FINALIZE;
        else if (KEY_RELEASED(DRAW_CANCEL_BUTTON)) current_action = DRAW_ACTION_CANCEL;
        else if ((cursor_8u_x != app_state.draw_cursor_8u_last_x) ||
                 (cursor_8u_y !=app_state.draw_cursor_8u_last_y)) current_action = DRAW_ACTION_NEW_DRAW_POSITION;


        // Un-draw from the last frame (XOR)
        // But only if the cursor moved (so it remains visible), it's being canceled, or being finalized (to take a clean undo snapshot)
        if (current_action != DRAW_ACTION_IDLE) {
            color(BLACK,WHITE,XOR);
            box(tool_start_x, tool_start_y, app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y, tool_fillstyle);
        }

        // If finalizing is requested, draw it normally
        if (current_action == DRAW_ACTION_FINALIZE) {
            // Finalize
            drawing_take_undo_snapshot();

            drawing_set_to_main_colors();
            if (app_state.draw_width == DRAW_WIDTH_MODE_1)
                box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);
            else
                draw_tool_rect_width_2_and_3(cursor_8u_x, cursor_8u_y);

            app_state.draw_tool_using_b_button_action = false;
            app_state.tool_currently_drawing = false;
        }
        else if (current_action == DRAW_ACTION_CANCEL) {

            // Cancel all drawing
            app_state.tool_currently_drawing = false;
            app_state.draw_tool_using_b_button_action = false;
        }
        else if (current_action == DRAW_ACTION_NEW_DRAW_POSITION) {
            // If moved, update the preview to the new position, XOR draw so it can be un-drawn later
            color(BLACK,WHITE,XOR);
            box(tool_start_x, tool_start_y, cursor_8u_x, cursor_8u_y, tool_fillstyle);
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


static void draw_tool_rect_circle_2_and_3(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    uint8_t start_x = tool_start_x;
    uint8_t start_y = tool_start_y;
    uint8_t end_x   = cursor_8u_x;
    uint8_t end_y   = cursor_8u_y;

    uint8_t radius = get_radius(end_x, end_y);

    if ((start_x - radius) < (IMG_X_START + 1u)) radius--;
    if ((start_y - radius) < (IMG_Y_START + 1u)) radius--;

    if (app_state.draw_width == DRAW_WIDTH_MODE_3) {
        // Draw area clipping to accommodate drawing a circle outside all the way around the primary location
        if ((start_x + radius) > (IMG_X_END   - 1u)) radius--;
        if ((start_y + radius) > (IMG_Y_END   - 1u)) radius--;
    }

    // First circle
    circle(tool_start_x, tool_start_y, radius, tool_fillstyle);

    if (app_state.draw_width == DRAW_WIDTH_MODE_2) {
    // Second
    // (Triple draw shifted left and up instead of "radius--" since integer stepping means
    // there would be gaps between the two circle lines in some areas
    circle(tool_start_x - 1u, tool_start_y,      radius, tool_fillstyle);
    circle(tool_start_x     , tool_start_y - 1u, radius, tool_fillstyle);
    circle(tool_start_x - 1u, tool_start_y - 1u, radius, tool_fillstyle);


    // Third circle if applicable (shifted right and down)
    } else { // if (app_state.draw_width == DRAW_WIDTH_MODE_3) {

        //
        circle(tool_start_x, tool_start_y, radius + 1u, tool_fillstyle);
        circle(tool_start_x, tool_start_y, radius - 1u, tool_fillstyle);
        circle(tool_start_x - 1u, tool_start_y,      radius, tool_fillstyle);
        circle(tool_start_x     , tool_start_y - 1u, radius, tool_fillstyle);
        circle(tool_start_x + 1u, tool_start_y,      radius, tool_fillstyle);
        circle(tool_start_x     , tool_start_y + 1u, radius, tool_fillstyle);
    }
}


static void draw_tool_circle_finalize_last_preview(void) {
    // Undraw last preview
    color(BLACK,WHITE,XOR);
    circle(tool_start_x, tool_start_y, get_radius(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y), tool_fillstyle);

    // Take undo snapshot
    drawing_take_undo_snapshot();

    // Draw finalized version
    drawing_set_to_main_colors();
    if (app_state.draw_width == DRAW_WIDTH_MODE_1)
        circle(tool_start_x, tool_start_y, get_radius(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y), tool_fillstyle);
    else
        draw_tool_rect_circle_2_and_3(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y);
}


static void draw_tool_circle(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {

    if (app_state.tool_currently_drawing == false) {

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
                app_state.tool_currently_drawing = true;
            }
        }

    } else {
        // Drawing is active, currently previewing position


        uint8_t current_action = DRAW_ACTION_IDLE;
        if      (KEY_TICKED(DRAW_MAIN_BUTTON))   current_action = DRAW_ACTION_FINALIZE;
        else if (KEY_RELEASED(DRAW_CANCEL_BUTTON)) current_action = DRAW_ACTION_CANCEL;
        else if ((cursor_8u_x != app_state.draw_cursor_8u_last_x) ||
                 (cursor_8u_y !=app_state.draw_cursor_8u_last_y)) current_action = DRAW_ACTION_NEW_DRAW_POSITION;


        // Un-draw from the last frame (XOR)
        // But only if the cursor moved (so it remains visible), it's being canceled, or being finalized (to take a clean undo snapshot)
        if (current_action != DRAW_ACTION_IDLE) {
            color(BLACK,WHITE,XOR);
            circle(tool_start_x, tool_start_y, get_radius(app_state.draw_cursor_8u_last_x, app_state.draw_cursor_8u_last_y), tool_fillstyle);
        }

        // If finalizing is requested, draw it normally
        if (current_action == DRAW_ACTION_FINALIZE) {
            // Finalize
            drawing_take_undo_snapshot();

            drawing_set_to_main_colors();
            if (app_state.draw_width == DRAW_WIDTH_MODE_1)
                circle(tool_start_x, tool_start_y, get_radius(cursor_8u_x, cursor_8u_y), tool_fillstyle);
            else
                draw_tool_rect_circle_2_and_3(cursor_8u_x, cursor_8u_y);

            app_state.draw_tool_using_b_button_action = false;
            app_state.tool_currently_drawing = false;
        }
        else if (current_action == DRAW_ACTION_CANCEL) {

            // Cancel all drawing
            app_state.tool_currently_drawing = false;
            app_state.draw_tool_using_b_button_action = false;
        }
        else if (current_action == DRAW_ACTION_NEW_DRAW_POSITION) {
            // If moved, update the preview to the new position, XOR draw so it can be un-drawn later
            color(BLACK,WHITE,XOR);
            circle(tool_start_x, tool_start_y, get_radius(cursor_8u_x, cursor_8u_y), tool_fillstyle);
        }
    }
}


static void draw_tool_eraser(uint8_t cursor_8u_x, uint8_t cursor_8u_y) {


    // Start drawing
    if (KEY_TICKED(DRAW_MAIN_BUTTON)) {
        // Take a undo snapshot only at the start of a drawing segment
        if (tool_undo_snapshot_taken == false) {
            drawing_take_undo_snapshot();
            tool_undo_snapshot_taken = true;
        }
        app_state.tool_currently_drawing = true;
    }
    else if (KEY_RELEASED(DRAW_MAIN_BUTTON)) {
        // End Drawing
        tool_undo_snapshot_taken = false;
        app_state.tool_currently_drawing = false;
    }

    // Draw if active
    if (app_state.tool_currently_drawing) {
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

    if (KEY_TICKED(DRAW_MAIN_BUTTON)) {

        drawing_take_undo_snapshot();

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
                while (flood_check_fillable(x - 1, y)) {
                    plot_point(x - 1, y);
                    x = x - 1;
                }
                if (x < x1) if (flood_queue_push(x, x1 - 1, y - dy, -dy) == FILL_OUT_OF_MEMORY) return;
            }

            while (x1 <= x2) {

                uint8_t x_st = x1;
                uint8_t x_end = 0;
                while (flood_check_fillable(x1, y)) {
                    x_end = x1; // plot_point(x1, y);
                    x1 = x1 + 1;
                }
                // Speed up horizontal runs (tested in the above loop)
                // by drawing them as a line instead of as a pixel
                if (x_end) line(x_st, y, x_end, y);

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

