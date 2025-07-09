#include "pinctrl.h"
#include "gpio.h"
#include "debugUtils.h"
#include "app_init.h"
#include "stdio.h"
#include "timer.h"
#include "chip_core_irq.h"
#include "watchdog.h"
#include "stdbool.h"
#define KEY_DEBOUNCE_MS 50
#define KEY_TIMER_INDEX 1

static timer_handle_t g_key_timer = NULL;
static volatile bool g_key_event_pending = false;

static void KeyDebounceTimerCb(uintptr_t data);

/* 1. 中断回调函数 */
void KeyIsr(pin_t pin, uintptr_t param)
{
    uapi_watchdog_kick();
    (void)param;
    const pin_t KEY_PIN = 8;
    /* 禁止后续中断，启动一次性定时器进行消抖 */
    uapi_gpio_disable_interrupt(KEY_PIN);

    if (g_key_timer != NULL)
    {
        /* 50ms -> 转微秒 */
        uapi_timer_start(g_key_timer, KEY_DEBOUNCE_MS * 1000, KeyDebounceTimerCb, 0);
    }
}

static void KeyDebounceTimerCb(uintptr_t data)
{
    (void)data;
    const pin_t KEY_PIN = 8;
    uapi_watchdog_kick();
    /* 定时器到期后，确认按键仍处于按下状态 */
    if (uapi_gpio_get_val(KEY_PIN) == GPIO_LEVEL_LOW)
    {
        log_info("Key pressed on GPIO8, set event flag");
        g_key_event_pending = true;
    }

    /* 清除并重新使能GPIO中断 */
    uapi_gpio_clear_interrupt(KEY_PIN);
    uapi_gpio_enable_interrupt(KEY_PIN);
}

/* 2. 初始化函数，负责 GPIO8 配置 + 中断注册 */
void KeyInit(void)
{
    const pin_t KEY_PIN = 8; // GPIO8

    /* 2-1. 将引脚复用到普通 GPIO 功能 */
    uapi_pin_set_mode(KEY_PIN, HAL_PIO_FUNC_GPIO);

    /* 2-2. 使能内部上拉（常见的"按下接地"接法）*/
    uapi_pin_set_pull(KEY_PIN, PIN_PULL_TYPE_UP);

    /* 2-3. 设为输入方向 */
    uapi_gpio_set_dir(KEY_PIN, GPIO_DIRECTION_INPUT);

    uapi_gpio_register_isr_func(KEY_PIN, 2, KeyIsr);

    // 创建定时器与适配
    uapi_timer_init();
    uapi_timer_adapter(KEY_TIMER_INDEX, TIMER_1_IRQN, 1);
    uapi_timer_create(KEY_TIMER_INDEX, &g_key_timer);
}

/* 供任务层轮询或事件队列调用，获取并清除按键事件 */
bool KeyConsumeEvent(void)
{
    bool ret = g_key_event_pending;
    g_key_event_pending = false;
    return ret;
}