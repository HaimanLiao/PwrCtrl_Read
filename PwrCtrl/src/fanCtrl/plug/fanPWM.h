/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: fanPWM.h
* SYSTEM NAME		: PWM风扇
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200519
* *******************************************************************************
*/

#ifndef __FANPWM_H__
#define __FANPWM_H__

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"
#include "../fanCtrl.h"

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

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

FAN_FUNCTIONS_STRUCT *Get_FanPWM_Functions(void);

#endif // __FANEBM_H__