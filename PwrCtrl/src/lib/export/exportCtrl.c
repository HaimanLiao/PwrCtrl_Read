/*
* *******************************************************************************
*	@(#)Copyright (C) 2019-2030
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: exportCtrl.c
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan 
* DEPARTMENT	: 
* DATE			: 2020-04-30
* *******************************************************************************
*/


/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/
#include "exportCtrl.h"
#include "export.h"
#include "export_ctrl.h"
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

#define		RELAY_BACK_ENABLE		1
#define		RELAY_BACK_DISABLE		0
#define		RELAY_BACK_OVERTIME		1000

#pragma pack(1)
typedef struct
{
	int 			fanRelayBack;	// 风扇继电器反馈		RELAY_BACK_ENABLE RELAY_BACK_DISABLE
	int 			fanRelayCtrl;	// 风扇继电器控制		OPEN CLOSE

	int 			acRelayCtrl;	// 接触器控制		OPEN CLOSE
	unsigned long 	acRelayTick;	// 接触器反馈计时

	int 			warnLedCtrl;	// 警告灯控制		OPEN CLOSE
	int 			errLedCtrl;		// 故障灯控制		OPEN CLOSE

	int				dorrLEDCtrl;	//开门灯控制		OPEN CLOSE

	int 			redLedCtrl[2];
	int 			greenLedCtrl[2];
	int 			blueLedCtrl[2];
	int 			yellowLedCtrl[2];
	int 			allLedCtrl[2];
}EXPORT_PRI_STRUCT;

#pragma pack()

static 	EXPORT_PRI_STRUCT	g_exportPri = {0};

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
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: GetWarnLed
* ABSTRACT	: 警告灯状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: 0-无反馈 1-反馈
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int GetWarnLed()
{
	int ret = g_exportPri.warnLedCtrl;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetErrLed
* ABSTRACT	: 故障灯状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: 0-无反馈 1-反馈
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int GetErrLed()
{
	int ret = g_exportPri.errLedCtrl;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetDoorLed
* ABSTRACT	: 开门灯状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: 0-无反馈 1-反馈
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int GetDoorLed()
{
	int ret = g_exportPri.dorrLEDCtrl;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetCB
* ABSTRACT	: 断路器脱扣状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: 0-无反馈 1-反馈
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int GetCB()
{
	int ret = CLOSE;

	ret = GetInputSta(IO_VIN3);
	ret = ret == 0 ? CLOSE : OPEN;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetSurging
* ABSTRACT	: 浪涌状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: 0-浪涌无反馈 1-浪涌反馈
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int GetSurging()
{
	int ret = CLOSE;

	ret = GetInputSta(IO_VIN4);
	ret = ret == 0 ? CLOSE : OPEN;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetQStop
* ABSTRACT	: 急停状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-未按下 1-急停按下
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetEStop()
{
	int ret = CLOSE;

	ret = GetInputSta(IO_VIN1);
	ret = ret == 0 ? CLOSE : OPEN;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetDoor
* ABSTRACT	: 门禁状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-未打开 1-打开
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetDoor()
{
	int ret = CLOSE;

	ret = GetInputSta(IO_VIN2);
	ret = ret == 0 ? CLOSE : OPEN;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetWaterLevel
* ABSTRACT	: 液位状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-未触发 1-触发
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetWaterLevel()
{
	int ret = CLOSE;

	ret = GetInputSta(IO_VIN6);
	ret = ret == 0 ? CLOSE : OPEN;

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetACRelay
* ABSTRACT	: 前端接触器反馈
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-断开 1-闭合 2-反馈超时
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetACRelay()
{	
	return g_exportPri.acRelayCtrl;
	
	int ret = CLOSE;
	int back = 0;

	back = GetInputSta(IO_VIN6);

	if (g_exportPri.acRelayTick)
	{
		if (g_exportPri.acRelayCtrl == back)	// 成功反馈
		{
			g_exportPri.acRelayTick = 0;
			ret = back == 0 ? CLOSE : OPEN;
		}
		else	// 还未反馈成功
		{
			if (abs(get_timetick() - g_exportPri.acRelayTick) > RELAY_BACK_OVERTIME)
			{
				ret = OVERTIME;
			}
			else
			{
				ret = WAITING;
			}
		}
	}
	else
	{
		ret = back == 0 ? CLOSE : OPEN;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetFanRelay
* ABSTRACT	: 风扇继电器反馈
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 0-断开 1-闭合
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetFanRelay()
{
	int ret = CLOSE;

	if (g_exportPri.fanRelayCtrl == OPEN)
	{
		ret = OPEN;
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: SetWarnLed
* ABSTRACT	: 控制警告灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetWarnLed(int ctrl)
{
	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPA, KC_LED4, CLOSE);
			printf("SetWarnLed Close\n\r");
			break;
		case OPEN:
			SetOutputSta(GPA, KC_LED4, OPEN);
			printf("SetWarnLed OPEN\n\r");
			break;
		default:
			break;
	}

	g_exportPri.warnLedCtrl = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetErrLed
* ABSTRACT	: 控制故障灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetErrLed(int ctrl)
{
	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPA, KC_LED5, CLOSE);
			printf("SetErrLed Close\n\r");
			break;
		case OPEN:
			SetOutputSta(GPA, KC_LED5, OPEN);
			printf("SetErrLed Open\n\r");
			break;
		default:
			break;
	}

	g_exportPri.errLedCtrl = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetACRelay
* ABSTRACT	: 控制前端接触器
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetACRelay(int ctrl)
{
	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPA, KC_OUT1, CLOSE);
			SetOutputSta(GPA, KC_OUT2, CLOSE);
			break;
		case OPEN:
			SetOutputSta(GPA, KC_OUT1, OPEN);
			SetOutputSta(GPA, KC_OUT2, OPEN);
			break;
		default:
			break;
	}

	g_exportPri.acRelayCtrl = ctrl;
	g_exportPri.acRelayTick = get_timetick();
}

/*
* *******************************************************************************
* MODULE	: SetFanRelay
* ABSTRACT	: 控制风扇继电器
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetFanRelay(int ctrl)
{
	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPA, KC_OUT3, CLOSE);
			break;
		case OPEN:
			SetOutputSta(GPA, KC_OUT3, OPEN);
			break;
		default:
			break;
	}

	g_exportPri.fanRelayCtrl = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetFanSpeed
* ABSTRACT	: 控制风扇转速，PWM
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetFanSpeed(int pwm)
{
	if ((pwm >= 0) && (pwm <= 100))
	{
		pwm_ctrl(PWM_FAN, pwm);
	}
}

/*
* *******************************************************************************
* MODULE	: SetDoorLED
* ABSTRACT	: 一体机中控制开门灯，分体机由chgctrl控制刷卡指示灯
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void SetDoorLED(int ctrl)
{
	switch (ctrl)
	{
	case CLOSE:
		SetOutputSta(GPA, KC_OUT4, CLOSE);
		break;
	case OPEN:
		SetOutputSta(GPA, KC_OUT4, OPEN);
		break;
	default:
		break;
	}

	g_exportPri.dorrLEDCtrl = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetRedLed
* ABSTRACT	: 控制灯板红灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetRedLed(int ctrl, int idx)
{
	if ((idx != 1) && (idx != 2))
	{
		return;
	}

	int KC_LED = idx == 1 ? KC_LED1 : KC_LED4;
	int GPX = KC_LED == KC_LED1 ? GPB : GPA;

	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPX, KC_LED, CLOSE);
			break;
		case OPEN:
			SetOutputSta(GPX, KC_LED, OPEN);
			break;
		default:
			break;
	}

	g_exportPri.redLedCtrl[idx-1] = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetBlueLed
* ABSTRACT	: 控制灯板蓝灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetBlueLed(int ctrl, int idx)
{
	if ((idx != 1) && (idx != 2))
	{
		return;
	}

	int KC_LED = idx == 1 ? KC_LED3 : KC_LED6;
	int GPX = GPA;
	
	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPX, KC_LED, CLOSE);
			break;
		case OPEN:
			SetOutputSta(GPX, KC_LED, OPEN);
			break;
		default:
			break;
	}

	g_exportPri.blueLedCtrl[idx-1] = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetGreenLed
* ABSTRACT	: 控制灯板绿灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetGreenLed(int ctrl, int idx)
{
	if ((idx != 1) && (idx != 2))
	{
		return;
	}

	int KC_LED = idx == 1 ? KC_LED2 : KC_LED5;
	int GPX = KC_LED == KC_LED2 ? GPB : GPA;

	switch (ctrl)
	{
		case CLOSE:
			SetOutputSta(GPX, KC_LED, CLOSE);
			break;
		case OPEN:
			SetOutputSta(GPX, KC_LED, OPEN);
			break;
		default:
			break;
	}

	g_exportPri.greenLedCtrl[idx-1] = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetYellowLed
* ABSTRACT	: 控制灯板黄灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetYellowLed(int ctrl, int idx)
{
	if ((idx != 1) && (idx != 2))
	{
		return;
	}

	SetRedLed(ctrl, idx);
//	SetBlueLed(ctrl, idx);
	SetGreenLed(ctrl, idx);

	g_exportPri.yellowLedCtrl[idx-1] = ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetAllLed
* ABSTRACT	: 控制灯板所有灯
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void SetAllLed(int ctrl, int idx)
{
	if ((idx != 1) && (idx != 2))
	{
		return;
	}

	SetRedLed(ctrl, idx);
	SetBlueLed(ctrl, idx);
	SetGreenLed(ctrl, idx);
	SetYellowLed(ctrl, idx);
	
	g_exportPri.allLedCtrl[idx-1] = ctrl;
}

/*
* *******************************************************************************
* MODULE	: GetRedLed
* ABSTRACT	: 红灯状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetRedLed(int idx)
{
	return g_exportPri.redLedCtrl[idx-1];
}

/*
* *******************************************************************************
* MODULE	: GetBlueLed
* ABSTRACT	: 蓝灯状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetBlueLed(int idx)
{
	return g_exportPri.blueLedCtrl[idx-1];
}

/*
* *******************************************************************************
* MODULE	: GetGreenLed
* ABSTRACT	: 绿灯状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetGreenLed(int idx)
{
	return g_exportPri.greenLedCtrl[idx-1];
}

/*
* *******************************************************************************
* MODULE	: GetYellowLed
* ABSTRACT	: 黄灯状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetYellowLed(int idx)
{
	return g_exportPri.yellowLedCtrl[idx-1];
}

/*
* *******************************************************************************
* MODULE	: GetAllLed
* ABSTRACT	: 灯状态
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int GetAllLed(int idx)
{
	return g_exportPri.allLedCtrl[idx-1];
}

/*****************************************************************************
* Function		 : ExportCtrl_Init
* Description	 : 
* Input 		 : 
* Output		 : 
* Return		 : 
* Others		 : 
* Record
* 1.Date		 : 
*	 Author 	 : 
*	 Modification:
												
*****************************************************************************/
void ExportCtrl_Init(void)
{
	Export_Init();
}

/*****************************************************************************
* Function		 : Export_Uninit
* Description	 : 
* Input 		 : 
* Output		 : 
* Return		 : 
* Others		 : 
* Record
* 1.Date		 : 
*	 Author 	 : 
*	 Modification:
												
*****************************************************************************/
void Export_Uninit(void)
{

}