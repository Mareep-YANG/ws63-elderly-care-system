#include "wm8978.h"
#include "soft_iic.h"
#include "soc_osal.h"
#include "debugUtils.h"
// 本地寄存器值表
static uint16_t WM8978_REGVAL_TBL[58] = {
    0X0000, 0X0000, 0X0000, 0X0000, 0X0050, 0X0000, 0X0140, 0X0000,
    0X0000, 0X0000, 0X0000, 0X00FF, 0X00FF, 0X0000, 0X0100, 0X00FF,
    0X00FF, 0X0000, 0X012C, 0X002C, 0X002C, 0X002C, 0X002C, 0X0000,
    0X0032, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000, 0X0000,
    0X0038, 0X000B, 0X0032, 0X0000, 0X0008, 0X000C, 0X0093, 0X00E9,
    0X0000, 0X0000, 0X0000, 0X0000, 0X0003, 0X0010, 0X0010, 0X0100,
    0X0100, 0X0002, 0X0001, 0X0001, 0X0039, 0X0039, 0X0039, 0X0039,
    0X0001, 0X0001};

// 初始化WM8978
uint8_t WM8978_Init(void)
{
    uint8_t res;
    log_debug("debug1");
    // 初始化IIC
    iic_init();
    log_debug("debug2");

    // 软件复位
    res = WM8978_Write_Reg(0, 0);
    log_debug("res:%0x", res);
    log_debug("debug3");

    if (res)
        return 1;
    log_debug("debug4");

    // 配置为通用设置
    WM8978_Write_Reg(1, 0X1B);    // MIC使能,模拟部分使能,VMID=5K 00011011
    WM8978_Write_Reg(2, 0X1B0);   // 耳机输出使能,BOOST使能 000110110000
    WM8978_Write_Reg(3, 0X6C);    // 扬声器输出使能,混音器使能0000000001101100
    WM8978_Write_Reg(6, 0);       // MCLK由外部提供
    WM8978_Write_Reg(43, 1 << 4); // 反相,扬声器增强
    WM8978_Write_Reg(47, 1 << 8); // 左通道MIC增益20倍放大
    WM8978_Write_Reg(48, 1 << 8); // 右通道MIC增益20倍放大
    WM8978_Write_Reg(49, 1 << 1); // 温度保护使能
    WM8978_Write_Reg(10, 1 << 3); // 软静音关闭,128x过采样
    WM8978_Write_Reg(14, 1 << 3); // ADC 128x过采样使能

    WM8978_Write_Reg(7, 0 << 1 | 0 << 2 | 0 << 3);
    log_debug("debug5");

    // 48k采样率
    return 0;
}

// 写WM8978寄存器
uint8_t WM8978_Write_Reg(uint8_t reg, uint16_t val)
{
    iic_start();
    iic_send_byte((WM8978_ADDR << 1) | 0); // 发送器件地址+写命令
    if (iic_wait_ack())
        return 1;

    iic_send_byte((reg << 1) | ((val >> 8) & 0X01)); // 发送寄存器地址
    if (iic_wait_ack())
        return 2;

    iic_send_byte(val & 0XFF); // 发送低8位数据
    if (iic_wait_ack())
        return 3;

    iic_stop();
    WM8978_REGVAL_TBL[reg] = val; // 保存到本地表
    return 0;
}

// 配置ADC/DAC
void WM8978_ADDA_Cfg(uint8_t dacen, uint8_t adcen)
{
    uint16_t regval;
    regval = WM8978_REGVAL_TBL[3];
    if (dacen)
        regval |= 3 << 0; // 开启DACR&DACL
    else
        regval &= ~(3 << 0); // 关闭DACR&DACL
    WM8978_Write_Reg(3, regval);

    regval = WM8978_REGVAL_TBL[2];
    if (adcen)
        regval |= 3 << 0; // 开启ADCR&ADCL
    else
        regval &= ~(3 << 0); // 关闭ADCR&ADCL
    WM8978_Write_Reg(2, regval);
}

// 配置输入通道
void WM8978_Input_Cfg(uint8_t micen, uint8_t lineinen, uint8_t auxen)
{
    uint16_t regval;
    regval = WM8978_REGVAL_TBL[2];
    if (micen || lineinen)
        regval |= 3 << 2; /* MICL/MICR 电源 */
    else
        regval &= ~(3 << 2);
    WM8978_Write_Reg(2, regval);

    regval = WM8978_REGVAL_TBL[44]; // 读取R44
    if (micen)
        regval |= 3 << 4 | 3 << 0; // 开启LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
    else
        regval &= ~(3 << 4 | 3 << 0); // 关闭LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
    if (lineinen)
    {
        regval |= 1 << 2 | 1 << 6; // 关闭linein
        regval |= 1 << 5 | 1 << 1;
        WM8978_LINEIN_Gain(7); // LINE IN 0dB增益
    }
    else
    {
        regval &= ~(1 << 2 | 1 << 6); // 关闭linein
        WM8978_LINEIN_Gain(0);        // 关闭LINE IN
    }

    WM8978_Write_Reg(44, regval); // 设置R44

    if (auxen)
        WM8978_AUX_Gain(7); // AUX 6dB增益
    else
        WM8978_AUX_Gain(0); // 关闭AUX输入
}

// 配置输出通道
void WM8978_Output_Cfg(uint8_t dacen, uint8_t bpsen)
{
    uint16_t regval = 0;
    if (dacen)
        regval |= 1 << 0; // DAC输出使能
    if (bpsen)
    {
        regval |= 1 << 1; // BYPASS使能
        regval |= 5 << 2; // 0dB增益
    }
    WM8978_Write_Reg(50, regval); // R50设置
    WM8978_Write_Reg(51, regval); // R51设置
}

// 设置MIC增益
void WM8978_MIC_Gain(uint8_t gain)
{
    gain &= 0X3F;                        // 限制范围0~63
    WM8978_Write_Reg(45, gain | 1 << 8); // R45,左通道PGA增益
    WM8978_Write_Reg(46, gain | 1 << 8); // R46,右通道PGA增益
}

// 设置耳机音量
void WM8978_HPvol_Set(uint8_t voll, uint8_t volr)
{
    voll &= 0X3F; // 限制范围0~63
    volr &= 0X3F;
    if (voll == 0)
        voll |= 1 << 6; // 音量为0时,直接mute
    if (volr == 0)
        volr |= 1 << 6;
    WM8978_Write_Reg(52, voll);
    WM8978_Write_Reg(53, volr | (1 << 8)); // 同步更新(HPVU=1)
}

// 设置扬声器音量
void WM8978_SPKvol_Set(uint8_t volx)
{
    volx &= 0X3F; // 限制范围0~63
    if (volx == 0)
        volx |= 1 << 6; // 音量为0时,直接mute
    WM8978_Write_Reg(54, volx);
    WM8978_Write_Reg(55, volx | (1 << 8)); // 同步更新
}

// 配置I2S接口
void WM8978_I2S_Cfg(uint8_t fmt, uint8_t len)
{
    fmt &= 0X03;
    len &= 0X03;                                  // 限定范围
    WM8978_Write_Reg(4, (fmt << 3) | (len << 5)); // R4,WM8978工作模式设置
}

// WM8978 L2/R2(也就是Line In)增益设置(L2/R2-->ADC输入部分的增益)
// gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_LINEIN_Gain(uint8_t gain)
{
    uint16_t regval;
    gain &= 0X07;
    regval = WM8978_REGVAL_TBL[47];           // 读取R47
    regval &= ~(7 << 4);                      // 清除原来的设置
    WM8978_Write_Reg(47, regval | gain << 4); // 设置R47
    regval = WM8978_REGVAL_TBL[48];           // 读取R48
    regval &= ~(7 << 4);                      // 清除原来的设置
    WM8978_Write_Reg(48, regval | gain << 4); // 设置R48
}

// WM8978 AUXR,AUXL(PWM音频部分)增益设置(AUXR/L-->ADC输入部分的增益)
// gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_AUX_Gain(uint8_t gain)
{
    uint16_t regval;
    gain &= 0X07;
    regval = WM8978_REGVAL_TBL[47];           // 读取R47
    regval &= ~(7 << 0);                      // 清除原来的设置
    WM8978_Write_Reg(47, regval | gain << 0); // 设置R47
    regval = WM8978_REGVAL_TBL[48];           // 读取R48
    regval &= ~(7 << 0);                      // 清除原来的设置
    WM8978_Write_Reg(48, regval | gain << 0); // 设置R48
}