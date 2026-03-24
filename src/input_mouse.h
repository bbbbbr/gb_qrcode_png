#ifndef _INPUT_MOUSE_H
#define _INPUT_MOUSE_H

#define MOUSE_BUTTON_LEFT          0x01u
#define MOUSE_BUTTON_RIGHT         0x02u
#define MOUSE_BUTTON_MIDDLE        0x04u
#define MOUSE_BUTTON_MASK          (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT | MOUSE_BUTTON_MIDDLE)

#define MOUSE_PRESSED(K)  (mouse_buttons & (K))
#define MOUSE_TICKED(K)   ((mouse_buttons & (K)) && !(mouse_buttons_last & (K)))
#define MOUSE_RELEASED(K) (!(mouse_buttons & (K)) && (mouse_buttons_last & (K)))

enum {
    MOUSE_TYPE_NONE,
    MOUSE_TYPE_SGB,
    MOUSE_TYPE_USB,
};

extern uint8_t mouse_type;
extern int8_t  mouse_x_move;
extern int8_t  mouse_y_move;
extern int8_t  mouse_x_move_last;
extern int8_t  mouse_y_move_last;
extern uint8_t mouse_buttons;
extern uint8_t mouse_buttons_last;

#endif