#ifndef _MOTOR_H_
#define _MOTOR_H_

// 电机的DIR引脚
#define PIN_DIR_1 9
#define PIN_DIR_2 10
// 舵机引脚
#define PWM_GPIO_1 1
#define PWM_GPIO_2 2
#define PWM_GPIO_3 3

void motor_gpio_init(void);

void motor_forward(void);

void motor_backward(void);

void motor_stop(void);


#endif