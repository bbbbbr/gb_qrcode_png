#ifndef _USB_MOUSE_H
#define _USB_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

#define USB_MOUSE_BUTTON_LEFT          0x01u
#define USB_MOUSE_BUTTON_RIGHT         0x02u
#define USB_MOUSE_BUTTON_MIDDLE        0x04u  // Usually. Other fancy side buttons may be 0x08, 0x10, etc
#define USB_MOUSE_BUTTON_MASK          (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT | MOUSE_BUTTON_MIDDLE)
#define USB_MOUSE_X_DIR_POS            0x10u
#define USB_MOUSE_Y_DIR_POS            0x20u

// The design with the AVR in between GB and CH559 means
// it's not possible to tell the difference between no data (0x00) and zero
//so force all bytes to be non-zero using various bits as indicators
#define FORCE_NONZERO_BIT_BUTTONS  0x40u
#define FORCE_NONZERO_BIT_MOVE     0x80u
#define FORCE_NONZERO_BOTH_MASK    (FORCE_NONZERO_BIT_MOVE | FORCE_NONZERO_BIT_BUTTONS)

#define USB_MOUSE_X_MOVE_MAX       127   // Max value without MSBit set
#define USB_MOUSE_X_MOVE_MASK      0x7Fu
#define USB_MOUSE_Y_MOVE_MAX       127   // Max value without MSBit set
#define USB_MOUSE_Y_MOVE_MASK      0x7Fu

#define SIO_USB_IDX_INCREMENT_WRAP(var) if (var == SIO_USB_BUF_SZ) var = 0u;

#define SIO_USB_MOUSE_PACKET_SZ   3u
#define SIO_USB_PACKET_BUF_COUNT  4u  // SZ x COUNT should not be > 255
#define SIO_USB_BUF_SZ            (SIO_USB_MOUSE_PACKET_SZ * SIO_USB_PACKET_BUF_COUNT)

enum {
    USB_MOUSE_STATE_WAIT_SYNC,
    USB_MOUSE_STATE_BUTTON_BYTE,
    USB_MOUSE_STATE_X_MOVE_BYTE,
    USB_MOUSE_STATE_Y_MOVE_BYTE,
};

typedef struct usb_mouse_data_t {
    uint8_t buttons_and_flags;
    uint8_t move_x;
    uint8_t move_y;
} usb_mouse_data_t;

// extern usb_mouse_data_t mouse;

extern uint8_t sio_usb_rx_data[SIO_USB_BUF_SZ];
extern uint8_t sio_usb_write_head;
extern uint8_t sio_usb_read_tail;
extern uint8_t sio_usb_count;



bool usb_mouse_input_update(void);
void usb_mouse_ISR_SIO(void);
void usb_mouse_install(void);
void usb_mouse_deinstall(void);

#endif // _USB_MOUSE_H
