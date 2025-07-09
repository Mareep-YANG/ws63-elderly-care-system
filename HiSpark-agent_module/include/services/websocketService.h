#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* WebSocket 客户端，发送整文件
     * server_ip   : 服务器 IP 字符串，点分十进制
     * port        : 服务器监听端口
     * ws_path     : WebSocket 资源路径，如 "/upload"
     * filename    : 首次文本帧发送的文件名 (含扩展名)
     * file_data   : 文件数据指针
     * file_len    : 文件字节数
     * 返回 0 表示成功，其余为错误码 */
    int websocket_send_file(const char *server_ip, uint16_t port, const char *ws_path,
                            const char *filename,
                            const uint8_t *file_data, size_t file_len);

    /* WebSocket 客户端，直接从本地文件系统读取并分块发送
     * filepath   : 需要发送的文件在文件系统中的完整路径（如 "/rec_123.wav"）
     * 其余参数同上
     * 内部自动按 CHUNK 大小循环读/发，避免一次性占用大量 SRAM
     */
    int websocket_send_file_from_fs(const char *server_ip, uint16_t port, const char *ws_path,
                                    const char *filepath);

#ifdef __cplusplus
}
#endif