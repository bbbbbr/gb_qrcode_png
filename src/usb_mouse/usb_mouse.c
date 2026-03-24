#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "usb_mouse.h"
#include "../input_mouse.h"


uint8_t sio_usb_rx_data[SIO_USB_BUF_SZ];
uint8_t sio_usb_write_head;
uint8_t sio_usb_read_tail;
uint8_t sio_usb_count;
uint8_t usb_mouse_rx_state;

usb_mouse_data_t cur_data;


static void usb_mouse_read_start(void);
static uint8_t usb_mouse_get_num_packets_ready(void);


inline uint8_t usb_mouse_has_packets_ready(void) {

    // If the ISR writes more bytes during this read
    // that's ok. The ISR NEVER decrements it, only code
    // from does that using a Critical section.
    return (sio_usb_count >= SIO_USB_MOUSE_PACKET_SZ);
}


// Some data sampling with a logitech mouse:
//
// ~link port byte transfer ~1msec (DMG), ~0.5msec (GBC 2x)
//
// RX Zero bytes = 3,724
// Rx Bad sync   =     0
// Rx Mouse Data =   314 bytes (138 Packets)
// Frames(Msec)  = 4,001       (239 Frames)
//                 239 / 59.73 = 4.001 * 1000

bool usb_mouse_input_update(void) {

    if (usb_mouse_has_packets_ready()) {

        // Copy mouse data out of RX buffer
        usb_mouse_data_t mouse = *((usb_mouse_data_t *)&sio_usb_rx_data[sio_usb_read_tail]);

        // Increment read tail and free the data in the RX buffer
        sio_usb_read_tail += SIO_USB_MOUSE_PACKET_SZ;
        SIO_USB_IDX_INCREMENT_WRAP(sio_usb_read_tail);
        // Removes 1 packet from the RX buffer, so removed count is locked to a multiple of packet size
        // That keeps alignment and write calculations easier
        CRITICAL {
            sio_usb_count -= SIO_USB_MOUSE_PACKET_SZ;
        }

        // ===== RELATIVE MODE =====
        //
        if (mouse.buttons_and_flags & USB_MOUSE_BUTTON_LEFT)   mouse_buttons |= MOUSE_BUTTON_LEFT;
        if (mouse.buttons_and_flags & USB_MOUSE_BUTTON_RIGHT)  mouse_buttons |= MOUSE_BUTTON_RIGHT;
        if (mouse.buttons_and_flags & USB_MOUSE_BUTTON_MIDDLE) mouse_buttons |= MOUSE_BUTTON_MIDDLE;

        mouse_x_move = (mouse.move_x & USB_MOUSE_X_MOVE_MASK);
        if (!(mouse.buttons_and_flags & USB_MOUSE_X_DIR_POS)) mouse_x_move *= -1;

        mouse_y_move = (mouse.move_y & USB_MOUSE_Y_MOVE_MASK);
        if (!(mouse.buttons_and_flags & USB_MOUSE_Y_DIR_POS)) mouse_y_move *= -1;

        return true;
    }

    return false; // Mouse data not found
}


// Call this to start the process of reading the mouse via serial
// interrupt transfers
static void usb_mouse_read_start(void) {

    // Start a new transfer, the SIO interrupt will fire when it's done
    SB_REG = 0x00u;
    SC_REG = SIOF_XFER_START | SIOF_CLOCK_INT;
}


void usb_mouse_isr_sio(void) {

    uint8_t rx_data = SB_REG;

    // All valid data guaranteed to have at least 1 bit set, bail otherwise
    if (rx_data != 0x00u) {

        if (usb_mouse_rx_state == USB_MOUSE_STATE_WAIT_SYNC) {
            // Only the button byte has bit .7 unset, drop data until that's matched
            if ((rx_data & FORCE_NONZERO_BOTH_MASK) == FORCE_NONZERO_BIT_BUTTONS) {
                // Hopefully found a mouse button byte, allow it to be processed below
                usb_mouse_rx_state = USB_MOUSE_STATE_BUTTON_BYTE;
            }
        }

        // Switch won't process if usb_mouse_rx_state == USB_MOUSE_WAIT_SYNC
        switch(usb_mouse_rx_state) {

            case USB_MOUSE_STATE_BUTTON_BYTE:
                    if (rx_data & FORCE_NONZERO_BIT_BUTTONS)
                        { cur_data.buttons_and_flags = rx_data; usb_mouse_rx_state++; }
                    else { usb_mouse_rx_state == USB_MOUSE_STATE_WAIT_SYNC; } // Sync lost
                    break;

            case USB_MOUSE_STATE_X_MOVE_BYTE:
                    if (rx_data & FORCE_NONZERO_BIT_MOVE)
                        { cur_data.move_x = rx_data; usb_mouse_rx_state++; }
                    else { usb_mouse_rx_state == USB_MOUSE_STATE_WAIT_SYNC; } // Sync lost
                    break;

            case USB_MOUSE_STATE_Y_MOVE_BYTE:
                    if (rx_data & FORCE_NONZERO_BIT_MOVE) {
                        // Only commit packet data if there is space
                        // (it will always be available and written in full packet size increments)
                        // If space is not avaialble the packet is discarded
                        if ((sio_usb_count != SIO_USB_BUF_SZ)) {
                            sio_usb_rx_data[sio_usb_write_head++] = cur_data.buttons_and_flags;
                            sio_usb_rx_data[sio_usb_write_head++] = cur_data.move_x;
                            sio_usb_rx_data[sio_usb_write_head++] = rx_data;
                            SIO_USB_IDX_INCREMENT_WRAP(sio_usb_write_head);
                            sio_usb_count += SIO_USB_MOUSE_PACKET_SZ;
                        }
                        // Wrap to next packet
                        usb_mouse_rx_state = USB_MOUSE_STATE_BUTTON_BYTE;
                    }
                    else { usb_mouse_rx_state == USB_MOUSE_STATE_WAIT_SYNC; } // Sync lost
                    break;
        }
    }

    SB_REG = 0x00u;
    SC_REG = SIOF_XFER_START | SIOF_CLOCK_INT;
}



void usb_mouse_install(void) {

    CRITICAL {
        sio_usb_write_head = 0u;
        sio_usb_read_tail  = 0u;
        sio_usb_count      = 0u;
        usb_mouse_rx_state = USB_MOUSE_STATE_WAIT_SYNC;

        // Remove first to avoid accidentally double-adding it
        remove_SIO(usb_mouse_isr_sio);
        add_SIO(usb_mouse_isr_sio);
    }

    // Enable Serial interrupt
    set_interrupts(IE_REG | SIO_IFLAG);

    // Initiate a request to prime the interrupt read loop
    usb_mouse_read_start();
}


// Called in de-install the mouse SIO interrupt
void usb_mouse_deinstall(void) {

    CRITICAL {
        sio_usb_write_head = 0u;
        sio_usb_read_tail  = 0u;
        sio_usb_count      = 0u;

        remove_SIO(usb_mouse_isr_sio);
    }
    set_interrupts(IE_REG & ~SIO_IFLAG);
}
