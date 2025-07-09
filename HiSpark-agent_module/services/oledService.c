#include "my_ssd1306_fonts.h"
#include "my_ssd1306.h"

#include "pinctrl.h"
#include "soc_osal.h"
#include "i2c.h"
#include <stdlib.h>
#define I2C_MASTER_PIN_MODE 2   // I2C功能模式
#define I2C_SET_BAUDRATE 400000 // 400kHz
#define I2C_TASK_DURATION_MS 500
#define CONFIG_I2C_MASTER_BUS_ID 1

#include "frame_1.h"
#include "frame_2.h"
#include "frame_3.h"
#include "frame_audio_1.h"
#include "frame_audio_2.h"
#include "frame_audio_3.h"
#include "frame_audio_4.h"
#include "frame_audio_5.h"
#include "frame_audio_6.h"
#include "frame_audio_7.h"
#include "frame_audio_8.h"
#include "frame_audio_9.h"
#include "frame_audio_10.h"
#include "frame_audio_11.h"
#include "frame_audio_12.h"
#include "frame_audio_13.h"
#include "frame_audio_14.h"
#include "oledService.h"
#define FRAME_BUF_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)
#define OPEN_EYE_DELAY_MS 2000
#define HALF_EYE_DELAY_MS 150
#define CLOSE_EYE_DELAY_MS 100
#define MIN_BLINK_INTERVAL_MS 1000    // 1s
#define MAX_BLINK_INTERVAL_MS 5000    // 5s
#define AUDIO_FRAME_DELAY_MS 100

/* OLED 当前模式（默认为 IDLE） */
static volatile int g_oled_mode = OLED_MODE_IDLE;

void OledSetMode(int mode)
{
    g_oled_mode = mode;
}

static void OledInitPins(void)
{
    // 初始化 I2C 引脚为上拉输入输出
    uapi_pin_set_mode(15, I2C_MASTER_PIN_MODE);
    uapi_pin_set_mode(16, I2C_MASTER_PIN_MODE);
    uapi_pin_set_pull(15, PIN_PULL_TYPE_UP);
    uapi_pin_set_pull(16, PIN_PULL_TYPE_UP);
}

static void ShowFrame(const unsigned char *frame)
{
    ssd1306_DrawBitmap(frame, FRAME_BUF_SIZE);
    ssd1306_UpdateScreen();
}

void *OledTask(const char *arg)
{
    (void)arg;
    // 初始化 I2C 控制器
    OledInitPins();
    uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID, I2C_SET_BAUDRATE, 0x0); // 主机地址 0
    // 初始化屏幕并清屏
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    /* 机器人眼睛帧序列与对应延时 */
    const unsigned char *eye_frames[] = {gImage_frame_1, gImage_frame_2, gImage_frame_3, gImage_frame_2};
    uint32_t eye_delays[] = {OPEN_EYE_DELAY_MS, HALF_EYE_DELAY_MS, CLOSE_EYE_DELAY_MS, HALF_EYE_DELAY_MS};
    const uint32_t eye_frameCnt = sizeof(eye_frames) / sizeof(eye_frames[0]);

    /* 音频动画帧序列 */
    const unsigned char *audio_frames[] = {
        gImage_frame_audio_1, gImage_frame_audio_2, gImage_frame_audio_3, gImage_frame_audio_4,
        gImage_frame_audio_5, gImage_frame_audio_6, gImage_frame_audio_7, gImage_frame_audio_8,
        gImage_frame_audio_9, gImage_frame_audio_10, gImage_frame_audio_11, gImage_frame_audio_12,
        gImage_frame_audio_13, gImage_frame_audio_14};
    const uint32_t audio_frameCnt = sizeof(audio_frames) / sizeof(audio_frames[0]);

    while (1)
    {
        if (g_oled_mode == OLED_MODE_AUDIO)
        {
            /* 循环显示音频帧，直到模式切回 IDLE */
            for (uint32_t i = 0; i < audio_frameCnt && g_oled_mode == OLED_MODE_AUDIO; ++i)
            {
                ShowFrame(audio_frames[i]);
                osal_msleep(AUDIO_FRAME_DELAY_MS);
            }
            continue; /* 重新检查模式 */
        }

        /* 为睁眼帧设置 1~5 秒的随机停留时间 */
        uint32_t rnd = (uint32_t)(rand() % (MAX_BLINK_INTERVAL_MS - MIN_BLINK_INTERVAL_MS + 1));
        eye_delays[0] = MIN_BLINK_INTERVAL_MS + rnd;

        for (uint32_t i = 0; i < eye_frameCnt; ++i)
        {
            ShowFrame(eye_frames[i]);
            osal_msleep(eye_delays[i]);
            if (g_oled_mode == OLED_MODE_AUDIO)
            {
                /* 录音开始，立即跳出眼睛动画 */
                break;
            }
        }
    }

    return NULL;
}