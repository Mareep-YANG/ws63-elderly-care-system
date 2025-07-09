#include "stddef.h"
#include "stdint.h"
#define TASKS_TEST_TASK_STACK_SIZE 0x2000
#define TASKS_TEST_TASK_PRIO (osPriority_t)(17)
#define TASKS_TEST_DURATION_MS 1000
#define MIC_TASK_STACK_SIZE 0x1000
#define MIC_TASK_PRIO (osPriority_t)(18)
#define PLAY_TASK_STACK_SIZE 0x1000
#define PLAY_TASK_PRIO (osPriority_t)(19)
#define BUF_FRAMES 128 // 128×2 字节 ≈ 4 ms，足够平滑
#define RECORD_SECONDS 5
#define SAMPLE_RATE 48000 /* 16 kHz 采样率 */
#define RECORD_SAMPLES (SAMPLE_RATE * RECORD_SECONDS)

/* ===============================================================
 * SoundService API
 * ==============================================================*/
#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * 录音接口
     * @param buf          用于存放录制数据的缓冲区（单声道 16bit）
     * @param max_samples  缓冲区可容纳的采样点数（单位: 帧）
     * @return             实际录制到缓冲区中的采样点数
     */
    size_t sound_record(uint16_t *buf, size_t max_samples);

    /*
     * 播放接口
     * @param buf      指向待播放的 PCM 数据（单声道 16bit）
     * @param samples  PCM 数据中包含的采样点数（单位: 帧）
     */
    void sound_play(const uint16_t *buf, size_t samples);

    /*
     * 分离式录音接口，用于长时间/分块录音场景，可避免反复初始化 WM8978
     * 使用流程：
     *   1. 调用 sound_recorder_open() 使能 WM8978 与 I2S 录音硬件；
     *   2. 调用 sound_recorder_read() 获取一段录音数据；可重复调用以持续录音；
     *   3. 录音结束后调用 sound_recorder_close() 关闭硬件。
     */

    /* 打开录音硬件，成功返回 0，失败返回负值 */
    int sound_recorder_open(void);

    /* 读取一段录音数据
     * @param buf         用于存放录制数据的缓冲区（单声道 16bit）
     * @param max_samples 本次期望录制的采样点数
     * @return            实际录制到缓冲区中的采样点数
     */
    size_t sound_recorder_read(uint16_t *buf, size_t max_samples);

    /* 关闭录音硬件 */
    void sound_recorder_close(void);

#ifdef __cplusplus
}
#endif