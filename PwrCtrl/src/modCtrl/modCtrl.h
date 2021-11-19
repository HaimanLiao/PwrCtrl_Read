/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: MDLCtrl.h
* SYSTEM NAME		:
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				:
* *******************************************************************************
*/
#ifndef __MDLCTRL_H__
#define __MDLCTRL_H__
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

#define 		DCM_303AA				1				// 台达30KW模块
#define 		DCM_123AA				2				// 台达12.5KW模块
#define 		UR_50060E				3				// 星星充电悠悠绿能30KW模块低压
#define 		UR_75040E				4				// 星星充电悠悠绿能30KW模块高压
#define 		UR_100030SW				5				// 星星充电悠悠绿能30KW模块高低压

#define 		NORMAL_MODE				0				// 默认
#define 		PARALLEL_MODE			2				// 低压模式
#define 		SERIES_MODE 			1				// 高压模式

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/
typedef enum
{
	GROUP_CTRL_FREE,		// 空闲
	GROUP_CTRL_RUN,			// 运行
	GROUP_CTRL_STOP,		// 停止
	GROUP_CTRL_STEADY,		// 切换中
}GROUP_CTRL_ENUM;

typedef enum mdl_fault_enum
{
	MDL_FAULT_FREE=1,		// 空闲
	MDL_FAULT_OUTPUT,		// 输出
	MDL_FAULT_UNDER_VOL,	// 欠压
	MDL_FAULT_OVER_VOL,		// 过压
	MDL_FAULT_OVER_TEMP,	// 过温
	MDL_FAULT_OFFLINE,		// 掉线
}MDL_FAULT_ENUM;

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
	unsigned char 		groupId;
	unsigned char		SN[MDL_SN_MAX_LENGTH];
	unsigned int		kwh;
}SN_STRUCT;

typedef struct
{
	unsigned char 		mdlCount;			// 模块数量
	unsigned char 		mdlType;			// 模块类型
	unsigned int  		mdlOverVol;				// 过压阈值
	unsigned int  		mdlUnderVol;			// 欠压阈值
	SN_STRUCT			mdlSN[MDL_MAX_NUM];		// 模块SN、组号设置
}MDL_PARA_STRUCT;

typedef struct
{
	int 				volMax;				// 最大允许电压
	int 				volMinH;			// 高压模式下最小电压
	int 				volMinL;			// 低压模式下最小电压
	int 				curMax;				// 最大电流
	int 				pwrMax;				// 最大功率
}MDL_ALLOW_STRUCT;

/* MDL real data */
typedef struct
{
	unsigned char 		id;
	unsigned char 		groupId;	// 组ID

	unsigned int		volOut;		// 实时电压
	unsigned int 		curOut;		// 实时电流
	unsigned int 		pwrOut;		// 实时功率

	unsigned int 		temp;		// 温度
	MDL_FAULT_ENUM 		fault;
	unsigned int		volIn;		// 三相电输入电压
	unsigned char		SN[MDL_SN_MAX_LENGTH];		// SN编号

	unsigned int		volNeed;	// 需求电压
	unsigned int 		curNeed;	// 需求电流
	unsigned int 		pwrMax;		// 最大功率

	unsigned int 		mdlTime;	// 充电时间

	unsigned char 		mode;		// 高低压模式
	unsigned int 		sts;		// 模块故障 highTemp温度过高，internalFailure内部故障，fanFailure风扇故障，lowInputVol输入欠压，highInput输入过压，PFCFailure PFC硬件故障，DCDCFailure DCDC硬件缺陷，outputFuase输出熔丝熔断，inputFuse输入熔丝熔断，OPPFault输出功率过高，highOutputVol输出过压，lowOutputVol输出欠压
}MDL_INFO_STRUCT;

typedef struct
{
	unsigned int 		id;			// 组ID

	unsigned int		volOut;		// 实时电压
	unsigned int 		curOut;		// 实时电流

	unsigned int 		mdlNum;				// 这组模块个数
	MDL_INFO_STRUCT 	mdl[GROUP_MDL_NUM];

	unsigned int 		mdlOnlineNum;		// 这组模块实际在线个数

	GROUP_CTRL_ENUM 	ctrl;
	unsigned int		volNeed;
	unsigned int 		curNeed;
	unsigned int 		pwrNeed;
	unsigned int 		pwrMax;

	unsigned char 		mode;		// 高低压模式
	unsigned int 		sts;		// 模块故障 highTemp温度过高，internalFailure内部故障，fanFailure风扇故障，lowInputVol输入欠压，highInput输入过压，PFCFailure PFC硬件故障，DCDCFailure DCDC硬件缺陷，outputFuase输出熔丝熔断，inputFuse输入熔丝熔断，OPPFault输出功率过高，highOutputVol输出过压，lowOutputVol输出欠压
}GROUP_INFO_STRUCT;

typedef struct
{
	void				(*Init)						();
	void				(*Uninit)					();
	void				(*GetMdlInfo)				(MDL_INFO_STRUCT mdl[MDL_MAX_NUM]);
	void		 		(*SetMdlUI)					(MDL_INFO_STRUCT mdl);
	int 				(*SetSN)					(int addr, unsigned char *sn, int groupId, unsigned int kwh);	//顺便把初始时间也放在这里，强盗行为
	void 				(*SetMdlInfo)				(int num);
	void				(*DisConnectMdl)			();						//让所有模块离线
	int					(*GetMdlTotalNum)			(void);					//获取所有在线模块
	int					(*GetGroupIdByAddr)			(int addr);				//根据addr(一般是数组下标),获取模块组号
	unsigned long		(*GetKwhByAddr)				(int addr);				//根据addr获取运行时间
	int					(*GetSingleMdlPowerByVol)	(int group, int vol);	//根据输出电压，获取模块能够输出的功率,分辨率0.1KW
	int					(*GetMdlVolByAddr)			(int addr);				//获取某个模块电压
	int					(*GetMdlCurByAddr)			(int addr);				//获取某个模块电流
	int					(*GetGroupMaxPwr)			(int group);			//获取组最大功率
	int					(*GetGroupEnvMaxPwr)		(int group);			//获取当前环境下能够输出的最大功率
	int					(*GetGroupPwrByVol)			(int group, int vol);	//根据当前输出电压，获取模块能够输出的功率，后面可能要考虑温度
	unsigned long		(*GetGroupKwhByAverage)		(int group);			//获取模组的平均运行时间
	int 				(*GetGroupMaxCurByVolPwr)	(int groupId, int vol, int pwr);	//获取当前模组中能输出的最大电流
	void				(*SetGroupChgSign)			(int groupId, int sign);
	void 				(*SetGroupChgMode)			(int groupId, int mode);		//修改模组的工作模式，高压还是低压。
	void 				(*SetACVoltageLimit)		(int underVol, int overVol);	//设置输入过欠压阀值
}MDL_FUNCTIONS_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

void Mdl_SetPara(MDL_PARA_STRUCT para);
void Mdl_GetPara(MDL_PARA_STRUCT *pMdl);

int Mdl_GetGrpInfo(GROUP_INFO_STRUCT *pGrp, int grpId);
int Mdl_GetMdlInfo(MDL_INFO_STRUCT *pMdl, int mdlId);
void Mdl_GetMdlAllow(int type, MDL_ALLOW_STRUCT *pMdl);

void Mdl_Init();
void Mdl_Uninit();

#endif // __MDLCTRL_H__
