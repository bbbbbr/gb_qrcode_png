#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "input.h"
#include "input_mouse.h"

#include "sgb_mouse_on_gb.h"
#ifdef EXTRA_HW_USB_MOUSE
    #include "usb_mouse/usb_mouse.h"
#endif

uint8_t keys = 0x00;
uint8_t previous_keys = 0x00;
uint8_t key_repeat_count = 0x00;
uint8_t key_repeat_count_last = 0x00;
joypads_t joypads;
bool sgb_found = false;
bool mouse_input_is_valid = false;
uint8_t usb_mouse_keys = 0u;

void UPDATE_KEYS(void) {
    previous_keys = keys;

    mouse_buttons_last = mouse_buttons;
    mouse_x_move_last  = mouse_x_move;
    mouse_y_move_last  = mouse_y_move;
    mouse_buttons = 0u;
    mouse_input_is_valid = false;
    mouse_type = MOUSE_TYPE_NONE;

    if (sgb_found) {
        joypad_ex(&joypads);
        keys = joypads.joy0;

        mouse_input_is_valid = sgb_mouse_input_update();
        if (mouse_input_is_valid) {
            mouse_type = MOUSE_TYPE_SGB;
            // Merge (OR) mouse buttons into main button state
            if (mouse_buttons & MOUSE_BUTTON_LEFT)  keys |= J_A;
            if (mouse_buttons & MOUSE_BUTTON_RIGHT) keys |= J_B;
        }

    }
    else {
        keys = joypad();
    }

    // USB Mouse should be called after SGB Mouse since SGB mouse will
    // have data EVERY frame, but USB Mouse may have data ready intermittently
    #ifdef EXTRA_HW_USB_MOUSE
        // When USB mouse input is valid (only on movement/click) it will overwrite any SGB mouse data set above
        // if (usb_mouse_input_update()) {
        while (usb_mouse_input_update()) {
            mouse_input_is_valid = true;
            mouse_type = MOUSE_TYPE_USB;
            // Only update usb mouse buttons -> gamepad buttons when mouse data is ready
            // since timing may not always line up and there may be frames lacking
            // mouse data where the button is actually held, and that should be persisted.
            usb_mouse_keys = 0u;
            if (mouse_buttons & MOUSE_BUTTON_LEFT)  usb_mouse_keys |= J_A;
            if (mouse_buttons & MOUSE_BUTTON_RIGHT) usb_mouse_keys |= J_B;
        }
        keys |= usb_mouse_keys;
    #endif

}


// Reduce CPU usage by only checking once per frame
void waitpadticked_lowcpu(uint8_t button_mask) {

    while (1) {

        vsync(); // yield CPU
        UPDATE_KEYS();
        if (KEY_TICKED(button_mask))
            break;
    };

    // Prevent passing through any key ticked
    // event that may have just happened
    UPDATE_KEYS();
}

// Reduce CPU usage by only checking once per frame
void waitpadup_lowcpu(uint8_t button_mask) {

    while (1) {

        vsync(); // yield CPU
        UPDATE_KEYS();
        if (!(KEY_PRESSED(button_mask)))
            break;
    };

    // Prevent passing through any key ticked
    // event that may have just happened
    UPDATE_KEYS();
}
