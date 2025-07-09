#include "wavRecorderService.h"
#include "debugUtils.h"
#include "soundService.h"
#include "littlefs_adapt.h"
#include "websocketService.h"
#include "fcntl.h"
#include "string.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdio.h"
#include "watchdog.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include <time.h>
#include "common_def.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "persistentWsClient.h"
#include "oledService.h"
#include "gpio.h" // 新增，用于按键状态检测
#define WAV_BITS_PER_SAMPLE 16
#define WAV_NUM_CHANNELS 2

/* WAV 文件头结构，大小固定 44 字节（不含可选字段） */
typedef struct __attribute__((packed))
{
    char riff_id[4];    /* "RIFF" */
    uint32_t riff_size; /* 文件总长 - 8 */
    char wave_id[4];    /* "WAVE" */

    char fmt_id[4];           /* "fmt " */
    uint32_t fmt_size;        /* 16 for PCM */
    uint16_t audio_format;    /* 1 = PCM */
    uint16_t num_channels;    /* 1 或 2 */
    uint32_t sample_rate;     /* 16000 等 */
    uint32_t byte_rate;       /* sample_rate * channels * bits/8 */
    uint16_t block_align;     /* channels * bits/8 */
    uint16_t bits_per_sample; /* 16 */

    char data_id[4];    /* "data" */
    uint32_t data_size; /* 后面数据长度 */
} wav_header_t;

static void fill_wav_header(wav_header_t *hdr, uint32_t sample_rate)
{
    memcpy(hdr->riff_id, "RIFF", 4);
    hdr->riff_size = 0; /* 先写 0，后续再补 */
    memcpy(hdr->wave_id, "WAVE", 4);

    memcpy(hdr->fmt_id, "fmt ", 4);
    hdr->fmt_size = 16;
    hdr->audio_format = 1;
    hdr->num_channels = WAV_NUM_CHANNELS;
    hdr->sample_rate = sample_rate;
    hdr->byte_rate = sample_rate * WAV_NUM_CHANNELS * (WAV_BITS_PER_SAMPLE / 8);
    hdr->block_align = WAV_NUM_CHANNELS * (WAV_BITS_PER_SAMPLE / 8);
    hdr->bits_per_sample = WAV_BITS_PER_SAMPLE;

    memcpy(hdr->data_id, "data", 4);
    hdr->data_size = 0; /* 后续补齐 */
}

/* 每次推流写入的录音采样数（与 I2S DMA Merge 模式保持对齐, 1024 帧 = 4096 字节） */
#define CHUNK_SAMPLES 1024
static uint16_t g_sample_buf[CHUNK_SAMPLES * 2];

/* 实时录音并推流到 WebSocket */
int wav_record_and_stream(uint32_t seconds, uint32_t sample_rate,
                          const char *ws_server_ip, uint16_t ws_server_port,
                          const char *ws_path)
{
    if (seconds == 0 || sample_rate == 0)
    {
        return -1;
    }

    /* 1. 确保长连接已就绪 */
    /* 切换 OLED 为录音动画模式 */
    OledSetMode(OLED_MODE_AUDIO);
    if (ws_client_sock() < 0)
    {
        if (ws_client_init(ws_server_ip, ws_server_port, ws_path) != 0)
        {
            OledSetMode(OLED_MODE_IDLE);
            return -2;
        }
    }
    int ws_sock = ws_client_sock();

    /* 1.5 发送 UPLOAD <filename> 文本帧，符合新协议 */
    char filename[32];
    snprintf(filename, sizeof(filename), "rec.wav");
    char upload_cmd[64];
    snprintf(upload_cmd, sizeof(upload_cmd), "UPLOAD %s", filename);
    if (ws_client_send_text(upload_cmd) != 0)
    {
        log_error("[WS] send UPLOAD cmd fail\r\n");
        OledSetMode(OLED_MODE_IDLE);
        return -3;
    }

    /* 2. 发送 WAV 头部（独立完整二进制帧，FIN=1）*/
    wav_header_t hdr;
    fill_wav_header(&hdr, sample_rate);
    uint32_t total_samples_wanted = seconds * sample_rate * WAV_NUM_CHANNELS;
    uint32_t data_bytes = total_samples_wanted * sizeof(uint32_t);
    hdr.data_size = data_bytes;
    hdr.riff_size = data_bytes + sizeof(wav_header_t) - 8;

    if (ws_client_send_binary((const uint8_t *)&hdr, sizeof(hdr), 1) != 0)
    {
        log_error("[WS] send wav header fail\r\n");
        OledSetMode(OLED_MODE_IDLE);
        return -4;
    }

    /* 3. 初始化录音设备 */
    if (sound_recorder_open() != 0)
    {
        log_error("[Recoder] recorder init failed");
        OledSetMode(OLED_MODE_IDLE);
        return -5;
    }

    /* 4. 边录制边推流 */
    uint32_t total_samples_sent = 0;
    int ret = 0;
    while (total_samples_sent < total_samples_wanted)
    {
        uapi_watchdog_kick();
        uint32_t remain = total_samples_wanted - total_samples_sent;
        uint32_t this_samples = (remain > CHUNK_SAMPLES) ? CHUNK_SAMPLES : remain;

        /* 录制音频数据 */
        size_t got = sound_recorder_read(g_sample_buf, this_samples);
        if (got == 0)
        {
            log_error("[WS] record failed\r\n");
            ret = -6;
            break;
        }

        /* 将本块作为独立完整帧发送（不再使用 CONTINUATION） */
        if (ws_client_send_binary((const uint8_t *)g_sample_buf, got * sizeof(uint16_t), 1) != 0)
        {
            log_error("[WS] send audio data fail\r\n");
            ret = -7;
            break;
        }

        total_samples_sent += (uint32_t)got;
    }
    osal_mdelay(100);
    /* 5. 发送 EOF 文本帧，通知服务器流结束 */
    if (ws_client_send_text("EOF") != 0)
    {
        log_error("[WS] send EOF fail\r\n");
    }
    /* 6. 仅清理录音资源，不关闭 WS 连接 */
    log_debug("wav recorder debug 1\n");
    sound_recorder_close();
    /* 切回 IDLE 模式 */
    OledSetMode(OLED_MODE_IDLE);
    log_debug("wav recorder debug 2\n");
    log_info("[WS] stream complete, ret=%d\r\n", ret);
    return ret;
}

/* ==========================================
 * 对外接口：清空 LittleFS 录音缓存分区
 * ==========================================*/
void wav_cache_clear(void)
{
    /* 一行调用即可完成格式化并清空所有文件 */
    fs_adapt_format();
    fs_adapt_unmount();
}

/* =============================================================
 * 按住录音并实时推流：按键按下开始录音，松开后发送 EOF 结束
 * =============================================================*/
int wav_record_hold_and_stream(uint32_t sample_rate,
                               const char *ws_server_ip, uint16_t ws_server_port,
                               const char *ws_path, int key_pin)
{
    if (sample_rate == 0)
    {
        return -1;
    }

    /* OLED 显示录音动画 */
    OledSetMode(OLED_MODE_AUDIO);

    /* 确保 websocket 长连接已就绪 */
    if (ws_client_sock() < 0)
    {
        if (ws_client_init(ws_server_ip, ws_server_port, ws_path) != 0)
        {
            OledSetMode(OLED_MODE_IDLE);
            return -2;
        }
    }

    /* 发送 UPLOAD 指令，固定文件名 rec.wav（按协议要求）*/
    char upload_cmd[64];
    snprintf(upload_cmd, sizeof(upload_cmd), "UPLOAD %s", "rec.wav");
    if (ws_client_send_text(upload_cmd) != 0)
    {
        log_error("[WS] send UPLOAD cmd fail\r\n");
        OledSetMode(OLED_MODE_IDLE);
        return -3;
    }

    /* 发送不定长 WAV 头，data_size/riff_size 设置为 0 以表示未知长度 */
    wav_header_t hdr;
    fill_wav_header(&hdr, sample_rate);
    hdr.data_size = 0;
    hdr.riff_size = 0;

    if (ws_client_send_binary((const uint8_t *)&hdr, sizeof(hdr), 1) != 0)
    {
        log_error("[WS] send wav header fail\r\n");
        OledSetMode(OLED_MODE_IDLE);
        return -4;
    }

    /* 打开录音硬件 */
    if (sound_recorder_open() != 0)
    {
        log_error("[Recoder] recorder init failed");
        OledSetMode(OLED_MODE_IDLE);
        return -5;
    }

    int ret = 0;
    /* 主循环：按键保持低电平期间持续录音 */
    while (uapi_gpio_get_val(key_pin) == GPIO_LEVEL_LOW)
    {
        uapi_watchdog_kick();

        /* 录制音频数据 */
        size_t got = sound_recorder_read(g_sample_buf, CHUNK_SAMPLES);
        if (got == 0)
        {
            log_error("[WS] record failed\r\n");
            ret = -6;
            break;
        }

        /* 发送音频数据 */
        if (ws_client_send_binary((const uint8_t *)g_sample_buf, got * sizeof(uint16_t), 1) != 0)
        {
            log_error("[WS] send audio data fail\r\n");
            ret = -7;
            break;
        }
    }

    /* 松开后稍作延时，等待最后一帧发送完成 */
    osal_mdelay(100);

    /* 发送 EOF 文本帧通知服务器 */
    if (ws_client_send_text("EOF") != 0)
    {
        log_error("[WS] send EOF fail\r\n");
    }

    /* 关闭录音硬件 */
    sound_recorder_close();

    /* OLED 复位到 IDLE */
    OledSetMode(OLED_MODE_IDLE);

    log_info("[WS] hold stream complete, ret=%d\r\n", ret);
    return ret;
}