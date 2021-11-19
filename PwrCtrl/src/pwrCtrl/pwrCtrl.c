/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: pwrCtrl
* SYSTEM NAME	: 功率控制
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	:
* DATE			: 20200304
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "pwrCtrl.h"
#include "matrix.h"
#include "groupsCtrl.h"
#include "relayCtrl.h"
#include "timer.h"
#include "exportCtrl.h"
#include "fault.h"

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

#define WAIT_MODULE_POWERON_TIME				30000			// 30秒
#define WAIT_RESTART_POLICY_TIME				(1000*10)		// 5分钟
#define WAIT_ACRELAY_CLOSE_DELAY_TIME			(1000*300)		// 5分钟
#define WAIT_POWER_CHANGE_TIME					15000			// 15s

#pragma pack(1)

typedef struct
{
	unsigned char acRelay;		// 前端接触器状态//lhm: AC输入
	unsigned long acRelayTick;	// 前端接触器闭合时间

}FULSH_DATA_STRUCT;

typedef struct
{
	unsigned char isEMC;		// 0-允许启动 1-不允许启动

	int 		  grpPwrMax;	// 单组模块功率

	unsigned long changeTick[GUN_DC_MAX_NUM];	// 功率分配计时
	unsigned long stopTick;		// 重新进行功率分配计数

	unsigned long chgStopTick[GUN_DC_MAX_NUM];	//lhm: 充电停止时间
	int 		  stopPolicy[GUN_DC_MAX_NUM];	// 0-未进行停止策略分配，1-进行

	unsigned char isStop[GUN_DC_MAX_NUM];		// 0-表示可以启动，1-表示整流柜主动停，此时不能启动，直到终端发了停止

	unsigned long flushTick;
}PWR_PRI_STRUCT;

#pragma pack()

static 	FULSH_DATA_STRUCT		g_flushData = {0};					// 刷新数据
static 	PWR_PRI_STRUCT			g_pwrPri = {0};
static 	UNIT_PARA_STRUCT		g_unitPara = {0};					// 整流柜参数
static 	CHG_INFO_STRUCT			g_chgInfo[GUN_DC_MAX_NUM] = {0};	// 枪数据

static 	CHG_POLICY_RES_STRUCT 	g_resOld = {0};						// 策略分配上一次结果
static 	CHG_POLICY_STRUCT 		g_chgResOld[GUN_DC_MAX_NUM] = {0};	// 策略分配每把枪上一次结果
static 	CHG_POLICY_RES_STRUCT 	g_res = {0};						// 策略分配结果

static 	MDL_ALLOW_STRUCT 		g_mdl = {0};						// lhm: 模块的极限参数（最大电压，最大电流，最大功率这些）
static 	SINGLE_GROUP_FUNCTINS 	*g_pGrpFun = NULL;					// 模块控制函数

static 	pthread_mutex_t 		g_mutex[GUN_DC_MAX_NUM];			// 锁

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
* MODULE	: CheckGunId
* ABSTRACT	: 检查枪号
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int CheckGunId(int gunId)
{
	int ret = RESULT_OK;

	if ((gunId <= 0) || (gunId > g_unitPara.gunNum))
	{
		ret = RESULT_ERR;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: LockInit
* ABSTRACT	: 线程锁初始化
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void LockInit()
{
	int i = 0;

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		pthread_mutex_init(&g_mutex[i], NULL);
	}
}

/*
* *******************************************************************************
* MODULE	: LockDestory
* ABSTRACT	: 线程锁销毁
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void LockDestroy()
{
	int i = 0;

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		pthread_mutex_destroy(&g_mutex[i]);
	}
}

/*
* *******************************************************************************
* MODULE	: Lock
* ABSTRACT	: 线程锁加锁
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void Lock(int gunId)
{
	if (CheckGunId(gunId+1) == RESULT_OK)
	{
		pthread_mutex_lock(&g_mutex[gunId]);
	}
}

/*
* *******************************************************************************
* MODULE	: Unlock
* ABSTRACT	: 线程锁解锁
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void Unlock(int gunId)
{
	if (CheckGunId(gunId+1) == RESULT_OK)
	{
		pthread_mutex_unlock(&g_mutex[gunId]);
	}
}

/*
* *******************************************************************************
* MODULE	: SetChgSta
* ABSTRACT	: 设置枪充电状态
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetChgSta(int gunId, int sta)
{
	Lock(gunId);

	g_chgInfo[gunId].pwrSta = sta;//lhm: 0-未知，1-运行、暂停，2-停止, 3-故障

	Unlock(gunId);

	return 0;
}

/*
* *******************************************************************************
* MODULE	: GetChgStep
* ABSTRACT	: 获得枪充电状态（阶段）//lhm: 参考CHG_MAIN_STATUS_ENUM（pwrCtrl.h）
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static CHG_MAIN_STATUS_ENUM GetChgStep(int gunId)
{
	int ret = 0;

	Lock(gunId);

	ret = g_chgInfo[gunId].pwrStep;

	Unlock(gunId);

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SetChgStep
* ABSTRACT	: 设置枪充电状态（阶段）
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetChgStep(int gunId, CHG_MAIN_STATUS_ENUM sta)
{
	Lock(gunId);

	g_chgInfo[gunId].pwrStep = sta;
	DEBUG("gunId = %d, step = %d", gunId, sta);
	log_send("gunId = %d, step = %d", gunId, sta);
	Unlock(gunId);

	return 0;
}

/*
* *******************************************************************************
* MODULE	: GetUnitAllowVolMax
* ABSTRACT	: 获得允许输出最大电压
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetUnitAllowVolMax()
{
	return g_unitPara.volMax;
}

/*
* *******************************************************************************
* MODULE	: GetUnitAllowVolMin
* ABSTRACT	: 获得允许输出最小电压
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetUnitAllowVolMin()
{
	return g_unitPara.volMin;
}

/*
* *******************************************************************************
* MODULE	: GetUnitAllowPwr
* ABSTRACT	: 获得允许输出功率
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetUnitAllowPwr()
{
	return g_unitPara.pwrMax;
}

/*
* *******************************************************************************
* MODULE	: SetUnitAllowPwr
* ABSTRACT	: 设置允许输出功率
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void SetUnitAllowPwr(int pwr)
{
	g_unitPara.pwrMax = pwr;
}

/*
* *******************************************************************************
* MODULE	: SetEVSEAllow
* ABSTRACT	: 设置桩端最大允许
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void SetEVSEAllow(int pwrMax, int curMax, int gunId)//lhm: 最大允许电压和最大允许电流和整流柜的一样
{
	g_chgInfo[gunId].EVSEPwrMax = pwrMax;
	g_chgInfo[gunId].EVSECurMax = curMax;
	g_chgInfo[gunId].EVSEVolMax = GetUnitAllowVolMax();
	g_chgInfo[gunId].EVSEVolMin = GetUnitAllowVolMin();
}

/*
* *******************************************************************************
* MODULE	: GetEVSEPwrMax
* ABSTRACT	: 获得桩端最大功率
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetEVSEPwrMax(int gunId)
{
	return g_chgInfo[gunId].EVSEPwrMax;
}

/*
* *******************************************************************************
* MODULE	: GetChgVolOut
* ABSTRACT	: 获得枪输出电压
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgVolOut(int gunId)
{
	return g_chgInfo[gunId].volOut;
}

/*
* *******************************************************************************
* MODULE	: GetChgCurOut
* ABSTRACT	: 获得枪输出电流
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgCurOut(int gunId)
{
	return g_chgInfo[gunId].curOut;
}

/*
* *******************************************************************************
* MODULE	: GetChgPwrOut
* ABSTRACT	: 获得枪输出功率
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgPwrOut(int gunId)
{
	g_chgInfo[gunId].pwrOut = GetChgVolOut(gunId) * GetChgCurOut(gunId) / POWER_CALC_RATIO;		// 单位0.1kW

	return g_chgInfo[gunId].pwrOut;
}

/*
* *******************************************************************************
* MODULE	: GetChgCurOut
* ABSTRACT	: 获得枪输出电流//lhm: 设置枪的输出电压和输出电流
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void SetChgUIOut(int gunId, int vol, int cur)
{
	g_chgInfo[gunId].volOut = vol;
	g_chgInfo[gunId].curOut = cur;
}

/*
* *******************************************************************************
* MODULE	: GetChgVolNeed
* ABSTRACT	: 获得枪需求电压
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgVolNeed(int gunId)
{
	return g_chgInfo[gunId].volNeed;
}

/*
* *******************************************************************************
* MODULE	: GetChgCurNeed
* ABSTRACT	: 获得枪需求电流
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgCurNeed(int gunId)
{
	return g_chgInfo[gunId].curNeed;
}

/*
* *******************************************************************************
* MODULE	: GetChgPwrNeed
* ABSTRACT	: 获得枪需求功率
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgPwrNeed(int gunId)
{
	return g_chgInfo[gunId].pwrNeed;
}

/*
* *******************************************************************************
* MODULE	: GetEVCurMax
* ABSTRACT	: 获得车端最大允许电流
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetEVCurMax(int gunId)
{
	return g_chgInfo[gunId].EVCurMax;
}

/*
* *******************************************************************************
* MODULE	: GetEVPwrMax
* ABSTRACT	: 获得车端最大允许功率
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetEVPwrMax(int gunId)
{
	return g_chgInfo[gunId].EVPwrMax;
}

/*
* *******************************************************************************
* MODULE	: GetGunPwrMax
* ABSTRACT	: 获得枪端最大允许功率
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgPwrMax(int gunId)
{
	return g_chgInfo[gunId].ChgPwrMax;
}

/*
* *******************************************************************************
* MODULE	: GetChgMode
* ABSTRACT	: 获得枪高低压模式
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetChgMode(int gunId)
{
	return g_chgInfo[gunId].mode;
}

/*
* *******************************************************************************
* MODULE	: SetChgAllow
* ABSTRACT	: 设置车端最大允许输出能力
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void SetChgAllow(int gunId, int vol, int cur, int pwr)
{
	if ((!g_chgInfo[gunId].EVVolMax) || (!g_chgInfo[gunId].EVCurMax)
									|| (!g_chgInfo[gunId].EVPwrMax))	// 防止车端变化，只需要记录第一次有效值就行
	{
		g_chgInfo[gunId].EVVolMax = vol;
		g_chgInfo[gunId].EVCurMax = cur;
		g_chgInfo[gunId].EVPwrMax = pwr;
		g_chgInfo[gunId].mode = vol > 5000 ? SERIES_MODE : PARALLEL_MODE;
	}
}

/*
* *******************************************************************************
* MODULE	: SetChgPwr
* ABSTRACT	: 设置车端最大允许输出能力
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void SetChgPwrMax(int gunId, int pwr)
{
	g_chgInfo[gunId].ChgPwrMax = pwr;
}

/*
* *******************************************************************************
* MODULE	: SetChgUINeed
* ABSTRACT	: 设置枪需求电压、电流、功率
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void SetChgUINeed(int gunId, int vol, int cur, int pwr)
{
	g_chgInfo[gunId].volNeed = vol;
	g_chgInfo[gunId].curNeed = cur;
	g_chgInfo[gunId].pwrNeed = pwr;
}

/*
* *******************************************************************************
* MODULE	: SetChgStartTime
* ABSTRACT	: 设置枪启动时间
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void SetChgStartTime(int gunId)
{
	g_chgInfo[gunId].startTime = get_timetick();
}

/*
* *******************************************************************************
* MODULE	: GetChgStartTime
* ABSTRACT	: 获得枪启动时间
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static unsigned long GetChgStartTime(int gunId)
{
	return g_chgInfo[gunId].startTime;
}

/*
* *******************************************************************************
* MODULE	: SetChgIsStop
* ABSTRACT	: 设置整流柜主动停止
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetChgIsStop(int gunId, int stopReason)
{
	if ((stopReason >= STOP_ESTOP) && (stopReason <= STOP_MDLHWERR))
	{
		DEBUG("SetChgIsStop %d", gunId);
		g_pwrPri.isStop[gunId] = 1;
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: ClearChgIsStop
* ABSTRACT	: 清除整流柜主动停止
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int ClearChgIsStop(int gunId)
{
	if (g_pwrPri.isStop[gunId])
	{
		DEBUG("ClearChgIsStop %d", gunId);
		log_send("ClearChgIsStop %d", gunId);
	}

	g_pwrPri.isStop[gunId] = 0;
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: SetChgStopReason
* ABSTRACT	: 设置枪停止原因
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetChgStopReason(int gunId, int stopReason)
{
	if (stopReason == 0)
	{
		g_chgInfo[gunId].stopReason = stopReason;
	}
	else if (g_chgInfo[gunId].stopReason == 0)
	{
		SetChgIsStop(gunId, stopReason);
		g_chgInfo[gunId].stopReason = stopReason;
	}
	
	if (stopReason != g_chgInfo[gunId].stopReason)
	{
		DEBUG("gunId = %d, stopReason = %x", gunId, g_chgInfo[gunId].stopReason);
		log_send("gunId = %d, stopReason = %x", gunId, g_chgInfo[gunId].stopReason);
	}

	g_pwrPri.chgStopTick[gunId] = get_timetick();
	
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: CheckChgMdlSts
* ABSTRACT	: 检查枪对应的模块故障//lhm: 获取充电停止的原因（正常停止 or 出现故障）
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int CheckChgMdlSts(int gunId, GROUP_INFO_STRUCT group)//lhm: 这里的group指的是模组，另外调用者自己要确保gunId和group是对应的
{
	int stopReason = STOP_NORMAL;
	if (group.sts)
	{
		DEBUG("gunId = %d, grpId = %d, sts = %x", gunId, group.id, group.sts);
		log_send("gunId = %d, grpId = %d, sts = %x", gunId, group.id, group.sts);
	}

	if ((group.sts >> 0) & 0x01)
	{
		stopReason = STOP_MDLTEMP;
	}
	else if ((group.sts >> 1) & 0x01)
	{
		stopReason = STOP_MDLINTERERR;
	}
	else if ((group.sts >> 2) & 0x01)
	{
		stopReason = STOP_MDLFANERR;
	}
	else if ((group.sts >> 3) & 0x01)
	{
		DEBUG();
//		stopReason = STOP_MDLUNDERVOL;
	}
	else if ((group.sts >> 4) & 0x01)
	{
		DEBUG();
//		stopReason = STOP_MDLOVERVOL;
	}
	else if ((group.sts >> 5) & 0x01)
	{
		stopReason = STOP_MDLPFCERR;
	}
	else if ((group.sts >> 6) & 0x01)
	{
		stopReason = STOP_MDLDCDCERR;
	}
	else if ((group.sts >> 7) & 0x01)
	{
		stopReason = STOP_MDLOUTFUSEERR;
	}
	else if ((group.sts >> 8) & 0x01)
	{
		stopReason = STOP_MDLINFUSEERR;
	}
	else if ((group.sts >> 9) & 0x01)
	{
		stopReason = STOP_MDLOUTPWR;
	}
	else if ((group.sts >> 10) & 0x01)
	{
//		stopReason = STOP_MDLOUTOVERVOL;
	}
	else if ((group.sts >> 11) & 0x01)
	{
//		stopReason = STOP_MDLOUTUNDERVOL;
	}
	else if ((group.sts >> 12) & 0x01)
	{
		stopReason = STOP_MDLHWERR;
	}

	return stopReason;
}

/*
* *******************************************************************************
* MODULE	: GetPwrRatio
* ABSTRACT	: 限功率，获取功率系数//lhm: 实际输出功率占最大总允许功率的百分之几
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: N * 100 , <100说明超功率了，>100没有超功率
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetPwrRatio()
{
	int pwrRatio = 0;

	if (PwrCtrl_GetTotalPwrOut())
	{
		pwrRatio = GetUnitAllowPwr() * 100 / PwrCtrl_GetTotalPwrOut();
	}
	
	return pwrRatio;
}

/*
* *******************************************************************************
* MODULE	: FlushOtherInfo
* ABSTRACT	: 获取其他功能模块的数据，进行赋值
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void FlushOtherInfo()
{
	int i, j;
	int gunVol, gunCur;
	int grpNum, grpVol, grpCur, grpId[GROUP_MAX_NUM];
	int pwrMax, cur, curMax;
	GROUP_INFO_STRUCT group;
	int stopReason = 0;
	int acRelayFlag = 0;

	/* 前端接触器控制 */
	if (g_pwrPri.isEMC == 0)
	{
		for (i = 0; i < g_unitPara.gunNum; i++)
		{
			if ((GetChgStep(i) != CHG_MAIN_FREE) && (GetChgStep(i) != CHG_MAIN_STOP)
				&& (GetChgStep(i) != CHG_MAIN_EMC_STOP))
			{
				acRelayFlag = 1;
			}
		}

		if (acRelayFlag)	// 有枪输出
		{
			if (GetACRelay() != OPEN)
			{
				SetACRelay(OPEN);
			}

			if (g_flushData.acRelay == 0)
			{
				g_flushData.acRelay = 1;
				g_flushData.acRelayTick = get_timetick();
			}
		}
		else	// 枪全部停止了,等一会再断开前端接触器
		{
			if (g_flushData.acRelay)
			{
				g_flushData.acRelay = 0;
				g_flushData.acRelayTick = get_timetick();
			}
			else if ((abs(get_timetick() - g_flushData.acRelayTick) > WAIT_ACRELAY_CLOSE_DELAY_TIME)
				&& g_flushData.acRelayTick)
			{
				if (GetACRelay() != CLOSE)
				{
					SetACRelay(CLOSE);
				}
				g_flushData.acRelay = 0;
				g_flushData.acRelayTick = 0;
			}
		}
	}
	else	// EMC紧急停止，立即关闭前端接触器
	{
		if (GetACRelay() != CLOSE)
		{
			SetACRelay(CLOSE);
		}
		g_flushData.acRelay = 0;
		g_flushData.acRelayTick = 0;
	}


	if (GetACRelay() == OPEN)
	{
		g_pGrpFun->SetGroupChgSign(1);//lhm: 应该是相当于通知“模块可操作”
	}
	else
	{
		g_pGrpFun->SetGroupChgSign(0);
	}

	/* 刷新枪端输出电压电流数据，刷新模块故障 */
	for (i = 0; i < g_unitPara.gunNum; i++)
	{
		grpNum = 0;
		if (GetChgStep(i) == CHG_MAIN_FREE)
		{
			grpNum = 0;
		}
		else if (GetChgStep(i) == CHG_MAIN_RUN)
		{
			grpNum = g_res.resAry[i].groupNum;
			for (j = 0; j < grpNum; j++)
			{
				grpId[j] = g_res.resAry[i].groupId[j];
			}
		}
		else if (GetChgStep(i) == CHG_MAIN_STOP)					// 结束充电，模块全部释放状态
		{
			if (!g_pwrPri.stopPolicy[i])		// 需要先等待策略结束
			{
				continue;
			}

			grpNum = g_chgResOld[i].freeGrpNum;
			for (j = 0; j < grpNum; j++)
			{
				grpId[j] = g_chgResOld[i].freeGrpId[j];
				g_pGrpFun->GetGroupUI(grpId[j], &grpVol, &grpCur);

//				DEBUG("Flush CHG_MAIN_STOP i = %d, grpId = %d, grpVol = %d, grpCur = %d", i, grpId[j], grpVol, grpCur);
			}
		}
		else if (GetChgStep(i) == CHG_MAIN_RUN_POLICY)				// 此时res被覆盖，需要使用上一次的结果计算
		{
			grpNum = g_chgResOld[i].groupNum;
			for (j = 0; j < grpNum; j++)
			{
				grpId[j] = g_chgResOld[i].groupId[j];
				g_pGrpFun->GetGroupUI(grpId[j], &grpVol, &grpCur);		// 获取该组模块电压、电流

//				DEBUG("Flush CHG_MAIN_RUN_POLICY i = %d, grpId = %d, grpVol = %d, grpCur = %d", i, grpId[j], grpVol, grpCur);
			}
		}
		else if (GetChgStep(i) == CHG_MAIN_CHANGE)					// 正在切入切出的过程中
		{
			grpNum = g_chgResOld[i].groupNum;
			for (j = 0; j < grpNum; j++)
			{
				grpId[j] = g_chgResOld[i].groupId[j];
				g_pGrpFun->GetGroupUI(grpId[j], &grpVol, &grpCur);		// 获取该组模块电压、电流

//				DEBUG("Flush CHG_MAIN_CHANGE i = %d, grpId = %d, grpVol = %d, grpCur = %d", i, grpId[j], grpVol, grpCur);
			}
		}

		gunVol = 0;
		gunCur = 0;
		for (j = 0; j < grpNum; j++)
		{
			g_pGrpFun->GetGroupUI(grpId[j], &grpVol, &grpCur);		// 获取该组模块电压、电流
			gunVol = gunVol < grpVol ? grpVol : gunVol;				// 电压取较大值
			gunCur += grpCur;										// 电流累加计算

			if ((abs(get_timetick() - g_flushData.acRelayTick) > 10000) 
				&& (g_flushData.acRelay == 1))						// 前端接触器打开的时候判断
			{
				group.id = grpId[j];
				g_pGrpFun->GetGroupInfo(&group);
				stopReason = CheckChgMdlSts(i, group);				// 检查模块故障停止
				if (stopReason)
				{
					SetChgStep(i, CHG_MAIN_STOP);
					SetChgStopReason(i, stopReason);
				}
			}
		}

		if ((GetChgStep(i) != CHG_MAIN_FREE) && (GetChgStep(i) != CHG_MAIN_RUN)
			&& (GetChgStep(i) != CHG_MAIN_START))
		{
//			DEBUG("Flush CHG i = %d, gunVol = %d, gunCur = %d", i, gunVol, gunCur);
		}
		
		SetChgUIOut(i, gunVol, gunCur);
	}

	/* 刷新桩端最大，处理限功率 */
	for (i = 0; i < g_unitPara.gunNum; i++)
	{
		if (GetChgStep(i) == CHG_MAIN_FREE)
		{
			continue;
		}

		cur = 0;
		curMax = 0;
		pwrMax = 0;
		grpNum = 0;

		for (j = 0; j < g_res.resAry[i].groupNum; j++)		// 循环模组
		{
			grpId[0] = g_res.resAry[i].groupId[j];
			if (g_pGrpFun->GetGroupOnline(grpId[0]) == RESULT_OK)
			{
				grpNum++;
				if (cur == 0)
				{
					cur = g_pGrpFun->GetGroupMaxCurByVol(grpId[0], GetChgVolOut(i));
				}
			}
			else
			{
				DEBUG();
			}
		}

		if (GetChgVolOut(i) >= GetUnitAllowVolMin())
		{
			curMax = cur / 100 * 100 * grpNum;		// 降低电流精度1A
		}
		else
		{
			cur = g_mdl.curMax;
			curMax = cur * grpNum;
		}

		if (g_res.resAry[i].groupNum)
		{
			pwrMax = g_res.resAry[i].pwrMax * grpNum / g_res.resAry[i].groupNum;
		}
		else
		{
			pwrMax = g_res.resAry[i].pwrMax;
		}

		if ((GetPwrRatio() < 100) && (GetPwrRatio() > 0))		// 限功率了
		{
			DEBUG("ratio = %d", GetPwrRatio());
		//	pwrMax = pwrMax * GetPwrRatio() / 100;
			curMax = curMax * GetPwrRatio() / 100;
		}

		if (pwrMax > GetChgPwrMax(i))
		{
			curMax = GetChgPwrMax(i) * curMax / pwrMax;
		}

		SetEVSEAllow(pwrMax, curMax, i);
	}
}

/*
* *******************************************************************************
* MODULE	: WaitMdlPowerOn
* ABSTRACT	: 刚充电，模块处于断电状态，需要等待模块上线
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int WaitMdlPowerOn()
{
	int ret = RESULT_OK;
	int onlineCount = 0;
	int offlineCount = 0;
	int i = 0;

	for (i = 0; i < g_unitPara.groupNum; i++)		// 循环模组
	{
		if (g_pGrpFun->GetGroupOnline(i+1) == RESULT_OK)
		{
			onlineCount++;
			Matrix_SetGroupSta(i+1, 0);		// 在线
		}
		else
		{
			offlineCount++;
			Matrix_SetGroupSta(i+1, 1);		// 离线
		}
	}

//	DEBUG("onlineCount = %d, offlineCount = %d", onlineCount, offlineCount);
	/* 判断前端接触器是否闭合以及闭合时间，如果闭合时间超过了30s，则不在一直等待模块上线，否则会强制等待模块上线直到超时 */
	if (g_flushData.acRelay) 		// 早就上电了
	{
		if (abs(get_timetick() - g_flushData.acRelayTick) > WAIT_MODULE_POWERON_TIME)
		{
			if (onlineCount)
			{
				ret = onlineCount;
			}
			else
			{
				ret = RESULT_ERR;
			}
		}
	}

	if (offlineCount == 0)		// 模块已经全部在线
	{
		ret = onlineCount;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ModGrpChangeDo
* ABSTRACT	: 策略分配完成后，进行切入或切出操作，根据枪号操作
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int ModGrpChangeDo(CHG_POLICY_RES_STRUCT res, int gunId)
{
	int ret = RESULT_ERR;
	int j = 0;
	CHG_POLICY_STRUCT resAry;

	int grpId, grpVol, grpCur, chgVol, grpNum, resGrpId[GROUP_MAX_NUM];
	int ctrlGroupCount, tmpGrpCount, ctrlRlyCount, tmpRlyCount;

	memcpy(&resAry, &res.resAry[gunId], sizeof(CHG_POLICY_STRUCT));

	tmpGrpCount = 0;
	if (gunId == resAry.gunId - 1)
	{
		if (resAry.gunRes == CHANGE_OUT)				// 模块切出
		{
			grpNum = resAry.freeGrpNum;
			for (j = 0; j < grpNum; j++)
			{
				resGrpId[j] = resAry.freeGrpId[j];
			}
		}
		else if (resAry.gunRes == CHANGE_IN)			// 模块切入
		{
			grpNum = resAry.addGrpNum;
			for (j = 0; j < grpNum; j++)
			{
				resGrpId[j] = resAry.addGrpId[j];
			}
		}
		else
		{
			return ret;
		}

		if (resAry.gunRes == CHANGE_IN)		// 模块切入，先判断模块是否处于同一高低压模式，如果不是则关机设置模式，还要判断模块是否处于空闲状态
		{
			GROUP_INFO_STRUCT group;
			int modeAll = 0;
			int modeUsed = 0;
			for (j = 0; j < grpNum; j++)
			{
				group.id = resGrpId[j];
				g_pGrpFun->GetGroupInfo(&group);
				if (group.mode != GetChgMode(gunId))		// 先判断模块是否处于同一高低压模式
				{
//					DEBUG("gunId = %d, mode = %d, %d", gunId, group.mode, GetChgMode(gunId));
					g_pGrpFun->SetGroupUI(group.id, 0, 0);
					g_pGrpFun->SetGroupMode(group.id, GetChgMode(gunId));
					modeAll = 1;
				}
			}

			if (modeAll || modeUsed)
			{
				return RESULT_ERR;
			}
		}

		ctrlGroupCount = grpNum;

		for (j = 0; j < grpNum; j++)
		{
			grpId = resGrpId[j];
			g_pGrpFun->GetGroupUI(grpId, &grpVol, &grpCur);		// 获取该组模块电压、电流
			chgVol = GetChgVolOut(gunId);
//			DEBUG("gunId = %d, chgVol = %d, grpId = %d, grpVol = %d, grpCur = %d", gunId, chgVol, grpId, grpVol, grpCur);
			
			/* 压差小于10V，电流小于2A */
			if (resAry.gunRes == CHANGE_OUT)
			{
				// 设置模块电压电流
				g_pGrpFun->SetGroupUI(grpId, grpVol, 100);
				if (grpCur < 200)	// 切出，只需要判断电流就行了
				{
					tmpGrpCount++;
				}
			}
			else if (resAry.gunRes == CHANGE_IN)
			{
				// 设置模块电压电流
				g_pGrpFun->SetGroupUI(grpId, chgVol, 100);
				if ((abs(chgVol - grpVol) < 100) && (grpCur < 200))		// 切入，电压电流都要判断
				{
					tmpGrpCount++;
				}
			}
		}

//		DEBUG("tmpGrpCount = %d, ctrlGroupCount = %d", tmpGrpCount, ctrlGroupCount);
		tmpRlyCount = 0;
		if (tmpGrpCount == ctrlGroupCount)		// 需要释放的模块降流完成
		{
			ctrlRlyCount = resAry.relayNum;
			for (j = 0; j < resAry.relayNum; j++)
			{
				RelayCtrl_SetSwitch(resAry.relaySW[j], resAry.relayId[j]);		// 发送指令，断开对应的继电器
				if (RelayCtrl_GetCtrlBack(resAry.relayId[j]) == RELAY_BACK_OK)		// 判断继电器反馈状态
				{
					tmpRlyCount++;
				}
			}
//			DEBUG("tmpRlyCount = %d, ctrlRlyCount = %d", tmpRlyCount, ctrlRlyCount);
			if (tmpRlyCount == ctrlRlyCount)		// 继电器断开完成
			{
				/* 切出的模块需要泄放 */
				if (resAry.gunRes == CHANGE_OUT)
				{
					grpNum = resAry.freeGrpNum;
					for (j = 0; j < grpNum; j++)
					{
						grpId = resGrpId[j];
						g_pGrpFun->SetGroupUI(grpId, 0, 0);
					}
				}

				ret = RESULT_OK;
			}
		}
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ModGrpStopDo
* ABSTRACT	: 枪停止充电，泄放模块，断开继电器
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int ModGrpStopDo(CHG_POLICY_STRUCT resAry, int gunId)
{
	int ret = RESULT_ERR;
	int j = 0;

	int grpId, chgVol, chgCur;

	if (gunId == resAry.gunId - 1)
	{
		if (resAry.gunRes == CHANGE_OUT)
		{
//			DEBUG("ModGrpStopDo gunId = %d, freeGrpNum = %d", gunId, resAry.freeGrpNum);
			for (j = 0; j < resAry.freeGrpNum; j++)
			{
				grpId = resAry.freeGrpId[j];
	
				// 设置模块电压电流，泄放
				g_pGrpFun->SetGroupUI(grpId, 0, 0);
			}

			chgVol = GetChgVolOut(gunId);
			chgCur = GetChgCurOut(gunId);

//			DEBUG("ModGrpStopDo gunId = %d, chgVol = %d, chgCur = %d", gunId, chgVol, chgCur);

			/* 压差小于50V，电流小于2A */
			if ((chgVol < 500) && (chgCur < 200))
			{
				unsigned char sw[6] = {0};
				for (j = 0; j < resAry.relayNum; j++)
				{
					RelayCtrl_SetSwitch(sw, resAry.relayId[j]);		// 发送指令，断开对应的继电器
					if (RelayCtrl_GetCtrlBack(resAry.relayId[j]) == RELAY_BACK_OK)			// 判断继电器反馈状态
					{
						ret = RESULT_OK;
					}
				}

				if (resAry.relayNum == 0)
				{
					ret = RESULT_OK;
				}
			}
		}

		if (resAry.freeGrpNum == 0)
		{
			ret = RESULT_OK;
		}
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ChgPolicyDo
* ABSTRACT	: 功率分配算法及结果处理
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int ChgPolicyDo(int step, int gunId)
{
	int i = 0;
	CHG_POLICY_NEED_STRUCT gun = {0};

	if (step == CHG_MAIN_START)
	{
		gun.gunId = gunId + 1;
		gun.pwrNeed = GetEVPwrMax(gunId);
		gun.startTime = GetChgStartTime(gunId);
	}
	else if (step == CHG_MAIN_STOP)
	{
		gun.gunId = gunId + 1;
		gun.pwrNeed = 0;
	}
	else  if (step == CHG_MAIN_RUN)
	{
		/* 充电过程中，枪需求逐渐减小，如果有空闲模块则可以切出来 */
		for (i = 0; i < g_unitPara.gunNum; i++)
		{
			if (GetChgStep(i) == CHG_MAIN_RUN)
			{
				if (abs(get_timetick() - GetChgStartTime(i)) > WAIT_RESTART_POLICY_TIME)	// 启动时间超过了5min
				{
					if ((GetEVSEPwrMax(i) > GetChgPwrNeed(i))
						&& ((GetEVSEPwrMax(i) - GetChgPwrNeed(i)) > (g_pwrPri.grpPwrMax + 100)))	// 有空闲模块，释放出来,10kW余量
					{
//						gun.gunId = i + 1;
//						gun.pwrNeed = GetChgPwrNeed(i);
//						DEBUG("power change out");
						break;
					}
				}
			}
		}

		if (gun.gunId == 0)		// 主要是为了与上面互斥，不能同步计算策略
		{
			if ((abs(get_timetick() - g_pwrPri.stopTick) > WAIT_RESTART_POLICY_TIME)
													&& g_pwrPri.stopTick)	// 枪启动或停止超过5min，重新看下是否需要功率分配
			{
				int grpUseNum = 0;
				for (i = 0; i < g_unitPara.gunNum; i++)
				{
					grpUseNum += g_res.resAry[i].groupNum;`
				}

				if (grpUseNum < WaitMdlPowerOn())		// 有模块空闲，可以分给需要的枪
				{
					for (i = 0; i < g_unitPara.gunNum; i++)
					{
						/* 枪端最大需求大于桩端所提供，且输出功率接近桩端功率，可能桩端不满足车端的实际，车端在降功率输出中 */
						if (GetChgStep(i) == CHG_MAIN_RUN)
						{
							DEBUG("EVPwrMax = %d, EVSEPwrMax = %d, ChgPwrNeed = %d", GetEVPwrMax(i), GetEVSEPwrMax(i), GetChgPwrNeed(i));
							if (((GetEVPwrMax(i) - GetEVSEPwrMax(i)) > 150) 
							&& ((GetEVSEPwrMax(i) - GetChgPwrNeed(i)) < 150))
							{
								gun.gunId = i + 1;
								gun.pwrNeed = GetEVPwrMax(i);	// 用车端最大值重新策略一次
								DEBUG("power change in");
								break;
							}
						}
					}
				}

				g_pwrPri.stopTick = 0;
			}
		}
	}

	if (gun.gunId == 0)		// 没有枪需要策略计算
	{
		return RESULT_ERR;
	}

	if (GetChgStep(gun.gunId-1) == CHG_MAIN_STOP)		// 需要停止，则直接策略计算，不会影响到其他枪
	{

	}
	else
	{
		for (i = 0; i < g_unitPara.gunNum; i++)			// 先判断是否有枪正在策略分配
		{
			if ((GetChgStep(i) == CHG_MAIN_RUN_POLICY)
			 	|| (GetChgStep(i) == CHG_MAIN_CHANGE))
			{
				break;
			}
		}

		if (i != g_unitPara.gunNum)				// 有枪正在策略或计算中，需等待
		{
			return RESULT_ERR;
		}
	}
	
	DEBUG("gunId = %d, pwrNeed = %d", gun.gunId, gun.pwrNeed);
	log_send("gunId = %d, pwrNeed = %d", gun.gunId-1, gun.pwrNeed);
//	memcpy(&g_resOld.resAry[gunId], &g_res.resAry[gunId], sizeof(CHG_POLICY_STRUCT));
	memcpy(&g_resOld, &g_res, sizeof(CHG_POLICY_RES_STRUCT));
	g_res = Matrix_Policy(gun);
	char string[2048] = {0};
	MatrixPrint(g_res, string);
	DEBUG("%s", string);
	log_send("%s", string);

	if (g_res.result == POLICY_OK)				// 分配成功
	{
		for (i = 0; i < g_unitPara.gunNum; i++)
		{
			if (g_res.resAry[i].gunRes == CHANGE_NO)
			{

			}
			else if (g_res.resAry[i].gunRes == CHANGE_IN)
			{
				/* 一般出现切入情况1、枪启动 2、充电过程中模块空闲，直接进行功率切换即可 */
				SetChgStep(i, CHG_MAIN_CHANGE);
				memcpy(&g_chgResOld[i], &g_resOld.resAry[i], sizeof(CHG_POLICY_STRUCT));
			}
			else if (g_res.resAry[i].gunRes == CHANGE_OUT)
			{
				if (g_res.resAry[i].pwrMax)
				{
					SetChgStep(i, CHG_MAIN_RUN_POLICY);
					memcpy(&g_chgResOld[i], &g_resOld.resAry[i], sizeof(CHG_POLICY_STRUCT));
				}
				else	// 没有功率，停止
				{
					SetChgStep(i, CHG_MAIN_STOP);
					memcpy(&g_chgResOld[i], &g_res.resAry[i], sizeof(CHG_POLICY_STRUCT));

					if (GetChgStep(i) != CHG_MAIN_STOP)
					{
						SetChgStopReason(i, STOP_PWRPOLICY);
						DEBUG("gunId = %d, ChgPolicyDo STOP_PWRPOLICY", i);
						log_send("gunId = %d, ChgPolicyDo STOP_PWRPOLICY", i);
					}
				}
			}
		}
	}
	else if (g_res.result == POLICY_ERR)		// 分配失败
	{
		SetChgStep(gun.gunId-1, CHG_MAIN_STOP);
		SetChgStopReason(gun.gunId-1, STOP_PWRPOLICY);
		DEBUG("gunId = %d, ChgPolicyDo STOP_PWRPOLICY", gun.gunId-1);
		log_send("gunId = %d, ChgPolicyDo STOP_PWRPOLICY", gun.gunId-1);
	}
	else if (g_res.result == POLICY_NOCHANGE)	// 分配不变，这里可能有问题
	{
		if (g_res.resAry[i].pwrMax)
		{
			SetChgStep(gun.gunId-1, CHG_MAIN_RUN);
		}
	}

	return g_res.result;
}

/*
* *******************************************************************************
* MODULE	: ChgFreeDo
* ABSTRACT	: 枪空闲状态，清空一些数据及状态
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgFreeDo(int gunId)
{
	SetChgSta(gunId, 2);
	g_chgInfo[gunId].EVPwrMax = 0;
	g_chgInfo[gunId].EVVolMax = 0;
	g_chgInfo[gunId].EVCurMax = 0;
	g_chgInfo[gunId].EVSEPwrMax = 0;
	g_chgInfo[gunId].EVSEVolMax = 0;
	g_chgInfo[gunId].EVSECurMax = 0;
	g_chgInfo[gunId].volNeed = 0;
	g_chgInfo[gunId].curNeed = 0;
	g_chgInfo[gunId].pwrNeed = 0;
	g_chgInfo[gunId].volOut = 0;
	g_chgInfo[gunId].curOut = 0;
	g_chgInfo[gunId].pwrOut = 0;
	g_chgInfo[gunId].startTime = 0;

	if (abs(get_timetick()-g_pwrPri.chgStopTick[gunId]) > 10000)
	{
		SetChgStopReason(gunId, 0);
		ClearChgIsStop(gunId);
	}
}

/*
* *******************************************************************************
* MODULE	: ChgStartDo
* ABSTRACT	: 枪准备启动，先判断此时功率是否稳定，即没有其他枪在进行功率分配
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgStartDo(int gunId)
{
	int ret = 0;
	int i = 0;

	ret = WaitMdlPowerOn();
	if (ret == RESULT_OK)				// 等待模块上线后再操作
	{
		return;
	}
	else if (ret == RESULT_ERR)			// 超时没有模块在线，需要报故障
	{
		SetChgStep(gunId, CHG_MAIN_FREE);
		SetChgStopReason(gunId, STOP_MDLCOMM);
		DEBUG("gunId = %d, WaitMdlPowerOn Err", gunId);
		log_send("gunId = %d, WaitMdlPowerOn Err", gunId);
		return;
	}

	if (GetEVPwrMax(gunId) != 0)			// 等待车端最大允许电流，谁先来最大允许先处理谁
	{
		SetChgStartTime(gunId);				// 收到EV最大允许数据后才认为启动时间

		for (i = 0; i < g_unitPara.gunNum; i++)					
		{
			if ((GetChgStep(i) == CHG_MAIN_RUN_POLICY)	// 防止出现有些枪还在工作就算法变化了
				|| (GetChgStep(i) == CHG_MAIN_CHANGE)
				|| (GetChgStep(i) == CHG_MAIN_STOP))
			{
				return;
			}
		}

		/* 进行策略计算 */
		ChgPolicyDo(CHG_MAIN_START, gunId);

		g_pwrPri.stopTick = 0;
	}
}

/*
* *******************************************************************************
* MODULE	: ChgChangeDo
* ABSTRACT	: 枪功率切换中，需要做些啥
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgChangeDo(int gunId)
{
	int i = 0;

	/* 先处理运行中模块切出完成后，再处理刚充电的枪的模块切入 */
	for (i = 0; i < g_unitPara.gunNum; i++)					
	{
		if (GetChgStep(i) == CHG_MAIN_RUN_POLICY)	// 需要先等待其他枪功率到位后才能CHANGE
		{
			return;
		}

		if (((GetChgStep(i) == CHG_MAIN_CHANGE) 
			|| (GetChgStep(i) == CHG_MAIN_STOP))
			&& (g_res.resAry[i].gunRes == CHANGE_OUT))
		{
			if (g_res.resAry[gunId].gunRes == CHANGE_IN)	// 枪切出可以一起进行，但是需要先完成所有的切出再进行切入
			{
				return;
			}
		}
	}

	if (g_pwrPri.changeTick[gunId] == 0)
	{
		g_pwrPri.changeTick[gunId] = get_timetick();	// 开始计时
	}

	/* 进行切入或切出操作，超时15s */
	if (abs(get_timetick() - g_pwrPri.changeTick[gunId]) > WAIT_POWER_CHANGE_TIME)
	{
		SetChgStep(gunId, CHG_MAIN_STOP);
		SetChgStopReason(gunId, STOP_PWRCHANGE);
		g_pwrPri.changeTick[gunId] = 0;
		DEBUG("gunId = %d, ChgChangeDo Err", gunId);
		log_send("gunId = %d, ChgChangeDo Err", gunId);
	}
	else
	{
		if (ModGrpChangeDo(g_res, gunId) == RESULT_OK)		// 切换完成，可以开始充电了
		{
			SetChgStep(gunId, CHG_MAIN_RUN);	
			g_pwrPri.changeTick[gunId] = 0;
		}
	}
}

/*
* *******************************************************************************
* MODULE	: ChgRunDo
* ABSTRACT	: 枪正常充电中，控制模块输出
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgRunDo(int gunId)
{
	int i, j;
	int vol = GetChgVolNeed(gunId);
	int cur = GetChgCurNeed(gunId);
	int grpNum, curTmp, mdlCurMax, volOut;
	int grpId[GROUP_MAX_NUM], gunRes; 

	mdlCurMax = 0;
	gunRes = g_res.resAry[gunId].gunRes;
	if (((GetChgStep(gunId) == CHG_MAIN_CHANGE) && (gunRes == CHANGE_IN))
		|| (GetChgStep(gunId) == CHG_MAIN_RUN_POLICY))
	{
		grpNum = g_chgResOld[gunId].groupNum;
		for (j = 0; j < grpNum; j++)		// 循环模组
		{
			grpId[j] = g_chgResOld[gunId].groupId[j];
			if (g_pGrpFun->GetGroupOnline(grpId[j]) == RESULT_OK)
			{
				if (mdlCurMax == 0)
				{
					mdlCurMax = g_pGrpFun->GetGroupMaxCurByVol(grpId[j], vol);
				}
			}
			else
			{
				DEBUG();
			}
		}
	}
	else
	{
		grpNum = g_res.resAry[gunId].groupNum;
		for (j = 0; j < grpNum; j++)		// 循环模组
		{
			grpId[j] = g_res.resAry[gunId].groupId[j];
			if (g_pGrpFun->GetGroupOnline(grpId[j]) == RESULT_OK)
			{
				if (mdlCurMax == 0)
				{
					mdlCurMax = g_pGrpFun->GetGroupMaxCurByVol(grpId[j], vol);
				}
			}
			else
			{
				DEBUG();
			}
		}
	}

	if (vol == 0)
	{
		vol = 0;
		mdlCurMax = 0;
	}

//	DEBUG("volNeed = %d, curNeed = %d, mdlCurMax = %d", vol, cur, mdlCurMax);
	/* 控制模块输出，根据电流输出所需模块，多余分配的模块不做输出 */
	for (i = 0; i < grpNum; i++)
	{
//		grpId[i] = g_res.resAry[gunId].groupId[i];
		if (g_pGrpFun->GetGroupOnline(grpId[i]) == RESULT_ERR)
		{
			DEBUG();
			continue;
		}

		if (cur >= mdlCurMax)
		{
			curTmp = mdlCurMax;
			cur -= mdlCurMax;
		}
		else
		{
			curTmp = cur;
			cur = 0;
		}

		if ((curTmp) && (vol))						// 需要正常输出的模块
		{
			g_pGrpFun->SetGroupUI(grpId[i], vol, curTmp);
		}
		else if ((vol) && (curTmp == 0))			// 不需要工作的模块以最小电压保持住
		{
			volOut = GetChgVolOut(gunId) > 500 ? (GetChgVolOut(gunId) - 500) : GetChgVolOut(gunId);
			volOut = volOut > GetChgVolNeed(gunId) ? GetChgVolNeed(gunId) : volOut;

			if (GetChgMode(gunId) == SERIES_MODE)
			{
				volOut = volOut < g_mdl.volMinH ? g_mdl.volMinH : volOut;
				g_pGrpFun->SetGroupUI(grpId[i], volOut, 10);
			}
			else
			{
				volOut = volOut < g_mdl.volMinL ? g_mdl.volMinL : volOut;
				g_pGrpFun->SetGroupUI(grpId[i], volOut, 10);
			}
		}
		else										// 泄放等操作
		{
			g_pGrpFun->SetGroupUI(grpId[i], 0, 0);
		}
	}
}

/*
* *******************************************************************************
* MODULE	: ChgRunPolicyDo
* ABSTRACT	: 枪充电中，策略计算完成，判断桩端是否满足车端需求，等待车端需求变化
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgRunPolicyDo(int gunId)
{
	/*
	* 1、运行过程中，某枪充电或停充，则模块会切出或切入
	* 2、某枪充电，该枪模块切出，先判断功率是否满足，满足则直接切出，不满足则通知车端降功率
	* 3、某枪停止，判断当前功率是否满足车端需求，满足则继续充，不满足则做功率分配
	*/
	int isChange = g_res.resAry[gunId].gunRes;
	int pwrMax = g_res.resAry[gunId].pwrMax;
	
	if (isChange == CHANGE_OUT)					// 有模块需要切出，通知并等待车端降功率
	{
		if (pwrMax >= GetChgPwrNeed(gunId))		// 车端功率降下来了，可以开始切出了
		{
			SetChgStep(gunId, CHG_MAIN_CHANGE);
		}
	}
	else if (isChange == CHANGE_IN)				// 有模块需要切入，通知车端升功率
	{
		SetChgStep(gunId, CHG_MAIN_CHANGE);
	}
}

/*
* *******************************************************************************
* MODULE	: ChgStopDo
* ABSTRACT	: 枪停止充电，需要做些啥
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgStopDo(int gunId)
{
	static unsigned long stopTick[GUN_DC_MAX_NUM] = {0};
	int ret = POLICY_ERR;
	int i = 0;

	if (stopTick[gunId] == 0)
	{
		/* 策略里将这个释放出去 */
		ret = ChgPolicyDo(CHG_MAIN_STOP, gunId);
		if (ret == POLICY_OK)		// 分配成功
		{
			stopTick[gunId] = get_timetick();					// 开始停止计时
			g_pwrPri.stopPolicy[gunId] = 1;
		}
		else if (ret == POLICY_ERR)
		{
//			SetChgStep(gunId, CHG_MAIN_FREE);
			return;
		}
	}
	
	if (abs(get_timetick() - stopTick[gunId]) > WAIT_POWER_CHANGE_TIME)			// 停止操作，超时15s
	{
		unsigned char sw[6] = {0};
		for (i = 0; i < g_chgResOld[gunId].relayNum; i++)
		{
			RelayCtrl_SetSwitch(sw, g_chgResOld[gunId].relayId[i]);				// 发送指令，强制断开对应的继电器
		}

		SetChgStep(gunId, CHG_MAIN_FREE);
		SetChgStopReason(gunId, STOP_PWRCHANGE);
		stopTick[gunId] = 0;
		DEBUG("gunId = %d, ChgStopDo Err", gunId);
		log_send("gunId = %d, ChgStopDo Err", gunId);
	}
	else
	{
		/* 释放模块，断开PDU继电器 */
		if (ModGrpStopDo(g_chgResOld[gunId], gunId) == RESULT_OK)			// 停止完成
		{
			SetChgStep(gunId, CHG_MAIN_FREE);
			stopTick[gunId] = 0;
			g_pwrPri.stopTick = get_timetick();
			g_pwrPri.stopPolicy[gunId] = 0;
			g_pwrPri.changeTick[gunId] = 0;
		}
	}
}

/*
* *******************************************************************************
* MODULE	: ChgEMCStopDo
* ABSTRACT	: 安全相关的停止，如急停、门禁、限位等，需要禁止停止
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ChgEMCStopDo(int gunId)
{
	// 全部停止
	ChgStopDo(gunId);
}

/*
* *******************************************************************************
* MODULE	: PwrCtrlDeal
* ABSTRACT	: 功率分配，功率控制
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void PwrCtrlDeal()
{
	static int gunId = 0;				// 枪号,0开始计数

	if (abs(get_timetick() - g_pwrPri.flushTick) > 200)
	{
		g_pwrPri.flushTick = get_timetick();

		ChgPolicyDo(CHG_MAIN_RUN, 0xFF);	// 检查桩端需求是否满足车端需求

		FlushOtherInfo();					// 不断的刷新一些外部数据
	}

	gunId++;
	if (CheckGunId(gunId+1) == RESULT_ERR)	// 判断枪号，并循环
	{
		gunId = 0;
	}

	switch (GetChgStep(gunId))
	{
		case CHG_MAIN_FREE:				// 枪空闲，不需要做任何操作
			ChgFreeDo(gunId);
			break;
		case CHG_MAIN_START:			// 枪准备启动
			ChgStartDo(gunId);
			break;
		case CHG_MAIN_CHANGE:			// 该枪功率正在切换中
			ChgChangeDo(gunId);			// 这里有个BUG，切换时不能影响这把枪不需要动的模块的输出
			ChgRunDo(gunId);
			break;
		case CHG_MAIN_RUN:				// 正常输出
			ChgRunDo(gunId);
			break;
		case CHG_MAIN_RUN_POLICY:		// 充电时进行策略计算，枪还需要继续输出
			ChgRunPolicyDo(gunId);
			ChgRunDo(gunId);
			break;
		case CHG_MAIN_STOP:				// 枪充电停止
			ChgStopDo(gunId);
			break;
		case CHG_MAIN_EMC_STOP:			// 安全相关的停止，如急停、门禁、限位等，需要禁止停止
			ChgEMCStopDo(gunId);
			break;
		default:
			break;
	}
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: PwrCtrl_SetChgAllow
* ABSTRACT	: 设置车端最大输出能力
* FUNCTION	:
* ARGUMENT	: index:gunid;
* NOTE		:
* RETURN	: success or fail
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_SetChgAllow(int vol, int cur, int pwrMax, int gunPwr, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	gunId -= 1;

	SetChgAllow(gunId, vol, cur, pwrMax);
	SetChgPwrMax(gunId, gunPwr);

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_SetChgUI
* ABSTRACT	: 设置枪需求电压、电流、功率
* FUNCTION	:
* ARGUMENT	: index:gunid;
* NOTE		:
* RETURN	: success or fail
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_SetChgUI(int vol, int cur, int pwr, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	gunId -= 1;

	if (vol)
	{
		if (vol > GetUnitAllowVolMax())		// 与整流柜最大输出电压做比较
		{
			vol = GetUnitAllowVolMax();
		}
		if (vol < GetUnitAllowVolMin())		// 与整流柜最小输出电压做比较
		{
			vol = GetUnitAllowVolMin();
		}
		if (cur > GetEVCurMax(gunId))		// 与整流柜最大输出电流做比较
		{
			cur = GetEVCurMax(gunId);
		}
	}

	SetChgUINeed(gunId, vol, cur, pwr);

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_GetChgInfo
* ABSTRACT	: 获取枪信息
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: success or fail
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_GetChgInfo(CHG_INFO_STRUCT *pChg, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	gunId -= 1;

	memcpy(pChg, &g_chgInfo[gunId], sizeof(CHG_INFO_STRUCT));

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_SetUnitPara
* ABSTRACT	: 用户设置整流柜最大输出
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void PwrCtrl_SetUnitPara(UNIT_PARA_STRUCT unit)
{	
	g_unitPara.pwrMax = unit.pwrMax;
	g_unitPara.gunNum = unit.gunNum;
	g_unitPara.groupNum = unit.groupNum * unit.pwrUnitNum;
	g_unitPara.relayNum = unit.relayNum;
	g_unitPara.mdlType = unit.mdlType;
	g_unitPara.pwrUnitNum = unit.pwrUnitNum;

	Mdl_GetMdlAllow(unit.mdlType, &g_mdl);

	g_unitPara.volMax = g_mdl.volMax;
	g_unitPara.volMin = g_mdl.volMinL;
	g_unitPara.curMax = g_mdl.curMax;

	MATRIX_INIT_STRUCT init;
	init.gunNum = unit.gunNum;
	init.groupNum = unit.groupNum;
	init.relayNum = unit.relayNum;
	init.policyType = unit.pwrType;
	init.groupPwr = g_mdl.pwrMax;
	init.grpSync = unit.pwrUnitNum;

	Matrix_Init(init);

	if (unit.pwrType == MATRIX2HAND)
	{
		g_pwrPri.grpPwrMax = g_mdl.pwrMax * unit.pwrUnitNum;
	}
	else
	{
		g_pwrPri.grpPwrMax = g_mdl.pwrMax;
	}
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_GetUnitPara
* ABSTRACT	: 获得整流柜最大输出
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void PwrCtrl_GetUnitPara(UNIT_PARA_STRUCT *pUnit)
{
	memcpy(pUnit, &g_unitPara, sizeof(UNIT_PARA_STRUCT));
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_GetEVSEPwrMax
* ABSTRACT	: 获得整流柜对于某枪的最大功率
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_GetEVSEPwrMax(int gunId)
{
	return g_chgInfo[gunId].EVSEPwrMax;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_GetChgIsBusy
* ABSTRACT	: 获得目前是否在充电
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: CHG_MAIN_FREE-所有枪空闲，CHG_MAIN_RUN-有枪不是空闲
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_GetChgIsBusy()
{
	int i = 0;
	for (i = 0; i < g_unitPara.gunNum; i++)
	{
		if (GetChgStep(i) != CHG_MAIN_FREE)
		{
			return CHG_MAIN_RUN;
		}
	}

	return CHG_MAIN_FREE;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_LimitPower
* ABSTRACT	: 限功率
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_LimitPower(int pwr)
{
	if (pwr != 0)
	{
		SetUnitAllowPwr(pwr);
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_GetTotalPwrOut
* ABSTRACT	: 获得当前总输出功率
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_GetTotalPwrOut()
{
	int i = 0;
	int pwrOut = 0;

	for (i = 0; i < g_unitPara.gunNum; i++)
	{
		pwrOut += GetChgPwrOut(i);
	}

	return pwrOut;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_StartStop
* ABSTRACT	: 设置枪的启动和停止
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: success or fail
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_StartStop(int ctrl, int stopReason, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	gunId -= 1;

	if ((ctrl != CHG_START) && (ctrl != CHG_STOP))
	{
		return RESULT_ERR;
	}

	if (ctrl == CHG_START)				// 请求启动
	{
		if (GetChgStep(gunId) != CHG_MAIN_FREE)		// 如果不是空闲状态，说明枪当前正在运行
		{
			return RESULT_OK;
		}

		DEBUG("gunId = %d, isStop = %d", gunId, g_pwrPri.isStop[gunId]);
		if (g_pwrPri.isEMC || g_pwrPri.isStop[gunId])			// EMC停止，禁止启动 
		{
			return RESULT_OK;
		}

		SetChgStep(gunId, CHG_MAIN_START);
		SetChgSta(gunId, 1);
		SetChgStopReason(gunId, 0);
	}
	else if (ctrl == CHG_STOP)			// 请求停止
	{		
		SetChgStopReason(gunId, stopReason);
		ClearChgIsStop(gunId);

		if (GetChgStep(gunId) == CHG_MAIN_FREE ||
			GetChgStep(gunId) >= CHG_MAIN_STOP ||
			GetChgStep(gunId) == CHG_MAIN_EMC_STOP)		//在这3种情况下，无需停止了
		{
			return RESULT_OK;
		}

		SetChgStep(gunId, CHG_MAIN_STOP);
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_EMCStop
* ABSTRACT	: 紧急停止，紧急断开前端接触器
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void PwrCtrl_EMCStop(int stopReason)
{
	int i = 0;

	if (stopReason)
	{
		if (g_pwrPri.isEMC == 0)
		{
			g_pwrPri.isEMC = 1;
			DEBUG("EMC stopReason : %x", stopReason);
			log_send("EMC stopReason : %x", stopReason);

			for (i = 0; i < g_unitPara.gunNum; i++)
			{
				if (GetChgStep(i) != CHG_MAIN_FREE)
				{
					SetChgStep(i, CHG_MAIN_EMC_STOP);
					SetChgStopReason(i, stopReason);
				}
			}
		}
	}
	else
	{
		g_pwrPri.isEMC = 0;
	}
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_FaultStop
* ABSTRACT	: 整流柜故障停止
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: 
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int PwrCtrl_FaultStop(int stopReason, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	gunId -= 1;

	if (GetChgStep(gunId) == CHG_MAIN_FREE ||
		GetChgStep(gunId) >= CHG_MAIN_STOP ||
		GetChgStep(gunId) == CHG_MAIN_EMC_STOP)		//在这3种情况下，无需停止了
	{
		return RESULT_OK;
	}

	log_send("Fault gunId = %d, stopReason : %x", gunId, stopReason);

	SetChgStep(gunId, CHG_MAIN_STOP);
	SetChgStopReason(gunId, stopReason);

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_Deal
* ABSTRACT	: 初始化
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void PwrCtrl_Deal()
{
	PwrCtrlDeal();
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_Init
* ABSTRACT	: 初始化
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void PwrCtrl_Init()
{
	LockInit();
	g_pGrpFun = GetSingleGroupFun();
}

/*
* *******************************************************************************
* MODULE	: PwrCtrl_Uninit
* ABSTRACT	: 注销
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void PwrCtrl_Uninit()
{
	LockDestroy();
}