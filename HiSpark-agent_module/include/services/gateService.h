#include "sle_ssap_client.h"
// 数据接收回调函数
void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                            errcode_t status);

// 接收数据后的回调函数
void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data,
                              errcode_t status);
int MqttInit(void);
int MqttStartSubscribeTask(const char *topic);

/* 预订阅主题，在接收线程启动前调用 */
int MqttPreSubscribe(const char *topic);

/* 启动阻塞接收线程（只调用一次）*/
int MqttStartRecvTask(void);