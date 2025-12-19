#include "bt_conn_ctrl.h"
#include "main.h"
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>

LOG_MODULE_REGISTER(conn_ctrl, LOG_LEVEL_INF);

/* =========================================================================
 *  Part 1: 蓝牙连接状态回调 (Connection Callbacks)
 * ========================================================================= */

/**
 * @brief 连接成功回调
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err 0x%02x)", err);
        return;
    }

    LED_STATUS = BL_CONNECTED;

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected to: %s", addr);

    /* 更新连接参数: 
     * Min/Max Interval: 30ms-50ms (相对快速的响应)
     * Latency: 0 (无延迟)
     * Timeout: 4000ms (4秒超时)
     */
    struct bt_le_conn_param *param = BT_LE_CONN_PARAM(
        BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MAX, 0, 400);

    int param_err = bt_conn_le_param_update(conn, param);
    if (param_err) {
        LOG_WRN("Param update request failed: %d", param_err);
    }

    /* 请求 L2 安全等级 (Just Works: 加密但无认证) */
    /* 注意：如果已经配对过，这里会直接启用加密；如果是新设备，会触发配对流程 */
    int sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (sec_err) {
        LOG_WRN("Failed to set security (err %d)", sec_err);
    }
}

/**
 * @brief 断开连接回调
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason 0x%02x)", reason);

    LED_STATUS = BL_UNCONNECTED;
    
    // 断开后立即重新开始广播，等待下一次连接
    bt_advertise_start(); 
}

/**
 * @brief 连接参数更新回调
 * @details 当连接间隔(Interval)、延迟(Latency)或超时(Timeout)发生变化时调用
 */
static void le_param_updated(struct bt_conn *conn, uint16_t interval,
                             uint16_t latency, uint16_t timeout)
{
    // Interval * 1.25ms = 实际时间
    LOG_INF("Connection parameters updated: interval %d (%.2f ms), latency %d, timeout %d ms",
            interval, interval * 1.25, latency, timeout * 10);
}

/**
 * @brief 物理层 (PHY) 更新回调
 * @details 当速度从 1Mbps 变为 2Mbps 或 Coded PHY 时调用
 */
static void le_phy_updated(struct bt_conn *conn,
                           struct bt_conn_le_phy_info *param)
{
    LOG_INF("PHY updated: TX PHY %u, RX PHY %u", param->tx_phy, param->rx_phy);
}

/**
 * @brief 安全等级变化回调
 * @details 非常重要！用于确认加密是否真正生效。
 *          level: BT_SECURITY_L1 (无加密), L2 (加密), L3 (加密+认证), L4 (加密+认证+ECDH)
 */
static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        LOG_INF("Security changed: %s level %u", addr, level);
        if (level >= BT_SECURITY_L2) {
            LOG_INF("--> Link is now ENCRYPTED");
        }
    } else {
        LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
    }
}

/**
 * @brief 数据长度扩展 (DLE) 更新回调
 * @details 当数据包最大负载长度发生变化时调用 (默认27字节，最高251字节)
 */
static void le_data_len_updated(struct bt_conn *conn,
                                struct bt_conn_le_data_len_info *info)
{
    LOG_INF("Data length updated: TX %u bytes, RX %u bytes", 
            info->tx_max_len, info->rx_max_len);
}

/* 注册连接回调结构体 */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_updated = le_param_updated,
    .le_phy_updated = le_phy_updated,
    .security_changed = security_changed,
    .le_data_len_updated = le_data_len_updated,
};


/* =========================================================================
 *  Part 2: 安全认证动作回调 (Action Callbacks)
 *  这里定义需要用户交互或确认的事件
 * ========================================================================= */

/**
 * @brief 取消配对回调
 */
static void auth_cancel(struct bt_conn *conn)
{
    LOG_ERR("Pairing cancelled");
}

/**
 * @brief 配对确认请求 (Just Works 流程中可能触发)
 * @details 如果手机发起了不需要密码的配对请求，协议栈可能会问“要不要配对”。
 *          我们在代码里自动确认，实现“无感”。
 */
static void auth_pairing_confirm(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("Pairing confirmation request from %s", addr);
    
    // 自动同意配对
    bt_conn_auth_pairing_confirm(conn);
}

/**
 * @brief 密码显示回调 (Just Works 模式下通常不触发，除非对方强行要求显示)
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    LOG_INF("Passkey display: %06u (Unexpected for Just Works)", passkey);
}

/**
 * @brief 密码输入回调 (Just Works 模式下不触发)
 */
static void auth_passkey_entry(struct bt_conn *conn)
{
    LOG_INF("Passkey entry requested (Unexpected)");
}

/**
 * @brief 密码确认回调 (用于数值比较配对，Just Works 模式下不触发)
 */
static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    LOG_INF("Passkey confirm: %06u", passkey);
    bt_conn_auth_passkey_confirm(conn); // 自动确认
}

#if 0
static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
    .pairing_confirm = auth_pairing_confirm, // 重要：自动确认配对请求
    .passkey_display = auth_passkey_display, // 下面这几个通常用不到，但写全防止NULL
    .passkey_entry = auth_passkey_entry,
    .passkey_confirm = auth_passkey_confirm, 
};
#else
static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = NULL,
    .passkey_entry = NULL,
    .passkey_confirm = NULL,
    .pairing_confirm = NULL,
    .cancel = NULL,
};
#endif


/* =========================================================================
 *  Part 3: 安全认证信息回调 (Info Callbacks)
 *  这里定义通知类的事件（只是告诉你结果，不需要你操作）
 * ========================================================================= */

/**
 * @brief 配对流程完成
 */
static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
    LOG_INF("Pairing Complete. Bonded: %d", bonded);
}

/**
 * @brief 配对失败
 */
static void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    // 常见错误：BT_SECURITY_ERR_PIN_OR_KEY_MISSING (密钥丢失/手机取消配对)
    LOG_ERR("Pairing Failed (reason %d)", reason);
}

/**
 * @brief 绑定被删除
 */
static void auth_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
    LOG_INF("Bond deleted for peer");
}

static struct bt_conn_auth_info_cb auth_cb_info = {
    .pairing_complete = auth_pairing_complete,
    .pairing_failed = auth_pairing_failed,
    .bond_deleted = auth_bond_deleted,
};


/* =========================================================================
 *  Part 4: 初始化函数
 * ========================================================================= */

int app_setup_security(void)
{
    int err;
    
    // 注册动作类回调 (处理配对请求)
    err = bt_conn_auth_cb_register(&auth_cb_display);
    if (err) {
        LOG_ERR("Failed to register auth callbacks: %d", err);
        return err;
    }

    // 注册信息类回调 (接收配对结果)
    err = bt_conn_auth_info_cb_register(&auth_cb_info);
    if (err) {
        LOG_ERR("Failed to register auth info callbacks: %d", err);
        return err;
    }

    // 不需要设置固定密码，因为我们走的是 Just Works
    LOG_INF("Security callbacks registered (Just Works mode)");
    
    return 0;
}