
//
// 移植spi协议5.79寸电子纸，芯片SSD1683，Height：272，Width：792
// BUSY引脚 GPIO13
//

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
#include "EPD.h"

void EPD_READBUSY(void)
{
	for (uint8_t i = 0; i < 200; i++)
	{
		osal_msleep(100);
		if (uapi_gpio_get_val(13) == GPIO_LEVEL_LOW)
		{
			osal_printk("BUSY_LOW\r\n");
			break;
		}
	}
}

// 已移植
void EPD_HW_RESET(void)
{
	osal_msleep(50);
	uapi_gpio_set_val(14, GPIO_LEVEL_LOW);
	osal_msleep(100);
	uapi_gpio_set_val(14, GPIO_LEVEL_HIGH);
	osal_msleep(10);
}

void EPD_WriteByte(uint8_t byte)
{
	uint8_t i;
	EPD_CS_Clr();
	for (i = 0; i < 8; i++)
	{
		EPD_SCL_Clr();
		if (byte & 0x80)
		{
			EPD_SDA_Set();
		}
		else
		{
			EPD_SDA_Clr();
		}
		EPD_SCL_Set();
		byte <<= 1;
	}
	EPD_CS_Set();
}

// Send a byte to the command register
void EPD_WR_REG(uint8_t byte)
{
	uapi_gpio_set_val(10, GPIO_LEVEL_LOW);
	EPD_WriteByte(byte);
	uapi_gpio_set_val(10, GPIO_LEVEL_HIGH);
}

void EPD_WR_DATA8(uint8_t byte)
{
	uapi_gpio_set_val(10, GPIO_LEVEL_HIGH);
	EPD_WriteByte(byte);
	uapi_gpio_set_val(10, GPIO_LEVEL_HIGH);
}

// 已移植
//  Initialize the oled screen
void EPD_Init(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();
	EPD_WR_REG(0x12);
	EPD_READBUSY();
}

void EPD_DeepSleep(void)
{
	EPD_WR_REG(0x10);
	EPD_WR_DATA8(0x01);
	osal_msleep(200);
}

// 完整进行三色LUT加载和更新
// Write the screenbuffer with changed to the screen
void EPD_Update(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xF7);
	osal_msleep(10);
	EPD_WR_REG(0x20);
	osal_msleep(10);
	osal_printk("Updating...\r\n");
	EPD_READBUSY();
}

void EPD_Load_LUT_BWR(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xB1);
	osal_msleep(10);
	EPD_WR_REG(0x20);
	osal_printk("load LUT BWR...\r\n");
	EPD_READBUSY();
}
void EPD_Load_LUT_BW(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xB9);
	osal_msleep(10);
	EPD_WR_REG(0x20);
	osal_printk("load LUT BW...\r\n");
	EPD_READBUSY();
}
void EPD_Update_BWR(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xC7);
	osal_msleep(10);
	EPD_WR_REG(0x20);
	osal_printk("Update BWR...\r\n");
	EPD_READBUSY();
}
void EPD_Update_BW(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xCF);
	osal_msleep(10);
	EPD_WR_REG(0x20);
	osal_printk("Update BW...\r\n");
	EPD_READBUSY();
}

void EPD_SetRAMMP(void)
{
	EPD_WR_REG(0x11);	// Data Entry mode setting
	EPD_WR_DATA8(0x05); // 1 ¨CY decrement, X increment
	EPD_WR_REG(0x44);	// Set Ram X- address Start / End position
	EPD_WR_DATA8(0x00); // XStart, POR = 00h
	EPD_WR_DATA8(0x31); // 400/8-1
	EPD_WR_REG(0x45);	// Set Ram Y- address  Start / End position
	EPD_WR_DATA8(0x0f);
	EPD_WR_DATA8(0x01); // 300-1
	EPD_WR_DATA8(0x00); // YEnd L
	EPD_WR_DATA8(0x00);
}

void EPD_SetRAMMA(void)
{
	EPD_WR_REG(0x4e);
	EPD_WR_DATA8(0x00);
	EPD_WR_REG(0x4f);
	EPD_WR_DATA8(0x0f);
	EPD_WR_DATA8(0x01);
}

void EPD_SetRAMSP(void)
{
	EPD_WR_REG(0x91);
	EPD_WR_DATA8(0x04);
	EPD_WR_REG(0xc4);	// Set Ram X- address Start / End position
	EPD_WR_DATA8(0x31); // XStart, POR = 00h
	EPD_WR_DATA8(0x00);
	EPD_WR_REG(0xc5); // Set Ram Y- address  Start / End position
	EPD_WR_DATA8(0x0f);
	EPD_WR_DATA8(0x01);
	EPD_WR_DATA8(0x00); // YEnd L
	EPD_WR_DATA8(0x00);
}

void EPD_SetRAMSA(void)
{
	EPD_WR_REG(0xce);
	EPD_WR_DATA8(0x31);
	EPD_WR_REG(0xcf);
	EPD_WR_DATA8(0x0f);
	EPD_WR_DATA8(0x01);
}

void EPD_Display_Clear(void)
{
	uint16_t i, j;
	EPD_SetRAMMP();
	EPD_SetRAMMA();
	EPD_WR_REG(0x24);
	for (i = 0; i < Gate_BITS; i++)
	{
		for (j = 0; j < Source_BYTES; j++)
		{
			EPD_WR_DATA8(0xFF);
		}
	}
	EPD_SetRAMMA();
	EPD_WR_REG(0x26);
	for (i = 0; i < Gate_BITS; i++)
	{
		for (j = 0; j < Source_BYTES; j++)
		{
			EPD_WR_DATA8(0x00);
		}
	}
	EPD_SetRAMSP();
	EPD_SetRAMSA();
	EPD_WR_REG(0xA4);
	for (i = 0; i < Gate_BITS; i++)
	{
		for (j = 0; j < Source_BYTES; j++)
		{
			EPD_WR_DATA8(0xFF);
		}
	}
	EPD_SetRAMSA();
	EPD_WR_REG(0xA6);
	for (i = 0; i < Gate_BITS; i++)
	{
		for (j = 0; j < Source_BYTES; j++)
		{
			EPD_WR_DATA8(0x00);
		}
	}
}

void EPD_Display_BW(const uint8_t *ImageBW)
{
	uint32_t i;
	uint8_t tempOriginal;
	uint32_t tempcol = 0;
	uint32_t templine = 0;
	EPD_SetRAMMP();
	EPD_SetRAMMA();
	EPD_WR_REG(0x24);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageBW + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
	EPD_SetRAMSP();
	EPD_SetRAMSA();
	EPD_WR_REG(0xa4); // write RAM for black(0)/white (1)
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageBW + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
}

void EPD_Display_R(const uint8_t *ImageR)
{
	uint32_t i;
	uint8_t tempOriginal;
	uint32_t tempcol = 0;
	uint32_t templine = 0;
	EPD_SetRAMMP();
	EPD_SetRAMMA();
	EPD_WR_REG(0x26);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageR + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
	EPD_SetRAMSP();
	EPD_SetRAMSA();
	EPD_WR_REG(0xA6);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageR + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
}

void EPD_Display(const uint8_t *ImageBW, const uint8_t *ImageR)
{
	uint32_t i;
	uint8_t tempOriginal;
	uint32_t tempcol = 0;
	uint32_t templine = 0;
	EPD_SetRAMMP();
	EPD_SetRAMMA();
	EPD_WR_REG(0x24);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageBW + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
	EPD_SetRAMSP();
	EPD_SetRAMSA();
	EPD_WR_REG(0xa4); // write RAM for black(0)/white (1)
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageBW + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
	tempcol = 0;
	templine = 0;
	EPD_SetRAMMP();
	EPD_SetRAMMA();
	EPD_WR_REG(0x26);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageR + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
	EPD_SetRAMSP();
	EPD_SetRAMSA();
	EPD_WR_REG(0xA6);
	for (i = 0; i < ALLSCREEN_BYTES; i++)
	{
		tempOriginal = *(ImageR + templine * Source_BYTES * 2 + tempcol);
		templine++;
		if (templine >= Gate_BITS)
		{
			tempcol++;
			templine = 0;
		}
		EPD_WR_DATA8(tempOriginal);
	}
}
