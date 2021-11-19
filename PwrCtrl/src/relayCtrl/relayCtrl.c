/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: relayCtrl
* SYSTEM NAME	: 功率继电器控制
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	:
* DATE			: 20200221
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "relayCtrl.h"
#include "./plug/relaySCII.h"

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

static 	RELAY_FUNCTIONS_STRUCT *g_pRelayFun = NULL;
static	RELAY_PARA_STRUCT		g_rlyPara = {0};

static pthread_mutex_t 			g_mutex[PDU_MAX_NUM];			// 锁

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
* MODULE	: CheckRlyId
* ABSTRACT	: 检查PDU编号
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int CheckRlyId(int rlyId)
{
	int ret = RESULT_OK;

	if ((rlyId <= 0) || (rlyId > g_rlyPara.pduNum))
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

	for (i = 0; i < PDU_MAX_NUM; i++)
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

	for (i = 0; i < PDU_MAX_NUM; i++)
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
static void Lock(int rlyId)
{
	rlyId -= 1;
	pthread_mutex_lock(&g_mutex[rlyId]);
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
static void Unlock(int rlyId)
{
	rlyId -= 1;
	pthread_mutex_unlock(&g_mutex[rlyId]);
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: RelayCtrl_GetCtrlBack
* ABSTRACT	: 获得继电器控制的反馈状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int RelayCtrl_GetCtrlBack(int relayNo)
{
	if (CheckRlyId(relayNo) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	Lock(relayNo);

	RELAY_INFO_STRUCT info = {0};
	if (g_pRelayFun)
	{
		g_pRelayFun->GetInfo(&info, relayNo);
	}

	Unlock(relayNo);

	return info.ctrlBack;
}

/*
* *******************************************************************************
* MODULE	: RelayCtrl_GetInfo
* ABSTRACT	: 获取继电器信息
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int RelayCtrl_GetInfo(RELAY_INFO_STRUCT *pInfo, int relayNo)
{
	if (CheckRlyId(relayNo) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	Lock(relayNo);

	if (g_pRelayFun)
	{
		g_pRelayFun->GetInfo(pInfo, relayNo);
	}

	Unlock(relayNo);

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: RelayCtrl_SetSwitch
* ABSTRACT	: 控制继电器
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int RelayCtrl_SetSwitch(unsigned char *pSW, int relayNo)
{
	if (CheckRlyId(relayNo) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	Lock(relayNo);

	if (g_pRelayFun)
	{
		g_pRelayFun->SetSwitch(pSW, relayNo);
	}

	Unlock(relayNo);

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: RelayCtrl_SetPara
* ABSTRACT	: 设置参数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void RelayCtrl_SetPara(RELAY_PARA_STRUCT para)
{
	g_rlyPara.pduNum = para.pduNum;
	g_rlyPara.pduRlyNum = para.pduRlyNum;
	g_rlyPara.pduTempLevel = para.pduTempLevel;

	if (g_rlyPara.pduType != para.pduType)
	{
		if (g_pRelayFun)
		{
			g_pRelayFun->Uninit();
		}

		g_rlyPara.pduType = para.pduType;
		switch (g_rlyPara.pduType)
		{
			case RELAY_SCII:
				g_pRelayFun = Get_RelaySCII_Functions();
				system("ip link set can1 down");
				usleep(100 * 1000);
				system("ip link set can1 type can bitrate 250000");
				usleep(100 * 1000);
				system("ip link set can1 up");
				usleep(100 * 1000);
				break;
			default:
				g_pRelayFun = Get_RelaySCII_Functions();
				system("ip link set can1 down");
				usleep(100 * 1000);
				system("ip link set can1 type can bitrate 250000");
				usleep(100 * 1000);
				system("ip link set can1 up");
				usleep(100 * 1000);
				break;
		}

		if (g_pRelayFun)
		{
			g_pRelayFun->Init();
		}
	}
}

/*
* *******************************************************************************
* MODULE	: RelayCtrl_Init
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
void RelayCtrl_Init()
{
	LockInit();
}

/*
* *******************************************************************************
* MODULE	: RelayCtrl_Uninit
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
void RelayCtrl_Uninit()
{
	if (g_pRelayFun)
	{
		g_pRelayFun->Uninit();
	}
	LockDestroy();
}