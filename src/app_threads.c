#include "app_threads.h"
#include "main.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

//日志事件注册
LOG_MODULE_REGISTER(threads, LOG_LEVEL_INF);

//声明GPIO对应设备树结构体
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

//声明结构体

//声明状态变量
volatile enum led_state_t LED_STATUS = BL_UNCONNECTED;

/* led task */
void led_task(void)
{
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED not ready");
        return;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT);

    for (;;) {

        switch (LED_STATUS) {
        case BL_UNCONNECTED:
            gpio_pin_toggle_dt(&led);
            k_msleep(500);
            break;
        case BL_CONNECTED:
            gpio_pin_set_dt(&led, 1);
            break;
        }
    }
}

/* 启动线程 */
K_THREAD_DEFINE(led_task_thread_id, 256, led_task, NULL, NULL, NULL, 2, 0, 0);