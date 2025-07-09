#ifndef _EPD_H_
#define _EPD_H_

#include <stddef.h>
#include <stdint.h>

#define EPD_ReadBusy uapi_gpio_get_val(13)
#define EPD_RES_Clr() uapi_gpio_set_val(14, GPIO_LEVEL_LOW)
#define EPD_RES_Set() uapi_gpio_set_val(14, GPIO_LEVEL_HIGH)

#define EPD_SCL_Clr() uapi_gpio_set_val(7, GPIO_LEVEL_LOW)
#define EPD_SCL_Set() uapi_gpio_set_val(7, GPIO_LEVEL_HIGH)

#define EPD_SDA_Clr() uapi_gpio_set_val(9, GPIO_LEVEL_LOW)
#define EPD_SDA_Set() uapi_gpio_set_val(9, GPIO_LEVEL_HIGH)

#define EPD_DC_Clr() uapi_gpio_set_val(10, GPIO_LEVEL_LOW)
#define EPD_DC_Set() uapi_gpio_set_val(10, GPIO_LEVEL_HIGH)

#define EPD_CS_Clr() uapi_gpio_set_val(8, GPIO_LEVEL_LOW)
#define EPD_CS_Set() uapi_gpio_set_val(8, GPIO_LEVEL_HIGH)

#define EPD_W 800
#define EPD_H 272

#define WHITE 0xFF
#define BLACK 0x00

#define ALLSCREEN_GRAGHBYTES 27200 / 2

#define Source_BYTES 400 / 8
#define Gate_BITS 272
#define ALLSCREEN_BYTES Source_BYTES *Gate_BITS

// Procedure definitions
void EPD_READBUSY(void);
void EPD_HW_RESET(void);
void EPD_DeepSleep(void);
void EPD_Init(void);
void EPD_SetRAMMP(void);
void EPD_SetRAMMA(void);
void EPD_SetRAMSP(void);
void EPD_SetRAMSA(void);
void EPD_Display_Clear(void);                                    // 清屏
void EPD_Display(const uint8_t *ImageBW, const uint8_t *ImageR); // 全部加载
void EPD_Display_BW(const uint8_t *ImageBW);                     // 只加载黑白部分
void EPD_Display_R(const uint8_t *ImageR);                       // 只加载红色部分

void EPD_Update(void);       // load lut和update一起进行
void EPD_Load_LUT_BWR(void); // 只进行load lut
void EPD_Load_LUT_BW(void);  // 芯片手册有，但似乎这块屏幕不能进行只刷新黑白
void EPD_Update_BWR(void);   // 只进行update
void EPD_Update_BW(void);    // 实验失败，应该不支持

void EPD_WR_DATA8(uint8_t byte);
void EPD_WR_REG(uint8_t byte);
void EPD_WriteByte(uint8_t byte);

#endif