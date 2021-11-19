/*
* *******************************************************************************
*	@(#)Copyright (C) 2019-2030
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: exportCtrl.h
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan 
* DEPARTMENT	: 
* DATE			: 2020-04-30
* *******************************************************************************
*/
#ifndef __EXPORTCTRL_H__
#define __EXPORT CTRL_H__
/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/
#include "option.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************5dddd\
*/

#define 	OPEN		1		// 
#define 	CLOSE 		0
#define 	OVERTIME	2		// 反馈超时
#define 	WAITING		3		// 反馈等待

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

int GetWarnLed();
int GetErrLed();

int GetCB();
int GetSurging();
int GetDoor();
int GetEStop();
int GetWaterLevel();
int GetACRelay();
int GetFanRelay();

void SetWarnLed(int ctrl);
void SetErrLed(int ctrl);
void SetACRelay(int ctrl);
void SetFanRelay(int ctrl);
void SetFanSpeed(int pwm);

/* 用于180灯板控制 */
void SetRedLed(int ctrl, int idx);
void SetBlueLed(int ctrl, int idx);
void SetGreenLed(int ctrl, int idx);
void SetYellowLed(int ctrl, int idx);
void SetAllLed(int ctrl, int idx);
void SetDoorLED(int ctrl);

int GetRedLed(int idx);
int GetBlueLed(int idx);
int GetGreenLed(int idx);
int GetYellowLed(int idx);
int GetAllLed(int idx);
int GetDoorLed();

void ExportCtrl_Init(void);

#endif
