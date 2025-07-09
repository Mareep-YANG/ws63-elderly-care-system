#ifndef _SSD1363_H_
#define _SSD1361_H_
#include <stdint.h>

#define USE_HORIZONTAL 0  //ÉèÖÃÏÔÊ¾·½Ïò 0£ºÕýÏòÏÔÊ¾£¬1£ºÐý×ª180¶ÈÏÔÊ¾

#define OLED_CMD  0	//Ð´ÃüÁî
#define OLED_DATA 1	//Ð´Êý¾Ý

#define OLED_SDA 15
#define OLED_SCL 16

void OLED_WR_REG(uint8_t reg);//Ð´ÈëÒ»¸öÖ¸Áî
void OLED_WR_Byte(uint8_t dat);//Ð´ÈëÒ»¸öÊý¾Ý
void Column_Address(uint8_t a,uint8_t b);//ÉèÖÃÁÐµØÖ·
void Row_Address(uint8_t a,uint8_t b);//ÉèÖÃÐÐµØÖ·
void OLED_Fill(uint16_t xstr,uint16_t ystr,uint16_t xend,uint16_t yend,uint8_t color);//Ìî³äº¯Êý
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t *s,uint8_t sizey,uint8_t mode);//ÏÔÊ¾ºº×Ö´®
void OLED_ShowChinese16x16(uint8_t x,uint8_t y,uint8_t *s,uint8_t sizey,uint8_t mode);//ÏÔÊ¾16x16ºº×Ö
void OLED_ShowChinese24x24(uint8_t x,uint8_t y,uint8_t *s,uint8_t sizey,uint8_t mode);//ÏÔÊ¾24x24ºº×Ö
void OLED_ShowChinese32x32(uint8_t x,uint8_t y,uint8_t *s,uint8_t sizey,uint8_t mode);//ÏÔÊ¾32x32ºº×Ö
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t sizey,uint8_t mode);//ÏÔÊ¾×Ö·û
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *dp,uint8_t sizey,uint8_t mode);//ÏÔÊ¾×Ö·û´®
uint32_t oled_pow(uint8_t m,uint8_t n);//ÃÝº¯Êý
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t sizey,uint8_t mode);//ÏÔÊ¾ÕûÊý±äÁ¿
void OLED_DrawBMP(uint8_t x,uint8_t y,uint16_t length,uint8_t width,const uint8_t BMP[],uint8_t mode);//ÏÔÊ¾»Ò¶ÈÍ¼Æ¬
void OLED_DrawSingleBMP(uint8_t x,uint8_t y,uint16_t length,uint8_t width,const uint8_t BMP[],uint8_t mode);//ÏÔÊ¾µ¥É«Í¼Æ¬
void OLED_Init(void);

#endif