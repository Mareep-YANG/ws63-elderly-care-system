#include "motor.h"
#include "gpio.h"
#include "pinctrl.h"

void motor_gpio_init(void)
{
    // 电机DIR
    uapi_pin_set_mode(PIN_DIR_1, PIN_MODE_0);
    uapi_gpio_set_dir(PIN_DIR_1, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PIN_DIR_1, GPIO_LEVEL_LOW);
    uapi_pin_set_mode(PIN_DIR_2, PIN_MODE_0);
    uapi_gpio_set_dir(PIN_DIR_2, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PIN_DIR_2, GPIO_LEVEL_LOW);
    // 舵机三路
    uapi_pin_set_mode(PWM_GPIO_1, PIN_MODE_0);
    uapi_gpio_set_dir(PWM_GPIO_1, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PWM_GPIO_1, GPIO_LEVEL_LOW);
    uapi_pin_set_mode(PWM_GPIO_2, PIN_MODE_0);
    uapi_gpio_set_dir(PWM_GPIO_2, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PWM_GPIO_2, GPIO_LEVEL_LOW);
    uapi_pin_set_mode(PWM_GPIO_3, PIN_MODE_0);
    uapi_gpio_set_dir(PWM_GPIO_3, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PWM_GPIO_3, GPIO_LEVEL_LOW);
}

void motor_forward(void)
{
    uapi_gpio_set_val(PIN_DIR_1, GPIO_LEVEL_HIGH);
    uapi_gpio_set_val(PIN_DIR_2, GPIO_LEVEL_LOW);
}

void motor_backward(void)
{
    uapi_gpio_set_val(PIN_DIR_1, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(PIN_DIR_2, GPIO_LEVEL_HIGH);
}

void motor_stop(void)
{
    uapi_gpio_set_val(PIN_DIR_1, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(PIN_DIR_2, GPIO_LEVEL_LOW);
}