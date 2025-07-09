#ifndef __SOFT_IIC_H
#define __SOFT_IIC_H

#include <stdint.h>

// I2C引脚定义
#define SCL_PIN 0 // GPIO16作为SCL
#define SDA_PIN 1 // GPIO15作为SDA

// I2C操作宏定义
#define IIC_SCL_Set() uapi_gpio_set_val(SCL_PIN, GPIO_LEVEL_HIGH)
#define IIC_SCL_Clr() uapi_gpio_set_val(SCL_PIN, GPIO_LEVEL_LOW)
#define IIC_SDA_Set() uapi_gpio_set_val(SDA_PIN, GPIO_LEVEL_HIGH)
#define IIC_SDA_Clr() uapi_gpio_set_val(SDA_PIN, GPIO_LEVEL_LOW)
#define READ_SDA uapi_gpio_get_val(SDA_PIN)

// 函数声明
void iic_init(void);
void iic_start(void);
void iic_stop(void);
uint8_t iic_wait_ack(void);

void iic_ack(void);
void iic_nak(void);
void iic_send_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);

#endif