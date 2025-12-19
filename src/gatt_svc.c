#include "gatt_svc.h"
#include "param_parse_pack.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(gatt, LOG_LEVEL_INF);

/* UUID 定义 */
struct bt_uuid_16 adv_uuid = BT_UUID_INIT_16(0x1812);
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
    LOG_INF("GATT Write received on 0xFEC7, len: %u", len);
    LOG_HEXDUMP_INF(buf, len, "Received Data:");

    // --- 这里是你的核心业务逻辑 ---
    char dataIn[256] = {0};
    unsigned char dataInLen = 0;
    char dataOut[256] = {0};
    unsigned char dataOutLen = 0;

    if (len > sizeof(dataIn))
    {
        LOG_ERR("Incoming data too long!");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    dataInLen = len;
    memcpy(dataIn, buf, dataInLen);

    char result = param_parse(dataIn, dataInLen, dataOut, &dataOutLen);

    if (result < 0)
    {
        LOG_ERR("param_parse failed with code: %d", result);
    }
    else
    {
        if (dataOutLen > 0)
        {
            if (is_notify_enabled)
            {
                // 【关键】找到 Notify 特征值的句柄并发送
                // 注意：这个索引值 [4] 需要根据 BT_GATT_SERVICE_DEFINE 的结构来确定
                // 结构: [0]服务, [1]写Decl, [2]写Val, [3]Notify Decl, [4]Notify Val
                int err = bt_gatt_notify(conn, &my_service.attrs[4], dataOut, dataOutLen);
                if (err)
                {
                    LOG_ERR("bt_gatt_notify failed (err %d)", err);
                }
                else
                {
                    LOG_INF("Notification sent, len: %u", dataOutLen);
                    LOG_HEXDUMP_INF(dataOut, dataOutLen, "Sent Data:");
                }
            }
            else
            {
                LOG_WRN("Client has not enabled notifications, data dropped.");
            }
        }
    }

    return len; // 告诉协议栈已成功处理 len 字节
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
 */
static ssize_t read_fec9_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    LOG_INF("GATT Read requested on 0xFEC9");
    const char my_data[] = "\x11\x18\x19";
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, my_data, sizeof(my_data));
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
    LOG_INF("Notification state has been changed by client: %s", is_notify_enabled ? "ENABLED" : "DISABLED");
}