/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: fanPWM.c
* SYSTEM NAME	: PWM风扇
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	: 
* DATE			: 20200519
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "fanPWM.h"
#include "exportCtrl.h"
#include "timer.h"

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

static 	FAN_INFO_STRUCT			g_fanInfo[FAN_MAX_NUM] = { 0 };			// 风扇信息结构体
static 	unsigned long 			g_fanRunTick = 0;						// 风扇运行时间

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
* MODULE	: ClearInfo
* ABSTRACT	: 清除风扇信息
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ClearInfo(int addr)
{
	g_fanInfo[addr].speed = 0;
	g_fanInfo[addr].temp = 0;
	g_fanInfo[addr].runTime = 0;
	g_fanInfo[addr].error = 0;
	g_fanInfo[addr].warning = 0;
	g_fanRunTick = 0;
}

/*
* *******************************************************************************
* MODULE	: SetSpeed
* ABSTRACT	: 设置转速
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetSpeed(int addr, int speed)
{
	int ret = RESULT_ERR;
	if ((speed >= 0) && (speed <= 100))
	{
		speed = 100 - speed;
		SetFanSpeed(speed);
		
		if (speed == 0)
		{
			ClearInfo(0);
		}
		else
		{
			g_fanInfo[0].speed = 100-speed;
			g_fanRunTick = get_timetick();
		}
		
		ret = RESULT_OK;
	}
	
	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetInfo
* ABSTRACT	: 获得风扇信息
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetInfo(int addr, FAN_INFO_STRUCT *pInfo)
{
	g_fanInfo[0].online = 1;
	if (g_fanRunTick)
	{
		g_fanInfo[0].runTime += abs(get_timetick()-g_fanRunTick) / 1000;
		g_fanRunTick = get_timetick();
	}
	
	memcpy(pInfo, &g_fanInfo[0], sizeof(FAN_INFO_STRUCT));

	return 0;
}

/*
* *******************************************************************************
* MODULE	: Init
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
static void Init(void)
{

}

/*
* *******************************************************************************
* MODULE	: Uninit
* ABSTRACT	: 销毁
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void Uninit(void)
{

}

FAN_FUNCTIONS_STRUCT	FAN_PWM_Functions =
{
	.Init					= Init,
	.Uninit					= Uninit,
	.SetSpeed				= SetSpeed,
	.GetInfo				= GetInfo,
};

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: Get_FanPWM_Functions
* ABSTRACT	: 获取函数指针
* FUNCTION	: 
* ARGUMENT	: FAN_FUNCTIONS_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
FAN_FUNCTIONS_STRUCT *Get_FanPWM_Functions(void)
{
	return &FAN_PWM_Functions;
}