#ifndef PERSISTENT_WS_CLIENT_H
#define PERSISTENT_WS_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

    /* 初始化长连接（仅首次真正连接） */
    int ws_client_init(const char *server_ip, uint16_t port, const char *ws_path);

    /* 获取 socket */
    int ws_client_sock(void);

    /* 发送文本帧 */
    int ws_client_send_text(const char *text);

    /* 发送二进制帧，fin=1 为消息最后一帧 */
    int ws_client_send_binary(const uint8_t *data, size_t len, int fin);

    /* 发送续帧 (opcode=0x0) */
    int ws_client_send_cont(const uint8_t *data, size_t len, int fin);

    /* 简单轮询接收（可选） */
    void ws_client_poll(void);

    /* 关闭长连接 */
    void ws_client_close(void);

    /* 启动阻塞接收任务（线程） */
    void ws_client_start_recv_task(void);

    /* 注册消息回调，在接收到完整 WebSocket 帧后触发 */
    typedef void (*ws_client_msg_cb)(uint8_t opcode, const uint8_t *payload, size_t len);
    int ws_client_set_msg_callback(ws_client_msg_cb cb);

#ifdef __cplusplus
}
#endif

#endif /* PERSISTENT_WS_CLIENT_H */