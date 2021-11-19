/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: export.h
* SYSTEM NAME		: 
* BLOCK NAME		: 
* PROGRAM NAME		: 
* MODULE FORM		: 
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		: 
* DATE				: 
* *******************************************************************************
*/
#ifndef __EXPORT_H__
#define __EXPORT_H__

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/
#include "option.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

/****************扩展IO控制**************/
#define 	MCP23S17	"/dev/spidev0.0"
/* 实际IO控制对应引脚*/
#define 	GPA_DIR		(0x00)	// 0-输出，1-输入；GA7-GA0:00000000
#define 	GPB_DIR		(0xFC)	// GB7-GB0: 11111100

#define 	GPA0		0
#define 	GPA1		1
#define 	GPA2		2
#define 	GPA3		3
#define 	GPA4		4
#define 	GPA5		5
#define 	GPA6		6
#define 	GPA7		7

#define 	GPB0		0
#define 	GPB1		1
#define 	GPB2		2
#define 	GPB3		3
#define 	GPB4		4
#define 	GPB5		5
#define 	GPB6		6
#define 	GPB7		7

/****************IO控制**************/
#define 	GPIO_H		1
#define 	GPIO_L		0

/****************PWM******************/
#define 	PWM_K_5				"PWM5_OUT"	// 显示屏背光
#define 	PWM_K_7				"PWM7_OUT"	// 风扇PWM波

#define 	PWM_K_5_EXPORT			"/sys/class/pwm/pwmchip4/export"
#define		PWM_K_5_PERIOD_PATH		"/sys/class/pwm/pwmchip4/pwm0/period"
#define		PWM_K_5_DUTY_PATH		"/sys/class/pwm/pwmchip4/pwm0/duty_cycle"
#define		PWM_K_5_ENABLE_PATH		"/sys/class/pwm/pwmchip4/pwm0/enable"

#define 	PWM_K_7_EXPORT			"/sys/class/pwm/pwmchip6/export"
#define		PWM_K_7_PERIOD_PATH		"/sys/class/pwm/pwmchip6/pwm0/period"
#define		PWM_K_7_DUTY_PATH		"/sys/class/pwm/pwmchip6/pwm0/duty_cycle"
#define		PWM_K_7_ENABLE_PATH		"/sys/class/pwm/pwmchip6/pwm0/enable"

#define		PWM_PERIOD			"1000000"
#define		PWM_DUTY			"1000000"
#define		PWM_ENABLE			"1"
#define		PWM_DISABLE			"0"

/****************ADC******************/
#define		ADC_CH8				0
#define		ADC_CH9				1
/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/


/*
* *******************************************************************************
*                                  Union
* *******************************************************************************
*/


/*
* *******************************************************************************
*                                  Struct
* *******************************************************************************
*/


/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/
extern double ad_read(int channel);
extern int mcp23s17_write(int fd, unsigned char addr, unsigned char valA,unsigned char valB);
extern int mcp23s17_read(int fd, unsigned char addr, unsigned char *pdata);
extern int mcp23s17_init();
extern int gpio_get_value(unsigned int gpioIndex, unsigned int *value);
extern int gpio_set_value(unsigned int gpioIndex, unsigned int value);
extern int gpio_init(void);
extern int pwm_init(void);
extern int pwm_ctrl(char *pwm, int val);

#endif // __EXPORT_H__
