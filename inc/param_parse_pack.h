#ifndef PARAM_PARSE_PACK_H
#define PARAM_PARSE_PACK_H

#include <zephyr/types.h>

/* 1. 定义命令头 */
#define RECV_CMD_HEAD 0xAA
#define SEND_CMD_HEAD 0xAB

/* 2. 定义命令 ID */
#define CMD_FTE_BleUnlockSetCmd 0x01
#define CMD_FTE_BleLockSetCmd   0x02

/* 3. 声明解析函数 */
/**
 * @brief 解析来自手机的命令
 * @param dataIn   接收到的数据
 * @param inLen    接收到的数据长度
 * @param dataOut  准备回复的数据缓冲区
 * @param outLen   回复数据的长度指针
 * @return 0 成功, 负数失败
 */
int param_parse(const uint8_t *dataIn, uint8_t inLen, uint8_t *dataOut, uint8_t *outLen);

#endif /* PARAM_PARSE_PACK_H */