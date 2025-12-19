#ifndef __MAIN_H__
#define __MAIN_H__ 

#include <zephyr/types.h>

#define BL_MAC_ADDR_0 0x14
#define BL_MAC_ADDR_1 0x0F
#define BL_MAC_ADDR_2 0x42
#define BL_MAC_ADDR_3 0x06
#define BL_MAC_ADDR_4 0x4A
#define BL_MAC_ADDR_5 0x25

enum led_state_t {
    BL_UNCONNECTED = 0,
    BL_CONNECTED,
};

extern volatile enum led_state_t LED_STATUS;

extern uint8_t chipId[3];

#endif
