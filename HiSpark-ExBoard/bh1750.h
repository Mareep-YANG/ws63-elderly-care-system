#ifndef _BH1750_H_
#define _BH1750_H_

#include <stddef.h>
#include <stdint.h>

#define BH1750_ADDRESS 0x23

// 测量模式
#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2 0x11
#define BH1750_CONTINUOUS_LOW_RES_MODE 0x13
#define BH1750_ONE_TIME_HIGH_RES_MODE 0x20
#define BH1750_ONE_TIME_HIGH_RES_MODE_2 0x21
#define BH1750_ONE_TIME_LOW_RES_MODE 0x23

void BH1750_Init(void);
uint16_t BH1750_ReadLight(void);

#endif