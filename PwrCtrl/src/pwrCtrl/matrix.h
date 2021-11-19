/*
* *******************************************************************************
*    @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: matrix.h
* SYSTEM NAME		: 
* BLOCK NAME		: 
* PROGRAM NAME		: 
* MODULE FORM		: 
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		: 
* DATE				: 
* *******************************************************************************
*/
#ifndef __MATRIX_H__
#define __MATRIX_H__
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

#define 	HAND2HAND 			1		// 策略分配模式，手拉手
#define 	MATRIX2HAND 		2		// 策略分配模式，6枪1个整流柜
#define 	MATRIX2MATRIX		3		// 策略分配模式，12枪1个整流柜

#define 	POLICY_OK			0		// 策略分配成功
#define 	POLICY_ERR 			1		// 策略分配失败
#define 	POLICY_NOCHANGE		2		// 策略没有变化

#define 	CHANGE_NO			0		// 不操作
#define 	CHANGE_IN			1		// 切入
#define 	CHANGE_OUT 			2		// 切出

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

typedef struct{
	unsigned char 		gunNum;			// 支持的枪数量
	unsigned char 		groupNum;		// 支持的组数量
    unsigned char 		relayNum;		// 支持的继电器数量
    unsigned int 		groupPwr;		// 一组模块的功率
    unsigned char 		grpSync;		// 模组对称系数，即1、7、13..为兄弟组，2、8、14为兄弟组，grpId=(n+6*grpSync)
    unsigned char 		policyType;		// 策略模式
}MATRIX_INIT_STRUCT;

typedef struct
{
	unsigned int 		gunId;		// 枪号
	unsigned int 		volNeed;
	unsigned int 		curNeed;
	unsigned int 		pwrNeed;
	unsigned long		startTime;	// 开始充电时间
}CHG_POLICY_NEED_STRUCT;

typedef struct{
	unsigned char 			gunId;
    unsigned char 			gunUsed;         	// 1-已使用 0-未使用
    int						pwrMax;				// 最大提供的功率
    CHG_POLICY_NEED_STRUCT	gun;
}GUN_STRUCT;

typedef struct{
    unsigned char  		groupId;
    unsigned char 		groupUsed;         		// 1-已被使用 0-尚未使用
    unsigned char 		sta;					// 0-正常 1-失败，不能参与分配
}GROUP_STRUCT;

typedef struct
{
	unsigned int 		gunId;
	unsigned int 		gunRes;						// CHANGE_NO CHANGE_IN CHANGE_OUT

	unsigned int 		pwrMax;						// 分配好的功率,0.1kW

	unsigned int 		groupNum;
	unsigned int 		groupId[GROUP_MAX_NUM];		// 分配好的模组编号//lhm: 应该是模块编号

	unsigned int 		relayNum;					// 需要操作的PDU数量
	unsigned int		relayId[PDU_MAX_NUM];		// 需要操作的PDU编号
	unsigned char 		relaySW[PDU_MAX_NUM][6];	// 需要操作的PDU的继电器
	//lhm: relayId[i]存到是PDU编号（PDU不分组，从1~12），relaySW[i]（6个字节大小的元素）存的是该PDU继电器的通断情况

	unsigned int 		freeGrpNum;
	unsigned int 		freeGrpId[GROUP_MAX_NUM];	// 需要释放的模组//lhm: 应该是模块编号

	unsigned int 		addGrpNum;
	unsigned int 		addGrpId[GROUP_MAX_NUM];	// 需要添加的模组//lhm: 应该是模块编号
}CHG_POLICY_STRUCT;//lhm: 针对一把枪的策略部署（该枪所连接的PDU继电器通断情况）

typedef struct
{
	int 				result;		// POLICY_OK POLICY_ERR POLICY_NOCHANGE

	CHG_POLICY_STRUCT 	oldResAry[GUN_DC_MAX_NUM];
	CHG_POLICY_STRUCT 	resAry[GUN_DC_MAX_NUM];
}CHG_POLICY_RES_STRUCT;
//lhm: 策略分配算法————重新分配各把枪得到的模组

#pragma pack()

/*
* *******************************************************************************
*                                  Extern
* *******************************************************************************
*/

void Matrix_Init(MATRIX_INIT_STRUCT init);
CHG_POLICY_RES_STRUCT Matrix_Policy(CHG_POLICY_NEED_STRUCT chg);
void Matrix_Back();
void Matrix_FreeAll();
int Matrix_SetGroupSta(int grpId, int sta);//lhm: 提供操作matrix.c文件中变量g_group的接口（设置模块状态，同时将modCtrl.c的模块id和g_group的对应模块id对应起来）
//lhm: 本代码最多支持12个模块（g_group的数组元素个数是12），如果是MATRIX2MATRIX，对称系数只能是1；如果是MATRIX2HAND，对称系数可以是1，2
//lhm: 本代码最多支持6把枪，虽然MATRIX2MATRIX的本意应该是可以支持到12把枪，但是如果加入枪7或以上，那么代码会在多处地方出错
void MatrixPrint(CHG_POLICY_RES_STRUCT res, char *string);

#endif // __MATRIX_H__
