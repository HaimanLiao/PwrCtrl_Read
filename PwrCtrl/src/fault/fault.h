/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: fault.h
* SYSTEM NAME		: 故障处理代码
* BLOCK NAME		: 
* PROGRAM NAME		: 
* MODULE FORM		: 
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		: 
* DATE				: 
* *******************************************************************************
*/
#ifndef __FAULT_H__
#define __FAULT_H__
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

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/

#pragma pack(1)

typedef enum
{
	DTC_NORMAL = 0,					// 正常

	DTC_PWRERR = 0x200201,			// 功率控制输出异常
	DTC_ESTOP = 0x200202,			// 功率柜急停故障
	DTC_LIQ = 0x200203,				// 功率柜液位告警
	DTC_DOOR = 0x200204,			// 功率柜门打开
	DTC_OUTSHORT = 0x200205,		// 输出短路
	DTC_OVERVOL = 0x200206,			// 输入过压
	DTC_ACRELAY = 0x200207,			// 输入继电器控制故障
	DTC_UNDERVOL = 0x200208,		// 输入欠压
	DTC_CB = 0x200209,				// 断路器脱扣

	DTC_MDLTEMP = 0x200101,			// 电源模块过温
	DTC_PDUTEMP = 0x200102,			// PDU过温故障
	DTC_FANCOMM = 0x200103,			// 功率控制风扇通信异常
	DTC_FANERR = 0x200104,			// 功率控制风扇故障（报警）
	DTC_UNITTEMP = 0x200105,		// 功率柜过温告警

	DTC_MDLCOMM = 0x200001,			// 电源模块通信故障
	DTC_MDLERR = 0x200002,			// 电源模块报警、故障
	DTC_PDUERR = 0x200003,			// PDU继电器控制故障
	DTC_PDUCOMM = 0x200004,			// PDU通信故障
//	DTC_PDUCOMM = 0x200005,			// 触摸屏故障
//	DTC_PDUCOMM = 0x200006,			// 光感故障
	DTC_STORAGE = 0x200007,			// 功率柜外部存储异常
	DTC_SURGE = 0x200008,			// 输入浪涌保护告警
//	DTC_PDUCOMM = 0x200009,			// 前端交流电表故障
	DTC_HEARTOVERTIME = 0x20000A,	// 与终端通信异常

}DTC_UNIT_ENUM;

typedef enum
{
	STOP_NORMAL = 0,

	STOP_ESTOP = 0x200001,
	STOP_LIQ,
	STOP_DOOR,
	STOP_CB,
	STOP_PDUERR,
	STOP_PDUCOMM,
	STOP_MDLCOMM,
	STOP_ACRELAY,
	STOP_PWRPOLICY,
	STOP_PWRCHANGE,
	STOP_SURGE,

	STOP_HEARTOVERTIME = 0x200101,
	STOP_CHGOVERTIME,

	STOP_MDLTEMP = 0x200201,
	STOP_MDLINTERERR,
	STOP_MDLFANERR,
	STOP_MDLUNDERVOL,
	STOP_MDLOVERVOL,
	STOP_MDLPFCERR,
	STOP_MDLDCDCERR,
	STOP_MDLOUTFUSEERR,
	STOP_MDLINFUSEERR,
	STOP_MDLOUTPWR,
	STOP_MDLOUTOVERVOL,
	STOP_MDLOUTUNDERVOL,
	STOP_MDLHWERR,

}STOPREASON_UNIT_ENUM;

#pragma pack()

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

int Fault_GetEMC();
void Fault_GetDTC(int *pDTC, int gunId);

void Fault_Deal();
void Fault_Init();
void Fault_Uninit();

#endif // __FAULT_H__
