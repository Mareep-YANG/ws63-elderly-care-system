#include "bits/alltypes.h"
#include "common_def.h"
#include "i2s.h"
#include "osal_timer.h"
#include "wm8978.h"
#include "watchdog.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "soundService.h"
#include <string.h>
#if defined(CONFIG_I2S_SUPPORT_DMA)
#include "dma.h"
#endif
#include "hal_sio_v151.h" /* 为显式关闭 TX/RX 使能 */
#include "debugUtils.h"

#if defined(CONFIG_I2S_SUPPORT_DMA)
/* 全局 DMA 临时缓冲区，供 sound_recorder_read 循环复用，减少频繁 malloc/free 造成碎片 */
static uint32_t *g_dma_rx_buf = NULL;
static uint32_t *g_dma_tx_dummy = NULL;
static size_t g_dma_buf_frames = 0;
/* 标记 TX DMA 是否已启动，只需一次保持 BCLK */
static int g_tx_dma_started = 0;
/* 标记 I2S DMA 属性是否已经配置，避免每个分片重复配置 */
static int g_dma_attr_configured = 0;
#endif

/* 录音缓冲区（由 API 调用者提供） */
static volatile uint16_t *g_rec_buf = NULL;
static volatile size_t g_rec_max_samples = 0;
static volatile size_t g_rec_frames = 0;

/* 录音硬件启停状态 */
static int g_rec_hw_open = 0;

/* I2S RX 中断回调，将接收到的数据拷贝到录音缓冲 */
static void i2s_rx_callback(uint32_t *left_buff, uint32_t *right_buff, uint32_t length)
{
    uapi_watchdog_kick();
    (void)right_buff; /* 单声道，仅处理左声道 */

    if (length == 0 || g_rec_buf == NULL)
    {
        return;
    }

    size_t remain = g_rec_max_samples - g_rec_frames;
    size_t cp = (length < remain) ? length : remain;
    for (size_t i = 0; i < cp; ++i)
    {
        /* 使用低 16 位作为有效数据（若仍为 0，可结合调试信息再调整） */
        g_rec_buf[g_rec_frames + i] = (uint16_t)(left_buff[i] & 0xFFFF);
    }
    g_rec_frames += cp;
}

/*
 * 内部通用函数：初始化 WM8978 与 I2S 以进入录音模式
 */
static int recorder_hw_open(void)
{
    if (g_rec_hw_open)
    {
        return 0; /* 已经打开 */
    }

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 确保在使用 I2S DMA 前已初始化并打开 DMA 模块 */
    (void)uapi_dma_init();
    (void)uapi_dma_open();
#endif

    sio_porting_i2s_pinmux();

    if (WM8978_Init() != 0)
    {
        log_error("WM8978 init failed\r\n");
        return -1;
    }

    /* 配置 WM8978 进入录音模式 */
    WM8978_ADDA_Cfg(0, 1);
    WM8978_Input_Cfg(1, 0, 0);
    WM8978_MIC_Gain(30);
    WM8978_I2S_Cfg(2, 0);

    /* I2S 配置 */
    i2s_config_t i2s_cfg_rec = {
        .drive_mode = 1,
        .transfer_mode = 0,
        .data_width = 1,
        .channels_num = 0,
        .timing = 0,
        .clk_edge = 1,
        .div_number = 16,
        .number_of_channels = 2,
    };

    if (uapi_i2s_init(SIO_BUS_0, i2s_rx_callback) != ERRCODE_SUCC)
    {
        log_info("I2S init (rec) failed\r\n");
        return -1;
    }

    uapi_i2s_set_config(SIO_BUS_0, &i2s_cfg_rec);
    uapi_i2s_read_start(SIO_BUS_0);

    g_rec_hw_open = 1;
    return 0;
}

/*
 * 内部通用函数：关闭 I2S 与 WM8978
 */
static void recorder_hw_close(void)
{
    if (!g_rec_hw_open)
    {
        return;
    }

    /* 主动关闭 I2S TX/RX 以及 BCLK，避免后续 DMA 等硬件等待 */
    hal_sio_v151_txrx_disable(SIO_BUS_0);

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* 先停止 DMA，再关闭 I2S，避免 DMA 仍在工作时外设被关闭导致的异常 */
    uapi_dma_close();
    uapi_dma_deinit();
#endif

    /* 最后再关闭 I2S 外设 */
    uapi_i2s_deinit(SIO_BUS_0);

    g_rec_hw_open = 0;
}

/* ===============================================================
 * 新增的分离式录音 API 实现
 * =============================================================*/

int sound_recorder_open(void)
{
    return recorder_hw_open();
}

size_t sound_recorder_read(uint16_t *buf, size_t max_samples)
{
    if (buf == NULL || max_samples == 0)
    {
        return 0;
    }

    if (recorder_hw_open() != 0)
    {
        return 0;
    }

#if defined(CONFIG_I2S_SUPPORT_DMA)
    /* ------------------------ DMA 录音实现 ------------------------ */
    /* I2S DMA 总线参数配置：RX/TX 均启用 DMA。
       仅第一次调用时进行配置，防止硬件繁忙状态下重复配置失败 */
    if (!g_dma_attr_configured)
    {
        i2s_dma_attr_t dma_attr = {
            .tx_dma_enable = true,
            .tx_int_threshold = 0,
            .rx_dma_enable = true,
            .rx_int_threshold = 0,
        };
        uapi_i2s_dma_config(SIO_BUS_0, &dma_attr);
        g_dma_attr_configured = 1;
    }

    /* DMA 通道配置：32bit Merge 模式传输 */
    i2s_dma_config_t dma_cfg = {
        .src_width = 2, /* 源/目的 32bit */
        .dest_width = 2,
        .burst_length = 0, /* 单次 */
        .priority = 1,
    };

    /* 仅第一次调用时申请 RX DMA 缓冲区，后续复用以降低碎片；
       TX dummy 缓冲区在首次启动 TX DMA 时再单独申请 */
    if (g_dma_rx_buf == NULL || g_dma_buf_frames < max_samples)
    {
        if (g_dma_rx_buf)
        {
            osal_kfree(g_dma_rx_buf);
        }
        g_dma_rx_buf = (uint32_t *)osal_kmalloc(max_samples * sizeof(uint32_t), OSAL_GFP_KERNEL);
        if (g_dma_rx_buf == NULL)
        {
            log_info("sound_recorder_read: alloc dma rx buf failed\r\n");
            return 0;
        }
        g_dma_buf_frames = max_samples;
    }

    uint32_t *rx_tmp = g_dma_rx_buf;

    /* 如果 TX dummy 缓冲区尚未申请，则按当前 max_samples 大小申请一次 */
    if (g_dma_tx_dummy == NULL)
    {
        g_dma_tx_dummy = (uint32_t *)osal_kmalloc(max_samples * sizeof(uint32_t), OSAL_GFP_KERNEL);
        if (g_dma_tx_dummy == NULL)
        {
            log_info("sound_recorder_read: alloc dma tx dummy buf failed\r\n");
            return 0;
        }
        memset(g_dma_tx_dummy, 0, max_samples * sizeof(uint32_t));
    }

    uint32_t *tx_dummy = g_dma_tx_dummy;

    /* 仅第一次调用时启动一次 dummy TX DMA，使用循环模式且非阻塞，
       保持 I2S BCLK 长期开启，避免每块数据重新启动 DMA 失败 */
    if (!g_tx_dma_started)
    {
        /* 启动 TX DMA：loop_mode = 1, block = false */
        int32_t tx_ret = uapi_i2s_merge_write_by_dma(SIO_BUS_0, tx_dummy, max_samples, &dma_cfg, 1, false);
        if (tx_ret < 0)
        {
            log_info("sound_recorder_read: start dummy tx dma failed, err=0x%x\r\n", (unsigned int)tx_ret);
            return 0;
        }
        g_tx_dma_started = 1;
    }

    /* 阻塞方式启动 RX DMA，读取录音数据 */
    if (uapi_i2s_merge_read_by_dma(SIO_BUS_0, rx_tmp, max_samples, &dma_cfg, 0, true) < 0)
    {
        log_info("sound_recorder_read: dma read failed\r\n");
        return 0;
    }

    /* Merge 数据格式：低 16 位为左声道，高 16 位为右声道。*/
    size_t out_idx = 0;
    for (size_t i = 0; i < max_samples; ++i)
    {
        uint32_t frame = rx_tmp[i];
        buf[out_idx++] = (uint16_t)(frame & 0xFFFF); // L
        buf[out_idx++] = (uint16_t)(frame & 0xFFFF); // R
        if ((i & 0x3F) == 0)
        { /* 每 64 个采样踢一次狗 */
            uapi_watchdog_kick();
        }
    }
    uapi_watchdog_kick();
    return max_samples * 2;

#else
    /* --------------------- 旧中断+轮询实现 ---------------------- */

    /* 设置录音缓冲区信息，供回调使用 */
    g_rec_buf = buf;
    g_rec_max_samples = max_samples;
    g_rec_frames = 0;

    /* 触发 I2S RX，不断向 TX 发送 dummy 数据驱动时钟 */
    uint32_t dummy[32] = {0};

    while (g_rec_frames < max_samples)
    {
        size_t remain_frames = max_samples - g_rec_frames;
        size_t frames_to_send = (remain_frames < 32) ? remain_frames : 32;
        i2s_tx_data_t tx_dyn = {dummy, dummy, frames_to_send};
        uapi_i2s_write_data(SIO_BUS_0, &tx_dyn);
        osal_udelay(20);
    }

    return g_rec_frames;
#endif
}

void sound_recorder_close(void)
{
    recorder_hw_close();
#if defined(CONFIG_I2S_SUPPORT_DMA)
    if (g_dma_rx_buf)
    {
        osal_kfree(g_dma_rx_buf);
        g_dma_rx_buf = NULL;
    }
    if (g_dma_tx_dummy)
    {
        osal_kfree(g_dma_tx_dummy);
        g_dma_tx_dummy = NULL;
    }
    g_dma_buf_frames = 0;
    g_dma_attr_configured = 0;
    g_tx_dma_started = 0;
#endif
}