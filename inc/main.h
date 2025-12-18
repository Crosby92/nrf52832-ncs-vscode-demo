#ifndef __MAIN_H__
#define __MAIN_H__ 

enum led_state_t {
    BL_UNCONNECTED = 0,
    BL_CONNECTED,
};

extern volatile enum led_state_t LED_STATUS;

#endif
