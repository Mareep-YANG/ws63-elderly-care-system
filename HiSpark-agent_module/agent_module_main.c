#include "common_def.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "agent_module_main.h"
#include "wifiService.h"
#include "websocketService.h"
#include "soundService.h"
#include "agentMainService.h"
#include "stdbool.h"
#include "debugUtils.h"
#include "my_ssd1306_fonts.h"
#include "my_ssd1306.h"
#include "oledService.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "i2c.h"
#define I2C_TASK_STACK_SIZE 0x1000
#define I2C_TASK_PRIO ((osPriority_t)22)
bool isWifiConneted = false;
static void module_entry(void)
{

    osThreadAttr_t attr;
    attr.name = "sta_sample_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = WIFI_TASK_STACK_SIZE;
    attr.priority = 28;
    if (osThreadNew((osThreadFunc_t)sta_sample_init, NULL, &attr) == NULL)
    {
        log_info("sta_sample_init failed");
    } // 创建连WIFI进程
    attr.name = "sound_test_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 0x10000;
    attr.priority = 20;
    if (osThreadNew((osThreadFunc_t)agent_main_task, NULL, &attr) == NULL)
    {
        log_info("sound_test failed");
    } // 创建主任务
    osThreadAttr_t attr1 = {0};
    attr1.name = "OledTask";
    attr1.stack_size = I2C_TASK_STACK_SIZE;
    attr1.priority = I2C_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)OledTask, NULL, &attr1) == NULL)
    {
        log_error("oled tesk init falled");
        // 创建任务失败
    }
}

/* Run the tasks_test_entry. */
app_run(module_entry);

/*---------------------------------------------------------------------------
 * Mapping between server board char ('P'/'E') and corresponding conn_id
 *---------------------------------------------------------------------------*/
server_conn_t g_server_conn_map[SERVER_CONN_MAX] = {
    {'P', 0xFFFF},
    {'E', 0xFFFF},
};

void set_conn_id(char server_type, uint16_t conn_id)
{
    for (uint32_t i = 0; i < SERVER_CONN_MAX; i++)
    {
        if (g_server_conn_map[i].server_type == server_type)
        {
            g_server_conn_map[i].conn_id = conn_id;
            return;
        }
    }
    /* If not found (unexpected), overwrite first entry */
    g_server_conn_map[0].server_type = server_type;
    g_server_conn_map[0].conn_id = conn_id;
}

uint16_t get_conn_id(char server_type)
{
    for (uint32_t i = 0; i < SERVER_CONN_MAX; i++)
    {
        if (g_server_conn_map[i].server_type == server_type)
        {
            return g_server_conn_map[i].conn_id;
        }
    }
    return 0xFFFF; /* Invalid */
}
