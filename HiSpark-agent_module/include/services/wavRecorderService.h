#ifndef WAV_RECORDER_SERVICE_H
#define WAV_RECORDER_SERVICE_H

#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * 录制指定秒数的音频并通过 websocket 发送给上位机（先保存到文件再发送）。
     *
     * @param seconds          录制时长（秒）
     * @param sample_rate      采样率，例如 16000 或 32000
     * @param ws_server_ip     服务器 IP 字符串，如 "192.168.3.10"
     * @param ws_server_port   服务器端口，例如 8080
     * @param ws_path          websocket 路径，如 "/upload"
     * @return 0 成功，其它失败
     */
    int wav_record_and_send(uint32_t seconds, uint32_t sample_rate,
                            const char *ws_server_ip, uint16_t ws_server_port,
                            const char *ws_path);

    /*
     * 实时录制音频并通过 websocket 推流给上位机（边录制边发送，无需保存文件）。
     *
     * @param seconds          录制时长（秒）
     * @param sample_rate      采样率，例如 16000 或 32000
     * @param ws_server_ip     服务器 IP 字符串，如 "192.168.3.10"
     * @param ws_server_port   服务器端口，例如 8080
     * @param ws_path          websocket 路径，如 "/upload"
     * @return 0 成功，其它失败
     */
    int wav_record_and_stream(uint32_t seconds, uint32_t sample_rate,
                              const char *ws_server_ip, uint16_t ws_server_port,
                              const char *ws_path);

    /*
     * 按住录音：按键按下开始录音并推流，松开后发送 EOF 停止。
     * @param sample_rate       采样率，例如 16000 或 32000
     * @param ws_server_ip      服务器 IP，传入 NULL 表示使用已初始化的长连接
     * @param ws_server_port    服务器端口，未使用时传 0
     * @param ws_path           websocket 路径，未使用时传 NULL
     * @param key_pin           监测的按键 GPIO 引脚编号
     * @return 0 成功，其它失败
     */
    int wav_record_hold_and_stream(uint32_t sample_rate,
                                   const char *ws_server_ip, uint16_t ws_server_port,
                                   const char *ws_path, int key_pin);

    /* 清空录音缓冲区（LittleFS 分区） */
    void wav_cache_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* WAV_RECORDER_SERVICE_H */