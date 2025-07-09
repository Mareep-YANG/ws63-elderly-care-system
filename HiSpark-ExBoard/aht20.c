#include "aht20.h"
#include "my_iic.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <securec.h>
#include "pinctrl.h"
#include "gpio.h"
#include "soc_osal.h"

void AHT20_Init(void)
{
    iic_init();
    uint8_t cmd[3] = {0xBE, 0x08, 0x00}; // 初始化命令

    iic_start();
    iic_send_byte(AHT20_ADDRESS << 1);
    iic_wait_ack();
    iic_send_byte(cmd[0]);
    iic_wait_ack();
    iic_send_byte(cmd[1]);
    iic_wait_ack();
    iic_send_byte(cmd[2]);
    iic_wait_ack();
    iic_stop();

    osal_msleep(10); // 等待初始化完成
}

uint8_t AHT20_ReadData(double *temperature, double *humidity)
{
    uint8_t cmd[3] = {0xAC, 0x33, 0x00}; // 触发测量命令
    uint8_t data[6] = {0};

    // 发送测量命令
    iic_start();
    iic_send_byte(AHT20_ADDRESS << 1);
    if (iic_wait_ack())
    {
        iic_stop();
        return 1;
    }
    iic_send_byte(cmd[0]);
    if (iic_wait_ack())
    {
        iic_stop();
        return 1;
    }
    iic_send_byte(cmd[1]);
    if (iic_wait_ack())
    {
        iic_stop();
        return 1;
    }
    iic_send_byte(cmd[2]);
    if (iic_wait_ack())
    {
        iic_stop();
        return 1;
    }
    iic_stop();

    osal_msleep(80); // 等待测量完成

    // 读取数据
    iic_start();
    iic_send_byte((AHT20_ADDRESS << 1) | 0x01);
    if (iic_wait_ack())
    {
        iic_stop();
        return 1;
    }

    for (int i = 0; i < 6; i++)
    {
        data[i] = iic_read_byte(i == 5 ? 0 : 1);
    }
    iic_stop();

    // 检查状态位
    if ((data[0] & 0x80) == 0x80)
    {
        return 1; // 设备忙
    }

    // 转换数据
    uint32_t hum_raw = ((uint32_t)(data[1] & 0x0F) << 16) | ((uint32_t)data[2] << 8) | ((uint32_t)(data[3] & 0xF0));
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    *humidity = (float)hum_raw * 100.0 / 1048576.0; // 2^20 = 1048576
    *temperature = (float)temp_raw * 200.0 / 1048576.0 - 50.0;

    return 0;
}
