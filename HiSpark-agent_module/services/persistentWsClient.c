#include "persistentWsClient.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include "osal_debug.h"
#include "common_def.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "debugUtils.h"
#include "osal_task.h"
#include "soc_osal.h"
#include "wsAudioPlayer.h"
#include "watchdog.h"
#include "systick.h"
#include <errno.h>
/* GUID 常量 */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* 前向声明 */
static void handle_text_frame(const char *msg);

static int g_ws_sock = -1;
static char g_ip[16] = {0};
static uint16_t g_port = 0;
static char g_path[64] = {0};

/* ---------------------------------------------------------
 * 工具函数：生成 Sec-WebSocket-Key / Accept
 * ---------------------------------------------------------*/
static void ws_generate_key(char *out_b64, size_t out_len)
{
    uint8_t rand_key[16];
    for (size_t i = 0; i < sizeof(rand_key); i++)
    {
        rand_key[i] = (uint8_t)(rand() & 0xFF);
    }
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)out_b64, out_len, &olen,
                          rand_key, sizeof(rand_key));
    out_b64[olen] = '\0';
}

static void ws_calc_accept(const char *key, char *out_b64, size_t out_len)
{
    uint8_t sha1_out[20];
    char composite[64];
    snprintf(composite, sizeof(composite), "%s" WS_GUID, key);
    mbedtls_sha1((const unsigned char *)composite, strlen(composite), sha1_out);

    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)out_b64, out_len, &olen,
                          sha1_out, sizeof(sha1_out));
    out_b64[olen] = '\0';
}

/* ---------------------------------------------------------
 * WebSocket 帧发送（Mask 版，客户端必须 Mask）
 * ---------------------------------------------------------*/
static uint64_t htobe64_u64(uint64_t host_64)
{
    uint32_t high = (uint32_t)(host_64 >> 32);
    uint32_t low = (uint32_t)(host_64 & 0xFFFFFFFFULL);
    high = htonl(high);
    low = htonl(low);
    return (((uint64_t)low) << 32) | high;
}

static int ws_send_text_frame_int(int sock, const char *text)
{
    if (sock < 0)
        return -1;

    size_t payload_len = strlen(text);
    size_t header_len = 2;
    uint8_t hdr[14];
    log_debug("text debug 1\n");
    hdr[0] = 0x81; /* FIN=1, opcode=0x1 (text) */

    if (payload_len <= 125)
    {
        hdr[1] = 0x80 | (uint8_t)payload_len;
        log_debug("text debug 2\n");
    }
    else if (payload_len <= 0xFFFF)
    {
        hdr[1] = 0x80 | 126;
        uint16_t len16 = htons((uint16_t)payload_len);
        memcpy(&hdr[2], &len16, 2);
        header_len += 2;
        log_debug("text debug 3\n");
    }
    else
    {
        hdr[1] = 0x80 | 127;
        uint64_t len64 = htobe64_u64(payload_len);
        memcpy(&hdr[2], &len64, 8);
        header_len += 8;
        log_debug("text debug 4\n");
    }

    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    memcpy(&hdr[header_len], mask, 4);
    header_len += 4;
    log_debug("text debug 5\n");
    /* 先发送头部 */
    if (send(sock, hdr, header_len, 0) < 0)
        return -1;
    log_debug("text debug 6\n");
    /* 将整个 payload 先做 Mask，再一次性发送，避免逐字节 send() 失败 */
    uint8_t *masked = (uint8_t *)osal_kmalloc(payload_len, OSAL_GFP_KERNEL);
    log_debug("text debug 7\n");
    if (masked == NULL)
        return -1;
    log_debug("text debug 8\n");
    for (size_t i = 0; i < payload_len; ++i)
    {
        masked[i] = ((const uint8_t *)text)[i] ^ mask[i % 4];
    }
    log_debug("text debug 9\n");
    int ret = send(sock, masked, (int)payload_len, 0);
    log_debug("text debug 10\n");
    osal_kfree(masked);
    return (ret < 0) ? -1 : 0;
}

/* 优化：一次发送 256B 块，显著减少 send 调用次数，加快推流速度 */
#define WS_SEND_CHUNK 256
static int ws_send_binary_frame_int(int sock, const uint8_t *data, size_t data_len, int fin, uint8_t opcode)
{
    if (sock < 0)
        return -1;

    /* 1. 构建并发送头部（含 Mask）*/
    size_t header_len = 2;
    uint8_t hdr[14];

    hdr[0] = (uint8_t)(opcode | (fin ? 0x80 : 0));

    if (data_len <= 125)
    {
        hdr[1] = 0x80 | (uint8_t)data_len;
    }
    else if (data_len <= 0xFFFF)
    {
        hdr[1] = 0x80 | 126;
        uint16_t len16 = htons((uint16_t)data_len);
        memcpy(&hdr[2], &len16, 2);
        header_len += 2;
    }
    else
    {
        hdr[1] = 0x80 | 127;
        uint64_t len64 = htobe64_u64(data_len);
        memcpy(&hdr[2], &len64, 8);
        header_len += 8;
    }

    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    memcpy(&hdr[header_len], mask, 4);
    header_len += 4;

    if (send(sock, hdr, header_len, 0) < 0)
        return -1;

    /* 2. 分块 Mask+发送，chunk 大小 256 字节 */
    uint8_t chunk_buf[WS_SEND_CHUNK];
    size_t offset = 0;
    while (offset < data_len)
    {
        size_t chunk = data_len - offset;
        if (chunk > WS_SEND_CHUNK)
            chunk = WS_SEND_CHUNK;

        for (size_t i = 0; i < chunk; ++i)
        {
            chunk_buf[i] = data[offset + i] ^ mask[(offset + i) % 4];
        }
        if (send(sock, chunk_buf, (int)chunk, 0) < 0)
            return -1;
        offset += chunk;
    }
    return 0;
}

static int ws_send_close_frame_int(int sock)
{
    if (sock < 0)
        return -1;

    uint8_t hdr[6];
    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    hdr[0] = 0x88;        /* Close */
    hdr[1] = 0x80 | 0x00; /* mask + len 0 */
    memcpy(&hdr[2], mask, 4);

    return send(sock, hdr, sizeof(hdr), 0) >= 0 ? 0 : -1;
}

/* ---------------------------------------------------------
 * 对外 API
 * ---------------------------------------------------------*/
int ws_client_init(const char *server_ip, uint16_t port, const char *ws_path)
{
    if (g_ws_sock >= 0)
        return 0; /* 已连接 */

    if (!server_ip || !ws_path || port == 0)
        return -1;

    srand((unsigned int)time(NULL));

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        log_error("[WS-P] socket fail\r\n");
        return -1;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    srv.sin_addr.s_addr = inet_addr(server_ip);
    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0)
    {
        log_error("[WS-P] connect fail\r\n");
        lwip_close(sock);
        return -1;
    }

    char ws_key[32];
    ws_generate_key(ws_key, sizeof(ws_key));

    char http_req[256];
    int req_len = snprintf(http_req, sizeof(http_req),
                           "GET %s HTTP/1.1\r\n"
                           "Host: %s:%d\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Key: %s\r\n"
                           "Sec-WebSocket-Version: 13\r\n\r\n",
                           ws_path, server_ip, port, ws_key);

    if (send(sock, http_req, req_len, 0) < 0)
    {
        lwip_close(sock);
        return -1;
    }

    char resp[512] = {0};
    int rcv = recv(sock, resp, sizeof(resp) - 1, 0);
    if (rcv <= 0 || strstr(resp, " 101 ") == NULL)
    {
        log_error("[WS-P] handshake HTTP fail\r\n");
        lwip_close(sock);
        return -1;
    }

    char accept_expected[40];
    ws_calc_accept(ws_key, accept_expected, sizeof(accept_expected));
    if (strstr(resp, accept_expected) == NULL)
    {
        log_error("[WS-P] accept mismatch\r\n");
        lwip_close(sock);
        return -1;
    }

    /* 保存连接信息 */
    g_ws_sock = sock;
    strncpy(g_ip, server_ip, sizeof(g_ip) - 1);
    g_port = port;
    strncpy(g_path, ws_path, sizeof(g_path) - 1);

    log_info("[WS-P] connected %s:%d%s OK\r\n", server_ip, port, ws_path);
    return 0;
}

int ws_client_sock(void)
{
    return g_ws_sock;
}

int ws_client_send_text(const char *text)
{
    return ws_send_text_frame_int(g_ws_sock, text);
}

int ws_client_send_binary(const uint8_t *data, size_t len, int fin)
{
    return ws_send_binary_frame_int(g_ws_sock, data, len, fin, 0x02);
}

int ws_client_send_cont(const uint8_t *data, size_t len, int fin)
{
    return ws_send_binary_frame_int(g_ws_sock, data, len, fin, 0x00);
}

void ws_client_close(void)
{
    if (g_ws_sock < 0)
        return;

    /* 按协议优雅关闭 */
    ws_send_text_frame_int(g_ws_sock, "BYE");
    ws_send_close_frame_int(g_ws_sock);
    lwip_close(g_ws_sock);
    g_ws_sock = -1;
    ws_audio_player_reset();
}

/**************************** 新增: WebSocket 阻塞接收任务 ****************************/

#define WS_RECV_TASK_STACK_SIZE 0x1000
#define WS_RECV_TASK_NAME "WsRecvTask"
#define WS_RECV_TASK_PRIO OSAL_TASK_PRIORITY_LOW

typedef void (*ws_client_msg_cb)(uint8_t opcode, const uint8_t *payload, size_t len);

static ws_client_msg_cb g_ws_msg_cb = NULL;

/* STREAMING_PROTOCOL 状态 */
static char g_stream_id[12] = {0};
static int g_stream_active = 0;

/* 设置消息回调，可由上层注册 */
int ws_client_set_msg_callback(ws_client_msg_cb cb)
{
    g_ws_msg_cb = cb;
    return 0;
}

/* 阻塞接收指定长度数据 */
static int ws_recv_full(int sock, uint8_t *buf, size_t need)
{
    size_t off = 0;
    while (off < need)
    {
        int r = recv(sock, buf + off, (int)(need - off), 0);
        if (r <= 0)
        {
            int err = errno;
            if (r == 0)
            {
                log_info("[WS-P] peer closed socket (FIN) errno=%d\r\n", err);
            }
            else
            {
                log_error("[WS-P] recv failed r=%d errno=%d\r\n", r, err);
            }
            return -1;
        }
        off += (size_t)r;
    }
    return 0;
}

/* 解析一帧 WebSocket 数据，阻塞直到收到完整帧 */
static int ws_recv_frame_int(int sock, uint8_t **out_buf, size_t *buf_len,
                             uint8_t *out_opcode)
{
    if (sock < 0)
        return -1;

    uint8_t hdr[2];
    if (ws_recv_full(sock, hdr, 2) < 0)
        return -1;

    uint8_t byte0 = hdr[0];
    uint8_t byte1 = hdr[1];
    uint8_t opcode = byte0 & 0x0F;
    uint8_t mask_flag = byte1 & 0x80;
    uint64_t payload_len = byte1 & 0x7F;

    if (payload_len == 126)
    {
        uint8_t ext[2];
        if (ws_recv_full(sock, ext, 2) < 0)
            return -1;
        payload_len = ((uint16_t)ext[0] << 8) | ext[1];
    }
    else if (payload_len == 127)
    {
        uint8_t ext[8];
        if (ws_recv_full(sock, ext, 8) < 0)
            return -1;
        payload_len = ((uint64_t)ext[0] << 56) | ((uint64_t)ext[1] << 48) |
                      ((uint64_t)ext[2] << 40) | ((uint64_t)ext[3] << 32) |
                      ((uint64_t)ext[4] << 24) | ((uint64_t)ext[5] << 16) |
                      ((uint64_t)ext[6] << 8) | ((uint64_t)ext[7]);
    }

    uint8_t masking_key[4] = {0};
    if (mask_flag)
    {
        if (ws_recv_full(sock, masking_key, 4) < 0)
            return -1;
    }

    uint8_t *payload_buf = (uint8_t *)osal_kmalloc((unsigned long)payload_len, OSAL_GFP_KERNEL);
    if (payload_buf == NULL)
    {
        log_error("[WS-P] OOM when alloc %u bytes\r\n", (unsigned)payload_len);
        return -1;
    }

    if (ws_recv_full(sock, payload_buf, (size_t)payload_len) < 0)
    {
        osal_kfree(payload_buf);
        return -1;
    }

    if (mask_flag)
    {
        for (uint64_t i = 0; i < payload_len; ++i)
        {
            payload_buf[i] ^= masking_key[i % 4];
        }
    }

    *out_buf = payload_buf;
    *buf_len = (size_t)payload_len;
    if (out_opcode)
        *out_opcode = opcode;

    return 0; /* 成功 */
}

/* WebSocket 接收线程 */
static int ws_recv_task(void *arg)
{
    (void)arg;
    uint8_t *payload = NULL;
    size_t len;
    uint8_t opcode;

    while (1)
    {
        uapi_watchdog_kick();
        if (g_ws_sock < 0)
        {
            /*
             * Auto-reconnect logic
             * --------------------------------------------------
             * 当检测到当前 socket 已关闭 (g_ws_sock < 0) 时，且之前调用
             * ws_client_init 保存过服务器信息 (g_ip/g_port/g_path 不为空)，
             * 周期性地尝试重新建立 WebSocket 连接。
             */
            if (g_ip[0] != '\0' && g_port != 0 && g_path[0] != '\0')
            {
                log_info("[WS-P] try reconnect %s:%u%s\r\n", g_ip, g_port, g_path);
                if (ws_client_init(g_ip, g_port, g_path) == 0)
                {
                    /* reconnect success */
                    log_info("[WS-P] reconnect success\r\n");
                    /* 重新开始下一轮循环，由新连接继续后续收发 */
                    continue;
                }
            }
            /* 若未保存连接信息或重连失败，稍作等待后再尝试 */
            osal_msleep(1000);
            continue;
        }

        if (ws_recv_frame_int(g_ws_sock, &payload, &len, &opcode) == 0)
        {
            uapi_watchdog_kick();
            if (opcode == 0x1) /* 文本帧 */
            {
                /* 确保以\0结尾 */
                char *text = (char *)payload;
                if (len < 1024)
                    text[len] = '\0';
                handle_text_frame(text);
            }
            else if (opcode == 0x2 && g_stream_active)
            {
                ws_audio_player_feed_pcm(payload, len);
            }
            else if (opcode == 0x8) /* Close */
            {
                uint16_t code = 1000; /* default */
                char reason[64] = {0};
                if (len >= 2)
                {
                    code = ((uint16_t)payload[0] << 8) | payload[1];
                }
                if (len > 2)
                {
                    size_t rlen = len - 2;
                    if (rlen >= sizeof(reason))
                    {
                        rlen = sizeof(reason) - 1;
                    }
                    memcpy(reason, &payload[2], rlen);
                }
                log_info("[WS-P] CLOSE code=%u reason=%s\r\n", code, reason);
            }
            else if (opcode == 0x9) /* Ping */
            {
                /* echo Pong */
                ws_send_binary_frame_int(g_ws_sock, payload, len, 1, 0x0A);
            }
            else
            {
                /* 其它帧可交给外部回调 */
                if (g_ws_msg_cb)
                    g_ws_msg_cb(opcode, payload, len);
            }
            osal_kfree(payload);
            uapi_watchdog_kick();
        }
        else
        {
            log_error("[WS-P] recv_task error, closing socket\r\n");
            g_stream_active = 0;
            ws_audio_player_stop();
            ws_client_close();
            uapi_watchdog_kick();
            for (int i = 0; i < 100; i++)
            {
                uapi_watchdog_kick();
                osal_msleep(10);
            }
        }

        /* 周期性打印系统水位 */
        static uint64_t last_tick = 0;
        uint64_t now = uapi_systick_get_ms();
        if (now - last_tick > 5000)
        {
            uint32_t left = 0;
            uapi_watchdog_get_left_time(&left);
            log_info("[SYS] WDT_left=%u tick\r\n", (unsigned)left);
            last_tick = now;
        }
    }
    return 0;
}

/* 启动接收线程，允许多次调用但只会创建一次 */
void ws_client_start_recv_task(void)
{
    static int started = 0;
    if (started)
        return;
    started = 1;

    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create(ws_recv_task, NULL, WS_RECV_TASK_NAME, WS_RECV_TASK_STACK_SIZE);
    if (task_handle != NULL)
    {
        osal_kthread_set_priority(task_handle, WS_RECV_TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

/* 解析 JSON 字段简单工具 */
static int json_get_int(const char *json, const char *key)
{
    const char *p = strstr(json, key);
    if (!p)
        return -1;
    p += strlen(key);
    while (*p && (*p == ' ' || *p == '"' || *p == ':'))
        p++;
    return atoi(p);
}

static void handle_text_frame(const char *msg)
{
    if (strncmp(msg, "STREAM_START", 12) == 0)
    {
        /* STREAM_START <id> <file> {json} */
        const char *p = msg + 13; /* skip command and space */
        while (*p == ' ')
            p++;
        char id[12] = {0};
        int i = 0;
        while (*p && *p != ' ' && i < 11)
        {
            id[i++] = *p++;
        }
        id[i] = '\0';
        strcpy(g_stream_id, id);
        /* skip filename */
        const char *json = strchr(p, '{');
        if (json)
        {
            audio_format_t fmt;
            fmt.sample_rate = json_get_int(json, "sample_rate");
            fmt.channels = json_get_int(json, "channels");
            fmt.bit_depth = json_get_int(json, "bit_depth");
            ws_audio_player_start(&fmt);
            g_stream_active = 1;
        }
    }
    else if (strncmp(msg, "STREAM_END", 10) == 0)
    {
        g_stream_active = 0;
        ws_audio_player_stop();
    }
    else if (strncmp(msg, "STREAM_PAUSE", 12) == 0)
    {
        ws_audio_player_pause();
    }
    else if (strncmp(msg, "STREAM_RESUME", 13) == 0)
    {
        ws_audio_player_resume();
    }
    else if (strncmp(msg, "STREAM_STOP", 11) == 0)
    {
        g_stream_active = 0;
        ws_audio_player_stop();
    }
    /* 其它指令可继续扩展 */
}