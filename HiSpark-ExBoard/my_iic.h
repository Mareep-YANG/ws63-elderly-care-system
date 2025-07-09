#ifndef _MY_IIC_H_
#define _MY_IIC_H_

#include <stddef.h>
#include <stdint.h>

#define SDA_PIN 15
#define SCL_PIN 16

#define IIC_SCL_Clr() uapi_gpio_set_val(SCL_PIN, GPIO_LEVEL_LOW)
#define IIC_SCL_Set() uapi_gpio_set_val(SCL_PIN, GPIO_LEVEL_HIGH)

#define READ_SDA uapi_gpio_get_val(SDA_PIN)
#define IIC_SDA_Clr() uapi_gpio_set_val(SDA_PIN, GPIO_LEVEL_LOW)
#define IIC_SDA_Set() uapi_gpio_set_val(SDA_PIN, GPIO_LEVEL_HIGH)

void iic_init(void);
void iic_start(void);
void iic_stop(void);
void iic_send_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);
uint8_t iic_wait_ack(void);
void iic_ack(void);
void iic_nak(void);

void SDA_read_mode(void);
void SDA_send_mode(void);

#endif