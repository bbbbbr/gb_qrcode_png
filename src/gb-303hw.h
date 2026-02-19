
#ifndef GB_303HW
#define GB_303HW

#ifdef HARDWARE_GB_303

#define GB303_THRESH_ON              (0x10u)
#define GB303_THRESH_OFF             (0x08u)
#define GB303_KNOB_WIDTH_2_THRESHOLD ((255u / 3u) * 1u)
#define GB303_KNOB_WIDTH_3_THRESHOLD ((255u / 3u) * 2u)

extern uint8_t pot1;
extern uint8_t pot2;
extern uint8_t pot3;

extern uint8_t pot1_avg;
extern uint8_t pot2_avg;
extern uint8_t pot3_avg;

extern int16_t pot1_delta;
extern int16_t pot2_delta;
extern int16_t pot3_delta;

void gb_303_read_potentiometers(void);

#endif // HARDWARE_GB_303

#endif // GB_303HW
