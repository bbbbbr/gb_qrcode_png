#ifndef SAVE_AND_UNDO_H
#define SAVE_AND_UNDO_H

// ~ 2304 bytes per save

#define DRAWING_ROW_OF_TILES_SZ   (IMG_WIDTH_TILES * TILE_SZ_BYTES)
#define SCREEN_ROW_SZ             (DEVICE_SCREEN_WIDTH * TILE_SZ_BYTES)
#define DRAW_SAVE_SLOT_SIZE    (IMG_WIDTH_TILES * IMG_HEIGHT_TILES * TILE_SZ_BYTES)
#define DRAWING_VRAM_START        (APA_MODE_VRAM_START + (((IMG_TILE_Y_START * DEVICE_SCREEN_WIDTH) + IMG_TILE_X_START) * TILE_SZ_BYTES))

uint8_t * undo_get_last_snapshot_addr(void) BANKED;

void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;
void drawing_restore_from_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;

void drawing_take_undo_snapshot(void) BANKED;
void drawing_restore_undo_snapshot(bool take_redo_snapshot) BANKED;
void drawing_restore_redo_snapshot(void) BANKED;

#endif // SAVE_AND_UNDO_H