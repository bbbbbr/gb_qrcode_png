#ifndef UI_MAIN_H
#define UI_MAIN_H


void ui_init(void) NONBANKED;
void ui_update(void) BANKED;

void ui_redraw_after_qrcode(void) BANKED;

void ui_draw_width_cycle(void) BANKED;

void ui_cursor_cycle_speed(void) BANKED;
void ui_cursor_cycle_teleport(void) BANKED;

#endif // UI_MAIN_H