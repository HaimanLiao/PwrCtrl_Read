/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: smartOps.c
* SYSTEM NAME	: 智能运维
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	: 
* DATE			: 
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "smartOps.h"
#include "statJson.h"
#include "fanCtrl.h"
#include "relayCtrl.h"
#include "exportCtrl.h"
#include "timer.h"
#include "modCtrl.h"
#include "zmqCtrl.h"
#include "groupsCtrl.h"

/*
* *******************************************************************************
*                                  Public variables
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private variables
* *******************************************************************************
*/

#pragma pack(1)
typedef struct
{
	int acBack;
	int pduBack[PDU_MAX_NUM];
	int backCheck;			// 反馈检查，0-未检查，1-检查OK，2-检查失败

	/* Deal中使用的变量 */
	unsigned long statTick;
	unsigned long saveTick;
	unsigned long powerTick;
}SMARTOPS_PRI_STRUCT;
#pragma pack()

#define 	STAT_SAVE_TIME 				(1000*600)			// 一小时
#define 	WAIT_CHECK_TIME 			(300*1000)			// 5min
#define 	WAIT_CHECK_LOOP_TIME 		(20*1000)			// 20s
#define 	WAIT_CHECK_BACK_TIME 		(20*1000)			// 20s

static 		STAT_STRUCT 				g_stat = {0};		// 计算后的数据
static 		STAT_STRUCT 				g_initStat = {0};	// 初始数据
static 		SMARTOPS_PRI_STRUCT			g_pri = {0};
static 		SINGLE_GROUP_FUNCTINS 		*g_pGrpFun = NULL;

/*
* *******************************************************************************
*                                  Private function prototypes
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: FlushStatData
* ABSTRACT	: 从其他地方获取需要的数据并赋给g_stat结构体
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void FlushStatData(STAT_STRUCT *pstat)
{
	int i = 0, j = 0;

	/* 风扇数据获取 */
	FAN_INFO_STRUCT fanInfo = {0};
	if (FanCtrl_GetInfo(&fanInfo, 1) == RESULT_OK)
	{
		pstat->fanTime = g_initStat.fanTime + fanInfo.runTime;
	}
		
	MDL_INFO_STRUCT mdl = {0};
	pstat->mdlCount = 0;
	for (i = 0; i < MDL_MAX_NUM; i++)
	{
		if (Mdl_GetMdlInfo(&mdl, i+1) == RESULT_OK)
		{
			memcpy(pstat->mdl[i].SN, mdl.SN, MDL_SN_MAX_LENGTH);
			pstat->mdl[i].groupId = mdl.groupId;
			pstat->mdl[i].workTime = g_initStat.mdl[i].workTime + mdl.mdlTime;
			pstat->mdlCount++;
		}
	}

	/* PDU数据获取 */
	RELAY_INFO_STRUCT rlyInfo = {0};
	pstat->pduCount = 0;
	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		if (RelayCtrl_GetInfo(&rlyInfo, i+1) == RESULT_OK)
		{
			pstat->pdu[i].no = i + 1;
			for (j = 0; j < SINGLE_PDU_RELAY_NUM; j++)
			{
				pstat->pdu[i].relayTimes[j] = g_initStat.pdu[i].relayTimes[j] 
											+ rlyInfo.swTimes[j];
			}			
			pstat->pduCount++;
		}
	}
}

/*
* *******************************************************************************
* MODULE	: BackCheck
* ABSTRACT	: 
			  
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void BackCheck()
{
	unsigned char sw[6] = {1, 1, 1, 1, 1, 1};
	static unsigned long backTick = 0;
	int i = 0;

	if (backTick == 0)
	{
		backTick = get_timetick();
	}
	
	if (abs(get_timetick() - backTick) < WAIT_CHECK_BACK_TIME)
	{
		/* 先检测前端接触器 */
		if (g_pri.acBack == 0)
		{
			if (GetACRelay() == CLOSE)
			{
				SetACRelay(OPEN);
			}
			if (GetACRelay() == OPEN)
			{
				g_pri.acBack = 1;
			}
		}
		else if (g_pri.acBack == 1)
		{
			sleep(1);
			if (GetACRelay() == OPEN)
			{
				SetACRelay(CLOSE);
			}
			if (GetACRelay() == CLOSE)
			{
				g_pri.acBack = 2;
			}
		}
		else if (g_pri.acBack == 2)
		{
			if (GetACRelay() == CLOSE)
			{
				g_pri.acBack = 3;
			}
		}

		if (GetACRelay() == OVERTIME)
		{
			g_pri.backCheck = 2;
			g_pri.acBack = 1;
		}
		
		/* 再检测PDU反馈 */
		if (g_pri.acBack == 3)
		{
			for (i = 0; i < PDU_MAX_NUM; i++)
			{
				if (g_pri.pduBack[i] == 0)
				{
					if (RelayCtrl_SetSwitch(sw, i+1) == RESULT_OK)
					{
//						usleep(100*1000);
						if (RelayCtrl_GetCtrlBack(i+1) == RELAY_BACK_OK)
						{
							g_pri.pduBack[i] = 1;
						}
						else if (RelayCtrl_GetCtrlBack(i+1) == RELAY_BACK_ERR)
						{
							g_pri.backCheck = 2;
						}
					}
					else
					{
						g_pri.backCheck = 1;
					}

					break;
				}
				else if (g_pri.pduBack[i] == 1)
				{
//					usleep(100*1000);
					memset(sw, 0, sizeof(sw));
					if (RelayCtrl_SetSwitch(sw, i+1) == RESULT_OK)
					{
//						usleep(100*1000);
						if (RelayCtrl_GetCtrlBack(i+1) == RELAY_BACK_OK)
						{
							g_pri.pduBack[i] = 3;
						}
						else if (RelayCtrl_GetCtrlBack(i+1) == RELAY_BACK_ERR)
						{
							g_pri.backCheck = 2;
						}
					}
					break;
				}
			}

			if ((i == PDU_MAX_NUM) && (g_pri.pduBack[i-1] == 3))
			{
				g_pri.backCheck = 1;
			}
		}
	}
	else
	{
		g_pri.backCheck = 2;
	}
	
/*	DEBUG("acBack = %d", g_pri.acBack);
	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		DEBUG("pduBack[%d] = %d", i, g_pri.pduBack[i]);
	}
	DEBUG("backCheck = %d", g_pri.backCheck);*/
}

/*
* *******************************************************************************
* MODULE	: SmartOpsDeal
* ABSTRACT	: 定时保存统计数据
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void SmartOpsDeal()
{
	if (SmartOps_GetBackCheck() != 0)	// 必须先自检
	{
		/* 按周期刷数据 */
		if (abs(get_timetick() - g_pri.statTick) > 1000)
		{
			g_pri.statTick = get_timetick();
			FlushStatData(&g_stat);
		}

		/* 按周期进行保存 */
		if (abs(get_timetick() - g_pri.saveTick) > STAT_SAVE_TIME)
		{
			g_pri.saveTick = get_timetick();
			Stat_SetStatInfo(g_stat);
		}
	}
	else
	{
		BackCheck();
	}
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/
/*
* *******************************************************************************
* MODULE	: SmartOps_HWCheck
* ABSTRACT	: 硬件检测
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int SmartOps_HWCheck(HW_STRUCT hw)
{
	int ret = 0;

	if (hw.acRelay != 0)
	{
		if (hw.acRelay == 1)
		{
			SetACRelay(OPEN);
		}
		else
		{
			SetACRelay(CLOSE);
		}
		
		ret = 1;
	}

	if (hw.fanRelay != 0)
	{
		if (hw.fanRelay == 1)
		{
			SetFanRelay(OPEN);
		}
		else
		{
			SetFanRelay(CLOSE);
		}
		
		ret = 1;
	}

	if ((hw.fanSpeed != 0xFF) && (hw.fanSpeed <= 100))
	{
		FanCtrl_SetSpeed(hw.fanSpeed);
		ret = 1;
	}

	if ((hw.pduId > 0) && (hw.pduId < 13) && (hw.pduRelay != 0))    //暂时最多12个PDU
	{
		if (hw.pduRelayId != 0)
		{
			unsigned char sw[6] = { 0 };
			sw[hw.pduRelayId - 1] = hw.pduRelay == 1 ? 1 : 0;
			if (RelayCtrl_SetSwitch(sw, hw.pduId) == RESULT_OK)
			{
				ret = 1;
			}
		}
		else
		{
			if (hw.pduRelay == 1)
			{
				unsigned char sw1[6] = { 1, 1, 1, 1, 1, 1 };
				if (RelayCtrl_SetSwitch(sw1, hw.pduId) == RESULT_OK)
				{
					ret = 1;
				}
			}
			else
			{
				unsigned char sw2[6] = { 0 };
				if (RelayCtrl_SetSwitch(sw2, hw.pduId) == RESULT_OK)
				{
					ret = 1;
				}
			}
		}

	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SmartOps_GetBackCheck
* ABSTRACT	: 获取上电检查结果,0-未检查，1-检查OK，2-检查失败
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int SmartOps_GetBackCheck()
{
	return g_pri.backCheck;
}

/*
* *******************************************************************************
* MODULE	: SmartOps_GetStatInfo
* ABSTRACT	: 获取统计数据
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SmartOps_GetStatInfo(STAT_STRUCT *pstat)
{
//	DEBUG("%d %d ", g_stat.mdlCount, g_stat.pduCount);
	memcpy(pstat, &g_stat, sizeof(STAT_STRUCT));
}

/*
* *******************************************************************************
* MODULE	: SmartOps_Deal
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SmartOps_Deal()
{
	SmartOpsDeal();
}

/*
* *******************************************************************************
* MODULE	: SmartOps_Init
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SmartOps_Init()
{
	Stat_GetStatInfo(&g_initStat);
	g_pGrpFun = GetSingleGroupFun();

	g_pri.backCheck = 1;	// 暂不做自检
}

/*
* *******************************************************************************
* MODULE	: SmartOps_Uninit
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SmartOps_Uninit()
{

}