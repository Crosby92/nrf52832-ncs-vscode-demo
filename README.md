基于 **nRF Connect SDK (Zephyr RTOS)**

本项目演示了如何构建一个模块化的 BLE 应用，包含自定义 GATT 服务、基于状态机的 LED 指示、以及“Just Works”安全配对（绑定）功能。

## ✨ 主要功能

*   **自定义 GATT 服务**：实现了包含 Write、Read 和 Notify 特征值的自定义服务。
*   **模块化代码结构**：将蓝牙控制、GATT 服务、应用线程和公共头文件分离 (`src/` 和 `inc/`)，易于维护和扩展。
*   **LED 状态指示**：
    *   🔴 **未连接**：LED 闪烁 (500ms 间隔)。
    *   🟢 **已连接**：LED 常亮。
    *   使用 RTOS 独立线程控制 LED，通过原子变量或全局状态与蓝牙事件解耦。
*   **安全性 (Security)**：
    *   启用 **Bonding (绑定)**：设备会记住已配对的手机，实现自动重连。
    *   使用 **Just Works 配对**：无感连接，无需输入 PIN 码，但链路经过加密 (Security Level 2)。
    *   使用 NVS (Non-Volatile Storage) 持久化存储配对信息。

## 📂 项目结构

```text
.
├── CMakeLists.txt          # 构建脚本 (配置了 include 路径)
├── prj.conf                # Kconfig 配置文件 (蓝牙、NVS、日志等)
├── nrf52832wtkj.overlay    # (可选) 设备树覆盖文件
├── inc/                    # 头文件目录 (对外接口声明)
│   ├── main.h              # 全局状态定义 (LED_STATUS)
│   ├── gatt_svc.h          # GATT 服务接口
│   ├── bt_conn_ctrl.h      # 连接控制接口
│   └── app_threads.h       # 线程相关接口
├── src/                    # 源文件目录 (具体实现)
│   ├── main.c              # 程序入口，系统初始化
│   ├── gatt_svc.c          # 自定义 UUID 定义与数据读写回调
│   ├── bt_conn_ctrl.c      # 蓝牙连接回调、安全配对逻辑
│   └── app_threads.c       # LED 闪烁线程实现
└── BSP/                    # 外设驱动
```

## 📱 GATT 服务说明

| 名称 | UUID (16-bit) | 属性 | 说明 |
| :--- | :--- | :--- | :--- |
| **Service** | `0xFEE7` | - | 主服务 UUID |
| **Write Char** | `0xFEC7` | `Write` | 手机向设备发送数据 |
| **Notify Char** | `0xFEC8` | `Notify` | 设备主动向手机推送数据 |
| **Read Char** | `0xFEC9` | `Read` | 手机读取设备只读数据 |

> **注意**：
> 1. `Write` 特征值收到的数据如果开启了 Notify，会被回显（Echo）到 `Notify` 特征值。
> 2. `Read` 特征值包含固定字符串 "Zephyr-Device-ReadOnly"。

## 🛠️ 开发环境与构建

### 前置要求
*   **VS Code**
*   **nRF Connect for VS Code Extension Pack**
*   **nRF Connect SDK (NCS)**: V3.1.1

### 如何编译
1.  在 VS Code 中打开本文件夹。
2.  在 **nRF Connect** 侧边栏中，点击 **Add Application** 并选择本目录。
3.  创建 **Build Configuration**，选择开发板。
4.  点击 **Build**。

### 如何烧录
1.  使用 J-Link 连接开发板。
2.  在 **ACTIONS** 栏中点击 **Flash**。

### 开发板上电初始状态
*   蓝牙未连接时LED会闪烁
*   蓝牙已连接时LED会常亮
```
