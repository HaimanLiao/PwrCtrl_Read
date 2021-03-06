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
	unsigned char tempEn;		// ??????.0-?????????, 1-??????
	unsigned char humiEn;		// ??????
	unsigned char liqEn;		// ??????
	unsigned char atmoEn;		// ??????
	unsigned char gyroEn;		// ?????????
	unsigned char doorEn;		// ????????????
	unsigned int tempLevel;		// ??????
	unsigned int humiLevel;
	unsigned int atmoLevel;
}SENSORS_CNF_STRUCT;

typedef struct
{
    unsigned char 	powerType;					// 1-Hand In Hand 2-Matrix In Hand 3-Matrix & Matrix
	unsigned char 	powerCount;					// ?????????
	unsigned int  	powerMax;					// ????????????
	unsigned char 	powerUnitNum;				// ??????????????????

	unsigned char 	pduRelayCount;				// PDU???????????????
	unsigned char 	pduTempLevel;				// PDU????????????
	unsigned char 	pduType;					// pdu??????
	unsigned char 	pduCount;					// pdu??????

	unsigned char 	mdlType;					// ?????????????????????1-??????30KW???2-??????12.5KW???3-??????30KW????????? 4-??????30kW??????
	unsigned char 	mdlCount;					// ????????????
	unsigned int  	mdlOverVol;					// ????????????
	unsigned int  	mdlUnderVol;				// ????????????

    unsigned char 	fanCount;					// ????????????
    unsigned char 	fanType;					// ????????????

	MDL_CNF_STRUCT	mdl[MDL_MAX_NUM];			// ?????????????????????

	SENSORS_CNF_STRUCT	sensors;				// ???????????????

	unsigned char 	OTAStep;					// ???????????? 0-?????????1-???????????????2-????????????3-????????????
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
