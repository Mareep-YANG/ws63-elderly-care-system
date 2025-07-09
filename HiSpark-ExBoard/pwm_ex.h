#ifndef _PWM_EX_H_
#define _PWM_EX_H_

#include "pwm.h"
#include "tcxo.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "gpio.h"
#include "osal_debug.h"
#include "cmsis_os2.h"

#define PWM_PIN_MODE 1
#define PWM_GROUP_ID 0

#define PWM_GPIO_0 0
#define PWM_CHANNEL_0 0
// PWM频率为80MHz，实现20khz的pwm输出需要使用4000个计数点
// 不能输出1000hz及以下的pwm，channel2，3，4采用手搓
#define PWM_FREQUENCY 10000
#define PWM_MAIN_FREQUENCY 80000000

void PWM_init(void);

void PWM_Update(uint8_t channel, uint16_t duty);

#endif
