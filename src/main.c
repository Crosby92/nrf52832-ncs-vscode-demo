#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/hwinfo.h>

#include "gatt_svc.h"      /* 获取 UUID */
#include "bt_conn_ctrl.h"  /* 获取安全初始化函数 */
#include "main.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

uint8_t chipId[3] = {0};

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

/**
 * @brief 蓝牙就绪回调函数
 * @details 此函数在 bt_enable() 内部被调用，是设置初始 MAC 地址的最佳时机。
 */
static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth initialized successfully.");

    // 1. 立即检查并设置 MAC 地址，抢在 SDC 自动生成之前
    size_t id_count = 0;
    bt_id_get(NULL, &id_count);

    if (id_count == 0) {
        LOG_INF("No identity found, creating a new fixed MAC address.");

        bt_addr_le_t custom_addr = {
            .type = BT_ADDR_LE_RANDOM,
            .a = { .val = {BL_MAC_ADDR_5, BL_MAC_ADDR_4, BL_MAC_ADDR_3, BL_MAC_ADDR_2, BL_MAC_ADDR_1, BL_MAC_ADDR_0} }
        };
        custom_addr.a.val[5] |= 0xC0;

        err = bt_id_create(&custom_addr, NULL);
        if (err < 0) {
            LOG_ERR("Failed to create fixed MAC address (err %d)", err);
        } else {
            char addr_str[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(&custom_addr, addr_str, sizeof(addr_str));
            LOG_INF("Successfully set MAC address to: %s", addr_str);
        }
    } else {
        LOG_WRN("An identity already exists, skipping custom MAC creation.");
    }
    hwinfo_get_device_id(chipId, sizeof(chipId));
    LOG_INF("Chip ID: %02X:%02X:%02X", chipId[0], chipId[1], chipId[2]);

    // 2. 在设置完地址后，再加载其他设置 (如绑定信息)
    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        settings_load();
        LOG_INF("Settings loaded.");
    }

    // 3. 安全初始化
    if (app_setup_security() != 0) {
        LOG_ERR("Security setup failed");
    }

    // 4. 在所有初始化完成后，开始广播
    bt_advertise_start();
}

int main(void)
{
    bt_enable(bt_ready);

    LOG_INF("Main loop running");
    return 0;
}