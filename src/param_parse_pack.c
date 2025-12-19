#include "param_parse_pack.h"
#include <string.h>

// 假设 chipId 是一个全局变量，如果不是，你需要提供它
// 例如，可以从 hwinfo 获取
extern uint8_t chipId[3]; 

/**
 * @brief 异或校验函数
 */
static uint8_t _xorCheck(const uint8_t *src, uint8_t len)
{
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; ++i) {
        checksum ^= src[i];
    }
    return checksum;
}

/**
 * @brief 封装开锁回复
 */
static int _bleUnlockSetCmdPack(uint8_t *dataOut, uint8_t *outLen)
{
    dataOut[0] = SEND_CMD_HEAD;
    dataOut[1] = CMD_FTE_BleUnlockSetCmd;
    dataOut[2] = 0x01; // 直接回复成功
    dataOut[3] = chipId[0];
    dataOut[4] = chipId[1];
    dataOut[5] = chipId[2];
    dataOut[6] = _xorCheck(dataOut, 6);
    *outLen = 7;
    return 0;
}

/**
 * @brief 封装关锁回复
 */
static int _bleLockSetCmdPack(uint8_t *dataOut, uint8_t *outLen)
{
    dataOut[0] = SEND_CMD_HEAD;
    dataOut[1] = CMD_FTE_BleLockSetCmd;
    dataOut[2] = 0x01; // 直接回复成功
    dataOut[3] = chipId[0];
    dataOut[4] = chipId[1];
    dataOut[5] = chipId[2];
    dataOut[6] = _xorCheck(dataOut, 6);
    *outLen = 7;
    return 0;
}


int param_parse(const uint8_t *dataIn, uint8_t inLen, uint8_t *dataOut, uint8_t *outLen)
{
    // 基本检查
    if (dataIn == NULL || inLen < 3 || dataOut == NULL || outLen == NULL) {
        return -1; // 参数错误
    }

    // 校验 CRC
    uint8_t crc = _xorCheck(dataIn, inLen - 1);
    if (crc != dataIn[inLen - 1]) {
        return -2; // CRC 错误
    }

    // 校验包头
    if (dataIn[0] != RECV_CMD_HEAD) {
        return -3; // 包头错误
    }

    uint8_t cmdType = dataIn[1];
    switch (cmdType) {
        case CMD_FTE_BleUnlockSetCmd:
            // 不做任何判断，直接封装成功回复
            return _bleUnlockSetCmdPack(dataOut, outLen);

        case CMD_FTE_BleLockSetCmd:
            // 不做任何判断，直接封装成功回复
            return _bleLockSetCmdPack(dataOut, outLen);

        default:
            // 其他命令我们不处理
            return -4; // 未知命令
    }

    return 0; // 理论上不会到这里
}