/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: option.h
* SYSTEM NAME		: 
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: 
* DEPARTMENT		:
* DATE				: 
* *******************************************************************************
*/

#ifndef _OPTION_H
#define _OPTION_H

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
//#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "log_sys.h"
#include "dlog.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

#define 	MDL_MAX_NUM			12
#define 	GUN_MAX_NUM 		12
#define 	GROUP_MAX_NUM		12
#define 	PDU_MAX_NUM			12
#define 	FAN_MAX_NUM			2

#define 	MDL_SN_MAX_LENGTH	19
#define 	PDU_RELAY_MAX_NUM 	6

#define 	GUN_DC_MAX_NUM		6
#define 	GROUP_MDL_NUM		12

#define 	DTC_MAX_NUM			5

#define 	RESULT_OK			0
#define 	RESULT_TOUT			-2
#define 	RESULT_ERR			-1

#define 	CONNECT_OK			0
#define 	CONNECT_ERR			-1

#define 	MAX_BUFFER_LEN		(1024 * 6)		//这里的大小只兼容6把枪的配置
#define		BUFFER_LEN			1024

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/

typedef enum
{
	MAIN_STATUS_MODULE_CHANGE = 0,			// 模块变化(数量或者SN号)
	MAIN_STATUS_PWR_CHANGE,					// 功率参数变化
	MAIN_STATUS_FAN_CHANGE,					// 风扇变化
	MAIN_STATUS_RELAY_CHANGE,				// PDU变化
	MAIN_STATUS_PROGRAM_QUIT,				// 程序退出
}ENUM_MAIN_STATUS;

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

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

typedef int (*CallBackFun)(unsigned char *pframe, int len);

unsigned char add_check(unsigned char *buf, int len);
void hex2string(unsigned char *hex, int len, char *str);
void hex2ascii(unsigned char *hex, int len, char *str);

int SetMainStatusPending(int ctrl, ENUM_MAIN_STATUS type);

int  CurlDownloadFile(char *szCurlUrl,
                      char *pUsernamePassword,
                      char * fpDownloadFile,
                      int  nConnectTimeout, int nTimeout, char isPassiveMode);

#endif /* option.c */
