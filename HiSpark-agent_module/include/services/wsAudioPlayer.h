#ifndef WS_AUDIO_PLAYER_H
#define WS_AUDIO_PLAYER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int sample_rate; /* Hz */
        int channels;    /* 1 or 2 */
        int bit_depth;   /* 16 only supported currently */
    } audio_format_t;

    /* 启动播放，会根据 format 初始化 I2S/Codec */
    int ws_audio_player_start(const audio_format_t *fmt);

    /* 向播放器喂入 PCM 数据 */
    void ws_audio_player_feed_pcm(const uint8_t *pcm, size_t len);

    /* 暂停/恢复/停止 */
    void ws_audio_player_pause(void);
    void ws_audio_player_resume(void);
    void ws_audio_player_stop(void);

    /* 用于旧流程兼容，根据 opcode 判定 */
    void ws_audio_player_feed(uint8_t opcode, const uint8_t *data, size_t len);

    /* 复位内部所有状态，可在 WS 断开时调用 */
    void ws_audio_player_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* WS_AUDIO_PLAYER_H */