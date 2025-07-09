#include "sle_uart_server.h"
#include "bits/alltypes.h"
#include "common_def.h"
#include "osal_debug.h"
#include "debugUtils.h"
#include "sle_uart_client.h"
#include "agent_module_main.h"
#include "cJSON.h"
#include "MQTTClient.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "osal_task.h"
#include "soc_osal.h"
#include "watchdog.h"

// MQTT 服务器地址及客户端标识，可根据实际情况修改
#define MQTT_ADDRESS "tcp://192.168.1.111:1883"
#define MQTT_CLIENT_ID "GateServiceClient"
#define MQTT_QOS 0
#define MQTT_RETAINED 1

static MQTTClient g_mqtt_client;
static bool g_mqtt_inited = false;
static bool g_lib_inited = false; /* Paho 库初始化标记 */

/* 记录已订阅主题列表，便于断线重连后重新订阅 */
#define MAX_SUB_TOPICS 8
static char *g_sub_topics[MAX_SUB_TOPICS] = {0};
static int g_sub_count = 0;

/* 初始化 MQTT（单例） */
int MqttInit(void)
{
    if (g_mqtt_inited)
    {
        return 0;
    }
    /* 第一次调用时先初始化 Paho 库(会创建互斥锁) */
    if (!g_lib_inited)
    {
        MQTTClient_init_options init_opts = MQTTClient_init_options_initializer;
        MQTTClient_global_init(&init_opts); /* 创建内部互斥锁等资源 */
        g_lib_inited = true;
    }

    int rc = MQTTClient_create(&g_mqtt_client, MQTT_ADDRESS, MQTT_CLIENT_ID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTT create failed: %d (%s)\r\n", rc, MQTTClient_strerror(rc));
        return rc;
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.struct_version = 6; /* 使 maxInflightMessages 字段生效 */
    conn_opts.keepAliveInterval = 120;
    conn_opts.cleansession = 1;
    /* 允许并发在途消息，避免 -4 错误 */
    conn_opts.reliable = 0;
    conn_opts.maxInflightMessages = 50; /* 进一步放大队列上限 */

    rc = MQTTClient_connect(g_mqtt_client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTT connect failed: %d (%s)\r\n", rc, MQTTClient_strerror(rc));
        return rc;
    }

    g_mqtt_inited = true;
    log_info("MQTT connected to %s\r\n", MQTT_ADDRESS);
    return 0;
}

/* 发布数据到指定 topic */
static void mqtt_try_reconnect(void);

static void MqttPublish(const char *topic, const char *payload)
{
    if ((topic == NULL) || (payload == NULL))
    {
        return;
    }

    if (!g_mqtt_inited && (MqttInit() != 0))
    {
        return;
    }

    int rc = MQTTClient_publish(g_mqtt_client, topic, (int)strlen(payload), (void *)payload, MQTT_QOS, MQTT_RETAINED, NULL);
    if (rc == MQTTCLIENT_DISCONNECTED)
    {
        log_error("MQTT publish disconnected, try reconnect\r\n");
        mqtt_try_reconnect();
        rc = MQTTClient_publish(g_mqtt_client, topic, (int)strlen(payload), (void *)payload, MQTT_QOS, MQTT_RETAINED, NULL);
    }
    if (rc != MQTTCLIENT_SUCCESS)
    {
        osal_printk("MQTT publish failed: %d\r\n", rc);
    }
}

/* 处理单个 Json 对象并发布 */
static void HandleJsonObject(cJSON *object)
{
    if (!cJSON_IsObject(object))
    {
        return;
    }

    cJSON *topicItem = cJSON_GetObjectItem(object, "topic");
    if (!cJSON_IsString(topicItem))
    {
        return; // topic 字段不存在或不是字符串
    }

    /* 为原有 topic 添加父主题 "devices" */
    const char *origTopic = topicItem->valuestring;
    const char *prefix = "devices/";
    /* 如果已经包含前缀，则不再重复添加 */
    bool hasPrefix = (strncmp(origTopic, prefix, strlen(prefix)) == 0);
    /* 特判：当 topic 为 "CautionStatus" 时，不添加前缀 */
    bool isCautionStatus = (strcmp(origTopic, "CautionStatus") == 0);

    const char *finalTopic = (hasPrefix || isCautionStatus) ? origTopic : NULL;
    char *topicBuf = NULL;
    if (!hasPrefix && !isCautionStatus)
    {
        size_t newLen = strlen(prefix) + strlen(origTopic) + 1; /* 末尾 NUL */
        topicBuf = (char *)malloc(newLen);
        if (topicBuf == NULL)
        {
            return; /* 内存不足，直接返回 */
        }
        snprintf(topicBuf, newLen, "%s%s", prefix, origTopic);
        finalTopic = topicBuf;
    }

    char *payload = cJSON_PrintUnformatted(object);
    if (payload == NULL)
    {
        if (topicBuf != NULL)
        {
            free(topicBuf);
        }
        return;
    }

    MqttPublish(finalTopic, payload);

    if (topicBuf != NULL)
    {
        free(topicBuf);
    }

    cJSON_free(payload);
}

/* 解析 JSON 字符串，支持数组或单对象 */
static void ParseAndPublishJson(const char *jsonStr)
{
    if (jsonStr == NULL)
    {
        return;
    }

    cJSON *root = cJSON_Parse(jsonStr);
    if (root == NULL)
    {
        osal_printk("cJSON parse error\r\n");
        return;
    }

    if (cJSON_IsArray(root))
    {
        int size = cJSON_GetArraySize(root);
        for (int i = 0; i < size; ++i)
        {
            cJSON *item = cJSON_GetArrayItem(root, i);
            if (item != NULL)
            {
                HandleJsonObject(item);
            }
        }
    }
    else if (cJSON_IsObject(root))
    {
        HandleJsonObject(root);
    }

    cJSON_Delete(root);
}

/* 解析 report 主题的 JSON，有效字段：weather(int), caution(int), time(double) */
static void process_report_payload(const char *json)
{
    if (json == NULL)
        return;

    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        log_error("[MQTT] parse report json fail\r\n");
        return;
    }

    cJSON *weatherItem = cJSON_GetObjectItem(root, "weather");
    cJSON *cautionItem = cJSON_GetObjectItem(root, "caution");
    cJSON *timeItem = cJSON_GetObjectItem(root, "time");

    if (!cJSON_IsNumber(weatherItem) || !cJSON_IsNumber(cautionItem) || !cJSON_IsNumber(timeItem))
    {
        log_error("[MQTT] report fields type error\r\n");
        cJSON_Delete(root);
        return;
    }

    int weather = weatherItem->valueint;
    int caution = cautionItem->valueint;
    double timeVal = timeItem->valuedouble;

    /* 创建 JsonArray */
    cJSON *rootArr = cJSON_CreateArray();
    if (rootArr == NULL)
    {
        return;
    }
    cJSON *weatherObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(weatherObj, "weather", weather);
    cJSON_AddStringToObject(weatherObj, "topic", "WeatherReport");
    cJSON_AddItemToArray(rootArr, weatherObj);

    cJSON *cautionObj = cJSON_CreateObject();
    cJSON_AddStringToObject(cautionObj, "topic", "CautionReport");
    cJSON_AddNumberToObject(cautionObj, "caution", caution);
    cJSON_AddItemToArray(rootArr, cautionObj);

    cJSON *timeObj = cJSON_CreateObject();
    cJSON_AddStringToObject(timeObj, "topic", "TimeReport");
    cJSON_AddNumberToObject(timeObj, "time", timeVal);
    cJSON_AddItemToArray(rootArr, timeObj);

    char *retStr = cJSON_PrintUnformatted(rootArr);
    log_debug("payload:%s", retStr);
    sle_uart_client_send_data((uint8_t *)retStr, strlen(retStr), 0, get_conn_id('P'));
    cJSON_Delete(rootArr);
    cJSON_Delete(root);
}

// 接收信息的回调函数
void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                              errcode_t status)
{
    unused(client_id);
    unused(status);
    if (get_conn_id('E') == conn_id)
    {
        log_info("data?%s", data->data);
        /* 解析 JSON 并通过 MQTT 发布 */
        const char *jsonPayload = (const char *)(data->data);
        ParseAndPublishJson(jsonPayload);
        sle_uart_client_send_data(data->data, data->data_len, 0, get_conn_id('P'));
    }
}

void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                            errcode_t status)
{
    unused(client_id);
    unused(conn_id);
    unused(status);
}

/* ------------------------ MQTT 阻塞式订阅实现 ------------------------- */

#define MQTT_SUB_TASK_STACK_SIZE 0x2000
#define MQTT_SUB_TASK_NAME "MqttSubTask"
#define MQTT_SUB_TASK_PRIO OSAL_TASK_PRIORITY_LOW

/* MQTT 订阅线程 */
static int mqtt_sub_task(void *arg)
{
    (void)arg; /* 线程启动后不再需要额外参数 */

    /* 确保 MQTT 已连接 */
    if (!g_mqtt_inited && (MqttInit() != 0))
    {
        return -1;
    }

    while (1)
    {
        /* 喂狗防止系统复位 */
        uapi_watchdog_kick();

        char *recv_topic = NULL;
        int topic_len = 0;
        MQTTClient_message *msg = NULL;

        /* 阻塞等待消息，超时 1 秒，方便循环中维持喂狗 */
        int rc = MQTTClient_receive(g_mqtt_client, &recv_topic, &topic_len, &msg, 1000);
        if (rc == MQTTCLIENT_SUCCESS && msg != NULL)
        {
            if (msg->payloadlen > 0 && msg->payload != NULL)
            {
                /* 确保字符串结尾 */
                int copy_len = msg->payloadlen;
                if (copy_len >= 1024)
                    copy_len = 1023;
                char buf[1024];
                memcpy(buf, msg->payload, copy_len);
                buf[copy_len] = '\0';

                /* 如果主题是 report，则解析字段 */
                if (recv_topic && (strcmp(recv_topic, "report") == 0 || strcmp(recv_topic, "devices/report") == 0))
                {
                    process_report_payload(buf);
                }
                else if (recv_topic && (recv_topic[0] == 'C' && recv_topic[1] == 'o'))
                {
                    /* 在字符串两端添加 '[' 和 ']' 再发送，避免大数组占用线程栈 */
                    uint16_t send_len = (uint16_t)(copy_len + 2); /* 不含结尾 NUL */
                    uint8_t *send_buf = (uint8_t *)osal_vmalloc(send_len);
                    if (send_buf != NULL)
                    {
                        send_buf[0] = '[';
                        memcpy(send_buf + 1, buf, copy_len);
                        send_buf[copy_len + 1] = ']';

                        sle_uart_client_send_data(send_buf, send_len, 0, get_conn_id('E'));

                        osal_vfree(send_buf);
                    }
                }
                else
                {
                    log_info("MQTT payload: %s\r\n", buf);
                }
            }
            MQTTClient_freeMessage(&msg);
            MQTTClient_free(recv_topic);
        }
        else if (rc == MQTTCLIENT_DISCONNECTED)
        {
            /* 非超时错误，打印日志 */
            log_error("MQTT disconnected, try reconnect\r\n");
            mqtt_try_reconnect();
            osal_msleep(500);
        }
    }

    /* 不会执行到此处 */
    return 0;
}

/* 仅做订阅，不会创建接收线程（需保证在任何线程调用网络 API 前）*/
int MqttPreSubscribe(const char *topic)
{
    if (topic == NULL)
        return -1;

    /* 确保连接 */
    if (!g_mqtt_inited && (MqttInit() != 0))
        return -1;

    int rc_sub = MQTTClient_subscribe(g_mqtt_client, topic, MQTT_QOS);
    if (rc_sub == MQTTCLIENT_SUCCESS)
    {
        /* 记录 topic */
        int found = 0;
        for (int i = 0; i < g_sub_count; ++i)
        {
            if (strcmp(g_sub_topics[i], topic) == 0)
            {
                found = 1;
                break;
            }
        }
        if (!found && g_sub_count < MAX_SUB_TOPICS)
        {
            g_sub_topics[g_sub_count++] = strdup(topic);
        }
        log_info("MQTT pre-subscribed to %s\r\n", topic);
    }
    else
    {
        log_error("MQTT pre-subscribe failed: %d (%s)\r\n", rc_sub, MQTTClient_strerror(rc_sub));
    }
    return rc_sub;
}

/* 启动接收线程，仅创建一次 */
int MqttStartRecvTask(void)
{
    static int started = 0;
    if (started)
        return 0;
    started = 1;

    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create(mqtt_sub_task, NULL, MQTT_SUB_TASK_NAME, MQTT_SUB_TASK_STACK_SIZE);
    if (task_handle != NULL)
    {
        osal_kthread_set_priority(task_handle, MQTT_SUB_TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
    return 0;
}

/* 重新连接并重新订阅（供内部使用）*/
static void mqtt_try_reconnect(void)
{
    if (MQTTClient_isConnected(g_mqtt_client))
        return;

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.struct_version = 6;
    conn_opts.keepAliveInterval = 120;
    conn_opts.cleansession = 1;
    conn_opts.reliable = 0;
    conn_opts.maxInflightMessages = 50;

    for (int retry = 0; retry < 5; ++retry)
    {
        int rc = MQTTClient_connect(g_mqtt_client, &conn_opts);
        if (rc == MQTTCLIENT_SUCCESS)
        {
            log_info("MQTT reconnect ok\r\n");
            /* 重新订阅 */
            for (int i = 0; i < g_sub_count; ++i)
            {
                MQTTClient_subscribe(g_mqtt_client, g_sub_topics[i], MQTT_QOS);
            }
            return;
        }
        log_error("MQTT reconnect failed rc=%d\r\n", rc);
        osal_msleep(1000);
    }
}