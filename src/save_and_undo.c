#include <gbdk/platform.h>
#include <stdint.h>

#include <gbdk/emu_debug.h>  // Sensitive to duplicated line position across source files

#include "common.h"
#include "save_and_undo.h"
#include "ui_menu_area.h"

void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED {

    SWITCH_RAM(sram_bank);
    DISPLAY_OFF;
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAW_SAVE_SLOT_SIZE * save_slot));
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
    uint8_t * p_sram_save_slot = (uint8_t *)(SRAM_BASE_A000 + (DRAW_SAVE_SLOT_SIZE * save_slot));
    uint8_t * p_vram_drawing   = (uint8_t *)(DRAWING_VRAM_START);

    for (uint8_t tile_row = 0u; tile_row < IMG_HEIGHT_TILES; tile_row++) {
        vmemcpy(p_vram_drawing, p_sram_save_slot, DRAWING_ROW_OF_TILES_SZ); // Copy all tile patterns
        p_sram_save_slot += DRAWING_ROW_OF_TILES_SZ;
        p_vram_drawing   += SCREEN_ROW_SZ;
    }
    DISPLAY_ON;
}


// Undo snapshots are a ring buffer stored in SRAM
void drawing_take_undo_snapshot(void) BANKED {

    EMU_printf("Undo: Taking (count=%hu, slot=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.undo_slot_current);
    // Always take the undo snapshot, discard (overwrite) the oldest if needed.
    // Meaning increment count up to, but not past, the max number of slots
    if (app_state.undo_count < DRAW_UNDO_SLOT_COUNT) {
        app_state.undo_count++;
    }
    // Increment slot used, wrap around if needed    
    if (app_state.undo_slot_current == DRAW_UNDO_SLOT_MAX)
        app_state.undo_slot_current = DRAW_UNDO_SLOT_MIN;
    else
        app_state.undo_slot_current++;

    // Adjust SRAM access if undo slots are in the second SRAM bank (i.e. later half of undo states)
    uint8_t sram_bank = SRAM_BANK_UNDO_SNAPSHOTS_LO;
    uint8_t sram_slot = app_state.undo_slot_current;
    if (app_state.undo_slot_current >= DRAW_UNDO_SLOTS_PER_SRAM_BANK) {
        sram_bank = SRAM_BANK_UNDO_SNAPSHOTS_HI;
        sram_slot -= DRAW_UNDO_SLOTS_PER_SRAM_BANK;
    }
    drawing_save_to_sram(sram_bank, sram_slot);

    EMU_printf("  - Undo: Done (count=%hu, slot=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.undo_slot_current);
    // Display undo button if going from zero to 1 snapshots
    if (app_state.undo_count == 1u) {
        ui_undo_button_enable();
    }
}


void drawing_restore_undo_snapshot(void) BANKED {

    EMU_printf("Undo: Restore requested (count=%hu, slot=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.undo_slot_current);
    // Make sure there is an undo to restore from
    if (app_state.undo_count > DRAW_UNDO_COUNT_NONE) {

        // Adjust SRAM access if undo slots are in the second SRAM bank (i.e. later half of undo states)
        uint8_t sram_bank = SRAM_BANK_UNDO_SNAPSHOTS_LO;
        uint8_t sram_slot = app_state.undo_slot_current;
        if (app_state.undo_slot_current >= DRAW_UNDO_SLOTS_PER_SRAM_BANK) {
            sram_bank = SRAM_BANK_UNDO_SNAPSHOTS_HI;
            sram_slot -= DRAW_UNDO_SLOTS_PER_SRAM_BANK;
        }
        drawing_restore_from_sram(sram_bank, sram_slot);

        // Reduce size of undo queue
        app_state.undo_count--;

        // Increment slot, wrap around if needed
        if (app_state.undo_slot_current == DRAW_UNDO_SLOT_MIN)
            app_state.undo_slot_current = DRAW_UNDO_SLOT_MAX;
        else
            app_state.undo_slot_current--;

        EMU_printf("  - Undo: Restore completed (count=%hu, slot=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.undo_slot_current);

            // Remove undo button if changing to zero snapshots
            if (app_state.undo_count == 0u) {
                ui_undo_button_disable();
            }
    } else {
        EMU_printf("  - Undo: None Found (count=%hu, slot=%hu)\n", (uint8_t)app_state.undo_count, (uint8_t)app_state.undo_slot_current);
    }
}
