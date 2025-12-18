#ifndef BT_CONN_CTRL_H
#define BT_CONN_CTRL_H

/* 注册安全回调并设置固定密码 */
int app_setup_security(void);

/* 声明外部实现的广播函数 (main.c 实现) */
void bt_advertise_start(void);

#endif /* BT_CONN_CTRL_H */