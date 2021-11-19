/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: cnfJson.h
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
#ifndef __CNFJSON_H__
#define __CNFJSON_H__
/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"


#define PWRCTRL_RUN_DIRECTORY		"/ubi/conf/pwrctrl"	
#define CMD_MKDIR_RUN_DIR			"mkdir /ubi/conf/pwrctrl"
#define CMD_CNF_COPY_JSON			"cp /ubi/local/apps/pwrctrl/pwrCnf.json /ubi/conf/pwrctrl/"

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
}MDL_CNF_STRUCT;

typedef struct
{
	unsigned char tempEn;		// 温度.0-未使能, 1-使能
	unsigned char humiEn;		// 湿度
	unsigned char liqEn;		// 液位
	unsigned char atmoEn;		// 气压
	unsigned char gyroEn;		// 陀螺仪
	unsigned char doorEn;		// 行程开关
	unsigned int tempLevel;		// 阈值
	unsigned int humiLevel;
	unsigned int atmoLevel;
}SENSORS_CNF_STRUCT;

typedef struct
{
    unsigned char 	powerType;					// 1-Hand In Hand 2-Matrix In Hand 3-Matrix & Matrix
	unsigned char 	powerCount;					// 枪数量
	unsigned int  	powerMax;					// 功率阈值
	unsigned char 	powerUnitNum;				// 功率单元数量

	unsigned char 	pduRelayCount;				// PDU继电器个数
	unsigned char 	pduTempLevel;				// PDU温度阈值
	unsigned char 	pduType;					// pdu类型
	unsigned char 	pduCount;					// pdu数量

	unsigned char 	mdlType;					// 模块使用种类，1-台达30KW；2-台达12.5KW，3-星星30KW低压， 4-星星30kW高压
	unsigned char 	mdlCount;					// 模块数量
	unsigned int  	mdlOverVol;					// 过压阈值
	unsigned int  	mdlUnderVol;				// 欠压阈值

    unsigned char 	fanCount;					// 风扇数量
    unsigned char 	fanType;					// 风扇类型

	MDL_CNF_STRUCT	mdl[MDL_MAX_NUM];			// 每个模块的数据

	SENSORS_CNF_STRUCT	sensors;				// 传感器配置

	unsigned char 	OTAStep;					// 升级标志 0-未知，1-准备升级，2-升级中，3-升级结束
}PWR_CNF_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

int CNF_SetPwrCnf(PWR_CNF_STRUCT cnf);
int CNF_GetPwrCnf(PWR_CNF_STRUCT *pcnf);

int CNF_Init(void);
void CNF_Uninit(void);

#endif // __CNFJSON_H__
