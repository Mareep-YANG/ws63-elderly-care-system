// 拓展板程序，为hisoc -ExBoard，注释中通常注明为Ex Server
// 承担星闪中的Server0功能

#include "pinctrl.h"
#include "soc_osal.h"
#include "gpio.h"
#include "dma.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "pwm_ex.h"
#include "los_task.h"
#include "watchdog.h"
#include "common_def.h"
#include "math.h"
#include "adc.h"
#include "adc_porting.h"
#include "cJSON.h"
#include "string.h"
// private
#include "aht20.h"
#include "bh1750.h"
#include "ws2812.h"
#include "sle_uart_server.h"
#include "motor.h"
#include "ssd1363.h"
#include "button.h"
#include "bmp.h"

#define ExBoard_TASK_STACK_SIZE 0x4000
#define ExBoard_TASK_PRIO (osPriority_t)(17)

// WatchDog相关参数
#define TIME_OUT 2
#define WDT_MODE 1

#define BUZZ_PIN 14

// 电机舵机的线程控制（一般不动）
uint8_t sg90_ctrl = 0;
// 电机和舵机
uint16_t sg90_angles[4] = {0};
// 四个传感器
double temperature = 0.0, humidity = 0.0, air = 0.0;
uint16_t light = 0;
uint8_t weather;
// ws2812灯带
uint8_t led_mode = 0;
uint16_t rgbs[3] = {0};
// 蜂鸣器
uint8_t buzz_mode = 0;
// ADC参数
adc_scan_config_t adc_config;

static errcode_t watchdog_callback(uintptr_t param)
{
    UNUSED(param);
    osal_printk("watchdog kick timeout!\r\n");
    return ERRCODE_SUCC;
}

void gpio_callback(pin_t pin, uintptr_t param)
{
    UNUSED(pin);
    UNUSED(param);
    cJSON *message = cJSON_CreateArray();

    cJSON *cs_object = cJSON_CreateObject();
    cJSON_AddStringToObject(cs_object, "topic", "CautionStatus");
    cJSON_AddNumberToObject(cs_object, "caution", 1);
    cJSON_AddItemToArray(message, cs_object);

    char *message_str = cJSON_Print(message);

    osal_printk("%s length:%d\r\n", message_str, strlen(message_str + 1)); // 不支持直接输出浮点数

    // 星闪发送数据
    if (sle_uart_client_is_connected())
    {
        // 发送数据给EPD client
        sle_uart_server_send_report_by_handle((uint8_t *)message_str, strlen(message_str));
    }
    cJSON_Delete(message);
    osal_msleep(200);
}

// ADC回调
void adc_callback(uint8_t ch, uint32_t *buffer, uint32_t length, bool *next)
{
    unused(next);
    uint32_t total = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        total += buffer[i];
    }
    air = total / length / 40.0;
    osal_printk("channel: %d, voltage: %dmv\r\n", ch, total / length);
}

// 发送数据后的回调函数
static void sle_server_read_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
                                errcode_t status)
{
    osal_printk("%s ssaps read request cbk callback server_id:%x, conn_id:%x, handle:%x, status:%x\r\n",
                SLE_UART_SERVER_LOG, server_id, conn_id, read_cb_para->handle, status);
}

// 接收数据后的回调函数
static void sle_server_write_cbk(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *data,
                                 errcode_t status)
{
    osal_printk("%s Write request received, server_id: %d, conn_id: %d, status: 0x%x\r\n",
                SLE_UART_SERVER_LOG, server_id, conn_id, status);
    if (data != NULL)
    {
        cJSON *message_array = cJSON_Parse((char *)data->value);
        if (message_array == NULL)
        {
            return;
        }
        if (cJSON_IsArray(message_array))
        {
            int size = cJSON_GetArraySize(message_array);
            for (int j = 0; j < size; j++)
            {
                cJSON *item = cJSON_GetArrayItem(message_array, j);
                if (item != NULL && cJSON_IsObject(item))
                {
                    cJSON *topic = cJSON_GetObjectItem(item, "topic");
                    uapi_watchdog_kick();
                    if (strcmp(topic->valuestring, "EngineControl_1") == 0)
                    {
                        osal_printk("Engine_1\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Angle");
                        sg90_angles[1] = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "EngineControl_2") == 0)
                    {
                        osal_printk("Engine_2\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Angle");
                        sg90_angles[2] = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "EngineControl_3") == 0)
                    {
                        osal_printk("Engine_3\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Angle");
                        sg90_angles[3] = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "SteeringControl") == 0)
                    {
                        osal_printk("Steering\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Start");
                        sg90_angles[0] = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "RGBControl") == 0)
                    {
                        osal_printk("RGB\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Mode");
                        led_mode = value->valueint;
                        cJSON *value_r = cJSON_GetObjectItem(item, "Red");
                        rgbs[0] = value_r->valueint;
                        cJSON *value_g = cJSON_GetObjectItem(item, "Green");
                        rgbs[1] = value_g->valueint;
                        cJSON *value_b = cJSON_GetObjectItem(item, "Blue");
                        rgbs[2] = value_b->valueint;
                    }
                    else if (strcmp(topic->valuestring, "BuzzControl") == 0)
                    {
                        osal_printk("Buzz\r\n");
                        cJSON *value = cJSON_GetObjectItem(item, "Start");
                        buzz_mode = value->valueint;
                    }
                    cJSON_free(topic);
                }
                cJSON_free(item);
            }
        }
        cJSON_Delete(message_array);
    }
    osal_printk("%s Received data (len=%d):", SLE_UART_SERVER_LOG, data->length);
    osal_printk("%s\r\n", data->value);
    osal_printk("End Recieve\r\n");
}

void *sg90_start(const char *arg)
{
    uint8_t gpio = *(uint8_t *)arg;
    while (sg90_ctrl == 1)
    {
        uint32_t high = (uint32_t)(500 + sg90_angles[gpio] * 11.11 + 0.5);
        uint32_t low = 20000 - high;
        uapi_gpio_set_val(gpio, GPIO_LEVEL_HIGH);
        osal_udelay(high); // 会阻塞CPU
        uapi_gpio_set_val(gpio, GPIO_LEVEL_LOW);
        osal_msleep(low / 1000 - 10); // 较长的延迟（大于10ms）使用msleep，防止线程阻塞，可能有10ms左右的额外开销
    }
    return NULL;
}

void *BH_AHT_start(const char *arg)
{
    unused(arg);
    while (1)
    {
        // 读取AHT20数据
        AHT20_ReadData(&temperature, &humidity);

        // 读取BH1750数据
        light = BH1750_ReadLight();

        // adc空气传感器
        uapi_adc_auto_scan_ch_enable(ADC_CHANNEL_3, adc_config, adc_callback);
        uapi_adc_auto_scan_ch_disable(ADC_CHANNEL_3);

        cJSON *message = cJSON_CreateArray();

        cJSON *ts_object = cJSON_CreateObject();
        cJSON_AddStringToObject(ts_object, "topic", "TemperatureSenser");
        cJSON_AddNumberToObject(ts_object, "temperature", temperature);
        cJSON_AddItemToArray(message, ts_object);

        cJSON *hs_object = cJSON_CreateObject();
        cJSON_AddStringToObject(hs_object, "topic", "HumiditySenser");
        cJSON_AddNumberToObject(hs_object, "humidity", humidity);
        cJSON_AddItemToArray(message, hs_object);

        cJSON *ls_object = cJSON_CreateObject();
        cJSON_AddStringToObject(ls_object, "topic", "LightSenser");
        cJSON_AddNumberToObject(ls_object, "light", light);
        cJSON_AddItemToArray(message, ls_object);

        cJSON *as_object = cJSON_CreateObject();
        cJSON_AddStringToObject(as_object, "topic", "AirSenser");
        cJSON_AddNumberToObject(as_object, "air", air);
        cJSON_AddItemToArray(message, as_object);

        char *message_str = cJSON_Print(message);

        osal_printk("%s length:%d\r\n", message_str, strlen(message_str + 1)); // 不支持直接输出浮点数

        // 星闪发送数据
        if (sle_uart_client_is_connected())
        {
            // 发送传感器数据给EPD client
            sle_uart_server_send_report_by_handle((uint8_t *)message_str, strlen(message_str));
        }
        cJSON_Delete(message);

        if (sg90_angles[0])
        {
            // 启动电机
            motor_forward();
        }
        else
        {
            motor_stop();
        }
        // 蜂鸣器
        if (buzz_mode)
        {
            uapi_gpio_toggle(BUZZ_PIN);
        }
        else
        {
            uapi_gpio_set_val(BUZZ_PIN, GPIO_LEVEL_HIGH);
        }
        (void)uapi_watchdog_kick(); // 必须进行喂狗，不然会重启
        osal_msleep(5000);
    }
    return NULL;
}

static void *ExBoard_task(const char *arg)
{
    unused(arg);
    osKernelStart();
    uint8_t sle_connect_state[] = "sle_dis_connect";
    uint8_t buffer[SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE] = {0};
    uint16_t length = SLE_UART_SERVER_MSG_QUEUE_MAX_SIZE;
    errcode_t ret;

    // 初始化看门狗
    ret = uapi_watchdog_init(TIME_OUT);
    if (ret == ERRCODE_INVALID_PARAM)
    {
        osal_printk("param is error, timeout is %d.\r\n", TIME_OUT);
        return NULL;
    }
    (void)uapi_watchdog_enable((wdt_mode_t)WDT_MODE);
    (void)uapi_register_watchdog_callback(watchdog_callback);
    osal_printk("init watchdog\r\n");

    // 初始化各种驱动
    OLED_Init();
    uapi_dma_init();
    uapi_dma_open();
    PWM_init();
    motor_gpio_init();
    AHT20_Init();
    BH1750_Init();
    spi_init();
    uapi_adc_init(ADC_CLOCK_500KHZ);
    uapi_adc_power_en(AFE_SCAN_MODE_MAX_NUM, true);
    uapi_gpio_register_isr_func(BUTTON_PIN, GPIO_INTERRUPT_DEDGE, gpio_callback);
    // 初始化蜂鸣器
    uapi_pin_set_mode(BUZZ_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(BUZZ_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(BUZZ_PIN, GPIO_LEVEL_HIGH);
    adc_config.type = 0;
    adc_config.freq = 2;

    // 初始化星闪
    sle_uart_server_init(sle_server_read_cbk, sle_server_write_cbk);

    // 初始化sg90任务参数
    sg90_ctrl = 1;
    sg90_angles[0] = 0;
    sg90_angles[1] = 10;
    sg90_angles[2] = 10;
    sg90_angles[3] = 10;

    // 挂载sg90线程
    osThreadAttr_t sg90_task = {
        .name = "SG90Task_1",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 0x1000,
        .priority = 15};
    uint8_t gpios[3] = {PWM_GPIO_1, PWM_GPIO_2, PWM_GPIO_3};
    for (int i = 0; i < 3; i++)
    {
        osThreadNew((osThreadFunc_t)sg90_start, &gpios[i], &sg90_task);
        if (i)
        {
            sg90_task.priority = 14;
            sg90_task.name = "SG90Task_3";
        }
        else
        {
            sg90_task.priority = 13;
            sg90_task.name = "SG90Task_2";
        }
        osal_printk("SG90Task create succ\r\n");
    }

    // 挂载传感器线程
    osThreadAttr_t BH_AHT_task = {
        .name = "BH_AHT_task",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 0x1000,
        .priority = 16};
    if (osThreadNew((osThreadFunc_t)BH_AHT_start, NULL, &BH_AHT_task) == NULL)
    {
        osal_printk("BH_AHT_task create fail\r\n");
    }
    else
    {
        osal_printk("BH_AHT_task create succ\r\n");
    }

    OLED_Fill(0, 0, 256, 128, 0x00);
    OLED_DrawSingleBMP(0, 0, 256, 128, ExBoard_pic, 0);
    while (1)
    {
        ws2812_next(led_mode, rgbs[0], rgbs[1], rgbs[2]);
        osal_msleep(1);
    }

    sle_uart_server_delete_msgqueue();
    return NULL;
}

static void ExBoard_entry(void)
{
    osThreadAttr_t attr;

    attr.name = "ExBoardTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = ExBoard_TASK_STACK_SIZE;
    attr.priority = ExBoard_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)ExBoard_task, NULL, &attr) == NULL)
    {
        /* Create task fail. */
    }
}

/* Run the ExBoard_entry. */
app_run(ExBoard_entry);