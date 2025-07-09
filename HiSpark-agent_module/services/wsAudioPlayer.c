#include "wsAudioPlayer.h"
#include "sio_porting.h"
#include "i2s.h"
#include "wm8978.h"
#include "pinctrl.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include <string.h>
#if defined(CONFIG_I2S_SUPPORT_DMA)
#include "dma.h"
#endif
#include "watchdog.h"
#include "debugUtils.h"
#include "hal_sio_v151.h"

/* 播放器内部状态 */
static int g_channels = 2;
static int g_bits_per_sample = 16;
static int g_playback_inited = 0;
static int g_paused = 0;

#define BUF_FRAMES 64
#if defined(CONFIG_I2S_SUPPORT_DMA)
/*
 * DMA 发送时使用的合并缓冲区。
 * 之前放在 i2s_write_pcm 的栈上，DMA 仍可能访问已释放的栈区而触发 NMI。
 * 将其提升为静态全局，生命周期与播放器一致，彻底消除悬空指针问题。
 */
static uint32_t g_merge_buf[BUF_FRAMES];
/* 标记一次 DMA 事务是否仍在进行，用于 stop/reset 阶段等待其结束。*/
static volatile int g_dma_tx_busy = 0;
#endif

static uint8_t g_pcm_accum[BUF_FRAMES * 4]; /* 4 bytes per stereo frame */
static uint32_t g_accum_len = 0;

static void audio_playback_init(void)
{
    if (g_playback_inited)
        return;

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 确保在进行 I2S DMA 发送前 DMA 已就绪 */
    (void)uapi_dma_init();
    (void)uapi_dma_open();
#endif

    /* 默认使用 WM8978 + I2S 播放 */
    sio_porting_i2s_pinmux();

    if (WM8978_Init() != 0)
    {
        log_info("WM8978 init failed\r\n");
    }
    WM8978_ADDA_Cfg(1, 0);      /* DAC on */
    WM8978_Write_Reg(2, 0X1B0); // 耳机输出使能,BOOST使能 000110110000
    WM8978_Input_Cfg(0, 0, 0);
    WM8978_Output_Cfg(1, 1);
    WM8978_MIC_Gain(0);
    WM8978_I2S_Cfg(2, 0); /* Philips, 16bit */
    WM8978_HPvol_Set(60, 60);
    WM8978_SPKvol_Set(60);

    i2s_config_t i2s_cfg_play = {
        .drive_mode = 1,
        .transfer_mode = 0,
        .data_width = 1,                           /* 16bit */
        .channels_num = (g_channels == 1) ? 0 : 0, /* still 2ch path */
        .timing = 0,
        .clk_edge = 0,
        .div_number = 16,
        .number_of_channels = 2,
    };

    if (uapi_i2s_init(SIO_BUS_0, NULL) != ERRCODE_SUCC)
    {
        log_info("I2S init failed\r\n");
    }
    uapi_i2s_set_config(SIO_BUS_0, &i2s_cfg_play);

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 配置 I2S 使用 DMA 进行 TX */
    i2s_dma_attr_t dma_attr = {
        .tx_dma_enable = true,
        .tx_int_threshold = 0,
        .rx_dma_enable = false,
        .rx_int_threshold = 0,
    };
    uapi_i2s_dma_config(SIO_BUS_0, &dma_attr);
#endif

    g_playback_inited = 1;
    log_info("Audio playback init done.\r\n");
}

/* 将缓存的 PCM 写入 I2S，要求 len 为帧对齐 (4 bytes per frame stereo) */
static void i2s_write_pcm(const uint8_t *pcm, size_t len)
{
    if (!g_playback_inited)
        return;

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* ---------------- DMA 发送实现 ---------------- */
    static i2s_dma_config_t dma_cfg = {
        .src_width = 2,
        .dest_width = 2,
        .burst_length = 0,
        .priority = 1,
    };

    size_t offset = 0;
    static uint32_t dbg_loop = 0;

    while (offset < len)
    {
        uapi_watchdog_kick();
        size_t bytes_per_frame = (g_channels == 1) ? 2 : 4; /* 单声道:2字节, 立体声:4字节 */
        size_t frames = (len - offset) / bytes_per_frame;
        if (frames > BUF_FRAMES)
            frames = BUF_FRAMES;

        for (size_t i = 0; i < frames; i++)
        {
            const uint8_t *p = pcm + offset + i * bytes_per_frame;
            uint16_t left = (uint16_t)(p[0] | (p[1] << 8));
            uint16_t right;
            if (g_channels == 2)
                right = (uint16_t)(p[2] | (p[3] << 8));
            else
                right = left;
            g_merge_buf[i] = ((uint32_t)right << 16) | left;
        }

        /* 标记 DMA 进行中，供 reset 流程感知 */
        g_dma_tx_busy = 1;
        int32_t ret = uapi_i2s_merge_write_by_dma(SIO_BUS_0, g_merge_buf, (uint32_t)frames, &dma_cfg, 0, true);
        g_dma_tx_busy = 0;
        if (ret < 0)
        {
            log_info("[AUDIO] DMA write error %d at offset %u\r\n", ret, (unsigned)offset);
        }
        dbg_loop++;
        offset += frames * bytes_per_frame;
    }
#else
    /* ---------------- 原中断发送实现 ---------------- */
    size_t offset = 0;
    uint32_t l_buf[BUF_FRAMES + 1], r_buf[BUF_FRAMES + 1];

    while (offset < len)
    {
        uapi_watchdog_kick();
        size_t bytes_per_frame = (g_channels == 1) ? 2 : 4; /* 单声道:2字节, 立体声:4字节 */
        size_t frames = (len - offset) / bytes_per_frame;
        if (frames > BUF_FRAMES)
            frames = BUF_FRAMES;

        for (size_t i = 0; i < frames; i++)
        {
            const uint8_t *p = pcm + offset + i * bytes_per_frame;
            uint16_t left = (uint16_t)(p[0] | (p[1] << 8));
            uint16_t right;
            if (g_channels == 2)
                right = (uint16_t)(p[2] | (p[3] << 8));
            else
                right = left;
            l_buf[i] = left;
            r_buf[i] = right;
        }

        i2s_tx_data_t tx = {
            .left_buff = l_buf,
            .right_buff = r_buf,
            .length = frames,
        };
        uapi_i2s_write_data(SIO_BUS_0, &tx);
        offset += frames * bytes_per_frame;
    }
#endif
}

/* 重置播放器状态，可在停止或错误后调用 */
void ws_audio_player_reset(void)
{
#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 等待可能仍在进行的最后一次 DMA 传输结束，最长约几毫秒 */
    while (g_dma_tx_busy)
    {
        uapi_watchdog_kick();
        osal_msleep(1);
    }
#endif
    if (g_playback_inited)
    {
        /* 先显式关闭 TX/RX 以及 CRG clock，确保后续不会再触发 I2S 中断 */
        hal_sio_v151_txrx_disable(SIO_BUS_0);
        /* 再进行 deinit，释放资源 */
        uapi_i2s_deinit(SIO_BUS_0);
        g_playback_inited = 0;
    }

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 关闭 DMA，释放资源 */
    uapi_dma_close();
    uapi_dma_deinit();
#endif

    g_channels = 2;
    g_bits_per_sample = 16;
    g_accum_len = 0;
}

/* API: 启动播放 */
int ws_audio_player_start(const audio_format_t *fmt)
{
    if (fmt == NULL)
        return -1;
    /* 仅支持 16bit，目前硬件配置写死 16bit */
    if (fmt->bit_depth != 16)
        return -1;
    g_channels = (fmt->channels == 1) ? 1 : 2;
    g_bits_per_sample = 16;
    audio_playback_init();
    g_paused = 0;
    return 0;
}

/* API: 写入 PCM 数据 */
void ws_audio_player_feed_pcm(const uint8_t *pcm, size_t len)
{
    if (g_paused || !g_playback_inited)
        return;
    i2s_write_pcm(pcm, len);
}

void ws_audio_player_pause(void)
{
    g_paused = 1;
}

void ws_audio_player_resume(void)
{
    g_paused = 0;
}

void ws_audio_player_stop(void)
{
    ws_audio_player_reset();
}
