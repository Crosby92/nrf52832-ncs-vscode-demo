#include "gatt_svc.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(gatt, LOG_LEVEL_INF);

/* UUID 定义 */
struct bt_uuid_16 adv_uuid = BT_UUID_INIT_16(0xFEEF);
static struct bt_uuid_16 service_uuid = BT_UUID_INIT_16(0xFEE7);
static struct bt_uuid_16 write_chrc_uuid = BT_UUID_INIT_16(0xFEC7);
static struct bt_uuid_16 notify_chrc_uuid = BT_UUID_INIT_16(0xFEC8);
static struct bt_uuid_16 read_chrc_uuid = BT_UUID_INIT_16(0xFEC9);

/* 数据缓存 */
#define SHARED_DATA_BUFFER_SIZE 20
static uint8_t shared_data_buffer[SHARED_DATA_BUFFER_SIZE] = {0};
static const char read_only_data[] = "Zephyr-Device-ReadOnly";
static bool is_notify_enabled = false;

/* 回调声明 */
static ssize_t write_fec7_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t read_fec9_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset);
static void ccc_fec8_cfg_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);

/* GATT 服务定义 */
BT_GATT_SERVICE_DEFINE(my_service,
                       BT_GATT_PRIMARY_SERVICE(&service_uuid),
                       /* Write */
                       BT_GATT_CHARACTERISTIC(&write_chrc_uuid.uuid, BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE, NULL, write_fec7_cb, NULL),
                       /* Notify */
                       BT_GATT_CHARACTERISTIC(&notify_chrc_uuid.uuid, BT_GATT_CHRC_NOTIFY,
                                              0, NULL, NULL, NULL),
                       BT_GATT_CCC(ccc_fec8_cfg_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
                       /* Read */
                       BT_GATT_CHARACTERISTIC(&read_chrc_uuid.uuid, BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ, read_fec9_cb, NULL, (void *)read_only_data));

/**
 * @brief 函数名：write_fec7_cb
 * 
 * @details GATT 写特征 (Write Characteristic) 回调函数。
 *          当手机 (Client) 向 UUID 0xFEC7 写入数据时，蓝牙协议栈会自动调用此函数。
 *          本函数将接收到的数据存储到 shared_data_buffer，并根据 notify 状态决定是否回显数据。
 *
 * @param conn   [in] 连接句柄 (struct bt_conn*)，标识是谁发起的写入。
 * @param attr   [in] 属性句柄 (struct bt_gatt_attr*)，指向被写入的特征属性。
 * @param buf    [in] 数据缓冲区 (void*)，包含手机发送过来的实际数据。
 * @param len    [in] 数据长度 (uint16_t)，buf 中的字节数。
 * @param offset [in] 写入偏移量 (uint16_t)，用于长写入 (Long Write) 操作，通常短包为 0。
 * @param flags  [in] 写入标志 (uint8_t)，例如 BT_GATT_WRITE_FLAG_PREPARE 等。
 *
 * @return ssize_t
 *         - 成功：返回实际写入的字节数 (通常等于 len)。
 *         - 失败：返回负数错误码 (如 BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET))。
 */
static ssize_t write_fec7_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("Write received, len: %u", len);

    // 检查偏移量是否越界
    if (offset + len > sizeof(shared_data_buffer)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    // 将数据复制到本地缓冲区
    memcpy(shared_data_buffer + offset, buf, len);
    LOG_HEXDUMP_INF(shared_data_buffer, len, "Data stored:");

    // 如果通知已启用，将写入的数据通过 Notify 特征 (0xFEC8) 发回给手机
    if (is_notify_enabled) {
        /* 注意：&my_service.attrs[4] 必须指向 Notify 特征的值属性 */
        bt_gatt_notify(conn, &my_service.attrs[4], shared_data_buffer, len);
        LOG_INF("Notify sent");
    }
    return len;
}

/**
 * @brief 函数名：read_fec9_cb
 * 
 * @details GATT 读特征 (Read Characteristic) 回调函数。
 *          当手机 (Client) 请求读取 UUID 0xFEC9 的值时，蓝牙协议栈调用此函数。
 *          本函数将本地的只读数据 (read_only_data) 返回给手机。
 *
 * @param conn   [in] 连接句柄 (struct bt_conn*)。
 * @param attr   [in] 属性句柄 (struct bt_gatt_attr*)，指向被读取的特征属性。
 * @param buf    [out] 输出缓冲区 (void*)，用于存放返回给手机的数据。
 * @param len    [in] 缓冲区大小 (uint16_t)，手机能接收的最大长度。
 * @param offset [in] 读取偏移量 (uint16_t)，手机可能分段读取数据。
 *
 * @return ssize_t
 *         - 成功：返回读取的字节数。
 *         - 失败：返回负数错误码。
 *         - 注：bt_gatt_attr_read 是 Zephyr 提供的辅助函数，自动处理偏移和拷贝。
 */
static ssize_t read_fec9_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    LOG_INF("Read requested");
    // 使用 Zephyr 标准帮助函数处理读取逻辑
    return bt_gatt_attr_read(conn, attr, buf, len, offset, read_only_data, sizeof(read_only_data) - 1);
}

/**
 * @brief 函数名：ccc_fec8_cfg_changed_cb
 * 
 * @details CCCD (Client Characteristic Configuration Descriptor) 配置变更回调。
 *          当手机在 0xFEC8 特征上点击“开启通知”或“关闭通知”时调用。
 *          用于告知设备端是否允许发送 Notify 消息。
 *
 * @param attr   [in] 属性句柄 (struct bt_gatt_attr*)，指向 CCCD 描述符。
 * @param value  [in] 配置值 (uint16_t)，具体含义如下：
 *                - 0x0000: 关闭通知 (Notifications disabled)
 *                - 0x0001: 开启通知 (BT_GATT_CCC_NOTIFY)
 *                - 0x0002: 开启指示 (BT_GATT_CCC_INDICATE)
 *
 * @return void  无返回值。
 */
static void ccc_fec8_cfg_changed_cb(const struct bt_gatt_attr *attr, uint16_t value)
{
    // 更新全局标志位，判断是否开启了 Notify
    is_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notify state: %d", is_notify_enabled);
}