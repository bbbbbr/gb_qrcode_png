#ifndef _INPUT_H
#define _INPUT_H

#define J_ANY (J_UP | J_DOWN | J_LEFT | J_RIGHT | J_A | J_B | J_START | J_SELECT)
#define J_WAIT_ALL_RELEASED 0xFF
#define J_WAIT_ANY_PRESSED  0x00
#define J_DPAD (J_LEFT | J_RIGHT | J_UP | J_DOWN)

#define UPDATE_KEYS() previous_keys = keys; keys = joypad()
#define UPDATE_KEY_REPEAT(MASK) if (MASK & previous_keys & keys) { key_repeat_count++; } else { key_repeat_count_last = key_repeat_count; key_repeat_count=0; }
#define RESET_KEY_REPEAT(NEWVAL) key_repeat_count = NEWVAL
#define KEY_REPEAT_COUNT (key_repeat_count)
#define KEY_REPEAT_COUNT_LAST (key_repeat_count_last)

#define KEY_PRESSED(K) (keys & (K))
#define KEY_TICKED(K)   ((keys & (K)) && !(previous_keys & (K)))
#define KEY_RELEASED(K) (!(keys & (K)) && (previous_keys & (K)))

#define GET_KEYS_TICKED(K)   ((keys & ~previous_keys) & (K))

#define KEYS()   (keys)

#define ANY_KEY_PRESSED (keys)

void waitpadticked_lowcpu(uint8_t button_mask);

extern uint8_t keys;
extern uint8_t previous_keys;
extern uint8_t key_repeat_count;
extern uint8_t key_repeat_count_last;

#endif // _INPUT_H


