void *OledTask(const char *arg);
/* OLED 显示模式定义 */
enum {
    OLED_MODE_IDLE = 0,   /* 默认眼睛动画 */
    OLED_MODE_AUDIO = 1,  /* 录音推流过程中音频动画 */
};

/* 设置 OLED 当前显示模式 */
void OledSetMode(int mode);