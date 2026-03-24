#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "input.h"
#include "input_mouse.h"

uint8_t mouse_type = MOUSE_TYPE_NONE;
int8_t  mouse_x_move = 0;
int8_t  mouse_y_move = 0;
int8_t  mouse_x_move_last = 0;
int8_t  mouse_y_move_last = 0;
uint8_t mouse_buttons = 0u;
uint8_t mouse_buttons_last = 0u;

