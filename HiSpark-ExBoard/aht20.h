#ifndef _AHT20_H_
#define _AHT20_H_
#include <stddef.h>
#include <stdint.h>

#define AHT20_ADDRESS 0x38

void AHT20_Init(void);
uint8_t AHT20_ReadData(double *temperature, double *humidity);

#endif