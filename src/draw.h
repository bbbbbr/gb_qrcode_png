#ifndef DRAW_H
#define DRAW_H

void drawing_save_to_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;
void drawing_restore_from_sram(uint8_t sram_bank, uint8_t save_slot) BANKED;

void drawing_set_to_main_colors(void) BANKED;
void drawing_set_to_alt_colors(void) BANKED;

void drawing_clear(void) BANKED;

void draw_init(void) BANKED;
void draw_update(uint8_t cursor_8u_x, uint8_t cursor_8u_y) BANKED;
void draw_tools_cancel_and_reset(void) BANKED;

#endif // DRAW_H