/*
 * UR_100030SW.c
 *
 *  Created on: 2019-9-29
 *      Author: root
 */
/*
 * UR_100030SW.c
 *
 *  Created on: 2019-9-3
 *      Author: root
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
#include <time.h>
#include "UR_100030SW.h"
#include "dlog.h"
#include "timer.h"
#include "can_api.h"
#include "log_sys.h"
/**************************************************************************************
1、优优绿能的模块没有获取具体温度值的指令；
答：十进制的30 就是读取模块环境温度的CommandType；温度是摄氏度   1000倍

2、优优绿能的模块电压值的单位是mV，电流值的单位是mA,和台达中兴兼容时，注意换算；

3、同组内的电源模块的地址是如何设置的？有没有设置地址的指令？
答：设置模块地址对于30KW有两种方法：
    1.通过面板按键手动设定；
    2.通过系统端背板地址板拨码设置地址，这样模块会根据位置自动识别地址，不需要手动设置。

4、优优绿能的模块电压和电流值的设定是分开的两条CAN报文，是先发电压值，再发
   电流值吗？发完电压电流值的指令后，是否发送开机指令？
答：先电压或先电流都可以；需要发送开机指令；

5、优优绿能的模块如何设置组地址，数据格式协议中没有说明；
答：数据域就是想设置的组地址，3就是3，1就是1；

6、发完电压电流的设定值后，是不是再发送开机指令？
答：开机之前一定要设置相应的电压与电流才能开机  否则就是默认的电压电流值；

7、默认的电压电流值是多少？
答：默认的电压值应该是150V吧  有的模块是200V  电流值是最大的  50060是80A  75040是50A

****************************************************************************************/
/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

#define 	CAN_NO 		CAN0

/*****************************优优绿能 CANID********************************************/
#define LNMOD_PROTOCOL				(1)						// 当前的协议版本号
#define LNMOD_MONITORADDR			(1)						// 监控器地址
#define LNMOD_PRODUCTIONDAY			(0)						// 生产日期
#define LNMOD_SERIALNUMLOWPART		(0)						// 序列号低位
/*****************************优优绿能 CANID结束****************************************/

/******************BYTE0 信息类型(高4位为组地址，低4位位信息类型)*************************/
#define LN_SET_MOD_PARA				(0x00)					// 设置模块参数
#define LN_SET_MOD_PARA_REPLY		(0x01)					// 模块设置应答
#define LN_GET_MOD_PARA				(0x02)					// 读模块信息
#define LN_GET_MOD_PARA_REPLY		(0x03)					// 模块读信息应答
/******************BYTE0 END***********************************************************/

/******************BYTE1 命令信息类型(高4位为组地址，低4位位信息类型)*********************/
#define LN_GET_MOD_OUTVOL_R			(0)					    // 模块输出电压(只读)
#define LN_GET_MOD_OUTCUR_R			(48)					// 模块输出电流(只读)
#define LN_SET_MODVOL_WR			(2)					    // 模块输出电压设定值(读写)
#define LN_SET_MODCUR_WR			(3)					    // 模块输出限流点设定值(读写)
#define LN_SET_MOD_ONOFF_W			(4)					    // DCDC模块开关机(只写)
#define LN_GET_MOD_STATUS_R			(8)					    // 模块状态标志位(只读)
#define LN_GET_MODABVOL_R			(20)					// 线电压 AB(只读)
#define LN_GET_MODBCVOL_R			(21)					// 线电压 BC(只读)
#define LN_GET_MODCAVOL_R			(22)					// 线电压 CA(只读)
#define LN_SET_GROUP_WR				(89)					// 组地址(读写)
#define LN_GET_MODTMP_R				(30)					// 环境温度(只读)
#define LN_SET_HLVOL_W				(0x5F)					// 设置高低压模式
#define LN_GET_MOD_MODEL_R			(0x60)					// 读取高低压模式
/******************BYTE1 END***********************************************************/
#define LN_MOD_START				(0)						// 模块开机
#define LN_MOD_STOP					(1)						// 模块关机

#define LN_MOD_RESERVED				(0)						// 优优绿能数据帧中的保留位默认值
#define MOD_CAN_LEN					(8)						// 模块发送的CAN报文长度

#define MASSIGNID(addr)			(ASSIGNID|addr)
#define MSKIP(addr)				(SKIP|addr)
#define MSTART(addr)			(START|addr)
/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/
typedef enum mod_id_enum
{
	DISCONNECT			= 		0x0,
	START,
}MDL_ID_ENUM;

typedef enum mdl_ctrl_enum
{
	MDL_CTRL_FREE,
	MDL_CTRL_OUTPUT,
	MDL_CTRL_STOP,
//	MDL_CTRL_SWITCHING,
}MDL_CTRL_ENUM;
/*
* *******************************************************************************
*                                  Struct
* *******************************************************************************
*/
#pragma pack(1)

typedef union
{
	uint32_t      all;
	struct
	{     			// bits  description
	   uint32_t    SerialNumberLo:9;  			// 0:8
	   uint32_t	   ProductionDay:5;  			// 9:13
	   uint32_t    ModuleAddress:7;  			// 14:20
	   uint32_t    MonitorAddress:4;   			// 21:24
	   uint32_t	   Protocol:4; 					// 25:28

	   uint32_t    TXRQ:1;          			// 29
	   uint32_t    RTR:1;          				// 30
	   uint32_t    IDE:1;          				// 31
	}NormalBits;
}YYLN_CANMSGID;

struct  NORMAL_BITS
{      // bits   description
	uint32_t      SnHi_FrameNumber:16;		// 0:15
	uint32_t      CommandType:8;			// 16:23
	uint32_t      MessageType:4;    	   	// 24:27
	uint32_t      ModuleGroup:4;  		   	// 28:31
};

struct  CAN_L_BYTES
{      // bits   description
	uint32_t      BYTE3:8;     // 0:7
	uint32_t      BYTE2:8;     // 8:15
	uint32_t      BYTE1:8;     // 16:23
	uint32_t      BYTE0:8;     // 24:31
};

typedef union
{
	uint32_t                		all;
	struct  CAN_L_BYTES   			byte;
	struct  NORMAL_BITS				NormalBits;
}CAN_L_REG;

struct  CAN_H_BYTES
{      // bits   description
	uint32_t      BYTE7:8;     // 0:7
	uint32_t      BYTE6:8;     // 8:15
	uint32_t      BYTE5:8;     // 16:23
	uint32_t      BYTE4:8;     // 24:31
};

typedef union
{
	uint32_t                  	all;
	struct CAN_H_BYTES     		byte;
}CAN_H_REG;

struct STATUS_BITS
{
	uint16_t AcOV:1; 			// 0 交流输入过压
	uint16_t AcUV:1; 			// 1 交流输入欠压
	uint16_t AcOVDisconnected:1;// 2 交流输入过压脱离
	uint16_t Reserved_3:1; 		// 3
	uint16_t Reserved_4:1; 		// 4
	uint16_t Reserved_5:1; 		// 5
	uint16_t DcOV:1; 			// 6 直流输出过压
	uint16_t DcOVShutDown:1; 	// 7 直流输出过压关机
	uint16_t DcUV:1; 			// 8 直流输出欠压
	uint16_t FanNotRun:1;		// 9 风扇故障
	uint16_t Reserved_10:1; 	// 10
	uint16_t Reserved_11:1; 	// 11
	uint16_t AmbientOT:1; 		// 12 环境温度过温保护
	uint16_t Reserved_13:1; 	// 13
	uint16_t Pfc1_OT:1; 		// 14 PFC过温保护1
	uint16_t Pfc2_OT:1; 		// 15 PFC过温保护2
	uint16_t Dcdc1_OT:1; 		// 16 DC过温保护1
	uint16_t Dcdc2_OT:1; 		// 17 DC过温保护2
	uint16_t SciIsNotOK:1; 		// 18 原副边通信故障
	uint16_t Reserved_19:1; 	// 19
	uint16_t PfcFail:1; 		// 20 原边故障
	uint16_t DcdcFail:1; 		// 21 副边故障
	uint16_t Reserved_22:1; 	// 22
	uint16_t Reserved_23:1; 	// 23
	uint16_t Reserved_24:1; 	// 24
	uint16_t DcdcNotRun:1; 		// 25 开关机状态
	uint16_t Reserved_2627:2; 	// 26:27
	uint16_t Reserved_28:1; 	// 28
	uint16_t Reserved_29:1; 	// 29
	uint16_t Reserved_30:1; 	// 30
	uint16_t Reserved_31:1; 	// 31
};

typedef union
{
	uint32_t all;
	struct STATUS_BITS bit;
}STATUS_REG;


typedef struct
{
	STATUS_REG			sta;

	MDL_INFO_STRUCT 	mdl;
	unsigned int 		id;
	MDL_ID_ENUM			step;

	MDL_CTRL_ENUM 		ctrl;			//

	int 				sendSNFlag;		// 0-未发送，1-已发送， 2-收到

	unsigned long   	pingTick;		// ping计数
	unsigned long 		readTick;		// 读取电压电流的计数
	unsigned long 		aliveTick;		// alive计数，超时认为掉线
	unsigned long 		settingTick;	// 输出命令计数
	unsigned long 		modeTick;		// 读取高低压模式的计数

	unsigned long		workTime;		// 总共工作时间，单位S
	unsigned long		workTick;		// 每当工作时间达到一个间隔(比如5秒)，就加入到workTime中
}MDL_INFO_PRIVATE_STRUCT;

#pragma pack()

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

static 		MDL_INFO_PRIVATE_STRUCT			g_mdl[MDL_MAX_NUM + 1];	// most mdl num
static 		int 							g_mdlNum = 0;			// use mdl num, online mdl num
static 		SN_STRUCT						g_sn[MDL_MAX_NUM + 1];
static 		int								g_mdchgSign[MDL_MAX_NUM + 1] = {0};	// 这里从1开始，以模块个数最大作为参考，实际作用只是枪来算，这里需要注意。
static 		int								g_mdlMode[MDL_MAX_NUM + 1] = {0};	// mode: 0-normal mode; 2-parallel mode 500V; 1-series mode 1000V

static 		int								mdlACUnderVol = 200;
static 		int								mdlACOverVol = 260;

static 		pthread_mutex_t 				mutex_lock, mutex_lock2;
static		pthread_t 						thread_mdldeal;
static 		int								status_mdldeal;		//0-正常运行，1-要求退出

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
static void CANLock()
{
	pthread_mutex_lock(&mutex_lock);
}

static void CANUnlock()
{
	pthread_mutex_unlock(&mutex_lock);
}

static void Lock()
{
	pthread_mutex_lock(&mutex_lock2);
}

static void Unlock()
{
	pthread_mutex_unlock(&mutex_lock2);
}

/*
* *******************************************************************************
* MODULE	: MdlSend
* ABSTRACT	: 模块统一发送函数
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void MdlSend(struct can_frame frame)
{
	/* lock */
	CANLock();

	can_send(CAN_NO, frame);

	/* 不使用线程的话会影响原来线程的速度 */
	usleep(20000);
/*
	printf("can send id = %x, ", frame.can_id);
	printf("dlc = %d, ", frame.can_dlc);
	printf("data = ");
	int i = 0;
	for (; i < frame.can_dlc; i++)
	{
		printf("%x ", frame.data[i]);
	}
	printf("\n");
*/

	/* unlock */
	CANUnlock();
}

/*************************************************************
  Function: 	LN_DCDCM_ReadMessage
  Description:  该函数的主要功能是读取模块有关的数据（CMD02）
  Calls:       无
  Called By:
  Input:       GroupAddr  ->模块组地址
				ModuleAddr ->模块地址
				CommandType->读取模块命令类型
  Output:      无
  Return:
  Others:      例如：当组地址为1 模块地址为1时,
  读取模块输出电压     LN_DCDCM_ReadMessage(   ,0x01,0x01,0x00);
  读取模块输出电流     LN_DCDCM_ReadMessage(   ,0x01,0x01,0x01);
*************************************************************/
static void LN_DCDCM_ReadMessage(uint8_t GroupAddr, uint8_t ModuleAddr, uint8_t CommandType)
{
	struct can_frame sendFrame;;

	YYLN_CANMSGID 	id;
	CAN_L_REG		L_Reg;
	CAN_H_REG		H_Reg;

	id.NormalBits.Protocol          = LNMOD_PROTOCOL;			// 协议号
	id.NormalBits.MonitorAddress    = LNMOD_MONITORADDR;		// 监控地址
	id.NormalBits.ModuleAddress     = ModuleAddr;       		// 模块地址
	id.NormalBits.ProductionDay     = LNMOD_PRODUCTIONDAY;		// 生产日期
	id.NormalBits.SerialNumberLo    = LNMOD_SERIALNUMLOWPART;   // 序列号低位

	L_Reg.NormalBits.ModuleGroup      = GroupAddr;				// 组地址
	L_Reg.NormalBits.MessageType      = LN_GET_MOD_PARA;		// 信息类型：读模块信息
	L_Reg.NormalBits.CommandType      = CommandType;  			// 命令信息类型
	L_Reg.NormalBits.SnHi_FrameNumber = LN_MOD_RESERVED;		// 保留位
	H_Reg.all                         = LN_MOD_RESERVED;		// Command Data

	sendFrame.can_id = id.all;
	sendFrame.can_dlc = MOD_CAN_LEN;

	sendFrame.data[0] = L_Reg.byte.BYTE0;
	sendFrame.data[1] = L_Reg.byte.BYTE1;
	sendFrame.data[2] = L_Reg.byte.BYTE2;
	sendFrame.data[3] = L_Reg.byte.BYTE3;
	sendFrame.data[4] = H_Reg.byte.BYTE4;
	sendFrame.data[5] = H_Reg.byte.BYTE5;
	sendFrame.data[6] = H_Reg.byte.BYTE6;
	sendFrame.data[7] = H_Reg.byte.BYTE7;

	MdlSend(sendFrame);
}

/*************************************************************
  Function: 	LN_DCDCM_SettingMessage
  Description: 该函数的主要功能是设置模块有关的数据
  Calls:
  Called By:
  Input:      GroupAddr  ->模块组地址
              ModuleAddr ->模块地址     设为0  表示广播发送
		      CommandType->设置模块命令类型
		      DataValue  ->该命令的数据值
  Output:
  Return:
  Others:
例如：当组地址为1 模块地址为1时,
设置电压参考值为 475.55V    LN_DCDCM_SettingMessage(   ,0x01,0x01,0x02,0x7419E);
设置电流限流点为 10.5A      LN_DCDCM_SettingMessage(  ,0x01,0x01,0x03,0x2904);
模块开机                    LN_DCDCM_SettingMessage(  , 0x01,0x01,0x04,0x00);
模块关机   					LN_DCDCM_SettingMessage(  , 0x01,0x01,0x04,0x01);
*************************************************************/
static void LN_DCDCM_SettingMessage(uint8_t GroupAddr, uint8_t ModuleAddr, uint8_t CommandType, uint32_t DataValue)
{
	struct can_frame sendFrame;;

	YYLN_CANMSGID 	id;
	CAN_L_REG		L_Reg;
	CAN_H_REG		H_Reg;

	id.NormalBits.Protocol          = LNMOD_PROTOCOL;			// 协议号
	id.NormalBits.MonitorAddress    = LNMOD_MONITORADDR;		// 监控地址
	id.NormalBits.ModuleAddress     = ModuleAddr;       		// 模块地址
	id.NormalBits.ProductionDay     = LNMOD_PRODUCTIONDAY;		// 生产日期
	id.NormalBits.SerialNumberLo    = LNMOD_SERIALNUMLOWPART;   // 序列号低位

	L_Reg.NormalBits.ModuleGroup      = GroupAddr;				// 组地址
	L_Reg.NormalBits.MessageType      = LN_SET_MOD_PARA;		// 信息类型：读模块信息
	L_Reg.NormalBits.CommandType      = CommandType;  			// 命令信息类型
	L_Reg.NormalBits.SnHi_FrameNumber = LN_MOD_RESERVED;		// 保留位
	H_Reg.all                         = DataValue;              // Command Data

	sendFrame.can_id = id.all;
	sendFrame.can_dlc = MOD_CAN_LEN;

	sendFrame.data[0] = L_Reg.byte.BYTE0;
	sendFrame.data[1] = L_Reg.byte.BYTE1;
	sendFrame.data[2] = L_Reg.byte.BYTE2;
	sendFrame.data[3] = L_Reg.byte.BYTE3;
	sendFrame.data[4] = H_Reg.byte.BYTE4;
	sendFrame.data[5] = H_Reg.byte.BYTE5;
	sendFrame.data[6] = H_Reg.byte.BYTE6;
	sendFrame.data[7] = H_Reg.byte.BYTE7;

	MdlSend(sendFrame);
}

/*
* *******************************************************************************
* MODULE	: SetMdlFault
* ABSTRACT	: 设置模块状态/故障
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetMdlFault(int index, int fault)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}
	
/*	if (fault == MDL_FAULT_OFFLINE)
	{
		g_mdl[index].mdl.fault = fault;
	}
	else
	{
		if ((g_mdl[index].mdl.fault == MDL_FAULT_UNDER_VOL)
			|| (g_mdl[index].mdl.fault == MDL_FAULT_OVER_VOL)
			|| (g_mdl[index].mdl.fault == MDL_FAULT_OVER_TEMP))
		{
			
		}
		else
		{
			g_mdl[index].mdl.fault = fault;
		}
	}*/

	g_mdl[index].mdl.fault = fault;

//	DEBUG("mdl id = %d, fault = %d", index, fault);
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: GetMdlFault
* ABSTRACT	: 获得模块状态/故障
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetMdlFault(int index)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}

	return g_mdl[index].mdl.fault;
}

/*
* *******************************************************************************
* MODULE	: SetMdlSta
* ABSTRACT	: 设置模块运行、停止
* FUNCTION	:
* ARGUMENT	: void
* NOTE		: ok
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetMdlSta(int index, int sta)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}

	g_mdl[index].ctrl = sta;

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: GetMdlSta
* ABSTRACT	: 获得模块运行、停止
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetMdlSta(int index)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}

	return g_mdl[index].ctrl;
}

/*
* *******************************************************************************
* MODULE	: SetMdlOffline
* ABSTRACT	: 设置异常掉线时候模块状态
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetMdlOffline(int index)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}

	SetMdlFault(index, MDL_FAULT_OFFLINE);

	g_mdl[index].step = DISCONNECT;

	g_mdl[index].mdl.volOut = 0;
	g_mdl[index].mdl.curOut = 0;
	g_mdl[index].mdl.temp = 0;
	g_mdl[index].mdl.mode = NORMAL_MODE;
	g_mdl[index].aliveTick = 0;
	g_mdl[index].mdl.sts = 0;

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: SetMdlSts
* ABSTRACT	: 设置模块故障
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int SetMdlSts(int index)
{
	if ((index > MDL_MAX_NUM) || (index == 0))
	{
		return RESULT_ERR;
	}

	g_mdl[index].mdl.sts = 0;
	if (g_mdl[index].sta.bit.AmbientOT)
	{
		g_mdl[index].mdl.sts |= 1 << 0;
	}
	if (g_mdl[index].sta.bit.FanNotRun)
	{
		g_mdl[index].mdl.sts |= 1 << 2;
	}
	if (g_mdl[index].sta.bit.AcUV)
	{
		g_mdl[index].mdl.sts |= 1 << 3;
	}
	if ((g_mdl[index].sta.bit.AcOV) || (g_mdl[index].sta.bit.AcOVDisconnected))
	{
		g_mdl[index].mdl.sts |= 1 << 4;
	}
	if ((g_mdl[index].sta.bit.Pfc1_OT) || (g_mdl[index].sta.bit.Pfc2_OT)
		|| (g_mdl[index].sta.bit.PfcFail))
	{
		g_mdl[index].mdl.sts |= 1 << 5;
	}
	if ((g_mdl[index].sta.bit.Dcdc1_OT) || (g_mdl[index].sta.bit.Dcdc2_OT)
		|| (g_mdl[index].sta.bit.DcdcFail))
	{
		g_mdl[index].mdl.sts |= 1 << 6;
	}
	if ((g_mdl[index].sta.bit.DcOV) || (g_mdl[index].sta.bit.DcOVShutDown))
	{
		g_mdl[index].mdl.sts |= 1 << 10;
	}
	if (g_mdl[index].sta.bit.DcUV)
	{
		g_mdl[index].mdl.sts |= 1 << 11;
	}
	if (0)
	{
		g_mdl[index].mdl.sts |= 1 << 12;
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: MdlDealCallBack
* ABSTRACT	: CAN回调函数，处理接收到的数据
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int MdlDealCallBack(unsigned char *pframe, int len)
{
	struct can_frame recvFrame;
	memcpy(&recvFrame, pframe, sizeof(recvFrame));

	YYLN_CANMSGID id;
	CAN_L_REG	  recmsg_l;
	CAN_H_REG	  recmsg_h;

/*	printf("can recv id = %x, ", recvFrame.can_id);
	printf("dlc = %d, ", recvFrame.can_dlc);
	printf("data = ");
	int ii = 0;
	for (; ii < recvFrame.can_dlc; ii++)
	{
		printf("%x ", recvFrame.data[ii]);
	}
	printf("\n");*/

	id.all = recvFrame.can_id;
	if(LNMOD_MONITORADDR != id.NormalBits.MonitorAddress)
	{
		return RESULT_ERR;
	}

	recmsg_l.byte.BYTE0 = recvFrame.data[0];
	recmsg_l.byte.BYTE1 = recvFrame.data[1];
	recmsg_l.byte.BYTE2 = recvFrame.data[2];
	recmsg_l.byte.BYTE3 = recvFrame.data[3];

	recmsg_h.byte.BYTE4 = recvFrame.data[4];
	recmsg_h.byte.BYTE5 = recvFrame.data[5];
	recmsg_h.byte.BYTE6 = recvFrame.data[6];
	recmsg_h.byte.BYTE7 = recvFrame.data[7];

	unsigned char	addr 	= id.NormalBits.ModuleAddress;
	unsigned int	cmd 	= recmsg_l.NormalBits.CommandType;
	unsigned int	index   = 0;

	int i = 0;

	if((addr > g_mdlNum)||(addr == 0))
	{
		return RESULT_ERR;
	}

	if(recmsg_l.NormalBits.MessageType != LN_GET_MOD_PARA_REPLY)
	{
		return RESULT_ERR;
	}

	for(i = 1; i <= g_mdlNum; i++)
	{
		if(g_mdl[i].id == addr)
		{
			index = i;
			break;
		}
	}

	if(index == 0)
	{
		return RESULT_ERR;
	}

	if(g_mdl[index].mdl.groupId != recmsg_l.NormalBits.ModuleGroup)
	{
		/* 第一次接收到相同地址的模块，查看组号是否一致，不一致按照新的组号进行设置 */
		LN_DCDCM_SettingMessage(0, addr, LN_SET_GROUP_WR, g_mdl[index].mdl.groupId);
		return RESULT_ERR;
	}

	g_mdl[index].aliveTick = get_timetick();

	if(g_mdl[index].step == DISCONNECT)
	{
		/* 这里优优的模块并不具备序列号功能，但是为了兼容，这里采用地址作为序列号存储，后期模块更能协议会直接输出相关参数，到时侯再更改
		 * 另外模块的地址设置从1开始设置，分类好所属的模组号*/
		if (addr < 10)
		{
			g_mdl[index].mdl.SN[0] = addr + 0x30;
			g_mdl[index].mdl.SN[1] = '\0';
		}
		else
		{
			g_mdl[index].mdl.SN[0] = (addr / 10) + 0x30;
			g_mdl[index].mdl.SN[1] = (addr % 10) + 0x30;
			g_mdl[index].mdl.SN[2] = '\0';
		}
		g_mdl[index].workTime = g_sn[index].kwh;
		g_mdl[index].step = START;
	}

	switch (cmd)
	{
		/* 模块输出电压(只读) ,优优的模块的输出电压采样点在二极管前端，所以读到的电压是模块本身的电压*/
		case LN_GET_MOD_OUTVOL_R:
			g_mdl[index].mdl.volOut = (recmsg_h.all / 100); // 除以100是为了和台达及中兴模块一致，绿能单位mV
//			DEBUG("id = %d, vol = %d", index, g_mdl[index].mdl.volOut);
			break;
		/* 模块输出电流(只读) */
		case LN_GET_MOD_OUTCUR_R:
			g_mdl[index].mdl.curOut = (recmsg_h.all / 10);  // 除以10是为了和台达及中兴模块一致，绿能单位mA
			g_mdl[index].mdl.pwrOut = g_mdl[index].mdl.volOut * g_mdl[index].mdl.curOut / 1000;	// 单位W
//			DEBUG("id = %d, cur = %d", index, g_mdl[index].mdl.curOut);
			break;
		/* 模块输出电压设定值(读写) */
		case LN_SET_MODVOL_WR:
			// 暂时不需要
			break;
		/* 模块输出限流点设定值(读写) */
		case LN_SET_MODCUR_WR:
			// 暂时不需要
			break;
		/* 模块状态标志位(只读) */
		case LN_GET_MOD_STATUS_R:
			g_mdl[index].sta.all = recmsg_h.all;
//			DEBUG("mdlId = %d, all = %x", index, recmsg_h.all);
			SetMdlSts(index);
			if (g_mdl[index].sta.bit.DcdcNotRun == 1)
			{
				g_mdl[index].workTick = get_timetick();
			}
			else		//模块正在工作
			{
				/* 会有点不够准确，先这样 */
				if (diff_timetick(&g_mdl[index].workTick, 5000) == RESULT_OK)
				{
					g_mdl[index].workTime += 5;
					g_sn[index].kwh = g_mdl[index].workTime;
					g_mdl[index].mdl.mdlTime += 5;
					/* 需要更新到json文件中 */
				}
			}
			break;
		/* volA volB volC */
		case LN_GET_MODABVOL_R:
		case LN_GET_MODBCVOL_R:
		case LN_GET_MODCAVOL_R:
			g_mdl[index].mdl.volIn = recmsg_h.all/100;// 除以100是为了和台达及中兴模块一致，绿能单位mV
			if(g_mdl[index].mdl.volIn <= 2000)				//这里暂时以日本的线电压200V，为世界上的最低的线电压为参考依据。
			{
				break;
			}
			else
			{
				g_mdl[index].mdl.volIn = g_mdl[index].mdl.volIn * 1000 / 1732;		//线电压 转换成 相电压
			}
			/* undervoltage */
			if (g_mdl[index].mdl.volIn < mdlACUnderVol*10)
			{
				SetMdlFault(index, MDL_FAULT_UNDER_VOL);
			}
			/* overvoltage */
			else if (g_mdl[index].mdl.volIn > mdlACOverVol*10)
			{
				SetMdlFault(index, MDL_FAULT_OVER_VOL);
			}
			else
			{
				if((GetMdlFault(index) == MDL_FAULT_UNDER_VOL)||(GetMdlFault(index) == MDL_FAULT_OVER_VOL))
				{
					SetMdlFault(index, MDL_FAULT_FREE);
				}
			}
			break;
		/* 组地址(读写) */
		case LN_SET_GROUP_WR:
			// 暂时不需要
			break;
		/* 环境温度 */
		case LN_GET_MODTMP_R:
			g_mdl[index].mdl.temp = (recmsg_h.all * 0.01 + 500); // 模块读取的数值，要乘以0.001，才是实际的温度值；
			break;
		/* 读取高低压模式 */
		case LN_GET_MOD_MODEL_R:
			g_mdl[index].mdl.mode = recmsg_h.all;
//			DEBUG("index = %d, mode = %d, %d", index, g_mdl[index].mdl.mode, g_mdlMode[index]);
			break;
	}
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: MdlDeal
* ABSTRACT	: 模块自处理线程处理函数
* FUNCTION	: 建立通信，获取实时数据, 处理事件
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void *MdlDeal(void *p)
{
	int i;
	int vol, cur;
	static unsigned long cycleTick = 0;

	while (1)
	{
		if (status_mdldeal)
		{
			break;
		}

		if (diff_timetick(&cycleTick, 2000) == RESULT_OK)
		{
			for (i = 1; i <= g_mdlNum; i++)
			{
				if (g_mdchgSign[i] == 1)	//有已经开始充电的，就主动发送状态。这边实际按照枪的个数来，指示借用模块个数
				{
					/* 读取状态 */
					LN_DCDCM_ReadMessage(0, 0, LN_GET_MOD_STATUS_R);
					break;
				}
			}
		}

		/* deal event */
		for (i = 1; i <= g_mdlNum; i++)
		{
			if (g_mdl[i].step != START)
			{
				continue;
			}

			if (diff_timetick(&g_mdl[i].readTick, 1000) == RESULT_OK)
			{
				/* 读取电压电流 */
				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MOD_OUTVOL_R);
				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MOD_OUTCUR_R);
			}

			if (diff_timetick(&g_mdl[i].pingTick, 5000) == RESULT_OK)
			{
				/* 读取输入电压电流，和环境温度 */
				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MODABVOL_R);
				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MODBCVOL_R);
				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MODCAVOL_R);

				LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MODTMP_R);
			}

			if (diff_timetick(&g_mdl[i].modeTick, 800) == RESULT_OK)
			{
				/* 读取高低压模式 */
				if (g_mdl[i].mdl.mode != g_mdlMode[i])
				{
					LN_DCDCM_ReadMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_GET_MOD_MODEL_R);
				}
			}

			if (diff_timetick(&g_mdl[i].settingTick, 300) == RESULT_OK)
			{
				if (GetMdlSta(i) == MDL_CTRL_OUTPUT)
				{
					/* 要不要按功率来算？ */
					vol = g_mdl[i].mdl.volNeed;
					cur = g_mdl[i].mdl.curNeed;

					LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_MODVOL_WR, vol * 100);
					LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_MODCUR_WR, cur * 10);
					LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_MOD_ONOFF_W, LN_MOD_START);    /* 全部开机 */
					
					SetMdlFault(i, MDL_FAULT_OUTPUT);
				}
				else if (GetMdlSta(i) == MDL_CTRL_STOP)
				{
					LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_MOD_ONOFF_W, LN_MOD_STOP);

					if ((g_mdl[i].mdl.volOut < 500) && (g_mdl[i].mdl.curOut < 500))		// 判断电压50V电流5A降下来后才认为关机完成
					{
						SetMdlSta(i, MDL_CTRL_FREE);
						SetMdlFault(i, MDL_FAULT_FREE);
					}
					
					if (g_mdl[i].mdl.mode != g_mdlMode[i])		// 高低压模式发送
					{
						switch (g_mdlMode[i])
						{
							case PARALLEL_MODE:		// 低压
								LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_HLVOL_W, 2);
								break;
							case SERIES_MODE:		// 高压
								LN_DCDCM_SettingMessage(g_mdl[i].mdl.groupId, g_mdl[i].id, LN_SET_HLVOL_W, 1);
								break;	
						}
					}
				}
			}

			if ((diff_timetick(&g_mdl[i].aliveTick, 10000) == RESULT_OK) && (g_mdl[i].aliveTick != 0))
			{
				SetMdlOffline(i);
			}
			else if (g_mdl[i].aliveTick != 0)
			{
				if (GetMdlFault(i) == MDL_FAULT_OFFLINE)
				{
					SetMdlFault(i, MDL_FAULT_FREE);
				}
			}

			usleep(5000); // 稍微缓一缓，不需要那么快
		}
		
		usleep(20000);
	}
	return NULL;
}


/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: UR_50060E_SetUI
* ABSTRACT	: All parameter initial.
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
/*
	注意是设置单个模块的电压和电流
*/
void UR_100030SW_SetUI(MDL_INFO_STRUCT mdl)
{
	Lock();

	int 		i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == mdl.id)			//首先要addr一致
		{
			if (g_mdl[i].mdl.groupId == mdl.groupId)		//其次要组号一致
			{
				if ((mdl.volNeed != g_mdl[i].mdl.volNeed) || (mdl.curNeed != g_mdl[i].mdl.curNeed))
				{
//					DEBUG("SetUI addr = %d, vol = %d, cur = %d", i, mdl.volNeed, mdl.curNeed);
//					log_send("SetUI addr = %d, vol = %d, cur = %d", i, mdl.volNeed, mdl.curNeed);
				}
				
				if (mdl.volNeed == 0)
				{
					g_mdl[i].mdl.volNeed = 0;
					g_mdl[i].mdl.curNeed = 0;
					SetMdlSta(i, MDL_CTRL_STOP);
				}
				else
				{
					g_mdl[i].mdl.volNeed = mdl.volNeed;
					g_mdl[i].mdl.curNeed = mdl.curNeed;
					SetMdlSta(i, MDL_CTRL_OUTPUT);
				}
			}
			break;
		}
	}

	Unlock();
}

/*
* *******************************************************************************
* MODULE	: UR_50060E_GetInfo
* ABSTRACT	: 获得所有模块信息
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void UR_100030SW_GetInfo(MDL_INFO_STRUCT mdl[MDL_MAX_NUM])
{
	Lock();
	int i = 0;

	for (; i < g_mdlNum; i++)
	{
		memcpy(&mdl[i], &g_mdl[i+1].mdl, sizeof(MDL_INFO_STRUCT));
//		DEBUG("id = %d, %d, fault= %d",g_mdl[i+1].id, mdl[i].id, mdl[i].fault);
	}
	Unlock();
}

/*
* *******************************************************************************
* MODULE	: UR_50060E_SetMdlInfo
* ABSTRACT	: 设置可用模块数量
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void UR_100030SW_SetMdlInfo(int num)
{
	Lock();
	g_mdlNum = num;
	Unlock();
}

/*
* *******************************************************************************
* MODULE	: UR_50060E_SetSN
* ABSTRACT	: 设置SN号
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void UR_100030SW_SetSN(char *sn, int groupId)
{
	Lock();
	/* 匹配SN编号，设置组ID */
	int i;
	for (i = 1; i <= MDL_MAX_NUM; i++)
	{
		if (memcmp(sn, g_sn[i].SN, 18) == 0)
		{
			if (g_sn[i].groupId != groupId)
			{
				g_sn[i].groupId = groupId;
			}
		}
	}
	Unlock();
}

/*
* *******************************************************************************
* MODULE	: DCM_123AA_InitSN
* ABSTRACT	: 进程启动，初始化SN和组号
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int UR_100030SW_InitSN(int addr, unsigned char *sn, int groupId, unsigned int kwh)
{
	Lock();

	if ((groupId == 0) && (groupId > GROUP_MAX_NUM))
	{
		return RESULT_ERR;
	}

	/*
		addr由外部控制
	*/
	g_sn[addr].groupId = groupId;
	memcpy(g_sn[addr].SN, sn, 18);
	g_sn[addr].kwh = kwh;

	g_mdl[addr].mdl.groupId = groupId;
//	g_mdl[addr].id = g_sn[addr].SN[0]-0x30; //传过来的是数字的字符串，必须转换下
	g_mdl[addr].id = atoi((char *)g_sn[addr].SN);
	g_mdl[addr].mdl.id = g_mdl[addr].id;
	memcpy(g_mdl[addr].mdl.SN, sn, 18);

//	DEBUG("addr = %d, id = %d", addr, g_mdl[addr].id);
	/*LOG_DBG("---------------------");
	char sntest[19] = {0};
	memcpy(sntest, g_sn[addr].SN, 18);
	LOG_DBG("sn = %d, id = %d, addr = %d", g_mdl[addr].id, g_sn[addr].groupId, addr);
	LOG_DBG("---------------------");*/

	Unlock();
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: UR_50060E_Init
* ABSTRACT	: All parameter initial.
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void UR_100030SW_Init()
{
	pthread_mutex_init(&mutex_lock, NULL);
	pthread_mutex_init(&mutex_lock2, NULL);
	memset(g_mdl, 0, sizeof(MDL_INFO_STRUCT)*(MDL_MAX_NUM+1));

	/* 初始化can口 */
	can_pthread_init(CAN_NO, &MdlDealCallBack);

	/* 开机发送离线指令，不然需要等待模块自动掉线后才能重新上线。优优绿能的模块不存在主动连接方式*/

	status_mdldeal = 0;
	pthread_create(&thread_mdldeal, NULL, MdlDeal, NULL);
}

/*
* *******************************************************************************
* MODULE	: UR_50060E_Uninit
* ABSTRACT	: All parameter initial.
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void UR_100030SW_Uninit()
{
	/* 等待模块处理线程退出 */
	status_mdldeal = 1;
	pthread_join(thread_mdldeal, NULL);

	/* 注销CAN通信线程 */
	can_pthread_unInit(CAN1);

	/* 其他 */
	pthread_mutex_destroy(&mutex_lock);
	pthread_mutex_destroy(&mutex_lock2);
}

void UR_100030SW_MdlDisconnect()
{
	int					i;

	for (i = 1; i <= MDL_MAX_NUM; i++)
	{
		SetMdlOffline(i);
	}
}

int UR_100030SW_GetMdlTotalNum(void)
{
	return g_mdlNum;
}

int UR_100030SW_GetGroupIdByAddr(int addr)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == addr)
		{
			return g_mdl[i].mdl.groupId;
		}
	}
	return 0;
}

/*
	目前返回的仍然是时间
*/
unsigned long UR_100030SW_GetKwhByAddr(int addr)
{
	int							i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == addr)
		{
			return g_mdl[i].workTime;
		}
	}
	return 0;
}

/*
	根据SN号获取模块运行时间
	这个是用于外部更新json时间用的，并且保护这个部分的独立性
*/
long UR_100030SW_GetKwhBySN(unsigned char *sn)
{
	int							i;

	if (sn == NULL)
	{
		return -1;
	}

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (memcmp(g_sn[i].SN, sn, 1) == 0)
		{
			return (g_sn[i].kwh);
		}
	}

	return -1;
}

int UR_100030SW_CheckMdlStatus(void)
{
	int 				i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if(g_mdl[i].mdl.fault == MDL_FAULT_UNDER_VOL)
		{
			return 1;
		}
		else if(g_mdl[i].mdl.fault == MDL_FAULT_OVER_VOL)
		{
			return 2;
		}
		else if(g_mdl[i].mdl.fault == MDL_FAULT_OVER_TEMP)
		{
			return 3;
		}
	}

	return 0;
}

int UR_100030SW_GetSingleMdlMaxPower(void)
{
	return 300;
}

/*
	挑选模块中最高的温度
	如果是低温情况呢？比如零下？
	返回摄氏度，比例0.1℃
*/
static int GetHighestTemp(void)
{
	int					maxTemp = 0;
	int					zeroNum = 0;
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.temp > maxTemp)
		{
			maxTemp = g_mdl[i].mdl.temp;
		}
		if (g_mdl[i].mdl.temp == 0)
		{
			zeroNum ++;
		}
	}

	if (zeroNum == g_mdlNum)		//数据不准确,返回一个正常温度
	{
		return 220;
	}

	return (maxTemp - 500);
}

/*
	根据当前情况计算单个模块能够输出的最大功率
	如果要同时考虑温度的影响，需要对温度消抖，主要是功率变成0的地方，需要消抖。
*/
static int GetSingleMdlMaxPwrByVol(int groupId, int vol)
{
	int				targetPwr = 300;	// 这里默认30.0kW
	int 			mdlMode = 0;
	int 			i = 0;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			mdlMode = g_mdlMode[i];
			break;
		}
	}

	switch (mdlMode)
	{
		case SERIES_MODE:
				if (vol >= 5000)
				{
					targetPwr = 30 * 10;		//满功率部分
				}
				else if (vol < 5000 && vol >= 4000)
				{
					targetPwr = vol / 10 * 60 / 100;
				}
				else if (vol < 4000 && vol >= 2000)
				{
					targetPwr = (vol * vol + 800 * vol) / 8 / 10000;	// pwr=(vol*vol+800*vol)/8
				}
				break;
		default:
				if (vol >= 3000)
				{
					targetPwr = 30 * 10;		//满功率部分
				}
				else if (vol < 3000 && vol >= 2000)
				{
					targetPwr = vol * 100 / 1000;
				}
				break;
	}
	//targetPwr = targetPwr > maxTempPwr ? maxTempCur : targetPwr;
	//targetPwr = targetPwr > maxCurPwr ? maxCurPwr : targetPwr;
	return targetPwr;
}

int UR_100030SW_GetSingleMdlPowerByVol(int groupid, int vol)
{
	return (GetSingleMdlMaxPwrByVol(groupid, vol));
}

int UR_100030SW_GetMdlVolByAddr(int addr)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == addr)
		{
			return g_mdl[i].mdl.volOut;
		}
	}
	return 0;
}

int UR_100030SW_GetMdlCurByAddr(int addr)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == addr)
		{
			return g_mdl[i].mdl.curOut;
		}
	}
	return 0;
}

int UR_100030SW_GetMdlOnline(int addr)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].id == addr)
		{
			if (g_mdl[i].step == DISCONNECT)
			{
				return -1;
			}
			return 0;
		}
	}

	return -1;
}

int	UR_100030SW_GetMdlAllOnline(void)
{
	int					i;
	int					num = 0;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].step == DISCONNECT)
		{
			num ++;
		}
	}

	if (num == g_mdlNum)
	{
		return -1;
	}
	else if (num == 0)
	{
		return 0;
	}
	else if ( num < g_mdlNum)
	{
		return 1;
	}

	return -1;
}

/*

*/
int UR_100030SW_GetGroupMdlOnline(int groupId)
{
	int					i;
	int					groupNum = 0;
	int					onlineNum = 0;

	for (i = 1; i <= MDL_MAX_NUM; i++)
	{
		if (g_sn[i].groupId == groupId)
		{
			groupNum ++;			//某组应该有的模块数量
		}
	}

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			if (g_mdl[i].step != DISCONNECT)
			{
				onlineNum ++;		//某组实际有的模块数量
			}
		}
	}

	if (groupNum == 0)
	{
		return 0;
	}
	else if (onlineNum == 0)
	{
		return -1;
	}
	else if (onlineNum < groupNum)
	{
		return 1;
	}

	return 0;
}

/*
	注意返回的是0.1KW
*/
int UR_100030SW_GetGroupMaxPwr(int groupId)
{
	int					groupNum = 0;
	int					i;

	for (i = 1; i <= MDL_MAX_NUM; i++)
	{
		if (g_sn[i].groupId == groupId)
		{
			groupNum ++;			//某组应该有的模块数量
		}
	}

	return (groupNum * 300);
}

/*
	获取当前情况下能够提供的功率
	这个功率是要实时发送给车端的，会随着温度而变化
	为了能够有所缓冲，在这里计算的时候，会把温度提高UR_50060E_PRE_TEMP摄氏度，如果是负数，就降低5度
*/
#define UR_100030SW_PRE_TEMP			10
int UR_100030SW_GetGroupEnvMaxPwr(int groupId)
{
	int					groupNum = 0;
	int					i;
	int					maxTemp = GetHighestTemp();
	int					oneMaxPwr = 0;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			if (g_mdl[i].step != DISCONNECT)
			{
				groupNum ++;		//某组实际有的模块数量
			}
		}
	}

	if (maxTemp < 0)
	{
		maxTemp -= UR_100030SW_PRE_TEMP;
	}
	else if (maxTemp >= 0)
	{
		maxTemp += UR_100030SW_PRE_TEMP;
	}

	if (maxTemp < -400)
	{
		oneMaxPwr =  0;
	}
	else if (maxTemp >= -400 && maxTemp < 550)
	{
		oneMaxPwr = 300;
	}
	else if (maxTemp >= 550 && maxTemp <= 700)
	{
		oneMaxPwr = 850 - maxTemp;				//来自功率曲线
	}
	else
	{
		oneMaxPwr = 0;
	}

	return (oneMaxPwr * groupNum);
}

/*
	注意返回的是0.1KW
*/
int UR_100030SW_GetGroupPwrByVol(int groupId, int vol)
{
	int					groupNum = 0;
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			if (g_mdl[i].step != DISCONNECT)
			{
				groupNum ++;		//某组实际有的模块数量
			}
		}
	}

	return (GetSingleMdlMaxPwrByVol(groupId, vol) * groupNum);
}

unsigned long UR_100030SW_GetGroupKwhByAverage(int groupId)
{
	int					groupNum = 0;
	int					i;
	unsigned long		worktime = 0;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			if (g_mdl[i].step != DISCONNECT)
			{
				groupNum ++;		//某组实际有的模块数量
				worktime += g_mdl[i].workTime;
			}
		}
	}

	return (worktime / groupNum);
}

/*
	注意返回的是温度0.1摄氏度，偏移50摄氏度
*/
unsigned int UR_100030SW_GetMaxTemp(void)
{
	int					i;
	unsigned int 		Maxtemp = 0;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.temp >= Maxtemp)
		{
			Maxtemp = g_mdl[i].mdl.temp;
		}
	}

	return Maxtemp;
}

int UR_100030SW_GetGroupMaxCurByVolPwr(int groupId, int vol, int pwr)
{
	int					groupNum = 0;
	int					i;
	int 				curtmp, cur = 0;
	int					mdlNeedNum = 0;
	int					maxTemp = GetHighestTemp();
	int					oneMaxPwr = 0;
	int					groupOnlineNum = 0, mdlOnlineNum = 0;
	int 				mdlMode;
	
	if (maxTemp < 0)
	{
		maxTemp -= UR_100030SW_PRE_TEMP;
	}
	else if (maxTemp >= 0)
	{
		maxTemp += UR_100030SW_PRE_TEMP;
	}

	if (maxTemp < -400)
	{
		oneMaxPwr =  0;
	}
	else if (maxTemp >= -400 && maxTemp < 550)
	{
		oneMaxPwr = 300;
	}
	else if (maxTemp >= 550 && maxTemp <= 700)
	{
		oneMaxPwr = 850 - maxTemp;				//来自功率曲线
	}
	else
	{
		oneMaxPwr = 0;
	}

	oneMaxPwr = 300;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			mdlMode = g_mdlMode[i];
			groupNum++;
			if (g_mdl[i].step != DISCONNECT)
			{
				groupOnlineNum++;				//某组实际有的模块数量
			}
		}

		if (g_mdl[i].step != DISCONNECT)
		{
			mdlOnlineNum++;						//某组实际有的模块数量
		}
	}

	if (groupNum == 0)
	{
		return 0;
	}

	mdlNeedNum = pwr/300 + (((pwr%300)>0)? 1: 0);
	/*如果功率所需要的个数大于 该组个数，则说明用到其他组模块个数；
	 * 如果所需要个数大于总在线个数，则以实际在线个数计算；
	 * 如果所需要个数等于小于该组个数，则根据该组实际在线个数计算*/
	if (mdlNeedNum > groupNum)
	{
		if (mdlNeedNum > mdlOnlineNum)
		{
			mdlNeedNum = mdlOnlineNum;
		}
	}
	else
	{
		if (mdlNeedNum > groupOnlineNum)
		{
			mdlNeedNum = groupOnlineNum;
		}
	}

	if (vol < 1500)		// 原来是 if (vol == 0)
	{
		return 3000 * mdlNeedNum;								//暂时以1000V，30A作为计算点。
	}
	else if (vol > 10000)
	{
		curtmp = oneMaxPwr * 100 / 1000 * 100;					//这里单位W计算
	}
	else
	{
		curtmp = oneMaxPwr * 100 * 1000 / vol;					//这里单位W计算
	}

	switch (mdlMode)
	{
		case SERIES_MODE:
				if (vol >= 5000)
				{
					cur = curtmp;
				}
				else if (vol < 5000 && vol >= 4000)
				{
					cur = (curtmp > 6000) ? 6000: curtmp;
				}
				else if (vol < 4000 && vol >= 1500)
				{
					cur = (vol + 800) * 10 / 8;		// vol = 8*cur - 80
					cur = (curtmp > cur) ? cur: curtmp;
				}
				break;
		default:
				if (vol >= 3000)
				{
					cur = curtmp;
				}
				else if (vol < 3000 && vol >= 1500)
				{
					cur = (curtmp > 10000) ? 10000: curtmp;
				}

				break;
	}

	int	curcal = (pwr * 1000 * 100) / vol;
	
	return ((curcal > cur * mdlNeedNum)? cur * mdlNeedNum: curcal);

}

void UR_100030SW_SetGroupChgSign(int groupId, int sign)
{
	g_mdchgSign[groupId] = (sign >= 1)? 1: 0;	//从1开始
}

void UR_100030SW_SetGroupChgMode(int groupId, int mode)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			g_mdlMode[i] = mode;
		}
	}
}

int UR_100030SW_GetGroupChgMode(int groupId)
{
	int					i;

	for (i = 1; i <= g_mdlNum; i++)
	{
		if (g_mdl[i].mdl.groupId == groupId)
		{
			return g_mdlMode[i];
		}
	}

	return 0;
}

void UR_100030SW_SetACVoltageLimit(int underVol, int overVol)
{
	if ((underVol < 173) || (overVol > 259))
	{
		return;
	}

	mdlACUnderVol = underVol;
	mdlACOverVol = overVol;
}

MDL_FUNCTIONS_STRUCT	UR_100030SW_Functions =
{
	.Init					= UR_100030SW_Init,
	.Uninit					= UR_100030SW_Uninit,
	.GetMdlInfo				= UR_100030SW_GetInfo,
	.SetMdlUI				= UR_100030SW_SetUI,
	.SetSN				    = UR_100030SW_InitSN,						//DCM_123AA_SetSN,
	.SetMdlInfo				= UR_100030SW_SetMdlInfo,					//设置模块个数
	.DisConnectMdl		    = UR_100030SW_MdlDisconnect,
	.GetMdlTotalNum			= UR_100030SW_GetMdlTotalNum,
	.GetGroupIdByAddr		= UR_100030SW_GetGroupIdByAddr,
	.GetKwhByAddr			= UR_100030SW_GetKwhByAddr,
	.GetSingleMdlPowerByVol = UR_100030SW_GetSingleMdlPowerByVol,
	.GetMdlVolByAddr		= UR_100030SW_GetMdlVolByAddr,
	.GetMdlCurByAddr		= UR_100030SW_GetMdlCurByAddr,
	.GetGroupMaxPwr		    = UR_100030SW_GetGroupMaxPwr,				//我这么随便的增加接口有点不合适吧，以后要合并一些
	.GetGroupEnvMaxPwr		= UR_100030SW_GetGroupEnvMaxPwr,
	.GetGroupPwrByVol		= UR_100030SW_GetGroupPwrByVol,
	.GetGroupKwhByAverage   = UR_100030SW_GetGroupKwhByAverage,
	.GetGroupMaxCurByVolPwr	= UR_100030SW_GetGroupMaxCurByVolPwr,
	.SetGroupChgSign		= UR_100030SW_SetGroupChgSign,
	.SetGroupChgMode		= UR_100030SW_SetGroupChgMode,
	.SetACVoltageLimit 		= UR_100030SW_SetACVoltageLimit,
};

MDL_FUNCTIONS_STRUCT *Get_UR_100030SW_Functions(void)
{
	return &UR_100030SW_Functions;
}
