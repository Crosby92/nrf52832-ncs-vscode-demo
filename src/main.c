#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>

#include "gatt_svc.h"      /* 获取 UUID */
#include "bt_conn_ctrl.h"  /* 获取安全初始化函数 */

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* 广播数据定义 */
static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_UUID16_ALL, &adv_uuid.val, sizeof(adv_uuid.val)),
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY,
    800,
    801,
    NULL
);

/* 启动广播 */
void bt_advertise_start(void)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed (err %d)", err);
    } else {
        LOG_INF("Advertising started");
    }
}

/* 蓝牙底层初始化 */
void bt_init_start(void)
{
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth initialized");

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        settings_load();
        
        /* 首次启动检查并创建 Identity */
        size_t id_count = 0;
        bt_id_get(NULL, &id_count);
        if (id_count == 0) {
            LOG_WRN("Creating new ID");
            bt_id_create(NULL, NULL);
        }
    }
}

int main(void)
{
    bt_init_start();

    if (app_setup_security() != 0) {
        LOG_ERR("Security setup failed");
    }

    bt_advertise_start();

    LOG_INF("Main loop running");
    return 0;
}