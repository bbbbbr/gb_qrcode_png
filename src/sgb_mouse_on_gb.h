#ifndef _SGB_MOUSE_ON_GB_H
#define _SGB_MOUSE_ON_GB_H

// In SGB Player 2
#define SNES_MOUSE_X_DIR   0b10000000u   // .7: 1 = Left, 0 = Left, 6..0: Movement
#define SNES_MOUSE_X_MASK  0b01111111u

// In SGB Player 3
#define SNES_MOUSE_Y_DIR   0b10000000u   // .7: 1 = Up, 0 = Up, 6..0: Movement
#define SNES_MOUSE_Y_MASK  0b01111111u

// Status bits in SGB Player 4
#define SNES_MOUSE_BUTTON_LEFT       0b00000001u
#define SNES_MOUSE_BUTTON_RIGHT      0b00000010u
#define SNES_MOUSE_SGB_MENU_OPEN     0b00000100u
#define SNES_MOUSE_IS_CONNECTED      0b10100000u
#define SNES_MOUSE_IS_CONNECTED_MASK 0b11110000u
#define SNES_MOUSE_BUTTON_BOTH       (SNES_MOUSE_BUTTON_LEFT | SNES_MOUSE_BUTTON_RIGHT)
#define SNES_MOUSE_BUTTON_MASK       (SNES_MOUSE_BUTTON_BOTH)

extern int8_t  mouse_x_move;
extern int8_t  mouse_y_move;
extern int8_t  mouse_x_move_last;
extern int8_t  mouse_y_move_last;
extern uint8_t mouse_buttons;
extern uint8_t mouse_buttons_last;

void sgb_mouse_install(void) BANKED;
bool sgb_mouse_input_update(void) BANKED;


#define MOUSE_PRESSED(K) (mouse_buttons & (K))
#define MOUSE_TICKED(K)   ((mouse_buttons & (K)) && !(mouse_buttons_last & (K)))
#define MOUSE_RELEASED(K) (!(mouse_buttons & (K)) && (mouse_buttons_last & (K)))

#endif // _SGB_MOUSE_ON_GB_H
