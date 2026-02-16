#include <gbdk/platform.h>
#include <stdint.h>

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "save_and_undo.h"
#include "ui_menu_area.h"



// app_state.next_undo_slot -> Always points to an "unused" slot after operations are completed
//                             One slot past the most recent undo snapshot taken
//                             Incremented AFTER taking a new snapshot
//                             Decremented BEFORE restoring a snapshot
//
// app_state.undo_count     -> Number of undo snapshots taken
//                             Value of 1 means 1 snapshot available at (app_state.next_undo_slot - 1)
//
// app_state.redo_count     -> Number of redo snapshots available
//                             Value of 1 means 1 snapshot available at (app_state.next_undo_slot - 1)
//                             Reset to zero when undo snapshot taken
//                             Incremented when undo snapshot restored / undo_count decremented


static inline uint8_t get_next_undo_slot(void);

#define CALC_SRAM_BANK_AND_SLOT(slotnum, bankvar, slotvar) \
    bankvar = SRAM_BANK_UNDO_SNAPSHOTS_LO; \
    slotvar = (slotnum); \
    if ((slotnum) >= DRAW_UNDO_SLOTS_PER_SRAM_BANK) { \
        bankvar = SRAM_BANK_UNDO_SNAPSHOTS_HI; \
        slotvar -= DRAW_UNDO_SLOTS_PER_SRAM_BANK; \
    }


void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED {

    SWITCH_RAM(sram_bank);
    // VRAM/SRAM copy takes long enough with display off that it's distracting
    // (at least on emulators in DMG mode)
    //
    // TODO: OPTIMIZE: Pencil and Eraser have a few frames of lag since they snapshot when tool starts
    // drawing (others only do so on draw commit). Could snapshot while idle and then only commit the
    // snapshot once the tool starts drawing, but that wastes a slot. The lag doesn't seem to noticeable
    // right now when DMG-drawing.

    // DISPLAY_OFF;
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAW_SAVE_SLOT_SIZE * save_slot));
    uint8_t * p_vram_drawing   = (uint8_t *)(DRAWING_VRAM_START);

    for (uint8_t tile_row = 0u; tile_row < IMG_HEIGHT_TILES; tile_row++) {
        vmemcpy(p_sram_save_slot, p_vram_drawing, DRAWING_ROW_OF_TILES_SZ); // Copy all tile patterns
        p_sram_save_slot += DRAWING_ROW_OF_TILES_SZ;
        p_vram_drawing   += SCREEN_ROW_SZ;
    }
    // DISPLAY_ON;
}

void drawing_restore_from_sram(uint8_t sram_bank, uint8_t save_slot) BANKED {

    SWITCH_RAM(sram_bank);
    // DISPLAY_OFF;
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAW_SAVE_SLOT_SIZE * save_slot));
    uint8_t * p_vram_drawing   = (uint8_t *)(DRAWING_VRAM_START);

    for (uint8_t tile_row = 0u; tile_row < IMG_HEIGHT_TILES; tile_row++) {
        vmemcpy(p_vram_drawing, p_sram_save_slot, DRAWING_ROW_OF_TILES_SZ); // Copy all tile patterns
        p_sram_save_slot += DRAWING_ROW_OF_TILES_SZ;
        p_vram_drawing   += SCREEN_ROW_SZ;
    }
    // DISPLAY_ON;
}


static inline uint8_t get_next_undo_slot(void) {
    // Increment slot used, wrap around if needed    
    if (app_state.next_undo_slot == DRAW_UNDO_SLOT_MAX)
        return DRAW_UNDO_SLOT_MIN;
    else
        return app_state.next_undo_slot + 1u;
}


static inline uint8_t get_previous_undo_slot(void) {
    // Decrement slot used, wrap around if needed    
    if (app_state.next_undo_slot == DRAW_UNDO_SLOT_MIN)
        return DRAW_UNDO_SLOT_MAX;
    else
        return app_state.next_undo_slot - 1u;
}


// Undo snapshots are a ring buffer stored in SRAM
void drawing_take_undo_snapshot(void) BANKED {

    // EMU_printf("Undo: Taking (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    uint8_t sram_bank, sram_slot;

    // Hide redo button if showing
    // Taking an undo snapshot clears out any existing redo queue
    app_state.redo_count = DRAW_REDO_COUNT_NONE;
    ui_redo_button_refresh();

    // Always take the undo snapshot, discard (overwrite) the oldest if needed.
    // Meaning increment count up to, but not past, the max number of slots
    if (app_state.undo_count < DRAW_UNDO_SLOT_COUNT_USABLE) {
        app_state.undo_count++;
    }

    // Adjust SRAM access if undo slots are in the second SRAM bank (i.e. later half of undo states)
    CALC_SRAM_BANK_AND_SLOT(app_state.next_undo_slot, sram_bank, sram_slot);
    drawing_save_to_sram(sram_bank, sram_slot);

    // Increment slot used AFTER taking it
    app_state.next_undo_slot = get_next_undo_slot();

    // EMU_printf("  - Undo: Done (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);

    // Make sure undo button is enabled
    ui_undo_button_refresh();
}


// take_redo_snapshot is (currently) only false when called for QR code generating
// If sram size is increased the qrcode snapshotting could be split off from the undo stack and remove the need for this
void drawing_restore_undo_snapshot(bool take_redo_snapshot) BANKED {

    // EMU_printf("Undo: Restore requested (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    uint8_t sram_bank, sram_slot;

    // Make sure there is an undo to restore from
    if (app_state.undo_count > DRAW_UNDO_COUNT_NONE) {

        if (take_redo_snapshot) {
            // If redo count is zero it means this is the head of the undo queue
            // and a redo snapshot should be taken of the pre-undo state (added to
            // the end of the queue)
            // It may overwrite the Oldest undo entry if the queue is maximally full
            if (app_state.redo_count == DRAW_REDO_COUNT_NONE) {
                // Current slot points to an "empty" slot one past the last undo snapshot
                // so take the snapshot of the current d+rawing there before it gets
                // rewound during the undo restore
                CALC_SRAM_BANK_AND_SLOT(app_state.next_undo_slot, sram_bank, sram_slot);
                drawing_save_to_sram(sram_bank, sram_slot);

            }
            // Should not need a limiter on redo_count, the undo count limiter
            // should effectively cap it at DRAW_UNDO_SLOT_COUNT_USABLE
            // Make sure redo button is enabled
            app_state.redo_count++;
            ui_redo_button_refresh();
        }

        // Decrement to previous slot BEFORE restoring it
        app_state.next_undo_slot = get_previous_undo_slot();

        // Adjust SRAM access if undo slots are in the second SRAM bank (i.e. later half of undo states)
        CALC_SRAM_BANK_AND_SLOT(app_state.next_undo_slot, sram_bank, sram_slot);
        drawing_restore_from_sram(sram_bank, sram_slot);

        // Reduce size of undo queue
        // Remove undo button if zero snapshots
        app_state.undo_count--;
        ui_undo_button_refresh();

        // EMU_printf("  - Undo: Restore completed (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    } else {
        // EMU_printf("  - Undo: None Found (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    }
}


void drawing_restore_redo_snapshot(void) BANKED {

    // EMU_printf("Undo: REDO Restore requested (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    uint8_t sram_bank, sram_slot;

    // Make sure there is an redo to restore from
    if (app_state.redo_count > DRAW_REDO_COUNT_NONE) {

        // Current redo slot is always the *next* slot after current next undo slot
        CALC_SRAM_BANK_AND_SLOT( get_next_undo_slot(), sram_bank, sram_slot);
        drawing_restore_from_sram(sram_bank, sram_slot);

        // Hide redo button if going to zero snapshots
        // Taking an undo snapshot clears out any existing redo queue
        app_state.redo_count--;
        ui_redo_button_refresh();
        // Increment the undo count, basically transferring the snapshot
        // from the redo side of the queue into the undo side
        app_state.undo_count++;
        ui_undo_button_refresh();

        app_state.next_undo_slot = get_next_undo_slot();

        // EMU_printf("  - Undo: REDO Restore completed (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    } else {
        // EMU_printf("  - Undo: REDO None Found (count=%hu, slot=%hu, redo_sz=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.next_undo_slot, (uint8_t)app_state.redo_count);
    }
}

