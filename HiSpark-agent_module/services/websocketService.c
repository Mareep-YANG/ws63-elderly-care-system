#include "websocketService.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common_def.h" /* 提供 PRINT 宏，若无可自行修改 */
#include "uart.h"
#include "littlefs_adapt.h"
#include "fcntl.h"

/* ---------------- 内部工具函数 ---------------- */

/* 生成 Sec-WebSocket-Key */
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

/* 计算 Sec-WebSocket-Accept */
static void ws_calc_accept(const char *key, char *out_b64, size_t out_len)
{
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t sha1_out[20];
    char composite[64]; /* 24(key) + 36(GUID) + 1 = 61, 预留空间 */
    snprintf(composite, sizeof(composite), "%s%s", key, GUID);
    mbedtls_sha1((const unsigned char *)composite, strlen(composite), sha1_out);

    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)out_b64, out_len, &olen,
                          sha1_out, sizeof(sha1_out));
    out_b64[olen] = '\0';
}

/* 打包并发送客户端 -> 服务器的文本帧，自动 Mask */
static int ws_send_text_frame(int sock, const char *text)
{
    size_t payload_len = strlen(text);
    size_t header_len = 2; // 基础头
    uint8_t hdr[14];       // 最长: 2 + 8(长度) + 4(mask)

    hdr[0] = 0x81; // FIN=1, opcode=0x1 (text)

    if (payload_len <= 125)
    {
        hdr[1] = 0x80 | (uint8_t)payload_len; // mask=1
    }
    else if (payload_len <= 0xFFFF)
    {
        hdr[1] = 0x80 | 126;
        uint16_t len16 = htons((uint16_t)payload_len);
        memcpy(&hdr[2], &len16, 2);
        header_len += 2;
    }
    else
    {
        hdr[1] = 0x80 | 127;
        uint64_t len64 = htobe64((uint64_t)payload_len);
        memcpy(&hdr[2], &len64, 8);
        header_len += 8;
    }

    // 生成 mask
    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    memcpy(&hdr[header_len], mask, 4);
    header_len += 4;

    // 对 payload 做 mask
    size_t i;
    uint8_t *masked = (uint8_t *)malloc(payload_len);
    if (!masked)
        return -1;
    for (i = 0; i < payload_len; i++)
    {
        masked[i] = text[i] ^ mask[i % 4];
    }

    // 发送头 + payload
    int ret = send(sock, hdr, header_len, 0);
    if (ret < 0)
    {
        free(masked);
        return -1;
    }
    ret = send(sock, masked, payload_len, 0);
    free(masked);
    return (ret < 0) ? -1 : 0;
}

/* 发送二进制帧 (payload 不做拷贝/修改，需 caller 保证可读取)，mask version */
static int ws_send_binary_frame(int sock, const uint8_t *data, size_t data_len, int fin)
{
    (void)fin; /* 不再分片，形参保留以兼容旧调用 */
    /* 为兼容性简单处理：每块作为独立完整消息发送 (FIN=1, opcode=0x2) */
    size_t header_len = 2;
    uint8_t hdr[14];

    hdr[0] = 0x82; /* FIN=1, opcode=0x2 (binary) */

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
        uint64_t len64 = htobe64((uint64_t)data_len);
        memcpy(&hdr[2], &len64, 8);
        header_len += 8;
    }

    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    memcpy(&hdr[header_len], mask, 4);
    header_len += 4;

    // Streaming: 分段对原始数据做异或，避免分配大 buffer
    if (send(sock, hdr, header_len, 0) < 0)
    {
        return -1;
    }
    size_t sent_total = 0;
    uint8_t masked_byte;
    while (sent_total < data_len)
    {
        masked_byte = data[sent_total] ^ mask[sent_total % 4];
        if (send(sock, &masked_byte, 1, 0) < 0)
        {
            return -1;
        }
        sent_total++;
    }
    return 0;
}

/* 发送关闭帧，payload 长度 0，带 mask */
static int ws_send_close_frame(int sock)
{
    uint8_t hdr[6];
    uint8_t mask[4] = {rand(), rand(), rand(), rand()};
    hdr[0] = 0x88;        /* FIN=1, opcode=0x8 (Close) */
    hdr[1] = 0x80 | 0x00; /* mask=1, payload len=0 */
    memcpy(&hdr[2], mask, 4);
    if (send(sock, hdr, sizeof(hdr), 0) < 0)
    {
        return -1;
    }
    return 0;
}

#ifndef htobe64
static uint64_t htobe64(uint64_t host_64)
{
    uint32_t high = (uint32_t)(host_64 >> 32);
    uint32_t low = (uint32_t)(host_64 & 0xFFFFFFFF);
    high = htonl(high);
    low = htonl(low);
    return (((uint64_t)low) << 32) | high;
}
#endif

int websocket_send_file(const char *server_ip, uint16_t port, const char *ws_path,
                        const char *filename,
                        const uint8_t *file_data, size_t file_len)
{
    if (!server_ip || !ws_path || !filename || !file_data || file_len == 0)
    {
        return -1;
    }

    srand((unsigned int)time(NULL));

    /* ---------- 建 TCP socket ---------- */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        PRINT("[WS] socket fail\r\n");
        return -1;
    }
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    srv.sin_addr.s_addr = inet_addr(server_ip);
    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0)
    {
        PRINT("[WS] connect fail\r\n");
        lwip_close(sock);
        return -1;
    }

    /* ---------- 握手 ---------- */
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
        PRINT("[WS] handshake HTTP err\r\n");
        lwip_close(sock);
        return -1;
    }
    char accept_expected[40];
    ws_calc_accept(ws_key, accept_expected, sizeof(accept_expected));
    if (strstr(resp, accept_expected) == NULL)
    {
        PRINT("[WS] accept key mismatch\r\n");
        lwip_close(sock);
        return -1;
    }
    PRINT("[WS] handshake OK\r\n");

    /* ---------- 发送文件 ---------- */
    char upload_cmd[64];
    snprintf(upload_cmd, sizeof(upload_cmd), "UPLOAD %s", filename);
    if (ws_send_text_frame(sock, upload_cmd) != 0)
    {
        PRINT("[WS] send UPLOAD cmd fail\r\n");
        lwip_close(sock);
        return -1;
    }

    /* 按块发送二进制帧 */
    const size_t CHUNK = 1024; // 每帧 1KB，可根据 MTU 调整
    size_t offset = 0;
    while (offset < file_len)
    {
        size_t remain = file_len - offset;
        size_t this_len = (remain > CHUNK) ? CHUNK : remain;
        int fin = (offset + this_len >= file_len) ? 1 : 0;
        if (ws_send_binary_frame(sock, file_data + offset, this_len, fin) != 0)
        {
            PRINT("[WS] send binary fail\r\n");
            lwip_close(sock);
            return -1;
        }
        offset += this_len;
    }

    /* 发送 EOF */
    if (ws_send_text_frame(sock, "EOF") != 0)
    {
        PRINT("[WS] send EOF fail\r\n");
        lwip_close(sock);
        return -1;
    }

    /* 根据新协议可选发送 BYE，结束会话 */
    (void)ws_send_text_frame(sock, "BYE");

    /* 主动发送关闭帧，完成 WebSocket 优雅关闭 */
    ws_send_close_frame(sock);

    PRINT("[WS] file send done\r\n");

    lwip_close(sock);
    return 0;
}

/* ===============================================================
 * 新增：直接从文件系统读取并发送文件，节省 SRAM
 * =============================================================*/

int websocket_send_file_from_fs(const char *server_ip, uint16_t port, const char *ws_path,
                                const char *filepath)
{
    if (!server_ip || !ws_path || !filepath)
    {
        return -1;
    }

    /* 获取文件大小并提取文件名 */
    unsigned int file_size = 0;
    if (fs_adapt_stat(filepath, &file_size) != 0 || file_size == 0)
    {
        PRINT("[WS] stat fail\r\n");
        return -1;
    }

    const char *filename = filepath;
    const char *slash = strrchr(filepath, '/');
    if (slash && *(slash + 1) != '\0')
    {
        filename = slash + 1;
    }

    /* 打开文件 */
    int fd = fs_adapt_open(filepath, O_RDONLY);
    if (fd < 0)
    {
        PRINT("[WS] open file fail\r\n");
        return -1;
    }

    /* 与 websocket_send_file 实现几乎一致，区别在于循环读文件块 */

    srand((unsigned int)time(NULL));

    /* ---------- 建 TCP socket ---------- */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        PRINT("[WS] socket fail\r\n");
        fs_adapt_close(fd);
        return -1;
    }
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    srv.sin_addr.s_addr = inet_addr(server_ip);
    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0)
    {
        PRINT("[WS] connect fail\r\n");
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }

    /* ---------- 握手 ---------- */
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
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }

    char resp[512] = {0};
    int rcv = recv(sock, resp, sizeof(resp) - 1, 0);
    if (rcv <= 0 || strstr(resp, " 101 ") == NULL)
    {
        PRINT("[WS] handshake HTTP err\r\n");
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }
    char accept_expected[40];
    ws_calc_accept(ws_key, accept_expected, sizeof(accept_expected));
    if (strstr(resp, accept_expected) == NULL)
    {
        PRINT("[WS] accept key mismatch\r\n");
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }
    PRINT("[WS] handshake OK\r\n");

    /* ---------- 发送文件 ---------- */
    char upload_cmd_fs[64];
    snprintf(upload_cmd_fs, sizeof(upload_cmd_fs), "UPLOAD %s", filename);
    if (ws_send_text_frame(sock, upload_cmd_fs) != 0)
    {
        PRINT("[WS] send UPLOAD cmd fail\r\n");
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }

    const size_t CHUNK = 1024;
    uint8_t buf[CHUNK];
    size_t offset = 0;
    while (offset < file_size)
    {
        size_t to_read = (file_size - offset > CHUNK) ? CHUNK : (file_size - offset);
        int r = fs_adapt_read(fd, (char *)buf, to_read);
        if (r <= 0)
        {
            PRINT("[WS] file read fail at offset %d\r\n", (int)offset);
            fs_adapt_close(fd);
            lwip_close(sock);
            return -1;
        }
        int fin = ((offset + r) >= file_size) ? 1 : 0;
        if (ws_send_binary_frame(sock, buf, r, fin) != 0)
        {
            PRINT("[WS] send binary fail\r\n");
            fs_adapt_close(fd);
            lwip_close(sock);
            return -1;
        }
        offset += (size_t)r;
    }

    /* 发送 EOF */
    if (ws_send_text_frame(sock, "EOF") != 0)
    {
        PRINT("[WS] send EOF fail\r\n");
        fs_adapt_close(fd);
        lwip_close(sock);
        return -1;
    }

    /* 根据新协议可选发送 BYE，结束会话 */
    (void)ws_send_text_frame(sock, "BYE");

    /* 主动发送关闭帧，完成 WebSocket 优雅关闭 */
    ws_send_close_frame(sock);

    PRINT("[WS] file send done\r\n");

    fs_adapt_close(fd);
    lwip_close(sock);
    return 0;
}