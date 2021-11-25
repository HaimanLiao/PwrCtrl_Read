/**
*　　　　　　　 ┏┓　 ┏┓+ +
*　　　　　　　┏┛┻━━━┛┻┓ + +
*　　　　　　　┃　　　　　　┃ 　
*　　　　　　　┃　　　━　　 ┃ ++ + + +
*　　　　　　 ████━████  ┃+
*　　　　　　　┃　　　　　　　┃ +
*　　　　　　　┃　　　┻　　　┃
*　　　　　　　┃　　　　　　┃ + +
*　　　　　　　┗━┓　　　┏━┛
*　　　　　　　　 ┃　　　┃　　　　　　　　　　　
*　　　　　　　　 ┃　　　┃ + + + +
*　　　　　　　　 ┃　　　┃　　　　Code is far away from bug with the animal protecting
*　　　　　　　　 ┃　　　┃ + 　　　　神兽保佑,永无bug　　
*　　　　　　　　 ┃　　　┃
*　　　　　　　　 ┃　　　┃　　+　　　　　　　　　
*　　　　　　　　 ┃　 　 ┗━━━┓ + +	  这是一只草泥马
*　　　　　　　　 ┃ 　　　　   ┣┓
*　　　　　　　　 ┃ 　　　　　 ┏┛
*　　　　　　　　 ┗┓┓┏━┳┓┏┛ + + + +
*　　　　　　　　  ┃┫┫ ┃┫┫
*　　　　　　　　  ┗┻┛ ┗┻┛+ + + +
*/
/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"
#include "pwrCtrl.h"
#include "modCtrl.h"
#include "dbus.h"
#include "cnfJson.h"
#include "statJson.h"
#include "relayCtrl.h"
#include "fanCtrl.h"
#include "zmqCtrl.h"
#include "smartOps.h"
#include <sys/resource.h>
#include "groupsCtrl.h"
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

static int				MainStatus = 0;			//每一位代表了一个配置或功能变化

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
* MODULE	: GetMainStatusPending
* ABSTRACT	: 获取pending状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetMainStatusPending(ENUM_MAIN_STATUS type)
{
	if ((MainStatus & (1 << type)) != 0)
	{
		return 1;
	}

	return 0;
}

/*
* *******************************************************************************
* MODULE	: SetMainStatusPending
* ABSTRACT	: 设置pending状态
* FUNCTION	:
* ARGUMENT	: ctrl : 0-清空pending， 1-设置pending
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int SetMainStatusPending(int ctrl, ENUM_MAIN_STATUS type)
{
	if (ctrl != 0 && ctrl != 1)
	{
		return -1;
	}

	MainStatus &= (~(1 << type));
	MainStatus |= (ctrl << type);

	return 0;
}

/*
* *******************************************************************************
* MODULE	: InitPwrCtrlPara
* ABSTRACT	: pwrCtrl参数配置
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int InitPwrCtrlPara(void)
{
	int ret = RESULT_OK;
	PWR_CNF_STRUCT cnf;
	UNIT_PARA_STRUCT unit = {0};

	ret = CNF_GetPwrCnf(&cnf);
	if (ret == RESULT_ERR)
	{
		return ret;
	}

	unit.pwrMax = cnf.powerMax;
	unit.gunNum = cnf.powerCount;
	unit.groupNum = cnf.pduRelayCount;
	unit.relayNum = cnf.pduCount;
	unit.pwrType = cnf.powerType;
	unit.mdlType = cnf.mdlType;
	unit.pwrUnitNum = cnf.powerUnitNum;

	PwrCtrl_SetUnitPara(unit);

	return ret;
}

/*
* *******************************************************************************
* MODULE	: InitRelayCtrlPara
* ABSTRACT	: relayCtrl参数配置
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int InitRelayCtrlPara(void)
{
	int ret = RESULT_OK;
	PWR_CNF_STRUCT cnf;
	RELAY_PARA_STRUCT relay = {0};

	ret = CNF_GetPwrCnf(&cnf);
	if (ret == RESULT_ERR)
	{
		return ret;
	}

	relay.pduNum = cnf.pduCount;
	relay.pduRlyNum = cnf.pduRelayCount;
	relay.pduTempLevel = cnf.pduTempLevel;
	relay.pduType = cnf.pduType;

	RelayCtrl_SetPara(relay);

	return ret;
}

/*
* *******************************************************************************
* MODULE	: InitFanCtrlPara
* ABSTRACT	: fanCtrl参数配置
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int InitFanCtrlPara(void)
{
	int ret = RESULT_OK;
	PWR_CNF_STRUCT cnf;
	FAN_PARA_STRUCT fan = {0};

	ret = CNF_GetPwrCnf(&cnf);
	if (ret == RESULT_ERR)
	{
		return ret;
	}

	fan.fanNum = cnf.fanCount;
	fan.fanType = cnf.fanType;

	FanCtrl_SetPara(fan);

	return ret;
}

/*
* *******************************************************************************
* MODULE	: InitModulePara
* ABSTRACT	: 电源模块参数配置
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int InitModulePara(void)
{
	int ret = RESULT_OK;
	int i = 0;
	PWR_CNF_STRUCT cnf;
	MDL_PARA_STRUCT mdlPara = {0};

	ret = CNF_GetPwrCnf(&cnf);
	if (ret == RESULT_ERR)
	{
		return ret;
	}

	mdlPara.mdlType = cnf.mdlType;
	mdlPara.mdlCount = cnf.mdlCount;
	mdlPara.mdlOverVol = cnf.mdlOverVol;
	mdlPara.mdlUnderVol = cnf.mdlUnderVol;
	for (i = 0; i < mdlPara.mdlCount; i++)
	{
		mdlPara.mdlSN[i].groupId = cnf.mdl[i].groupId;
		memcpy(mdlPara.mdlSN[i].SN, cnf.mdl[i].SN, MDL_SN_MAX_LENGTH);
	}

	Mdl_SetPara(mdlPara);

	return 0; 
}

/*
* *******************************************************************************
* MODULE	: AllowCoreDebug
* ABSTRACT	: 
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int AllowCoreDebug(void)
{
	int 			iRes = RLIMIT_CORE;
	struct rlimit 	stRlim;
	
	/* 允许生成core文件 */
	stRlim.rlim_cur = stRlim.rlim_max = RLIM_INFINITY;
	if (0 != setrlimit(iRes, &stRlim))
	{
	 	DEBUG("Error: setrlimit failed");
		return -1;
	}

	return 0;
}

/*
* *******************************************************************************
* MODULE	: InitProgramEnv
* ABSTRACT	: 用于第一次烧写系统后配置运行环境
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int InitProgramEnv()
{
	if (access(PWRCTRL_RUN_DIRECTORY, F_OK) < 0)
	{
		DLOG_WARN("%s do not exist!", PWRCTRL_RUN_DIRECTORY);
		system(CMD_MKDIR_RUN_DIR);
		system(CMD_CNF_COPY_JSON);
		system(CMD_STAT_COPY_JSON);
		sleep(1);
	}

	return 0;
}

/*
* *******************************************************************************
* MODULE	: MainQuit
* ABSTRACT	: 处理退出，注意：这里的顺序是有要求的
* FUNCTION	:
* ARGUMENT	: 
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int MainQuit(void)
{
	PwrCtrl_Uninit();

	Mdl_Uninit();

	FanCtrl_Uninit();

	RelayCtrl_Uninit();

	SmartOps_Uninit();

	/* 注销配置文件 */
	CNF_Uninit();

	/* 注销dbus接口 */
	DBUS_Uninit();				//目前这个接口是空的

	return 0;
}

/*
* *******************************************************************************
* MODULE	: main
* ABSTRACT	: 程序运行入口
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int main(int argc, char *argv[])
{
	/* 初始化日志 */
	if (argc >= 2)
	{
		if (InitDlog(argv[1]) < 0)
		{
			InitDlog(NULL);	
		}
	}
	else
	{
		InitDlog(NULL);			//NULL是输出到标准输出
	}

	/* 配置第一次烧录环境 */
	if (InitProgramEnv() < 0)
	{
		DEBUG("InitProgramEnv error!");
		return -1;
	}
	
	/* 允许输出core文件，检测异常崩溃 */
	if (AllowCoreDebug() < 0)
	{
		DEBUG("AllowCoreDebug error!");
	}

	/* dbus初始化 */
	if (DBUS_Init() < 0)
	{
		DEBUG("DBUS_Init error!");
		return -1;
	}

	/* 装载配置文件，但是并不解析 */
	if (CNF_Init() < 0)
	{
		DEBUG("CNF_Init error!");
		return -1;
	}

	if (Stat_Init() < 0)
	{
		DEBUG("Stat_Init error!");
		return -1;
	}

	ExportCtrl_Init();

	Mdl_Init();
	InitModulePara();

	RelayCtrl_Init();
	InitRelayCtrlPara();

	FanCtrl_Init();
	InitFanCtrlPara();

	PwrCtrl_Init();
	InitPwrCtrlPara();

	ZmqCtrl_Init();

	Fault_Init();

	sleep(1);
	SmartOps_Init();

//	signal(SIGABRT, SIG_IGN);	// 用于忽略掉zmq中偶发的abort()产生的退出
//	signal(SIGPIPE, SIG_IGN);	// 用于忽略socket断开导致的进程退出

	DEBUG("All init OK, let's run!");
	log_send("All init OK, let's run!");

	while (1)
	{
		if (MainStatus)				// 状态不为0，说明有状态改变
		{
			/* 如果存在枪正在充电，那么不进行参数修改 */
			if (PwrCtrl_GetChgIsBusy() != CHG_MAIN_FREE)
			{
				goto next;
			}

			if (GetMainStatusPending(MAIN_STATUS_PROGRAM_QUIT) == 1)		// 要求程序退出
			{
				MainQuit();
				SetMainStatusPending(0, MAIN_STATUS_PROGRAM_QUIT);
				break;
			}

			if (GetMainStatusPending(MAIN_STATUS_MODULE_CHANGE) == 1)		// 模块信息发生变化
			{
				InitModulePara();
				SetMainStatusPending(0, MAIN_STATUS_MODULE_CHANGE);
			}

			if (GetMainStatusPending(MAIN_STATUS_PWR_CHANGE) == 1)			// 功率信息发生变化
			{
				InitPwrCtrlPara();
				SetMainStatusPending(0, MAIN_STATUS_PWR_CHANGE);
			}

			if (GetMainStatusPending(MAIN_STATUS_FAN_CHANGE) == 1)			// 风扇信息发生变化
			{
				InitFanCtrlPara();
				SetMainStatusPending(0, MAIN_STATUS_FAN_CHANGE);
			}

			if (GetMainStatusPending(MAIN_STATUS_RELAY_CHANGE) == 1)		// PDU信息发生变化
			{
				InitRelayCtrlPara();
				SetMainStatusPending(0, MAIN_STATUS_RELAY_CHANGE);
			}
		}

		ZmqCtrl_Deal();	//lhm: 存在一个zmq接收线程，可以保存接收到的数据，所以不会出现自己疑惑的情况
						//lhm: “在运行PwrCtrl_Deal()时，由于没有执行ZmqCtrl_Deal()，导致在此期间桩端发送的数据没有被接收”
						//lhm: ZmqCtrl_Deal()只是读取接收数据区，而接收数据区的更新是由zmq接收线程负责的
						//lhm: ZmqCtrl_Deal()的任务是把接收数据区的内容移到其他接口（如PwrCtrl_Deal()）的数据区（全局变量）
						//lhm: 之后其他接口（如PwrCtrl_Deal()）再根据自己数据区的内容变化情况进行相关操作
		PwrCtrl_Deal();
		FanCtrl_Deal();
		Fault_Deal();
		SmartOps_Deal();
next:
		usleep(50 * 1000);			// 50毫秒一次
	}

	return 0;
}
