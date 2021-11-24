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

//lhm: zmq接口中调用
int PwrCtrl_StartStop(int ctrl, int stopReason, int gunId);	//lhm: 修改g_chgInfo[GUN_DC_MAX_NUM]和g_pwrPri
															//lhm: （设置枪的停止和启动，其他充电阶段设置由接口内部控制，ctrl是CHG_START或CHG_STOP）
int PwrCtrl_SetChgAllow(int vol, int cur, int pwrMax, int gunPwr, int gunId);	//lhm: 修改g_chgInfo[GUN_DC_MAX_NUM]（设置车端最大输出能力）
int PwrCtrl_SetChgUI(int vol, int cur, int pwr, int gunId);						//lhm: 修改g_chgInfo[GUN_DC_MAX_NUM]（设置枪需求电压、电流、功率）
int PwrCtrl_GetChgInfo(CHG_INFO_STRUCT *pChg, int gunId);
int PwrCtrl_GetTotalPwrOut();						//lhm: 读取g_chgInfo[GUN_DC_MAX_NUM]（获取所有枪输出的总功率）
int PwrCtrl_GetChgIsBusy();							//lhm: 读取g_chgInfo[GUN_DC_MAX_NUM]（所有的枪是否都空闲）

//lhm: main中调用
void PwrCtrl_SetUnitPara(UNIT_PARA_STRUCT unit);	//lhm: 修改g_unitPara以及初始化matrix.c接口---Matrix_Init函数（输入参数 + 调用modCtrl.c接口）
													//lhm: （设置整流柜参数）
void PwrCtrl_Deal();
void PwrCtrl_Init();
void PwrCtrl_Uninit();

//lhm: fault.c中调用（ FaultDeal() ）
void PwrCtrl_EMCStop(int stopReason);						//lhm: 修改、读取g_chgInfo[GUN_DC_MAX_NUM]和g_pwrPri（紧急停止，紧急断开前端接触器）
int PwrCtrl_FaultStop(int stopReason, int gunId);			//lhm: 修改、读取g_chgInfo[GUN_DC_MAX_NUM]和g_pwrPri（整流柜故障停止）

//lhm: matrix.c中调用
int PwrCtrl_GetEVSEPwrMax(int gunId);						//lhm: 读取g_chgInfo[GUN_DC_MAX_NUM]（获得整流柜对于某枪的最大功率）

//lhm: 没有被调用
void PwrCtrl_GetUnitPara(UNIT_PARA_STRUCT *pUnit);
int PwrCtrl_LimitPower(int pwr);					//lhm: 修改g_unitPara（限功率）



#endif // __RELAYCTRL_H__
