#include "agentMainService.h"
#include "soundService.h"
#include "stddef.h"
#include "osal_debug.h"
#include "wavRecorderService.h"
#include "soc_osal.h"
#include "stdbool.h"
#include "persistentWsClient.h"
#include "gateService.h"
#include "bits/alltypes.h"
#include "sle_uart_server.h"
#include "stdbool.h"
#include "stdbool.h"
#include "debugUtils.h"
#include "watchdog.h"
#include "keyService.h"
#include "sle_uart_client.h"
// 外部全局变量，标记 WiFi 连接状态
extern bool isWifiConneted;

void *agent_main_task(const char *arg)
{
    log_info("Agent main task started");
    // 注册GPIO按键中断
    KeyInit();
    // 等待 WiFi 连接完成
    while (!isWifiConneted)
    {
        log_info("Waiting for WiFi connection...");
        osal_msleep(1000); // 等待 1 秒后重新检查
    }
    if (MqttInit() != 0)
    {
        log_error("MQTT init failed");
    }

    // 初始化星闪client
    sle_uart_client_init(sle_uart_notification_cb, sle_uart_indication_cb);
    MqttPreSubscribe("report");
    MqttPreSubscribe("devices/#");
    MqttPreSubscribe("Control/#");
    MqttStartRecvTask();
    log_info("WiFi connected, starting WebSocket long connection");
    if (ws_client_init("192.168.1.111", 8000, "/ws") != 0)
    {
        log_error("WebSocket long connection failed\r\n");
        return NULL;
    }
    ws_client_start_recv_task();
    while (true)
    {
        osal_msleep(200);
        uapi_watchdog_kick();

        if (KeyConsumeEvent())
        {
            log_info("Key event consumed, start record");
            /* 按住录音，松开发送 EOF */
            if (wav_record_hold_and_stream(48000, NULL, 0, NULL, 8) == -2)
            {
                log_error("ws long connection not init");
            }
        }
    }

    return NULL;
}