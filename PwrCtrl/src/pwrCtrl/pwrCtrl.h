/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: pwrCtrl.h
* SYSTEM NAME		: 功率控制
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200304
* *******************************************************************************
*/

#ifndef __PWRCTRL_H__
#define __PWRCTRL_H__

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

#define		CHG_START			1	// 启动充电
#define		CHG_STOP			2	// 停止充电

#define 	POWER_CALC_RATIO	100000	// 精度0.1kW

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/

typedef enum
{
	CHG_MAIN_FREE = 0,			// 枪空闲

	CHG_MAIN_START,				// 枪准备启动，进行功率分配
	CHG_MAIN_CHANGE,			// 功率切换中，切入切出
	CHG_MAIN_RUN,				// 枪正常充电中
	CHG_MAIN_RUN_POLICY,		// 枪正常充电中发生了策略分配//lhm: 实际中不支持

	CHG_MAIN_STOP,				// 枪停止中
	CHG_MAIN_EMC_STOP,			// 跟安全类相关的紧急停止
}CHG_MAIN_STATUS_ENUM;

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
	int 	volMax;		// 最大电压
	int 	curMax;		// 最大电流
	int 	pwrMax;		// 最大功率//lhm: 所有枪加起来的最大允许功率
	int 	mdlType;	// 模块类型

	unsigned char 	gunNum;		// 枪数量
	unsigned char 	groupNum;	// 组数量//lhm: 应该是模块的数量
    unsigned char 	relayNum;	// 继电器数量

    char 	pwrType;			// 功率分配模式
	char 	pwrUnitNum;			// 功率柜数量//lhm: 应该是MATRIX的数量（MATRIX2HAND是1个，MATRIX2MATRIX是2个）
}UNIT_PARA_STRUCT;

typedef struct
{
	unsigned char pwrSta;	// 状态，0-未知，1-运行、暂停，2-停止, 3-故障
	unsigned char pwrStep;	// 阶段，参考CHG_MAIN_STATUS_ENUM

	int 	ChgPwrMax;		// 枪端实时功率
	int 	EVPwrMax;		// 车端最大功率
	int 	EVVolMax;		// 车端最大电压
	int 	EVCurMax;		// 车端最大电流

	int 	EVSEPwrMax;		// 车端最大功率//lhm: 应该是桩端最大功率（这个可以修改，用于桩端根据实际情况限制枪的功率输出）
	int 	EVSEVolMax;		// 车端最大电压
	int 	EVSEVolMin;		// 车端最小电压
	int 	EVSECurMax;		// 车端最大电流

	int 	volNeed;		// 需求电压
	int 	curNeed;		// 需求电流
	int 	pwrNeed;		// 需求功率

	int 	volOut;			// 枪输出电压
	int 	curOut;			// 枪输出电流
	int 	pwrOut;			// 枪输出功率

	int 	startTime;		// 充电开始时间

	int 	stopReason;		// DTC故障码

	int 	mode;			// 枪处于高压模式还是低压模式
}CHG_INFO_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/
int PwrCtrl_SetChgAllow(int vol, int cur, int pwrMax, int gunPwr, int gunId);
int PwrCtrl_SetChgUI(int vol, int cur, int pwr, int gunId);
void PwrCtrl_SetUnitPara(UNIT_PARA_STRUCT unit);
void PwrCtrl_GetUnitPara(UNIT_PARA_STRUCT *pUnit);
int PwrCtrl_GetChgInfo(CHG_INFO_STRUCT *pChg, int gunId);
int PwrCtrl_GetTotalPwrOut();
int PwrCtrl_GetChgIsBusy();
int PwrCtrl_LimitPower(int pwr);
int PwrCtrl_StartStop(int ctrl, int stopReason, int gunId);
void PwrCtrl_EMCStop(int stopReason);
int PwrCtrl_FaultStop(int stopReason, int gunId);
int PwrCtrl_GetEVSEPwrMax(int gunId);

void PwrCtrl_Deal();
void PwrCtrl_Init();
void PwrCtrl_Uninit();

#endif // __RELAYCTRL_H__
