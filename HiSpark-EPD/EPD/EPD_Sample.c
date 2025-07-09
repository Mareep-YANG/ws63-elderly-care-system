// 中控面板电子纸主程序，作为hisoc服务端，连接两个服务端Ex Server和AI server
// 承担星闪client端

#include "pinctrl.h"
#include "soc_osal.h"
#include "gpio.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "los_sys.h"
#include "string.h"
#include "stdlib.h"
#include "sle_low_latency.h"
#include "common_def.h"
#include "sle_connection_manager.h"
#include "cJSON.h"
#include "watchdog.h"
// private
#include "sle_uart_server.h"
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"

// WatchDog相关参数
#define TIME_OUT 2
#define WDT_MODE 1

#define EPD_TASK_STACK_SIZE 0x8000
#define EPD_TASK_PRIO (osPriority_t)(17)

// 占位数据
uint16_t sg90_angles[4] = {0};
uint8_t led_mode = 0;
uint8_t buzz_mode = 0;

// 传感器数据，来自Ex server
double temperature = 0.0;
double humidity = 0.0;
uint16_t light = 0;
double air = 0.0;
// 信息、天气、警告
uint8_t weather = 0; // 用于判断当前天气显示
uint8_t caution = 0; // 用于判断当前警告信息显示
double time = 12.58; // AI发来的时间信息
// 转发用数组
uint8_t ImageBW[27200];
uint8_t ImageR[27200];
char temperature_string[30] = "";
char humidity_string[30] = "";
char light_string[30] = "";
char air_string[30] = "";

static errcode_t watchdog_callback(uintptr_t param)
{
    UNUSED(param);
    osal_printk("watchdog kick timeout!\r\n");
    return ERRCODE_SUCC;
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
        uapi_watchdog_kick();
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
                    cJSON *value = NULL;
                    uapi_watchdog_kick();
                    if (strcmp(topic->valuestring, "TemperatureSenser") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "temperature");
                        temperature = value->valuedouble;
                    }
                    else if (strcmp(topic->valuestring, "HumiditySenser") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "humidity");
                        humidity = value->valuedouble;
                    }
                    else if (strcmp(topic->valuestring, "LightSenser") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "light");
                        light = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "AirSenser") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "air");
                        air = value->valuedouble;
                    }
                    else if (strcmp(topic->valuestring, "WeatherReport") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "weather");
                        weather = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "CautionReport") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "caution");
                        caution = value->valueint;
                    }
                    else if (strcmp(topic->valuestring, "TimeReport") == 0)
                    {
                        value = cJSON_GetObjectItem(item, "time");
                        time = value->valuedouble;
                    }
                    uapi_watchdog_kick();
                    cJSON_free(topic);
                    cJSON_free(value);
                }
                cJSON_free(item);
            }
        }

        osal_printk("%s Received data (len=%d):", SLE_UART_SERVER_LOG, data->length);
        osal_printk("%s\r\n", data->value);
        cJSON_free(message_array);
        // cJSON_Delete(message_array);
    }
    uapi_watchdog_kick();
}

void init_gpio(void)
{
    // LED
    uapi_pin_set_mode(2, PIN_MODE_0);
    uapi_gpio_set_dir(2, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(2, GPIO_LEVEL_LOW);
    // SDA
    uapi_pin_set_mode(9, 0);
    uapi_gpio_set_dir(9, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(9, GPIO_LEVEL_HIGH);
    // SCL
    uapi_pin_set_mode(7, 0);
    uapi_gpio_set_dir(7, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(7, GPIO_LEVEL_HIGH);
    // CS
    uapi_pin_set_mode(8, 0);
    uapi_gpio_set_dir(8, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(8, GPIO_LEVEL_HIGH);

    // BUSY引脚初始化
    uapi_pin_set_mode(13, 0);
    uapi_pin_set_pull(13, PIN_PULL_TYPE_UP);
    uapi_gpio_set_dir(13, GPIO_DIRECTION_INPUT);
    // DC引脚初始化
    uapi_pin_set_mode(10, 0);
    uapi_gpio_set_dir(10, GPIO_DIRECTION_OUTPUT);
    // CS = High (not selected)
    uapi_gpio_set_val(8, GPIO_LEVEL_HIGH);

    // Reset the EPD
    uapi_pin_set_mode(14, 0);
    uapi_gpio_set_dir(14, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(14, GPIO_LEVEL_LOW);
    osal_msleep(100);
    uapi_gpio_set_val(14, GPIO_LEVEL_HIGH);
    osal_msleep(100);
}

void draw_main_scene(void)
{
    // 显示文字的y坐标好像不能太小，原因未知
    // y坐标向下递增，x坐标向右递增
    uapi_watchdog_kick();
    Paint_SelectImage(ImageBW);
    Paint_Clear(WHITE);
    EPD_ShowPicture(0, 0, 800, 272, UI_pic, BLACK);       // 显示背景图片
    EPD_ShowWatch(270, 20, (float)time, 4, 2, 48, BLACK); // 显示当前时间
    // 显示天气
    if (weather == 0)
    {
        // 统一为64宽度，50高度，坐标450，20
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_cloudy, BLACK);
    }
    else if (weather == 1)
    {
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_snowy, BLACK);
    }
    else if (weather == 2)
    {
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_rainy, BLACK);
    }
    else if (weather == 3)
    {
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_sunny, BLACK);
    }
    else if (weather == 4)
    {
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_overcast, BLACK);
    }
    else if (weather == 5)
    {
        EPD_ShowPicture(450, 20, 64, 50, gImage_weather_thunder, BLACK);
    }

    Paint_SelectImage(ImageR);
    Paint_Clear(BLACK);

    strcpy(temperature_string, "");
    strcpy(humidity_string, "");
    strcpy(light_string, "");
    strcpy(air_string, "");

    snprintf(temperature_string, 30, "%.1lf", temperature);
    snprintf(humidity_string, 30, "%.1lf", humidity);
    snprintf(light_string, 30, "%d", light);
    snprintf(air_string, 30, "%.1lf", air);

    EPD_ShowString(125, 105, (uint8_t *)temperature_string, 48, WHITE);
    EPD_ShowString(125, 205, (uint8_t *)humidity_string, 48, WHITE);
    EPD_ShowString(410, 205, (uint8_t *)light_string, 48, WHITE);
    EPD_ShowString(615, 205, (uint8_t *)air_string, 48, WHITE);

    // 显示警告信息
    if (caution == 0)
    {
        // 统一为280宽度，85高度，坐标400，85
        EPD_ShowPicture(410, 85, 280, 85, gImage_warning_common, WHITE);
    }

    EPD_Display(ImageBW, ImageR);
}

void *EPD_Update_Task(const char *arg)
{
    // 清屏
    osal_printk("Init EPD Start.\r\n");
    osal_msleep(100);
    uapi_watchdog_kick();
    EPD_Init();

    while (1)
    {
        draw_main_scene();
        uapi_watchdog_kick();
        EPD_Update();

        for (uint8_t i = 0; i < 41; i++) // 其他工作的执行时间大概18.6s
        {
            osal_msleep(1000);
            uapi_gpio_toggle(2);
            uapi_watchdog_kick();
        }
        osal_msleep(400); // 三分钟刷新一次，测试时1分钟一次
    }
}

static void *EPD_task(const char *arg)
{
    unused(arg);

    // 初始化看门狗
    errcode_t ret = uapi_watchdog_init(TIME_OUT);
    if (ret == ERRCODE_INVALID_PARAM)
    {
        osal_printk("param is error, timeout is %d.\r\n", TIME_OUT);
        return NULL;
    }
    (void)uapi_watchdog_enable((wdt_mode_t)WDT_MODE);
    (void)uapi_register_watchdog_callback(watchdog_callback);

    // 初始化星闪
    sle_uart_server_init(sle_server_read_cbk, sle_server_write_cbk);

    // 初始化引脚
    init_gpio();

    // 创建画布，其中BW是黑白，R是红色
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    Paint_Clear(WHITE);
    Paint_NewImage(ImageR, EPD_W, EPD_H, 0, WHITE);
    Paint_Clear(WHITE);
    uapi_watchdog_kick();

    osThreadAttr_t EPD_Update_task = {
        .name = "EPD_Update_task",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 0x8000,
        .priority = 20};
    if (osThreadNew((osThreadFunc_t)EPD_Update_Task, NULL, &EPD_Update_task) == NULL)
    {
        osal_printk("EPD_Update_task create fail\r\n");
    }
    else
    {
        osal_printk("EPD_Update_task create succ\r\n");
    }

    // 初始化大概21s

    while (1)
    {
        osal_printk("time:%lf\r\n", time);
        osal_msleep(1000); // 三分钟刷新一次，测试时1分钟一次
    }

    return NULL;
}

static void EPD_entry(void)
{
    osThreadAttr_t attr;

    attr.name = "EPDTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = EPD_TASK_STACK_SIZE;
    attr.priority = EPD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)EPD_task, NULL, &attr) == NULL)
    {
        /* Create task fail. */
    }
}

app_run(EPD_entry);