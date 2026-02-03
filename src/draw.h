#ifndef DRAW_H
#define DRAW_H

void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;
void drawing_restore_from_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;
void drawing_restore_default_colors(void) BANKED;

void redraw_workarea(void) NONBANKED;
// void redraw_workarea(void) BANKED;
void draw_init(void) BANKED;
void draw_update(void) BANKED;

#endif // DRAW_H