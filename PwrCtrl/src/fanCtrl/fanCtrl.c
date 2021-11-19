/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: fanCtrl.c
* SYSTEM NAME	: 风扇控制
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	: 
* DATE			: 20200214
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "fanCtrl.h"
#include "./plug/fanEBM.h"
#include "./plug/fanPWM.h"
#include "pwrCtrl.h"
#include "timer.h"
#include "exportCtrl.h"
#include "modCtrl.h"

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

#define WAIT_FANRELAY_CLOSE_DELAY_TIME			20000			// 20s

#pragma pack(1)

typedef struct
{
	int pwr;
	int temp;
	int speed;
}SPEED_TABLE_STRUCT;

typedef struct
{
	unsigned char 	fanRelay;		// 风扇继电器状态
	unsigned long 	fanRelayTick;	// 风扇继电器闭合时间

	int 			speed;			// 转速，0-100%
	int 			hwMode;			// 0-正常工作，1-调试模式

	/* Deal中使用的变量 */
	unsigned long 	fanTick;
}FAN_PRI_STRUCT;

#pragma pack()

static 	FAN_FUNCTIONS_STRUCT 	*g_pFanFun = NULL;
static 	FAN_PARA_STRUCT			g_fanPara = {0};
static 	FAN_PRI_STRUCT			g_fanPri = {0};

static 	pthread_mutex_t 		g_mutex[FAN_MAX_NUM];		// 锁

static 	SPEED_TABLE_STRUCT 		g_EBMSpeedTable[] = {
	// pwr 		// temp 	// speed
	{	0,			90,			0	},
	{	600,		95,			20	},
	{	1200,		100,		40	},
	{	1800,		105,		60	},
	{	2400,		110,		80	},
	{	3500,		113,		98	}
};

static 	SPEED_TABLE_STRUCT 		g_PWMSpeedTable[] = {
	// pwr 		// temp 	// speed
	{	0,			90,			0	},
	{	300,		95,			20	},
	{	600,		100,		40	},
	{	900,		105,		60	},
	{	1200,		110,		80	},
	{	1700,		113,		98	}
};

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
* MODULE	: CheckFanId
* ABSTRACT	: 检查风扇编号
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int CheckFanId(int fanId)
{
	int ret = RESULT_OK;

	if ((fanId <= 0) || (fanId > g_fanPara.fanNum))
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

	for (i = 0; i < FAN_MAX_NUM; i++)
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

	for (i = 0; i < FAN_MAX_NUM; i++)
	{
		pthread_mutex_destroy(&g_mutex[i]);
	}
}
#if 0
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
static void Lock(int fanId)
{
	fanId -= 1;
	pthread_mutex_lock(&g_mutex[fanId]);
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
static void Unlock(int fanId)
{
	fanId -= 1;
	pthread_mutex_unlock(&g_mutex[fanId]);
}
#endif
/*
* *******************************************************************************
* MODULE	: MatchSpeed
* ABSTRACT	: 根据功率、温度查表匹配转速
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int MatchSpeed(int pwr, int temp)
{
	int i = 0;
	int tblNum = 0;
	int speed = 0;
	SPEED_TABLE_STRUCT *speedTable = NULL;

	switch (g_fanPara.fanType)
	{
		case FAN_EBM:
			speedTable = g_EBMSpeedTable;
			tblNum = sizeof(g_EBMSpeedTable) / sizeof(SPEED_TABLE_STRUCT);
			break;
		case FAN_PWM:
			speedTable = g_PWMSpeedTable;
			tblNum = sizeof(g_PWMSpeedTable) / sizeof(SPEED_TABLE_STRUCT);
			break;
		default:
			speedTable = g_EBMSpeedTable;
			tblNum = sizeof(g_EBMSpeedTable) / sizeof(SPEED_TABLE_STRUCT);
			break;
	}


	for (i = 0; i < tblNum; i++)
	{
		if ((pwr > speedTable[i].pwr) || (temp > speedTable[i].temp))
		{
			speed = speedTable[i].speed;
		}
		else
		{
			break;
		}
	}

	return speed;
}

/*
* *******************************************************************************
* MODULE	: FlushOtherInfo
* ABSTRACT	: 获取其他功能模块的数据
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
	/* 当前功率及整流柜温度或PDU温度 */
	int pwr = PwrCtrl_GetTotalPwrOut();
	int temp = 0;
	int i = 0;

	MDL_INFO_STRUCT mdl;
	for (i = 0; i < MDL_MAX_NUM; i++)
	{
		if (Mdl_GetMdlInfo(&mdl, i) == RESULT_OK)
		{
			temp = temp < mdl.temp ? mdl.temp : temp;
		}
	}

	g_fanPri.speed = MatchSpeed(pwr, temp/10);

	/* 风扇继电器控制 */
	if (PwrCtrl_GetChgIsBusy() != CHG_MAIN_FREE)	// 有枪输出
	{
		if (GetFanRelay() == CLOSE)
		{
			SetFanRelay(OPEN);
		}
		else if (GetFanRelay() == OPEN)
		{
			g_fanPri.fanRelay = 1;
		}
	}
	else	// 枪全部停止了,等一会再断开风扇继电器
	{
		if (g_fanPri.fanRelay && (g_fanPri.speed == 0) && (g_fanPri.fanRelayTick == 0))
		{
			g_fanPri.fanRelayTick = get_timetick();
		}
		else if ((abs(get_timetick() - g_fanPri.fanRelayTick) > WAIT_FANRELAY_CLOSE_DELAY_TIME)
															&& (g_fanPri.fanRelayTick))
		{
			if (GetFanRelay() == OPEN)
			{
				SetFanRelay(CLOSE);
			}
			g_fanPri.fanRelay = 0;
			g_fanPri.fanRelayTick = 0;
		}
	}
}

/*
* *******************************************************************************
* MODULE	: FanDeal
* ABSTRACT	: 智能风扇，根据风扇个数、当前功率温度等信息，查表来实时控制风扇转速等
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void FanDeal()
{
	int i = 0;

	if (abs(get_timetick() - g_fanPri.fanTick) > 2000)
	{
		FlushOtherInfo();

		if ((g_fanPri.hwMode == 0) && g_fanPri.fanRelay)		// 目前处于正常工作模式
		{
			g_fanPri.fanTick = get_timetick();

			for (i = 0; i < g_fanPara.fanNum; i++)
			{
				if (g_pFanFun)	// 转起来
				{
					g_pFanFun->SetSpeed(i+1, g_fanPri.speed);
				}
			}				
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
* MODULE	: FanCtrl_GetInfo
* ABSTRACT	: 获得风扇数据
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int FanCtrl_GetInfo(FAN_INFO_STRUCT *pInfo, int fanNo)
{
	if (CheckFanId(fanNo) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	if (g_pFanFun)
	{
		g_pFanFun->GetInfo(fanNo, pInfo);
	}
	else
	{
		return RESULT_ERR;
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: FanCtrl_SetPara
* ABSTRACT	: 设置风扇配置参数
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void FanCtrl_SetPara(FAN_PARA_STRUCT para)
{
	int i = 0;
	g_fanPara.fanNum = para.fanNum;

	if (g_fanPara.fanType != para.fanType)
	{
		if (g_pFanFun)
		{
			g_pFanFun->Uninit();
		}

		g_fanPara.fanType = para.fanType;
		switch (g_fanPara.fanType)
		{
			case FAN_EBM:
				g_pFanFun = Get_FanEBM_Functions();
				break;
			case FAN_PWM:
				g_pFanFun = Get_FanPWM_Functions();
				break;
			default:
				g_pFanFun = Get_FanEBM_Functions();
				break;
		}

		if (g_pFanFun)
		{
			g_pFanFun->Init();
		}

		for (i = 0; i < g_fanPara.fanNum; i++)
		{
			g_pFanFun->SetSpeed(i+1, 0);
		}	
	}
}

/*
* *******************************************************************************
* MODULE	: FanCtrl_SetSpeed
* ABSTRACT	: 设置风扇转速,主要用于调试模式，speed>0则进入调试模式，speed==0则正常工作
* FUNCTION	: 
* ARGUMENT	: 
* NOTE		: 
* RETURN	: void
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void FanCtrl_SetSpeed(int speed)
{
	int i = 0;
	for (i = 0; i < g_fanPara.fanNum; i++)
	{
		if (g_pFanFun)
		{
			g_pFanFun->SetSpeed(i+1, speed);
		}
	}

	g_fanPri.hwMode = speed ? 1 : 0;
}

/*
* *******************************************************************************
* MODULE	: FanCtrl_Deal
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
void FanCtrl_Deal()
{
	FanDeal();
}

/*
* *******************************************************************************
* MODULE	: Fan_Init
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
void FanCtrl_Init()
{
	LockInit();
}

/*
* *******************************************************************************
* MODULE	: Fan_Uninit
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
void FanCtrl_Uninit()
{
	if (g_pFanFun)
	{
		g_pFanFun->Uninit();
	}

	LockDestroy();
}