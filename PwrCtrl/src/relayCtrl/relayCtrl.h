/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: relayCtrl.h
* SYSTEM NAME		: 功率继电器控制
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200221
* *******************************************************************************
*/

#ifndef __RELAYCTRL_H__
#define __RELAYCTRL_H__

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

#define 	RELAY_SCII				1		// 西埃PDU
#define 	SINGLE_PDU_RELAY_NUM	6		// 单个PDU继电器数量

#define 	RELAY_BACK_OK			1		// 反馈成功
#define 	RELAY_BACK_ERR			2		// 反馈失败
#define 	RELAY_BACK_WAIT			3		// 反馈等待中

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
	unsigned char 	pduNum;			// pdu数量
	unsigned char 	pduRlyNum;		// 单个pdu的继电器数量
	unsigned char 	pduTempLevel;	// pdu温度阈值
    unsigned char 	pduType;		// pdu类型
}RELAY_PARA_STRUCT;

typedef struct
{
	unsigned char id;								// 继电器编号
	unsigned char sw[SINGLE_PDU_RELAY_NUM];			// 继电器反馈状态
	unsigned int  swTimes[SINGLE_PDU_RELAY_NUM];	// 继电器操作次数
	int 		  temp[2];							// 温度
	unsigned char ctrlBack;							// 反馈状态,RESAY_BACK_OK RELAY_BACK_ERR RELAY_BACK_WAIT
	unsigned char online;							// 0-掉线，1-在线
}RELAY_INFO_STRUCT;

typedef struct
{
	void			(*Init)(void);
	void			(*Uninit)(void);
	int 			(*SetSwitch)(unsigned char *pSW, int relayNo);
	int 			(*GetInfo)(RELAY_INFO_STRUCT *pInfo, int relayNo);
}RELAY_FUNCTIONS_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

int RelayCtrl_GetCtrlBack(int relayNo);
int RelayCtrl_SetSwitch(unsigned char *pSW, int relayNo);
int RelayCtrl_GetInfo(RELAY_INFO_STRUCT *pInfo, int relayNo);
void RelayCtrl_SetPara(RELAY_PARA_STRUCT para);

void RelayCtrl_Init();
void RelayCtrl_Uninit();

#endif // __RELAYCTRL_H__
