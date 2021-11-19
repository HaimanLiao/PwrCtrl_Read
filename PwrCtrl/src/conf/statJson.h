/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: statJson.h
* SYSTEM NAME		: 统计数据的读取和保存
* BLOCK NAME		:
* PROGRAM NAME		:
* MODULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				:
* *******************************************************************************
*/
#ifndef __STATJSON_H__
#define __STATJSON_H__
/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"

#define PWRCTRL_RUN_DIRECTORY		"/ubi/conf/pwrctrl"	
#define CMD_MKDIR_RUN_DIR			"mkdir /ubi/conf/pwrctrl"
#define CMD_STAT_COPY_JSON			"cp /ubi/local/apps/pwrctrl/smartOps.json /ubi/conf/pwrctrl/"

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

typedef struct
{
	unsigned char SN[MDL_SN_MAX_LENGTH];
	unsigned char groupId;
	unsigned int  workTime;
}MDL_STAT_STRUCT;

typedef struct
{
	unsigned char no;
	unsigned int  relayTimes[PDU_RELAY_MAX_NUM];
}PDU_STAT_STRUCT;

typedef struct
{
	MDL_STAT_STRUCT	mdl2[MDL_MAX_NUM];
//	char buffer[50];
	unsigned char 	mdlCount;
	MDL_STAT_STRUCT	mdl[MDL_MAX_NUM];

	unsigned char 	pduCount;
	PDU_STAT_STRUCT	pdu[PDU_MAX_NUM];

	unsigned int	fanTime;				// 风扇运行时间，单位s
}STAT_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

int Stat_SetStatInfo(STAT_STRUCT stat);
int Stat_GetStatInfo(STAT_STRUCT *pstat);

int Stat_Init(void);
void Stat_Uninit(void);

#endif // __STATJSON_H__
