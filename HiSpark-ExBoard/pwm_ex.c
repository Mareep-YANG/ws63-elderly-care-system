#include "pwm_ex.h"

pwm_config_t pwm_config_1;
uint32_t pwm_points;

static errcode_t pwm_sample_callback(uint8_t channel)
{
    osal_printk("PWM %d, cycle done. \r\n", channel);
    return ERRCODE_SUCC;
}

void PWM_init(void)
{
    pwm_points = PWM_MAIN_FREQUENCY / PWM_FREQUENCY / 100;
    pwm_config_1.high_time = pwm_points * 50;
    pwm_config_1.low_time = pwm_points * 50;
    pwm_config_1.cycles = 0;
    pwm_config_1.offset_time = 0;
    pwm_config_1.repeat = true;
    osal_printk("PWM %d\r\n", pwm_config_1.high_time);

    uapi_pin_set_mode(PWM_GPIO_0, PWM_PIN_MODE);
    uapi_pwm_deinit();
    uapi_pwm_init();
    uapi_pwm_open(PWM_CHANNEL_0, &pwm_config_1);

    uapi_tcxo_delay_ms(1000);
    uapi_pwm_unregister_interrupt(PWM_CHANNEL_0);
    uapi_pwm_register_interrupt(PWM_CHANNEL_0, pwm_sample_callback);

    uint8_t channels[] = {PWM_CHANNEL_0};
    uapi_pwm_set_group(PWM_GROUP_ID, channels, 1);
    uapi_pwm_start_group(PWM_GROUP_ID);
}

void PWM_Update(uint8_t channel, uint16_t duty)
{
    uapi_pwm_close(channel);
    if (channel == PWM_CHANNEL_0)
    {
        pwm_config_1.high_time = pwm_points * duty;
        pwm_config_1.low_time = pwm_points * 100 - pwm_config_1.high_time;
        uapi_pwm_open(channel, &pwm_config_1);
        uapi_pwm_start_group(PWM_GROUP_ID);
    }
}
