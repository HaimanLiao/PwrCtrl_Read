/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: zmqProt.h
* SYSTEM NAME		: 协议层
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200219
* *******************************************************************************
*/

#ifndef __ZMQPROT_H__
#define __ZMQPROT_H__

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"
#include "statJson.h"
#include "modCtrl.h"
#include "relayCtrl.h"
#include "cnfJson.h"
#include "smartOps.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/

/* 前端数据相关方法 */
#define 	MTD_SET_PWR_REQ 	"setPwrReq"
#define 	MTD_SET_PWR_RES 	"setPwrRes"
#define 	MTD_GET_PWR_REQ 	"getPwrReq"
#define 	MTD_GET_PWR_RES 	"getPwrRes"
#define 	MTD_STAT_REQ 		"getStatReq"
#define 	MTD_STAT_RES 		"getStatRes"

/* 充电相关方法 */
#define 	MTD_HEART_REQ 	"heartReq"
#define 	MTD_HEART_RES 	"heartRes"
#define 	MTD_PWR_REQ 	"pwrReq"
#define 	MTD_PWR_RES 	"pwrRes"
#define 	MTD_STATUS 		"status"
#define 	MTD_OTA_REQ 	"OTAReq"
#define 	MTD_OTA_RES 	"OTARes"

/*
* *******************************************************************************
*                                  Union
* *******************************************************************************
*/


/*
* *******************************************************************************
*                                  Struct
* *******************************************************************************
*/

#pragma pack(1)

typedef struct
{
	int tempVal;
	int humVal;
	char liqVal;
	char atmoVal;
	char gyroVal;
	char doorVal;
	char CBVal;
	char surgingVal;
}SENSORS_INFO_STRUCT;

/****************************通用帧头数据结构体***********************************/
typedef struct
{
	unsigned int requestId;		// 帧唯一识别码，时间戳表示
	char 		  method[16];	// 报文类型
	unsigned char pwrUnitId;	// 桩编号
	unsigned char terminalId;	// 终端编号
	unsigned char connId;		// 枪编号
	char 		  body[MAX_BUFFER_LEN];	// body内容数据
}FRAME_DATA_STRUCT;

/****************************前端数据及操作结构体*********************************/
typedef struct
{
	char 	pwrType;			// 功率分配模式
	int 	pwrMax;				// 最大允许功率
	char 	pwrNum;				// 枪数量
	char 	pwrUnitNum;			// 功率柜数量
	char 	pduType;			// PDU类型
	char 	pduNum;				// PDU中数量
	char 	pduRlyNum;			// PDU中继电器数量
	char 	fanType;			// 风扇类型
}PWR_PARA_STRUCT;

/* 功率设置 */
typedef struct
{
	/* 桩设置 */
	char 	pwrSetEn;				// 0-disable，1-enbale
	PWR_PARA_STRUCT pwr;

	/* 模块设置 */
	char 	mdlSetEn;				// 0-disable，1-enbale
	struct
	{
		int 	mdlUnderVol;		// 欠压
		int 	mdlOverVol;			// 过压
		char 	mdlNum;				// 模块数量
		char 	mdlType;			// 模块类型
		SN_STRUCT sn[MDL_MAX_NUM];	// SN号
	}mdl;

	/* 传感器设置 */
	char 	sensorSetEn;			// 0-disable，1-enbale
	SENSORS_CNF_STRUCT sensor;

	/* 硬件检测 */
	char 	hwSetEn;				// 0-disable，1-enbale
	HW_STRUCT hw;

	char 	checkSetEn;				// 0-disable，1-enbale
	char 	pwrCheck;
	char 	pwrGunId;
	int 	pwrVol;
	int 	pwrCur;
}WEB_SET_STRUCT;

typedef struct
{
	char result;					// 0-未知，1-成功，2-失败
}WEB_SET_RESULT_STRUCT;

/* 功率状态 */
typedef struct
{
	PWR_PARA_STRUCT pwr;
	
	struct
	{
		char estop;
		char sd;
		int outPower;
		char fan;
		int fanSpeed;
	}status;

	unsigned char mdlCount;
	unsigned char mdlType;
	unsigned int mdlUnderVol;
	unsigned int mdlOverVol;
	MDL_INFO_STRUCT mdl[MDL_MAX_NUM];

	unsigned char pduCount;
	RELAY_INFO_STRUCT pdu[PDU_MAX_NUM];

	SENSORS_CNF_STRUCT sensor;
	SENSORS_INFO_STRUCT	senInfo;
}WEB_POWER_STATUS_STRUCT;

/* 智能运维 */
typedef STAT_STRUCT WEB_SMARTOPS_STRUCT;

/****************************充电主题相关结构体***********************************/
typedef struct
{
	char 		  time[24];		// 计时时间
}FRAME_DATA_HEART_STRUCT;

typedef struct
{
	unsigned char chgSta;		// 充电状态，1-运行、暂停，2-停止，3-故障
	unsigned char workMode;		// 工作模式，1-一体机，2-分体
	unsigned char cardSta;		// 刷卡状态，0-未知，1-刷卡成功，2-刷卡失败
}FRAME_DATA_STATUS_REQ_STRUCT;

typedef struct
{
	int pwrErr[DTC_MAX_NUM];	// DTC故障码
}FRAME_DATA_STATUS_STRUCT;

typedef struct
{
	unsigned char chgSta;		// 充电状态，1-运行、暂停，2-停止，3-故障
	unsigned char chgStep;		// 充电阶段，
	unsigned int GunPwrMax;		// 枪端最大功率
	unsigned int EVPwrMax;		// 车端最大功率
	unsigned int EVVolMax;		// 车端最大允许电压
	unsigned int EVCurMax;		// 车端最大允许电流
	unsigned int reqVol;		// 需求电压
	unsigned int reqCur;		// 需求电流
	unsigned int stopReason;	// 停止原因
}FRAME_DATA_PWR_REQ_STRUCT;

typedef struct
{
	unsigned char pwrSta;		// 功率状态，1-运行、暂停，2-停止，3-故障
	unsigned char pwrStep;		// 功率阶段，1-切换中，2-准备完成，3-停止
	unsigned int EVSEPwrMax;	// 桩端最大允许功率
	unsigned int EVSEVolMax;	// 桩端最大允许电压
	unsigned int EVSEVolMin;	// 桩端最小允许电压
	unsigned int EVSECurMax;	// 桩端最大允许电流，充电时可能会实时变化
	unsigned int realVol;		// 实时电压
	unsigned int realCur;		// 实时电流
	unsigned int stopReason;	// 停止原因
}FRAME_DATA_PWR_RES_STRUCT;

typedef struct
{
	unsigned char mode;			// 功率状态，模式，0-结束，1-功率检测模式
}FRAME_DATA_CHECK_REQ_STRUCT;

typedef struct
{
	unsigned char OTAEnable;	// 使能，模式，0-disable，1-enable
	unsigned char OTAPath[128];	// FTP服务端升级文件所在目录
}FRAME_DATA_OTA_REQ_STRUCT;

typedef struct
{
	unsigned char OTAStep;		// OTA阶段，0-未知，1-准备升级，2-升级中，3-升级成功，4-升级失败
	unsigned char failReason;	// 失败原因，0-未知，1-充电中无法升级，2-ftp升级失败，0xFF-其他
}FRAME_DATA_OTA_RES_STRUCT;

/****************************外部函数回调结构体***********************************/
typedef struct
{
	/* 数据获取 */
	CallBackFun	GetInfoFuns;

	/* 事件处理 */
	CallBackFun	eventHandle;
}HANDLE_STRUCT;

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/
void ZmqProt_SendSetPwrResFrame(FRAME_DATA_STRUCT frame);
void ZmqProt_SendGetPwrResFrame(FRAME_DATA_STRUCT frame);
void ZmqProt_SendStatResFrame(FRAME_DATA_STRUCT frame);

void ZmqProt_SendHeartResFrame(FRAME_DATA_STRUCT frame);
void ZmqProt_SendStatusFrame(FRAME_DATA_STRUCT frame);
void ZmqProt_SendPwrResFrame(FRAME_DATA_STRUCT frame);
void ZmqProt_SendOTAResFrame(FRAME_DATA_STRUCT frame);

void ZmqProt_Init(HANDLE_STRUCT *pHandle);

#endif // __ZMQPROT_H__
