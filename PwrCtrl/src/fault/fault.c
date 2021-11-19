/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: 
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: 
* DEPARTMENT	: 
* DATE			: 
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "fault.h"
#include "timer.h"
#include "pwrCtrl.h"
#include "exportCtrl.h"
#include "cnfJson.h"
#include "modCtrl.h"
#include "fanCtrl.h"
#include "relayCtrl.h"
#include "zmqCtrl.h"

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
	char estop;
	char liq;
	char door;
	char CB;

	int mdlTemp;
	int pduTemp[PDU_MAX_NUM];
	char fanCommErr;
	char fanErr;
	char unitTemp;

	char mdlCommErr;
	char mdlErr;
	char mdlOverVol;
	char mdlUnderVol;
	char pduErr[PDU_MAX_NUM];
	char pduCommErr[PDU_MAX_NUM];
	char storage;
	char surge;

	char acRelay;

	char heartCommErr[GUN_DC_MAX_NUM];	// 枪心跳报文
	char chgCommErr[GUN_DC_MAX_NUM];	// 枪充电报文

	char isEMC;					// 0-无故障，1-EMC故障
	unsigned long mdlFaultTick;
	unsigned long fanFaultTick;

	/* Deal中使用的变量 */
	unsigned long errorTick;
	int errCount;
	int warningCount[GUN_DC_MAX_NUM+1];

	/* 刷卡计时 */
	unsigned long cardTick[GUN_DC_MAX_NUM];
}FAULT_PRI_STRUCT;

#pragma pack()

#define 	ERROR_TOUT			100			// 故障的检测周期

static 		FAULT_PRI_STRUCT 	g_faultPri = {0};			// 检测数据
static 		int  				g_DTC[GUN_DC_MAX_NUM][DTC_MAX_NUM] = {0};	// 存放DTC
static 		int 				g_DTCCount[GUN_DC_MAX_NUM] = {0};			// DTC计数

static 		pthread_mutex_t 	mutex;  		// 锁

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

	if ((gunId < 0) || (gunId > GUN_DC_MAX_NUM))
	{
		ret = RESULT_ERR;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SetDTC
* ABSTRACT	: 设置DTC
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetDTC(int code, int gunId)
{
	int ret = RESULT_ERR;
	int i = 0;

	pthread_mutex_lock(&mutex);

//	DEBUG("code = %x, g_DTCCount[%d] = %x", code, gunId, g_DTCCount[gunId]);

	for (i = 0; i < g_DTCCount[gunId]; i++)
	{
		if (g_DTC[gunId][i] == code)
		{
			pthread_mutex_unlock(&mutex);
			return RESULT_OK;
		}
	}

	if (g_DTCCount[gunId] < 5)
	{
		g_DTC[gunId][g_DTCCount[gunId]++] = code;
		ret = RESULT_OK;
	}
	
	pthread_mutex_unlock(&mutex);

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SetDTCAll
* ABSTRACT	: 设置DTC
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetDTCAll(int code)
{
	int ret = RESULT_ERR;
	int i = 0;

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		SetDTC(code, i);
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ClearDTC
* ABSTRACT	: 清除DTC
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ClearDTC(int gunId)
{
	pthread_mutex_lock(&mutex);
	memset(g_DTC[gunId], 0, sizeof(int)*DTC_MAX_NUM);
	g_DTCCount[gunId] = 0;

	pthread_mutex_unlock(&mutex);
}

/*
* *******************************************************************************
* MODULE	: ClearDTCAll
* ABSTRACT	: 清除DTC
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ClearDTCAll()
{
	int i = 0;

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		ClearDTC(i);
	}
}

/*
* *******************************************************************************
* MODULE	: FlushCheckData
* ABSTRACT	: 从其他模块中刷新需要检测的数据
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void FlushCheckData()
{
	int i = 0;
	PWR_CNF_STRUCT cnf = {0};
	MDL_INFO_STRUCT mdl = {0};
	RELAY_INFO_STRUCT pdu = {0};
	FAN_INFO_STRUCT fan = {0};
	
	CNF_GetPwrCnf(&cnf);

	g_faultPri.estop = GetEStop() == OPEN ? 1 : 0;			// 急停
	if (cnf.sensors.liqEn == 1)
	{
		g_faultPri.liq = GetWaterLevel() == OPEN ? 1 : 0;	// 液位
	}
	else
	{
		g_faultPri.liq = 0;
	}

	if (cnf.sensors.doorEn == 1)
	{
		g_faultPri.door = GetDoor() == OPEN ? 1 : 0;		// 门禁
	}
	else
	{
		g_faultPri.door = 0;
	}

	g_faultPri.CB = GetCB() == OPEN ? 1 : 0;				// 断路器

	g_faultPri.acRelay = GetACRelay() == OVERTIME ? 1 : 0;	// 前端交流接触器

	g_faultPri.mdlTemp = 0;
	g_faultPri.mdlCommErr = 0;
	g_faultPri.mdlErr = 0;
	g_faultPri.mdlOverVol = 0;
	g_faultPri.mdlUnderVol = 0;
	if (GetACRelay() == OPEN)
	{
		if (abs(get_timetick() - g_faultPri.mdlFaultTick) > 10000)
		{
			for (i = 0; i < MDL_MAX_NUM; i++)
			{
				Mdl_GetMdlInfo(&mdl, i+1);
				g_faultPri.mdlTemp = g_faultPri.mdlTemp < mdl.temp ? mdl.temp : g_faultPri.mdlTemp;
				if ((mdl.sts) || (mdl.fault == MDL_FAULT_OVER_TEMP))
				{
					g_faultPri.mdlErr = 1;
				}

				if (mdl.fault == MDL_FAULT_OFFLINE)
				{
					g_faultPri.mdlCommErr = 1;
				}

				if ((mdl.fault == MDL_FAULT_OVER_VOL) && (g_faultPri.mdlOverVol == 0))
				{
					g_faultPri.mdlOverVol = 1;
				}

				if ((mdl.fault == MDL_FAULT_UNDER_VOL) && (g_faultPri.mdlUnderVol == 0))
				{
					g_faultPri.mdlUnderVol = 1;
				}
			}
		}
	}
	else
	{
		g_faultPri.mdlFaultTick = get_timetick();
	}

	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		g_faultPri.pduCommErr[i] = 0;
		if (RelayCtrl_GetInfo(&pdu, i+1) == RESULT_OK)
		{
			g_faultPri.pduCommErr[i] = pdu.online == 0 ? 1 : 0;
			g_faultPri.pduTemp[i] = pdu.temp[0];
			g_faultPri.pduErr[i] = pdu.ctrlBack == RELAY_BACK_ERR ? 1 : 0;
		}
	}

	g_faultPri.fanErr = 0;
	g_faultPri.fanCommErr = 0;
	if (GetFanRelay() == OPEN)
	{
		if (abs(get_timetick() - g_faultPri.fanFaultTick) > 10000)
		{
			for (i = 0; i < FAN_MAX_NUM; i++)
			{
				if (FanCtrl_GetInfo(&fan, i+1) == RESULT_OK)
				{
					if (fan.online == 0)
					{
						g_faultPri.fanCommErr = 1;
					}
					if (fan.warning || fan.error)
					{
						g_faultPri.fanErr = 1;
					}
				}
			}
		}
	}
	else
	{
		g_faultPri.fanFaultTick = get_timetick();
	}

	g_faultPri.storage = access("/media/mmcblk0p1/", 0) ? 1 : 0;	// SD卡
	g_faultPri.surge = GetSurging() == OPEN ? 1 : 0;				// 浪涌

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		g_faultPri.heartCommErr[i] = 0;
		g_faultPri.chgCommErr[i] = 0;
		if (ZmqCtrl_GetHeartOnline(i+1) == 0)
		{
			g_faultPri.heartCommErr[i] = 1;
		}
		if (ZmqCtrl_GetChgOnline(i+1) == 0)
		{
			g_faultPri.chgCommErr[i] = 1;
		}
	}
}

/*
* *******************************************************************************
* MODULE	: CheckError
* ABSTRACT	: 检测严重故障
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int CheckError()
{
	int ret = RESULT_OK;

	/* 功率柜急停故障 */
	if (g_faultPri.estop)
	{
		SetDTCAll(DTC_ESTOP);
		ret = RESULT_ERR;
	}

	/* 功率柜液位告警 */
	if (g_faultPri.liq)
	{
		SetDTCAll(DTC_LIQ);
		ret = RESULT_ERR;
	}

	/* 功率柜门打开 */
	if (g_faultPri.door)
	{
		SetDTCAll(DTC_DOOR);
		ret = RESULT_ERR;
	}

	/* 断路器脱扣 */
	if (g_faultPri.CB)
	{
		SetDTCAll(DTC_CB);
		ret = RESULT_ERR;
	}

	/* 输入过压 */
	if (g_faultPri.mdlOverVol)
	{
		SetDTCAll(DTC_OVERVOL);
		ret = RESULT_ERR;
	}

	/* 输入欠压 */
	if (g_faultPri.mdlUnderVol)
	{
		SetDTCAll(DTC_UNDERVOL);
		ret = RESULT_ERR;
	}

	/* 输入继电器控制故障 */
/*	if (g_faultPri.acRelay)
	{
		SetDTCAll(DTC_ACRELAY);
		ret = RESULT_ERR;
	}*/

	return ret;
}

/*
* *******************************************************************************
* MODULE	: CheckUnitWarning
* ABSTRACT	: 检测警告，对枪无影响
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int CheckUnitWarning()
{
	int ret = RESULT_OK;

	/* 电源模块过温 */
	if (g_faultPri.mdlTemp > (550+500))	// 500偏移
	{
		SetDTCAll(DTC_MDLTEMP);
		ret = RESULT_ERR;
	}

	/* 电源模块故障 */
	if (g_faultPri.mdlErr)
	{
		SetDTCAll(DTC_MDLERR);
		ret = RESULT_ERR;
	}

	/* 电源模块通信故障 */
	if (g_faultPri.mdlCommErr)
	{
		SetDTCAll(DTC_MDLCOMM);
		ret = RESULT_ERR;
	}

	/* 整流柜过温 */
	if (g_faultPri.unitTemp)
	{
//		SetDTCAll(DTC_UNITTEMP);
//		ret = RESULT_ERR;
	}

	/* 风扇通信故障 */
	if (g_faultPri.fanCommErr)
	{
		SetDTCAll(DTC_FANCOMM);
		ret = RESULT_ERR;
	}

	/* 风扇故障 */
	if (g_faultPri.fanErr)
	{
		SetDTCAll(DTC_FANERR);
		ret = RESULT_ERR;
	}

	/* SD卡故障 */
	if (g_faultPri.storage)
	{
		SetDTCAll(DTC_STORAGE);
		ret = RESULT_ERR;
	}

	/* 浪涌故障 */
	if (g_faultPri.surge)
	{
		SetDTCAll(DTC_SURGE);
		ret = RESULT_ERR;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: CheckGunWarning
* ABSTRACT	: 检测警告，对枪有影响
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int CheckGunWarning(int gunId)
{
	int ret = RESULT_OK;
	int gunFault = 0;
	int i = gunId;

	if (g_faultPri.pduTemp[i] > 600)
	{
		SetDTC(DTC_PDUTEMP, i);
		gunFault = 1;
	}

	if (g_faultPri.pduErr[i])
	{
		SetDTC(DTC_PDUERR, i);
		gunFault = 1;
	}

	if (g_faultPri.pduCommErr[i])
	{
		SetDTC(DTC_PDUCOMM, i);
		gunFault = 1;
	}

	if (g_faultPri.heartCommErr[i])
	{
//		SetDTC(DTC_HEARTOVERTIME, i);
//		gunFault = 1;
	}

	if (gunFault)
	{
		ret = RESULT_ERR;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: CheckStopReason
* ABSTRACT	: 检测外部导致的异常停止
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void CheckStopReason()
{
	int stopReason = STOP_NORMAL;
	int i = 0;

	if (g_faultPri.estop)		// 功率柜急停故障
	{
		stopReason = STOP_ESTOP;
	}
	else if (g_faultPri.liq)	// 功率柜液位告警
	{
		stopReason = STOP_LIQ;
	}
	else if (g_faultPri.door)	// 功率柜门打开
	{
		stopReason = STOP_DOOR;
	}
	else if (g_faultPri.CB)		// 断路器脱扣
	{
		stopReason = STOP_CB;
	}
	else if (g_faultPri.acRelay)	// 前端接触器反馈
	{
		stopReason = STOP_ACRELAY;
	}
	else if (g_faultPri.mdlOverVol)
	{
		stopReason = STOP_MDLOVERVOL;
	}
	else if (g_faultPri.mdlUnderVol)
	{
		stopReason = STOP_MDLUNDERVOL;
	}

	PwrCtrl_EMCStop(stopReason);

	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		stopReason = STOP_NORMAL;

		if (g_faultPri.pduErr[i])
		{
			stopReason = STOP_PDUERR;
		}
		else if (g_faultPri.pduCommErr[i])
		{
//			stopReason = STOP_PDUCOMM;
		}

		if (stopReason)
		{
			PwrCtrl_FaultStop(stopReason, i+1);
		}
	}

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		stopReason = STOP_NORMAL;

		if (g_faultPri.heartCommErr[i])
		{
			stopReason = STOP_HEARTOVERTIME;
		}
		else if (g_faultPri.chgCommErr[i])
		{
			stopReason = STOP_CHGOVERTIME;
		}
		
		if (stopReason)
		{
			PwrCtrl_FaultStop(stopReason, i+1);
		}
	}
}

/*
* *******************************************************************************
* MODULE	: StatusLedCtrl
* ABSTRACT	: 灯控制，用于整流柜，故障红，警告黄
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void StatusLedCtrl(int errCount, int warningCount)
{
	/* 故障灯控制 */
	if (errCount >= 3)
	{
		if (GetErrLed() == CLOSE)
		{
			SetErrLed(OPEN);
		}
	}
	else
	{
		if (GetErrLed() == OPEN)
		{
			SetErrLed(CLOSE);
		}
	}

	/* 警告灯控制 */
	if (warningCount >= 3)
	{
		if (GetWarnLed() == CLOSE)
		{
			SetWarnLed(OPEN);
		}
	}
	else
	{
		if (GetWarnLed() == OPEN)
		{
			SetWarnLed(CLOSE);
		}
	}
}

/*
* *******************************************************************************
* MODULE	: StatusLedCtrl2
* ABSTRACT	: 灯控制,用于一体机，待机绿，运行蓝，故障红，警告黄
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: status-1-运行、暂停，2-停止，3-停止中，4-故障，5-告警
*			  cardSta-刷卡状态，0-未知，1-刷卡成功，2-刷卡失败
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void StatusLedCtrl2(int gunId, int status, int cardSta, int errCount, int warningCount)
{
//	DEBUG("gunId = %d, status = %d, errCount = %d, warningCount = %d", gunId, status, errCount, warningCount);

	//开门灯控制
	int doorErrLED = GetDoor();
	if (doorErrLED == OPEN)
	{
		if (GetDoorLed() == CLOSE)
		{
			SetDoorLED(OPEN);
		}
	}
	else
	{
		if (GetDoorLed() == OPEN)
		{
			SetDoorLED(CLOSE);
		}
	}

	/* 故障灯控制 */
	if (errCount >= 3)
	{
//		DEBUG("%d", GetRedLed(gunId));
		if ((GetRedLed(gunId) == CLOSE) || (GetYellowLed(gunId) == OPEN))
		{
			SetYellowLed(CLOSE, gunId);
			SetAllLed(CLOSE, gunId);
			SetRedLed(OPEN, gunId);
		}
	}
	else if (((warningCount >= 3) || (status == 5)) && (cardSta == 0) && ((status != 1) || (status != 3)))
	{
//		DEBUG("%d, %d", gunId, GetYellowLed(gunId));
		if (GetYellowLed(gunId) == CLOSE)
		{
			SetAllLed(CLOSE, gunId);
			SetYellowLed(OPEN, gunId);
		}
	}
	else if (((status == 1) || (status == 3)) && (cardSta == 0))
	{
//		DEBUG("%d", GetBlueLed(gunId));
		if (GetBlueLed(gunId) == CLOSE)
		{
			SetAllLed(CLOSE, gunId);
			SetBlueLed(OPEN, gunId);
		}
	}
	else if ((status == 2) && (cardSta == 0))
	{
//		DEBUG("%d", GetGreenLed(gunId));
		if ((GetGreenLed(gunId) == CLOSE) || (GetYellowLed(gunId) == OPEN))
		{
			SetAllLed(CLOSE, gunId);
			SetGreenLed(OPEN, gunId);
		}
	}
	else if (status == 4)
	{
//		DEBUG("%d", GetRedLed(gunId));
		if ((GetRedLed(gunId) == CLOSE) || (GetYellowLed(gunId) == OPEN))
		{
			SetAllLed(CLOSE, gunId);
			SetRedLed(OPEN, gunId);
		}
	}

	/* 刷卡灯效操作 */
	if (cardSta)	// 正在刷卡
	{
		if (cardSta == 1)
		{
			if (abs(get_timetick()-g_faultPri.cardTick[gunId-1]) > 500)
			{
				g_faultPri.cardTick[gunId-1] = get_timetick();

				if (GetGreenLed(gunId) == CLOSE)
				{
					SetAllLed(CLOSE, gunId);
					SetGreenLed(OPEN, gunId);
				}
				else if (GetGreenLed(gunId) == OPEN)
				{
					SetAllLed(CLOSE, gunId);
					SetGreenLed(CLOSE, gunId);
				}
			}
		}
		else if (cardSta == 2)
		{
			if (abs(get_timetick()-g_faultPri.cardTick[gunId-1]) > 500)
			{
				g_faultPri.cardTick[gunId-1] = get_timetick();

				if (GetRedLed(gunId) == CLOSE)
				{
					SetAllLed(CLOSE, gunId);
					SetRedLed(OPEN, gunId);
				}
				else if (GetRedLed(gunId) == OPEN)
				{
					SetAllLed(CLOSE, gunId);
					SetRedLed(CLOSE, gunId);
				}
			}
		}
	}
}

/*
* *******************************************************************************
* MODULE	: FaultDeal
* ABSTRACT	: 定时操作
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void FaultDeal()
{
	int i = 0;
	int warningCount = 0;

	FlushCheckData();		// 刷数据

	CheckStopReason();		// 检测异常停止

	/* 立即停止充电 */
	if (abs(get_timetick() - g_faultPri.errorTick) > ERROR_TOUT)
	{
		g_faultPri.errorTick = get_timetick();

		ClearDTCAll();

		if (CheckError() == RESULT_ERR)
		{
			g_faultPri.errCount++;
			if (g_faultPri.errCount >= 3)	// 检测计数，连续三次出现故障则认为真的故障
			{
				g_faultPri.errCount = 3;
				g_faultPri.isEMC = 1;
			}
		}
		else
		{
			g_faultPri.isEMC = 0;
			g_faultPri.errCount = 0;
		}

		if (CheckUnitWarning() == RESULT_ERR)
		{
//			DEBUG("warningCount = %d", g_faultPri.warningCount[0]);
			g_faultPri.warningCount[0]++;
			if (g_faultPri.warningCount[0] >= 3)
			{
				g_faultPri.warningCount[0] = 3;
			}
		}
		else
		{
			g_faultPri.warningCount[0] = 0;
		}

		for (i = 0; i < GUN_DC_MAX_NUM; i++)
		{
			if (CheckGunWarning(i) == RESULT_ERR)
			{
				g_faultPri.warningCount[i+1]++;
				if (g_faultPri.warningCount[i+1] >= 3)
				{
					g_faultPri.warningCount[i+1] = 3;
				}
//				DEBUG("i = %d, warningCount = %d", i, g_faultPri.warningCount[i+1]);
			}
			else
			{
				g_faultPri.warningCount[i+1] = 0;
			}
		}

		if (ZmqCtrl_GetWorkMode() == 1)		// 一体机
		{
			for (i = 0; i < GUN_DC_MAX_NUM; i++)
			{
				warningCount = g_faultPri.warningCount[0]+g_faultPri.warningCount[i+1];
				StatusLedCtrl2(i+1, ZmqCtrl_GetChgSta(i+1), ZmqCtrl_GetCardSta(i+1), g_faultPri.errCount, warningCount);
			}
		}
		else		// 分体
		{
			for (i = 0; i < GUN_DC_MAX_NUM; i++)
			{
				if (warningCount < 3)
				{
					warningCount = g_faultPri.warningCount[0]+g_faultPri.warningCount[i+1];
				}
			}

			StatusLedCtrl(g_faultPri.errCount, warningCount);
		}
	}
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: Fault_GetEMC
* ABSTRACT	: 获得EMC停止
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int Fault_GetEMC()
{
	return g_faultPri.isEMC;
}

/*
* *******************************************************************************
* MODULE	: Fault_GetDTC
* ABSTRACT	: 获得DTC码
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void Fault_GetDTC(int *pDTC, int gunId)
{
	if (CheckGunId(gunId) == RESULT_ERR)
	{
		return;
	}

	pthread_mutex_lock(&mutex);

	gunId -= 1;
	memcpy(pDTC, g_DTC[gunId], sizeof(int)*DTC_MAX_NUM);

	pthread_mutex_unlock(&mutex);
}


/*
* *******************************************************************************
* MODULE	: Fault_Deal
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
void Fault_Deal()
{
	FaultDeal();
}

/*
* *******************************************************************************
* MODULE	: Fault_Init
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
void Fault_Init()
{
	pthread_mutex_init(&mutex, NULL);
}

/*
* *******************************************************************************
* MODULE	: Fault_Uninit
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
void Fault_Uninit()
{
	pthread_mutex_destroy(&mutex);
}
