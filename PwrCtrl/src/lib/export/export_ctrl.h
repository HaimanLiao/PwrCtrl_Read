/*
* *******************************************************************************
*	@(#)Copyright (C) 2019-2030
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: LED_CTRL
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: zhangjun 
* DEPARTMENT	: 
* DATE			: 2019-5-6 16:38
* *******************************************************************************
*/
#ifndef __EXPORT_CTRL_H__
#define __EXPORT_CTRL_H__
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
/*实际功能对应IO，由SPI、MCP23S17芯片控制 */
#define 	KC_LED3					GPA0	// 输出 LED3
#define 	KC_LED4					GPA1	// 输出 LED4
#define 	KC_LED5	 				GPA2	// 输出 LED5
#define 	KC_LED6					GPA3	// 输出 LED6
#define 	KC_OUT4	 				GPA4	// 输出 OUT4
#define 	KC_OUT3 				GPA5	// 输出 OUT3
#define 	KC_OUT2		 			GPA6	// 输出 OUT2
#define 	KC_OUT1		 			GPA7	// 输出 OUT1

#define 	KC_LED1 				GPB0	// 输出 LED1
#define 	KC_LED2 				GPB1	// 输出 LED2
#define 	IO_VIN6 				GPB2	// 输入 FE1_6
#define 	IO_VIN5 				GPB3	// 输入 FE1_5
#define 	IO_VIN4			 		GPB4	// 输入 FE1_4
#define 	IO_VIN3	 				GPB5	// 输入 FE1_3
#define 	IO_VIN2 				GPB6	// 输入 FE1_2
#define 	IO_VIN1 				GPB7	// 输入 FE1_1

#define		GPA						0
#define		GPB						1

/****************PWM******************/
#define 	PWM_LCD				PWM_K_5		// 显示屏背光
#define 	PWM_FAN				PWM_K_7		// 风扇PWM波

/****************ADC******************/
#define		TEMP				ADC_CH8
#define		LIGHT				ADC_CH9

/*
* *******************************************************************************
* MODULE	: LcdLightCtrl
* ABSTRACT	: 显示屏亮度调节
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
extern int LcdLightCtrl(int pwm);

/*
* *******************************************************************************
* MODULE	: GetLIGHT
* ABSTRACT	: 光照强度
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		:
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
extern int GetLIGHT(void);

/*
* *******************************************************************************
* MODULE	: GetTemp
* ABSTRACT	: 获得环境温度
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
extern int GetTemp(void);

/*
* *******************************************************************************
* MODULE	: GetInputSta
* ABSTRACT	: 获取输入状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-低 1-高
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
extern int GetInputSta(int bit);

/*****************************************************************************
* Function		 : SetOutputSta
* Description	 : port-a or b, bit-控制位, ctrl-0低1高
* Input 		 :
* Output		 :
* Return		 :
* Others		 :
* Record
* 1.Date		 :
*	 Author 	 :
*	 Modification:

*****************************************************************************/
extern void SetOutputSta(int port, int bit, int ctrl);

/*****************************************************************************
* Function		 : Export_Init
* Description	 : pwm,spi的IO芯片任务初始化
* Input 		 :
* Output		 :
* Return		 :
* Others		 :
* Record
* 1.Date		 :
*	 Author 	 :
*	 Modification:

*****************************************************************************/
extern void Export_Init(void);

#endif
