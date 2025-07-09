#include "ws2812.h"
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
#include "spi.h"

float hue = 0;
uint8_t led_mode_flag = 0;
uint8_t r[LED_COUNT], g[LED_COUNT], b[LED_COUNT];
uint8_t spi_data[LED_COUNT][24];

void spi_gpio_init(void)
{
    uapi_pin_set_mode(SPI_DO_PIN, 6);
    uapi_pin_set_ds(SPI_DO_PIN, 8);
    uapi_pin_set_mode(SPI_CLK_PIN, 3);
}

void spi_init(void)
{
    spi_attr_t config = {0};
    spi_extra_attr_t ext_config = {0};

    config.is_slave = false;
    config.slave_num = SPI_SLAVE_NUM;
    config.bus_clk = 32000000;
    config.freq_mhz = SPI_FREQUENCY;
    config.clk_polarity = SPI_CLK_POLARITY;
    config.clk_phase = SPI_CLK_PHASE;
    config.frame_format = SPI_FRAME_FORMAT;
    config.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
    config.frame_size = HAL_SPI_FRAME_SIZE_8;
    config.tmod = SPI_TMOD;
    config.sste = 1;

    ext_config.qspi_param.wait_cycles = SPI_WAIT_CYCLES;
    ext_config.tx_use_dma = true;
    spi_gpio_init();
    uapi_spi_init(SPI_ID, &config, &ext_config);
}

void ws2812_send_rgb(uint8_t led_num)
{
    spi_xfer_data_t data;
    data.tx_bytes = 24;

    for (uint8_t i = 0; i < led_num; i++)
    {
        data.tx_buff = spi_data[i];
        uapi_spi_master_write(SPI_ID, &data, 0xFFFFFFFF);
    }
}

void ws2812_reset(void)
{
    uint8_t reset_data[300] = {0}; // todo
    spi_xfer_data_t data;
    data.tx_buff = reset_data;
    data.tx_bytes = 300;
    uapi_spi_master_write(SPI_ID, &data, 0xFFFFFFFF);
}

// HSV转RGB函数
void hsl2rgb(float h, float s, float l, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c, x, m;
    float r1, g1, b1;

    if (s == 0)
    {
        *r = *g = *b = (uint8_t)(l * 255);
        return;
    }

    c = (1 - fabs(2 * l - 1)) * s;
    h /= 60;
    x = c * (1 - fabs(fmod(h, 2) - 1));
    m = l - c / 2;

    if (h >= 0 && h < 1)
    {
        r1 = c;
        g1 = x;
        b1 = 0;
    }
    else if (h >= 1 && h < 2)
    {
        r1 = x;
        g1 = c;
        b1 = 0;
    }
    else if (h >= 2 && h < 3)
    {
        r1 = 0;
        g1 = c;
        b1 = x;
    }
    else if (h >= 3 && h < 4)
    {
        r1 = 0;
        g1 = x;
        b1 = c;
    }
    else if (h >= 4 && h < 5)
    {
        r1 = x;
        g1 = 0;
        b1 = c;
    }
    else
    {
        r1 = c;
        g1 = 0;
        b1 = x;
    }

    *r = (uint8_t)((r1 + m) * 255);
    *g = (uint8_t)((g1 + m) * 255);
    *b = (uint8_t)((b1 + m) * 255);
}

void rgb2spi(uint16_t r, uint16_t g, uint16_t b, uint16_t now_pos)
{
    // 转换G分量（高位在前）
    for (int i = 0; i < 8; i++)
    {
        spi_data[now_pos][i] = (g & (1 << (7 - i))) ? 0xF8 : 0xC0;
    }

    // 转换R分量
    for (int i = 0; i < 8; i++)
    {
        spi_data[now_pos][8 + i] = (r & (1 << (7 - i))) ? 0xF8 : 0xC0;
    }

    // 转换B分量
    for (int i = 0; i < 8; i++)
    {
        spi_data[now_pos][16 + i] = (b & (1 << (7 - i))) ? 0xF8 : 0xC0;
    }
}

void ws2812_next(uint16_t mode,uint16_t red,uint16_t green, uint16_t blue)
{
    if (mode == 0)
    {
        if (led_mode_flag != mode)
        {
            hue = 0;
            led_mode_flag = mode;
        }
        for (int i = 0; i < LED_COUNT; i++)
        {
            // float led_hue = fmod(hue + i * 12, 360.0);
            hsl2rgb(hue, 1.0, 0.1, r + i, g + i, b + i);
            rgb2spi(r[i], g[i], b[i], i);
        }
        ws2812_send_rgb(LED_COUNT);
        //        for (int i = 0; i < LED_COUNT; i++)
        //            osal_printk("\t\trgb: %d.%d.%d\n", r[i], g[i], b[i]);
        //        osal_printk("\n\n");
        // ws2812_reset();
        hue = fmod(hue + 1, 360.0); // 色相变化
    }
    else if (mode == 1)
    {
        if (led_mode_flag != mode)
        {
            hue = 0;
            led_mode_flag = mode;
        }
        for (int i = 0; i < LED_COUNT; i++)
        {
            rgb2spi(red, green, blue, i);
        }
        ws2812_send_rgb(LED_COUNT);
        // ws2812_reset();不用清空
    }
}
