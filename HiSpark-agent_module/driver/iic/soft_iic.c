#include "soft_iic.h"
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

void SDA_read_mode(void){
    uapi_pin_set_mode(SDA_PIN, PIN_MODE_0);
    uapi_pin_set_pull(SDA_PIN, PIN_PULL_TYPE_UP);
    uapi_gpio_set_dir(SDA_PIN, GPIO_DIRECTION_INPUT);
}

void SDA_send_mode(void){
    uapi_pin_set_mode(SDA_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(SDA_PIN, GPIO_DIRECTION_OUTPUT);
}

void iic_init(void){
    uapi_pin_set_mode(SCL_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(SCL_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(SCL_PIN, GPIO_LEVEL_LOW);
}

void iic_start(void){
    SDA_send_mode();
    IIC_SDA_Set();
    IIC_SCL_Set();
    osal_udelay(5);
    IIC_SDA_Clr();
    osal_udelay(5);
    IIC_SCL_Clr();
    osal_udelay(2);
}

void iic_stop(void){
    SDA_send_mode();
    IIC_SCL_Clr();
    IIC_SDA_Clr();
    osal_udelay(2);
    IIC_SCL_Set();
    osal_udelay(5);
    IIC_SDA_Set();
    osal_udelay(5);
}

uint8_t iic_wait_ack(void){
    uint16_t ucErrTime=0;
	SDA_read_mode();
	IIC_SDA_Set();
    osal_udelay(1);	   
	IIC_SCL_Set();
 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>2500)
		{
			iic_stop();
			return 1;
		}
	}
	IIC_SCL_Clr();
	return 0;
}

void iic_ack(void)
{
	IIC_SCL_Clr();
	SDA_send_mode();
	IIC_SDA_Clr();
	osal_udelay(2);
	IIC_SCL_Set();
	osal_udelay(2);
	IIC_SCL_Clr();
}

void iic_nak(void)
{
	IIC_SCL_Clr();
	SDA_send_mode();
	IIC_SDA_Set();
	osal_udelay(2);
	IIC_SCL_Set();
	osal_udelay(2);
	IIC_SCL_Clr();
}

void iic_send_byte(uint8_t byte)
{                        
    uint8_t t;   
	SDA_send_mode(); 	    
    IIC_SCL_Clr();
    for(t=0;t<8;t++)
    {              
        //IIC_SDA=(txd&0x80)>>7;
		if((byte&0x80)>>7)
			IIC_SDA_Set();
		else
			IIC_SDA_Clr();
		byte<<=1; 	  
		osal_udelay(2);
		IIC_SCL_Set();
		osal_udelay(2);
		IIC_SCL_Clr();
		osal_udelay(2);
    }	 
}

uint8_t iic_read_byte(uint8_t ack)
{
	unsigned char i,receive=0;
	SDA_read_mode();
    for(i=0;i<8;i++ )
	{
        IIC_SCL_Clr();
        osal_udelay(2);
		IIC_SCL_Set();
        receive<<=1;
        if(READ_SDA)receive++;   
		osal_udelay(1);
    }					 
    if (!ack)
        iic_nak();
    else
        iic_ack();
    return receive;
}

