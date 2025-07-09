#include "ssd1363.h"
#include "ssd1363_fonts.h"
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

void IIC_delay(void)
{
    //uint8_t t = 1;
    //while (t--)
    //    ;
}

void SDA_OLED_read_mode(void)
{
	uapi_pin_set_mode(OLED_SDA, PIN_MODE_0);
	uapi_pin_set_pull(OLED_SDA, PIN_PULL_TYPE_UP);
	uapi_gpio_set_dir(OLED_SDA, GPIO_DIRECTION_INPUT);
}

void SDA_OLED_send_mode(void)
{
	uapi_pin_set_mode(OLED_SDA, PIN_MODE_0);
	uapi_gpio_set_dir(OLED_SDA, GPIO_DIRECTION_OUTPUT);
}

void iic_OLED_init(void)
{
	uapi_pin_set_mode(OLED_SCL, PIN_MODE_0);
	uapi_gpio_set_dir(OLED_SCL, GPIO_DIRECTION_OUTPUT);
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
}

void iic_OLED_start(void)
{
	SDA_OLED_send_mode();
	uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_HIGH);
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_HIGH);
	IIC_delay();
	uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_LOW);
	IIC_delay();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
	IIC_delay();
}

void iic_OLED_stop(void)
{
	SDA_OLED_send_mode();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
	uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_LOW);
	IIC_delay();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_HIGH);
	IIC_delay();
	uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_HIGH);
	IIC_delay();
}

void iic_OLED_ack(void)
{
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
	SDA_OLED_send_mode();
	uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_LOW);
	IIC_delay();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_HIGH);
	IIC_delay();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
}

void iic_OLED_send_byte(uint8_t byte)
{
	uint8_t t;
	SDA_OLED_send_mode();
	uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
	for (t = 0; t < 8; t++)
	{
		// IIC_SDA=(txd&0x80)>>7;
		if ((byte & 0x80) >> 7)
			uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_HIGH);
		else
			uapi_gpio_set_val(OLED_SDA, GPIO_LEVEL_LOW);
		byte <<= 1;
		IIC_delay();
		uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_HIGH);
		IIC_delay();
		uapi_gpio_set_val(OLED_SCL, GPIO_LEVEL_LOW);
	}
}

void OLED_WR_REG(uint8_t reg)
{
    iic_OLED_start();
    iic_OLED_send_byte(0x78);
    iic_OLED_ack();
    iic_OLED_send_byte(0x00);
    iic_OLED_ack();
    iic_OLED_send_byte(reg);
    iic_OLED_ack();
    iic_OLED_stop();
}

void OLED_WR_Byte(uint8_t dat)
{
    iic_OLED_start();
    iic_OLED_send_byte(0x78);
    iic_OLED_ack();
    iic_OLED_send_byte(0x40);
    iic_OLED_ack();
    iic_OLED_send_byte(dat);
    iic_OLED_ack();
    iic_OLED_stop();
}

void Column_Address(uint8_t a, uint8_t b)
{
    OLED_WR_REG(0x15); // Set Column Address
    OLED_WR_Byte(a + 0x08);
    OLED_WR_Byte(b + 0x08);
}

void Row_Address(uint8_t a, uint8_t b)
{
    OLED_WR_REG(0x75); // Row Column Address
    OLED_WR_Byte(a);
    OLED_WR_Byte(b);
    OLED_WR_REG(0x5C); // Ð´RAMÃüÁî
}

void OLED_Fill(uint16_t xstr, uint16_t ystr, uint16_t xend, uint16_t yend, uint8_t color)
{
    uint8_t x, y;
    xstr /= 4; // column address ¿ØÖÆ4ÁÐ
    xend /= 4;
    Column_Address(xstr, xend - 1);
    Row_Address(ystr, yend - 1);
    for (x = xstr; x < xend; x++)
    {
        for (y = ystr; y < yend; y++)
        {
            OLED_WR_Byte(color);
            OLED_WR_Byte(color);
        }
    }
}

void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t *s, uint8_t sizey, uint8_t mode)
{
    while (*s != 0)
    {
        if (sizey == 16)
            OLED_ShowChinese16x16(x, y, s, sizey, mode);
        else if (sizey == 24)
            OLED_ShowChinese24x24(x, y, s, sizey, mode);
        else if (sizey == 32)
            OLED_ShowChinese32x32(x, y, s, sizey, mode);
        else
            return;
        s += 2;
        x += sizey;
    }
}

void OLED_ShowChinese16x16(uint8_t x, uint8_t y, uint8_t *s, uint8_t sizey, uint8_t mode)
{
    uint8_t i, j, k, DATA1 = 0, DATA2 = 0;
    uint16_t HZnum;                                           
    uint16_t TypefaceNum;                                      
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey; 
    HZnum = sizeof(tfont16) / sizeof(typFNT_GB16);     
    Column_Address(x / 4, x / 4 + sizey / 4 - 1);
    Row_Address(y, y + sizey - 1);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1)))
        {
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    if (tfont16[k].Msk[i] & (0x01 << (j * 4)))
                    {
                        DATA1 = 0x0F;
                    }
                    if (tfont16[k].Msk[i] & (0x01 << (j * 4 + 1)))
                    {
                        DATA1 |= 0xF0;
                    }
                    if (tfont16[k].Msk[i] & (0x01 << (j * 4 + 2)))
                    {
                        DATA2 = 0x0F;
                    }
                    if (tfont16[k].Msk[i] & (0x01 << (j * 4 + 3)))
                    {
                        DATA2 |= 0xF0;
                    }
                    if (mode)
                    {
                        OLED_WR_Byte(~DATA2);
                        OLED_WR_Byte(~DATA1);
                    }
                    else
                    {
                        OLED_WR_Byte(DATA2);
                        OLED_WR_Byte(DATA1);
                    }
                    DATA1 = 0;
                    DATA2 = 0;
                }
            }
        }
        continue; 
    }
}

void OLED_ShowChinese24x24(uint8_t x, uint8_t y, uint8_t *s, uint8_t sizey, uint8_t mode)
{
    uint8_t i, j, k, DATA1 = 0, DATA2 = 0;
    uint16_t HZnum;                                         
    uint16_t TypefaceNum;                                     
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey; 
    HZnum = sizeof(tfont24) / sizeof(typFNT_GB24);            
    Column_Address(x / 4, x / 4 + sizey / 4 - 1);
    Row_Address(y, y + sizey - 1);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s + 1)))
        {
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    if (tfont24[k].Msk[i] & (0x01 << (j * 4)))
                    {
                        DATA1 = 0x0F;
                    }
                    if (tfont24[k].Msk[i] & (0x01 << (j * 4 + 1)))
                    {
                        DATA1 |= 0xF0;
                    }
                    if (tfont24[k].Msk[i] & (0x01 << (j * 4 + 2)))
                    {
                        DATA2 = 0x0F;
                    }
                    if (tfont24[k].Msk[i] & (0x01 << (j * 4 + 3)))
                    {
                        DATA2 |= 0xF0;
                    }
                    if (mode)
                    {
                        OLED_WR_Byte(~DATA2);
                        OLED_WR_Byte(~DATA1);
                    }
                    else
                    {
                        OLED_WR_Byte(DATA2);
                        OLED_WR_Byte(DATA1);
                    }
                    DATA1 = 0;
                    DATA2 = 0;
                }
            }
        }
        continue; 
    }
}

void OLED_ShowChinese32x32(uint8_t x, uint8_t y, uint8_t *s, uint8_t sizey, uint8_t mode)
{
    uint8_t i, j, k, DATA1 = 0, DATA2 = 0;
    uint16_t HZnum;                                            
    uint16_t TypefaceNum;                                      
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);           
    Column_Address(x / 4, x / 4 + sizey / 4 - 1);
    Row_Address(y, y + sizey - 1);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s + 1)))
        {
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    if (tfont32[k].Msk[i] & (0x01 << (j * 4)))
                    {
                        DATA1 = 0x0F;
                    }
                    if (tfont32[k].Msk[i] & (0x01 << (j * 4 + 1)))
                    {
                        DATA1 |= 0xF0;
                    }
                    if (tfont32[k].Msk[i] & (0x01 << (j * 4 + 2)))
                    {
                        DATA2 = 0x0F;
                    }
                    if (tfont32[k].Msk[i] & (0x01 << (j * 4 + 3)))
                    {
                        DATA2 |= 0xF0;
                    }
                    if (mode)
                    {
                        OLED_WR_Byte(~DATA2);
                        OLED_WR_Byte(~DATA1);
                    }
                    else
                    {
                        OLED_WR_Byte(DATA2);
                        OLED_WR_Byte(DATA1);
                    }
                    DATA1 = 0;
                    DATA2 = 0;
                }
            }
        }
        continue;
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey, uint8_t mode)
{
    uint8_t sizex, c, temp = 0, t = 2, DATA1 = 0, DATA2 = 0, m;
    uint16_t i, k, size2;
    x /= 4;
    sizex = sizey / 4 / 2;                                 
    size2 = (sizey / 16 + ((sizey % 16) ? 1 : 0)) * sizey; 
    c = chr - ' ';                                       
    Column_Address(x, x + sizex - 1);                     
    Row_Address(y, y + sizey - 1);                         
    for (i = 0; i < size2; i++)
    {
        if (sizey == 16)
        {
            temp = ascii_1608[c][i];
        }
        else if (sizey == 24)
        {
            temp = ascii_2412[c][i];
        }
        else if (sizey == 32)
        {
            temp = ascii_3216[c][i];
        }
        if (sizey % 16)
        {
            m = sizey / 16 + 1;
            if (i % m)
                t = 1;
            else
                t = 2;
        }
        for (k = 0; k < t; k++)
        {
            if (temp & (0x01 << (k * 4)))
            {
                DATA1 = 0x0F;
            }
            if (temp & (0x01 << (k * 4 + 1)))
            {
                DATA1 |= 0xF0;
            }
            if (temp & (0x01 << (k * 4 + 2)))
            {
                DATA2 = 0x0F;
            }
            if (temp & (0x01 << (k * 4 + 3)))
            {
                DATA2 |= 0xF0;
            }
            if (mode)
            {
                OLED_WR_Byte(~DATA2);
                OLED_WR_Byte(~DATA1);
            }
            else
            {
                OLED_WR_Byte(DATA2);
                OLED_WR_Byte(DATA1);
            }
            DATA1 = 0;
            DATA2 = 0;
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *dp, uint8_t sizey, uint8_t mode)
{
    while (*dp != '\0')
    {
        OLED_ShowChar(x, y, *dp, sizey, mode);
        dp++;
        x += sizey / 2;
    }
}

uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey, uint8_t mode)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (sizey / 2) * t, y, ' ', sizey, mode);
                continue;
            }
            else
                enshow = 1;
        }
        OLED_ShowChar(x + (sizey / 2) * t, y, temp + '0', sizey, mode);
    }
}

void OLED_DrawBMP(uint8_t x, uint8_t y, uint16_t length, uint8_t width, const uint8_t BMP[], uint8_t mode)
{
    uint16_t i, num;
    length = (length / 4 + ((length % 4) ? 1 : 0)) * 4;
    num = length * width / 2 / 2;
    x /= 4;
    length /= 4;
    Column_Address(x, x + length - 1);
    Row_Address(y, y + width - 1);
    for (i = 0; i < num; i++)
    {
        if (mode)
        {
            OLED_WR_Byte(~BMP[i * 2 + 1]);
            OLED_WR_Byte(~BMP[i * 2]);
        }
        else
        {
            OLED_WR_Byte(BMP[i * 2 + 1]);
            OLED_WR_Byte(BMP[i * 2]);
        }
    }
}

// 用这个，勾选字节内像素数据相反
void OLED_DrawSingleBMP(uint8_t x, uint8_t y, uint16_t length, uint8_t width, const uint8_t BMP[], uint8_t mode)
{
    uint8_t t = 2, DATA1 = 0, DATA2 = 0;
    uint16_t i, k, num = 0;
    length = (length / 8 + ((length % 8) ? 1 : 0)) * 8;
    num = length / 8 * width;
    x /= 4;
    length /= 4;
    Column_Address(x, x + length - 1);
    Row_Address(y, y + width - 1);
    for (i = 0; i < num; i++)
    {
        for (k = 0; k < t; k++)
        {
            if (BMP[i] & (0x01 << (k * 4)))
            {
                DATA1 = 0x0F;
            }
            if (BMP[i] & (0x01 << (k * 4 + 1)))
            {
                DATA1 |= 0xF0;
            }
            if (BMP[i] & (0x01 << (k * 4 + 2)))
            {
                DATA2 = 0x0F;
            }
            if (BMP[i] & (0x01 << (k * 4 + 3)))
            {
                DATA2 |= 0xF0;
            }
            if (mode)
            {
                OLED_WR_Byte(~DATA2);
                OLED_WR_Byte(~DATA1);
            }
            else
            {
                OLED_WR_Byte(DATA2);
                OLED_WR_Byte(DATA1);
            }
            DATA1 = 0;
            DATA2 = 0;
        }
    }
}

// OLEDµÄ³õÊ¼»¯
void OLED_Init(void)
{
    iic_OLED_init();
    OLED_WR_REG(0xfd); /*Command Lock*/
    OLED_WR_Byte(0x12);

    OLED_WR_REG(0xAE);

    OLED_WR_REG(0xC1);
    OLED_WR_Byte(0xFF);

    if (USE_HORIZONTAL)
    {
        OLED_WR_REG(0xA0);
        OLED_WR_Byte(0x24);
        OLED_WR_Byte(0x00);

        OLED_WR_REG(0xa2);  ////A2.Display Offset
        OLED_WR_Byte(0x80); ////Offset:: 0~127
    }
    else
    {
        OLED_WR_REG(0xA0);
        OLED_WR_Byte(0x32);
        OLED_WR_Byte(0x00);

        OLED_WR_REG(0xa2);  ////A2.Display Offset
        OLED_WR_Byte(0x20); ////Offset:: 0~127
    }

    OLED_WR_REG(0xca);  ////CA.Set Mux Ratio
    OLED_WR_Byte(0x7f); ////Mux:: 0~255

    OLED_WR_REG(0xad);  ////AD.Set IREF
    OLED_WR_Byte(0x90); ////Select:: Internal

    OLED_WR_REG(0xb3);  ////B3.Set Display Clock Divide Ratio/Oscillator Frequency
    OLED_WR_Byte(0x61); ////DivClk:: 0~255

    OLED_WR_REG(0xb9); ////B9 Default GAMMA

    OLED_WR_REG(0xAF); // Set Display On
}