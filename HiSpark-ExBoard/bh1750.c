#include "bh1750.h"
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

void BH1750_Init(void)
{
    uint8_t cmd = 0x01; // 电源开启

    iic_start();
    iic_send_byte(BH1750_ADDRESS << 1);
    iic_wait_ack();
    osal_udelay(150);

    iic_send_byte(cmd);
    iic_wait_ack();
    osal_udelay(150);

    iic_send_byte(BH1750_ONE_TIME_HIGH_RES_MODE);
    iic_wait_ack();
    osal_msleep(120);
    iic_stop();
}

uint16_t BH1750_ReadLight(void)
{
    uint8_t data[2] = {0};

    // 读取光照数据
    iic_start();
    iic_send_byte(BH1750_ADDRESS << 1);
    iic_wait_ack();
    osal_udelay(150);
    iic_send_byte(BH1750_ONE_TIME_HIGH_RES_MODE);
    iic_wait_ack();
    iic_stop();
    osal_msleep(120); // 等待单次测量完成

    iic_start();
    iic_send_byte((BH1750_ADDRESS << 1) | 1);
    iic_wait_ack();

    data[0] = iic_read_byte(1);
    osal_udelay(120);
    data[1] = iic_read_byte(0);
    osal_udelay(120);
    iic_stop();

    // 计算光照强度(lx)
    uint16_t light = ((data[0] << 8) | data[1]) * 10 / 12;

    return light;
}
