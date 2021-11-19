/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: msg.h
* SYSTEM NAME		: list、ringbuffer等方式的集合
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200228
* *******************************************************************************
*/

#ifndef __MSG_H__
#define __MSG_H__

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"

/*
* *******************************************************************************
*                                  Define
* *******************************************************************************
*/

#define 	MSG_TYPE_LIST	1
#define 	MSG_TYPE_RING	2

/*
* *******************************************************************************
*                                  Enum
* *******************************************************************************
*/


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

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

int msg_write(int fd, unsigned char *pframe, int len);
int msg_read(int fd, unsigned char *pframe, int len);
int msg_free(int fd, int len);
int msg_size(int fd);
int msg_init(int msgType);
int msg_unInit(int fd);

#endif // __MSG_H__
