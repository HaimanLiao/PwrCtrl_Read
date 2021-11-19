/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: smartOps.h
* SYSTEM NAME		: 智能运维，数据统计、硬件检测、自检
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200214
* *******************************************************************************
*/

#ifndef __SMARTOPS_H__
#define __SMARTOPS_H__

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"
#include "statJson.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

#define 	POWER_CHECK_ENABLE 		1
#define 	POWER_CHECK_DISABLE		2

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
#pragma pack(1)

typedef struct
{
	char 	warnLed;			// 警告灯，0-未知，1-亮，2-灭
	char 	errLed;				// 故障灯，0-未知，1-亮，2-灭
	char 	acRelay;			// 交流接触器，0-未知，1-闭合，2-断开
	char 	fanRelay;			// 风扇继电器，0-未知，1-闭合，2-断开
	char 	fanSpeed;			// 风扇转速
	char 	pduId;				// PDU编号
	char 	pduRelayId;			// PDU中继电器编号
	char 	pduRelay;			// PDU继电器，0-未知，1-闭合，2-断开
}HW_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

int SmartOps_HWCheck(HW_STRUCT hw);
int SmartOps_GetBackCheck();

void SmartOps_GetStatInfo(STAT_STRUCT *pstat);

void SmartOps_Deal();
void SmartOps_Init();
void SmartOps_Uninit();

#endif // __SMARTOPS_H__
