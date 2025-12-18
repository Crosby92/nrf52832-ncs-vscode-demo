#ifndef GATT_SVC_H
#define GATT_SVC_H

#include <zephyr/bluetooth/uuid.h>

/* 供 main.c 广播数据使用的 UUID 声明 */
extern struct bt_uuid_16 adv_uuid;

#endif /* GATT_SVC_H */