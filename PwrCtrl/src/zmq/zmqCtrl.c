/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: zmqCtrl.c
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	: 
* DATE			: 20200222
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "zmqCtrl.h"
#include "zmqProt.h"
#include "timer.h"
#include "pwrCtrl.h"
#include "cnfJson.h"
#include "relayCtrl.h"
#include "smartOps.h"
#include "fanCtrl.h"
#include "exportCtrl.h"
#include "fault.h"

/*
* *******************************************************************************
*                                  Public variables
* *******************************************************************************
*/

#define 	PWRUNIT_ID		1

/*
* *******************************************************************************
*                                  Private variables
* *******************************************************************************
*/

#define WAIT_HEART_REQ_TIME				70000			// 30秒
#define WAIT_PWR_REQ_TIME				15000			// 15秒
#define WAIT_START_TIME 				8000

#pragma pack(1)

typedef struct
{
	int 			gunNum;							// 枪数量

	int 			heartOnline[GUN_DC_MAX_NUM];	// 枪是否在线，0-离线，1-在线
	int 			chgOnline[GUN_DC_MAX_NUM];		// 枪充电报文，0-离线，1-在线
	unsigned long 	heartTick[GUN_DC_MAX_NUM];		// 心跳报文超时时间
	unsigned long 	chgTick[GUN_DC_MAX_NUM];		// 充电报文超时时间
	char 			checkSetEn;						// 强起标识，0-未强起，1-强起

	char 			chgSta[GUN_DC_MAX_NUM];			// 枪状态
	char 			workMode;						// 工作模式,1-一体机，2-分体
	char 			cardSta[GUN_DC_MAX_NUM];		// 刷卡状态，0-未知，1-刷卡成功，2-刷卡失败

	char 			connId2TerId[GUN_DC_MAX_NUM];

	/* Deal中使用的变量 */
	unsigned long 	emcTick;
	unsigned long 	statusTick[GUN_DC_MAX_NUM];
	int 			pwrErr[GUN_DC_MAX_NUM][DTC_MAX_NUM];
	int 			OTAFlag;
}CTRL_PRI_STRUCT;

#pragma pack()

static CTRL_PRI_STRUCT	g_ctrlPri = {0};

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

	if ((gunId <= 0) || (gunId > GUN_DC_MAX_NUM))
	{
		ret = RESULT_ERR;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SetPowerResFrame
* ABSTRACT	: 设置帧及响应
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void SetPowerResFrame(FRAME_DATA_STRUCT *pframe, int requestId, int connId, int result)
{
	WEB_SET_RESULT_STRUCT *pbody = (WEB_SET_RESULT_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_SET_PWR_RES, strlen(MTD_SET_PWR_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->connId = connId;

	/* frame body */
	pbody->result = (result == 1 ? 1 : 0);
}

/*
* *******************************************************************************
* MODULE	: GetPowerStatusFrame
* ABSTRACT	: 获得功率数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetPowerStatusFrame(FRAME_DATA_STRUCT *pframe, int requestId, int connId)
{
	int i = 0;
	WEB_POWER_STATUS_STRUCT *pbody = (WEB_POWER_STATUS_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_GET_PWR_RES, strlen(MTD_GET_PWR_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->connId = connId;

	/* frame body */
	PWR_CNF_STRUCT cnf = {0};
	CNF_GetPwrCnf(&cnf);

	pbody->pwr.pwrType = cnf.powerType;
	pbody->pwr.pwrMax = cnf.powerMax;
	pbody->pwr.pwrNum = cnf.powerCount;
	pbody->pwr.pwrUnitNum = cnf.powerUnitNum;
	pbody->pwr.pduType = cnf.pduType;
	pbody->pwr.pduNum = cnf.pduCount;
	pbody->pwr.pduRlyNum = cnf.pduRelayCount;
	pbody->pwr.fanType = cnf.fanType;

	FAN_INFO_STRUCT fan = {0};
	FanCtrl_GetInfo(&fan, 1);

	pbody->status.estop = GetEStop() == CLOSE ? 1 : 2;
	pbody->status.sd = access("/media/mmcblk0p1/", 0) ? 2 : 1;
	pbody->status.fan = fan.online == 1 ? 1 : 2;
	pbody->status.outPower = PwrCtrl_GetTotalPwrOut();
	pbody->status.fanSpeed = fan.speed;

	pbody->mdlType = cnf.mdlType;
	pbody->mdlOverVol = cnf.mdlOverVol;
	pbody->mdlUnderVol = cnf.mdlUnderVol;
	
	for (i = 0; i < MDL_MAX_NUM; i++)
	{
		if (Mdl_GetMdlInfo(&(pbody->mdl[i]), i+1) == RESULT_OK)
		{
//			DEBUG("id = %d, fault = %d", pbody->mdl[i].id, pbody->mdl[i].fault);
			pbody->mdlCount++;
		}
	}

	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		if (RelayCtrl_GetInfo(&(pbody->pdu[i]), i+1) == RESULT_OK)
		{
//			DEBUG("id = %d, temp = %d", pbody->pdu[i].id, pbody->pdu[i].temp[0]);
			pbody->pduCount++;
		}
	}

	memcpy(&pbody->sensor, &cnf.sensors, sizeof(SENSORS_CNF_STRUCT));

	pbody->senInfo.doorVal = GetDoor() == CLOSE ? 1 : 2;
	pbody->senInfo.liqVal = GetWaterLevel() == CLOSE ? 1 : 2;
	pbody->senInfo.CBVal = GetCB() == CLOSE ? 1 : 2;
	pbody->senInfo.surgingVal = GetSurging() == CLOSE ? 1 : 2;
}

/*
* *******************************************************************************
* MODULE	: GetSmartopsFrame
* ABSTRACT	: 获得智能运维数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetSmartopsFrame(FRAME_DATA_STRUCT *pframe, int requestId, int connId)
{
	WEB_SMARTOPS_STRUCT *pbody = (WEB_SMARTOPS_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_STAT_RES, strlen(MTD_STAT_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->connId = connId;

	/* frame body */
	SmartOps_GetStatInfo(pbody);
}

/*
* *******************************************************************************
* MODULE	: GetHeartResFrame
* ABSTRACT	: heartRes数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetHeartResFrame(FRAME_DATA_STRUCT *pframe, int requestId, int terminalId, int connId)
{
	FRAME_DATA_HEART_STRUCT *pbody = (FRAME_DATA_HEART_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_HEART_RES, strlen(MTD_HEART_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->terminalId = terminalId;
	pframe->connId = connId;

	/* frame body */
	get_localtime(pbody->time);
}

/*
* *******************************************************************************
* MODULE	: GetStatusFrame
* ABSTRACT	: status数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetStatusFrame(FRAME_DATA_STRUCT *pframe, int requestId, int terminalId, int connId)
{
	FRAME_DATA_STATUS_STRUCT *pbody = (FRAME_DATA_STATUS_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_STATUS, strlen(MTD_STATUS));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->terminalId = terminalId;
	pframe->connId = connId;

	/* frame body */
	Fault_GetDTC(pbody->pwrErr, connId);
}

/*
* *******************************************************************************
* MODULE	: GetPwrResFrame
* ABSTRACT	: pwrRes数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetPwrResFrame(FRAME_DATA_STRUCT *pframe, int requestId, int terminalId, int connId)
{
	CHG_INFO_STRUCT chgInfo;
	FRAME_DATA_PWR_RES_STRUCT *pbody = (FRAME_DATA_PWR_RES_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_PWR_RES, strlen(MTD_PWR_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->terminalId = terminalId;
	pframe->connId = connId;

	if (PwrCtrl_GetChgInfo(&chgInfo, connId) == RESULT_ERR)
	{
		return;
	}

	/* frame body */
	pbody->pwrSta = chgInfo.pwrSta;
	pbody->EVSEPwrMax = chgInfo.EVSEPwrMax;
	pbody->EVSEVolMax = chgInfo.EVSEVolMax;
	pbody->EVSEVolMin = chgInfo.EVSEVolMin;
	pbody->EVSECurMax = chgInfo.EVSECurMax;
	pbody->realVol = chgInfo.volOut;
	pbody->realCur = chgInfo.curOut;
	pbody->stopReason = chgInfo.stopReason;

	switch (chgInfo.pwrStep)
	{
		case CHG_MAIN_FREE:
		case CHG_MAIN_STOP:
		case CHG_MAIN_EMC_STOP:
			pbody->pwrStep = 3;		// 停止
			break;
		case CHG_MAIN_RUN:
			pbody->pwrStep = 2;		// 准备完成
			break;
		default:
			pbody->pwrStep = 1;		// 切换中
			break;
	}
}

/*
* *******************************************************************************
* MODULE	: GetOTAResFrame
* ABSTRACT	: OTA数据赋值
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void GetOTAResFrame(FRAME_DATA_STRUCT *pframe, int requestId, int OTAStep, int failReason)
{
	FRAME_DATA_OTA_RES_STRUCT *pbody = (FRAME_DATA_OTA_RES_STRUCT *)(pframe->body);

	/* frame head */
	pframe->requestId = requestId;
	memcpy(pframe->method, MTD_OTA_RES, strlen(MTD_OTA_RES));
	pframe->pwrUnitId = PWRUNIT_ID;
	pframe->terminalId = 1;
	pframe->connId = 1;

	/* frame body */
	pbody->OTAStep = OTAStep;
	pbody->failReason = failReason;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrlDeal
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
static void ZmqCtrlDeal()
{
	int i = 0;
	FRAME_DATA_STRUCT frameData = {0};
	CHG_INFO_STRUCT chgInfo;
	int pwrErr[DTC_MAX_NUM] = {0};

	/* EMC停止，需要快速发送给各终端 */
	if (Fault_GetEMC())
	{
		if ((abs(get_timetick() - g_ctrlPri.emcTick) > 200))			// 周期发送
		{
			g_ctrlPri.emcTick = get_timetick();
			for (i = 0; i < GUN_DC_MAX_NUM; i++)
			{
				PwrCtrl_GetChgInfo(&chgInfo, i+1);
				if (chgInfo.pwrSta == 1)	// 运行
				{
					GetPwrResFrame(&frameData, get_timetick(), g_ctrlPri.connId2TerId[i], i+1);
					ZmqProt_SendPwrResFrame(frameData);
				}
			}
		}
	}

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		if (g_ctrlPri.heartOnline[i])		// 有心跳才会发送
		{
			Fault_GetDTC(pwrErr, i+1);
			if (memcmp(g_ctrlPri.pwrErr[i], pwrErr, sizeof(pwrErr)))	// 状态变化立马发送一次
			{
				GetStatusFrame(&frameData, get_timetick(), g_ctrlPri.connId2TerId[i], i+1);
				ZmqProt_SendStatusFrame(frameData);
				memcpy(g_ctrlPri.pwrErr[i], pwrErr, sizeof(pwrErr));
			}

			if ((abs(get_timetick() - g_ctrlPri.statusTick[i]) > 30000))			// 周期发送
			{
				g_ctrlPri.statusTick[i] = get_timetick();
				GetStatusFrame(&frameData, get_timetick(), g_ctrlPri.connId2TerId[i], i+1);
				ZmqProt_SendStatusFrame(frameData);
			}
		}
	}

	/* 心跳报文超时检测 */
	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		if ((abs(get_timetick() - g_ctrlPri.heartTick[i]) > WAIT_HEART_REQ_TIME) && (g_ctrlPri.heartTick[i]))
		{
			g_ctrlPri.heartOnline[i] = 0;
		}

		if (g_ctrlPri.checkSetEn)		// 强起的时候，不判断报文超时
		{
			g_ctrlPri.heartOnline[i] = 1;
		}
	}

	/* 充电报文超时检测 */
	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		if ((abs(get_timetick() - g_ctrlPri.chgTick[i]) > WAIT_PWR_REQ_TIME) && (g_ctrlPri.chgTick[i]))
		{
			g_ctrlPri.chgOnline[i] = 0;
			g_ctrlPri.chgTick[i] = 0;
		}

		if (g_ctrlPri.checkSetEn)		// 强起的时候，不判断报文超时
		{
			g_ctrlPri.chgOnline[i] = 1;
		}
	}

	/* OTA升级标志发送，主要用于板子重启之后的第一次 */
	/*if (g_ctrlPri.OTAFlag)
	{
		PWR_CNF_STRUCT cnf = {0};
		if (CNF_GetPwrCnf(&cnf) == RESULT_OK)
		{
			if (cnf.OTAStep != 0)	// 发送升级完成报文
			{
				GetOTAResFrame(&frameData, get_timetick(), cnf.OTAStep, 0);
				ZmqProt_SendOTAResFrame(frameData);
			}

			cnf.OTAStep = 0;
			CNF_SetPwrCnf(cnf);
		}

		g_ctrlPri.OTAFlag = 0;
	}*/
}

/*
* *******************************************************************************
* MODULE	: EventHandle
* ABSTRACT	: 事件处理
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int EventHandle(unsigned char *pframe, int len)
{
	FRAME_DATA_STRUCT *pdata = (FRAME_DATA_STRUCT *)pframe;
	FRAME_DATA_STRUCT frameData = {0};
	int requestId = pdata->requestId;
	int terminalId = pdata->terminalId;
	int connId = pdata->connId;
	int result = 0;
	int i = 0;

	static unsigned long startTick  = 0;
	if (startTick == 0)
	{
		startTick = get_timetick();
	}

	if (abs(get_timetick() - startTick) < WAIT_START_TIME)	// 程序启动的开始，不处理数据，因为会出现一下子收到很多数据的情况
	{
		return RESULT_ERR;
	}

	if (CheckGunId(connId) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	if (!strcmp(pdata->method, MTD_HEART_REQ))
	{
		FRAME_DATA_HEART_STRUCT *pbody = (FRAME_DATA_HEART_STRUCT *)(pdata->body);
		struct tm tm_time;
		strptime(pbody->time, "%Y-%m-%d %H:%M:%S", &tm_time);

		if ((abs(time(NULL) - mktime(&tm_time)) > 3) && (terminalId == 1))	// 时间误差大于3s，只对1号终端有效
		{
			DEBUG("Time : %s", pbody->time);
			set_localtime(pbody->time);		// 对时
		}
		
		/* 回复响应帧 */
		GetHeartResFrame(&frameData, requestId, terminalId, connId);
		ZmqProt_SendHeartResFrame(frameData);

		g_ctrlPri.heartTick[connId-1] = get_timetick();
		g_ctrlPri.heartOnline[connId-1] = 1;
		g_ctrlPri.gunNum = g_ctrlPri.gunNum < connId ? connId : g_ctrlPri.gunNum;

		g_ctrlPri.connId2TerId[connId-1] = terminalId;
	}
	if (!strcmp(pdata->method, MTD_STATUS))
	{
		FRAME_DATA_STATUS_REQ_STRUCT *pbody = (FRAME_DATA_STATUS_REQ_STRUCT *)(pdata->body);

//		DEBUG("connId = %d, chgSta = %d", connId, pbody->chgSta);
		g_ctrlPri.chgSta[connId-1] = pbody->chgSta;
		g_ctrlPri.workMode = pbody->workMode;
		g_ctrlPri.cardSta[connId-1] = pbody->cardSta;

		if ((pbody->chgSta != 1) && (g_ctrlPri.chgOnline[connId-1] == 0))
		{
			g_ctrlPri.chgOnline[connId-1] = 1;		// 当没有收到停止报文时，会一直报充电报文停止超时故障，故在此做个异常消除
		}
	}
	else if (!strcmp(pdata->method, MTD_PWR_REQ))
	{
		FRAME_DATA_PWR_REQ_STRUCT *pbody = (FRAME_DATA_PWR_REQ_STRUCT *)(pdata->body);

//		DEBUG("connId = %d, chgSta = %d, heartOnline = %d", connId, pbody->chgSta, g_ctrlPri.heartOnline[connId-1]);
		
		if ((pbody->chgSta == 1) && (g_ctrlPri.heartOnline[connId-1])
			&& (pbody->chgStep >= 2) && (SmartOps_GetBackCheck() != 0))		// 启动充电，首先确保在线，并且桩自检
		{
			if (pbody->EVVolMax && pbody->EVCurMax)
			{
				pbody->EVPwrMax = pbody->EVVolMax * pbody->EVCurMax / POWER_CALC_RATIO;
			}

			PwrCtrl_StartStop(CHG_START, 0, connId);
			PwrCtrl_SetChgAllow(pbody->EVVolMax, pbody->EVCurMax, pbody->EVPwrMax, pbody->GunPwrMax, connId);
			PwrCtrl_SetChgUI(pbody->reqVol, pbody->reqCur, pbody->reqVol * pbody->reqCur / POWER_CALC_RATIO, connId);

			g_ctrlPri.chgTick[connId-1] = get_timetick();
			g_ctrlPri.chgOnline[connId-1] = 1;
		}
		else if (pbody->chgSta == 2)				// 停止充电
		{
			PwrCtrl_StartStop(CHG_STOP, pbody->stopReason, connId);

			g_ctrlPri.chgTick[connId-1] = 0;
			g_ctrlPri.chgOnline[connId-1] = 1;		// 目的是在停止充电后恢复chgOnline状态，防止fault那一直报故障
		}
		
		/* 回复响应帧 */
		GetPwrResFrame(&frameData, requestId, terminalId, connId);
		ZmqProt_SendPwrResFrame(frameData);
	}
	else if (!strcmp(pdata->method, MTD_OTA_REQ))
	{
		FRAME_DATA_OTA_REQ_STRUCT *pbody = (FRAME_DATA_OTA_REQ_STRUCT *)(pdata->body);
		int step = 4, failReason = 0xFF;
		DEBUG("OTAEnable = %c", pbody->OTAEnable);
		if (terminalId == 1)	// 只对1号终端响应
		{
			if (pbody->OTAEnable == 1)	// 开始升级
			{
				if (PwrCtrl_GetChgIsBusy() == CHG_MAIN_FREE)	// 所有枪空闲时，才能升级
				{
					GetOTAResFrame(&frameData, requestId, 1, 0);
					ZmqProt_SendOTAResFrame(frameData);
					usleep(1000*1000);

					DEBUG("OTAPath = %s", (char *)pbody->OTAPath);
					result = CurlDownloadFile((char *)pbody->OTAPath, "root:dh123", "/tmp/ubi.tar.gz", 3, 30, 1);
					usleep(2000 * 1000);

					if (result == 0)
					{
						step = 2;
						failReason = 0;
					}
					else
					{
						failReason = 2;
					}
				}
				else
				{
					failReason = 1;
				}
			}
		}

		GetOTAResFrame(&frameData, requestId, step, failReason);
		ZmqProt_SendOTAResFrame(frameData);

		if (step == 2)
		{
			PWR_CNF_STRUCT cnf = {0};
			if (CNF_GetPwrCnf(&cnf) == RESULT_OK)
			{
//				cnf.OTAStep = 3;
//				CNF_SetPwrCnf(cnf);
				usleep(2000*1000);
				mkdir("/ubi/upgrade", 0755);
				system("cp /tmp/ubi.tar.gz /ubi/upgrade/ubi.tar.gz");

				step = 3;
				GetOTAResFrame(&frameData, requestId, step, failReason);
				ZmqProt_SendOTAResFrame(frameData);

				usleep(2000*1000);
				system("reboot -f");

				DEBUG("ota ok");
			}
		}
	}
	else if (!strcmp(pdata->method, MTD_SET_PWR_REQ))
	{
		WEB_SET_STRUCT *pbody = (WEB_SET_STRUCT *)(pdata->body);
		if (pbody->pwrSetEn)			// 整流柜参数设置
		{
			PWR_CNF_STRUCT cnf = {0};
			if (CNF_GetPwrCnf(&cnf) == RESULT_OK)
			{
				cnf.powerType = pbody->pwr.pwrType;
				cnf.powerMax = pbody->pwr.pwrMax;
				cnf.powerCount = pbody->pwr.pwrNum;
				cnf.powerUnitNum = pbody->pwr.pwrUnitNum;
				cnf.pduType = pbody->pwr.pduType;
				cnf.pduCount = pbody->pwr.pduNum;
				cnf.pduRelayCount = pbody->pwr.pduRlyNum;
				cnf.fanType = pbody->pwr.fanType;

				CNF_SetPwrCnf(cnf);
				SetMainStatusPending(1, MAIN_STATUS_PWR_CHANGE);
				SetMainStatusPending(1, MAIN_STATUS_FAN_CHANGE);
				SetMainStatusPending(1, MAIN_STATUS_RELAY_CHANGE);
				result = 1;
			}
		}
		else if (pbody->mdlSetEn)		// 模块参数设置
		{
			PWR_CNF_STRUCT cnf = {0};
			if (CNF_GetPwrCnf(&cnf) == RESULT_OK)
			{
				cnf.mdlType = pbody->mdl.mdlType;
				cnf.mdlCount = pbody->mdl.mdlNum;
				cnf.mdlUnderVol = pbody->mdl.mdlUnderVol;
				cnf.mdlOverVol = pbody->mdl.mdlOverVol;

				for (i = 0; i < pbody->mdl.mdlNum; i++)
				{
					cnf.mdl[i].groupId = pbody->mdl.sn[i].groupId;
					memcpy(cnf.mdl[i].SN, pbody->mdl.sn[i].SN, MDL_SN_MAX_LENGTH);
				}

				CNF_SetPwrCnf(cnf);
				SetMainStatusPending(1, MAIN_STATUS_MODULE_CHANGE);
				result = 1;
			}
		}
		else if (pbody->sensorSetEn)		// 传感器参数设置
		{
			PWR_CNF_STRUCT cnf = {0};
			if (CNF_GetPwrCnf(&cnf) == RESULT_OK)
			{
				memcpy(&cnf.sensors, &pbody->sensor, sizeof(SENSORS_CNF_STRUCT));

				CNF_SetPwrCnf(cnf);
				result = 1;
			}
		}
		else if (pbody->hwSetEn)			// 硬件检测
		{
//			if (PwrCtrl_GetChgIsBusy() == CHG_MAIN_FREE)	// 所有枪空闲时，才能硬件检测
			{
				if (SmartOps_HWCheck(pbody->hw) == 1)
				{
					result = 1;
				}
			}

			if (pbody->checkSetEn)			// 功率检测
			{
				if ((pbody->pwrCheck == 1) && pbody->pwrGunId)
				{
					int pwr = pbody->pwrVol * pbody->pwrCur / POWER_CALC_RATIO;
					PwrCtrl_StartStop(CHG_START, 0, pbody->pwrGunId);
					PwrCtrl_SetChgAllow(pbody->pwrVol, pbody->pwrCur, pwr, pwr, pbody->pwrGunId);
					PwrCtrl_SetChgUI(pbody->pwrVol, pbody->pwrCur, pwr, pbody->pwrGunId);

					g_ctrlPri.checkSetEn = 1;
					g_ctrlPri.heartOnline[pbody->pwrGunId-1] = 1;
					g_ctrlPri.chgOnline[pbody->pwrGunId-1] = 1;
				}
				else
				{
					PwrCtrl_StartStop(CHG_STOP, 0, pbody->pwrGunId);

					g_ctrlPri.checkSetEn = 0;
				}

				result = 1;
			}
		}

		/* 回复响应帧 */
		SetPowerResFrame(&frameData, requestId, connId, result);
		ZmqProt_SendSetPwrResFrame(frameData);
	}
	else if (!strcmp(pdata->method, MTD_GET_PWR_REQ))
	{
		/* 回复响应帧 */
		GetPowerStatusFrame(&frameData, requestId, connId);
		ZmqProt_SendGetPwrResFrame(frameData);
	}
	else if (!strcmp(pdata->method, MTD_STAT_REQ))
	{
		/* 回复响应帧 */
		GetSmartopsFrame(&frameData, requestId, connId);
		ZmqProt_SendStatResFrame(frameData);
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

HANDLE_STRUCT handle = {
	.GetInfoFuns = NULL,
	.eventHandle = &EventHandle,
};

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_GetChgOnline
* ABSTRACT	: 获得充电状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-掉线 1-在线
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int ZmqCtrl_GetChgOnline(int gunId)
{
	int ret = 0;

	if (gunId > g_ctrlPri.gunNum)
	{
		return RESULT_ERR;
	}

	gunId -= 1;
	ret = g_ctrlPri.chgOnline[gunId];

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_GetHeartOnline
* ABSTRACT	: 获得心跳状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-掉线 1-在线
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int ZmqCtrl_GetHeartOnline(int gunId)
{
	int ret = 0;

	if (gunId > g_ctrlPri.gunNum)
	{
		return RESULT_ERR;
	}
	
	gunId -= 1;
	ret = g_ctrlPri.heartOnline[gunId];

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_GetChgSta
* ABSTRACT	: 获得工作状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 1-运行、暂停，2-停止，3-故障
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int ZmqCtrl_GetChgSta(int gunId)
{
	int ret = 0;

	if (gunId > g_ctrlPri.gunNum)
	{
		return RESULT_ERR;
	}

	gunId -= 1;
	ret = g_ctrlPri.chgSta[gunId];

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_GetWorkMode
* ABSTRACT	: 获得工作模式
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 1-一体机，2-分体
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int ZmqCtrl_GetWorkMode()
{
	return g_ctrlPri.workMode;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_GetCardSta
* ABSTRACT	: 获得刷卡状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 刷卡状态，0-未知，1-刷卡成功，2-刷卡失败
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int ZmqCtrl_GetCardSta(int gunId)
{
	int ret = 0;

	if (gunId > g_ctrlPri.gunNum)
	{
		return RESULT_ERR;
	}

	gunId -= 1;
	ret = g_ctrlPri.cardSta[gunId];

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_Deal
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
void ZmqCtrl_Deal()
{
	ZmqCtrlDeal();
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_Init
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
void ZmqCtrl_Init()
{
	int i = 0;
	ZmqProt_Init(&handle);

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		g_ctrlPri.chgOnline[i] = 1;
	}
}

/*
* *******************************************************************************
* MODULE	: ZmqCtrl_Uninit
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
void ZmqCtrl_Uninit()
{

}