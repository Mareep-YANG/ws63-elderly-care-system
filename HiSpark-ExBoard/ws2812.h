#ifndef _WS2812_H_
#define _WS2812_H_
#include <stddef.h>
#include <stdint.h>

#define LED_COUNT 120

#define SPI_ID 0
#define SPI_DI_PIN 11
#define SPI_DO_PIN 6
#define SPI_CLK_PIN 7

#define SPI_SLAVE_NUM 1
#define SPI_FREQUENCY 8
#define SPI_CLK_POLARITY 1
#define SPI_CLK_PHASE 1
#define SPI_FRAME_FORMAT 0
#define SPI_FRAME_FORMAT_STANDARD 0
#define SPI_FRAME_SIZE_8 0x1f
#define SPI_TMOD 0
#define SPI_WAIT_CYCLES 0x10

void spi_gpio_init(void);
void spi_init(void);
void ws2812_send_rgb(uint8_t led_num);
void ws2812_reset(void);
void hsl2rgb(float h, float s, float l, uint8_t *r, uint8_t *g, uint8_t *b);
void rgb2spi(uint16_t r, uint16_t g, uint16_t b, uint16_t now_pos);
void ws2812_next(uint16_t mode,uint16_t red,uint16_t green, uint16_t blue);

#endif