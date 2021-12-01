/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: relaySCII.c
* SYSTEM NAME		: 西埃PDU，私有协议
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200221
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "relaySCII.h"
#include "can_api.h"
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

#define 	CMD_HEART			0x14003055
#define 	CMD_RELAY_CTRL		0x0C2A3055
#define 	CMD_RELAY_RES		0x102B5530

#define		CAN_NO 				CAN1

typedef struct
{
	unsigned char sw[SINGLE_PDU_RELAY_NUM];		// 需要发送的数据//lhm: 发送有效数据6个（can帧数据段固定是8个字节）
	unsigned char isSend;						// 0-不发送，1-有数据需要发送

	unsigned long recvTick;						// 接收超时
}RELAY_PRI_STRUCT;

static 	RELAY_INFO_STRUCT	g_relayInfo[PDU_MAX_NUM] = {0};
static 	RELAY_PRI_STRUCT 	g_relayPri[PDU_MAX_NUM] = {0};
static 	pthread_t 			dealPid = 0;

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
* MODULE	: HeartFrame
* ABSTRACT	: 心跳帧组包
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void HeartFrame(int relayNo, struct can_frame *dstFrame)//lhm: relayNo是PDU地址
{
	dstFrame->can_id = CMD_HEART | (relayNo << 8);
	dstFrame->can_dlc = 0;
	//lhm: can_id是can帧的id域，can_dlc是can帧的数据长度域，data是can帧的数据域
}

/*
* *******************************************************************************
* MODULE	: RelayCtrlFrame
* ABSTRACT	: 继电器控制帧组包
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void R elayCtrlFrame(unsigned char *pSw, int relayNo, struct can_frame *dstFrame)//lhm: relayNo是PDU地址
{
	dstFrame->can_id = CMD_RELAY_CTRL | (relayNo << 8);
	dstFrame->can_dlc = 6; 
	memcpy(dstFrame->data, pSw, 6);
}

/*
* *******************************************************************************
* MODULE	: RelayResFrame
* ABSTRACT	: 继电器响应帧解析
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void RelayResFrame(struct can_frame recvFrame)//lhm: PDU回复数据存储在g_relayInfo[x]，发送给PDU的数据从g_relayPri[x]中取
{
	int i = 0;
	int addr = (recvFrame.can_id & 0x0F) - 1;

	g_relayInfo[addr].id = addr + 1;
	memcpy(g_relayInfo[addr].sw, recvFrame.data, 6);
	g_relayInfo[addr].temp[0] = recvFrame.data[6];//lhm: temp是PDU温度
	g_relayInfo[addr].temp[1] = recvFrame.data[7];
	/* 如控制第一个继电器闭合，其他断开，则发送 1 0 0 0 0 0，正确返回应该是0x11 0x00 0x00 0x00 0x00 0x00,
	   其中0x11高字节表示DC+反馈状态，低字节表示DC-反馈状态
	 */
	for (i = 0; i < 6; i++)
	{
		if (g_relayPri[addr].sw[i] == 0x00)
		{
			if (g_relayInfo[addr].sw[i] != 0x00)
			{
				break;
			}
		}

		if (g_relayPri[addr].sw[i] == 0x01)
		{
			if (g_relayInfo[addr].sw[i] != 0x11)
			{
				break;
			}
		}
	}

	if (i == 6)			// 反馈结果正确
	{
		g_relayPri[addr].isSend = 0;//lhm: g_relayPri[x]不需要重发
		g_relayInfo[addr].ctrlBack = RELAY_BACK_OK;
	}

	g_relayPri[addr].recvTick = get_timetick();
}

/*
* *******************************************************************************
* MODULE	: FilterBuffer
* ABSTRACT	: 帧过滤，判断帧格式
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int FilterBuffer(struct can_frame recvFrame, int len)
{
	int id = recvFrame.can_id & 0xFFFFFFF0;
	int addr = recvFrame.can_id & 0x0F;

	//lhm: 一把枪对应一个PDU；每个PDU有一个地址；挂在can总线上的是PDU控制板；一个PDU控制板连接两个PDU（2x6 = 12个继电器）
	if ((id == CMD_RELAY_RES) && (addr > 0) && (addr <= 12))	// 这里12暂时写死，实际是枪数量
	{
		return len;
	}

	return RESULT_ERR;
}

/*
* *******************************************************************************
* MODULE	: ProtDataCallBack
* ABSTRACT	: 数据接收回调函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
//lhm: 作为can线程的回调函数（can线程便可以修改本文件的私有变量g_relayInfo[x]）-----新建can线程去处理接收，自己文件只负责读取接收数据区，比如g_relayInfo[x]
static int ProtDataCallBack(unsigned char *pframe, int len)
{
	int ret = RESULT_OK;
	struct can_frame recvFrame;

	memcpy(&recvFrame, pframe, sizeof(struct can_frame));

	ret = FilterBuffer(recvFrame, len);
	if (ret > 0)
	{
		// 解析
		int id = recvFrame.can_id & 0xFFFFFFF0;

		switch (id)
		{
			case CMD_RELAY_RES:
				RelayResFrame(recvFrame);
				break;
			default:
				break;
		}
	}

	return ret;
}

/*
* *******************************************************************************
* MODULE	: ProtDataSend
* ABSTRACT	: 数据打包发送函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ProtDataSend(struct can_frame sendFrame, int len)
{
	can_send(CAN_NO, sendFrame);
}

/*
* *******************************************************************************
* MODULE	: SetSwitch
* ABSTRACT	: 操作继电器开关
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetSwitch(unsigned char *pSW, int relayNo)
{
	int ret = RESULT_OK;
	int idx = relayNo - 1;
	int i = 0;

	/* 继电器操作次数 */
	for (i = 0; i < SINGLE_PDU_RELAY_NUM; i++)
	{
		if (g_relayPri[idx].sw[i] != pSW[i])
		{
			g_relayInfo[idx].swTimes[i]++;//lhm: 继电器有切换，则操作次数+1
		}
	}

	if (memcmp(g_relayPri[idx].sw, pSW, SINGLE_PDU_RELAY_NUM))//lhm: 如果不相等（大于或小于）则需要发送
	{
		memcpy(g_relayPri[idx].sw, pSW, SINGLE_PDU_RELAY_NUM);
		g_relayPri[idx].isSend = 1;
		g_relayInfo[idx].ctrlBack = RELAY_BACK_WAIT;
	}
	
	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetInfo
* ABSTRACT	: 获得继电器信息
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetInfo(RELAY_INFO_STRUCT *pInfo, int relayNo)
{
	int ret = RESULT_OK;
	int idx = relayNo - 1;

	memcpy(pInfo, &g_relayInfo[idx], sizeof(RELAY_INFO_STRUCT));
	
	return ret;
}

/*
* *******************************************************************************
* MODULE	: RelayDeal
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void *RelayDeal(void *p)
{
	int i = 0;
	struct can_frame sendFrame;
	unsigned long sendTick[PDU_MAX_NUM] = {0};//lhm: 代码中的tick应该理解为时间戳（记录了一个完整的时间点），而非频率脉动次数
	unsigned long heartTick[PDU_MAX_NUM] = {0};
	unsigned long canTick = 0;

	/* 初始化，断开所有继电器 */
	unsigned char sw[SINGLE_PDU_RELAY_NUM] = {0};
	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		RelayCtrlFrame(sw, i+1, &sendFrame);
		ProtDataSend(sendFrame, sizeof(struct can_frame));
		usleep(5000);//lhm: 5000us = 5ms
	}
	
	while (1)
	{
		for (i = 0; i < PDU_MAX_NUM; i++)
		{
			//lhm: recvTick在发送的地方会把当时的时间戳赋值给它
			//lhm: 如果PDU模块接收超时
			if (abs(get_timetick() - g_relayPri[i].recvTick) > 5000)//lhm: 心跳帧会一直发送
			{
				g_relayInfo[i].online = 0;
			}
			else
			{
				g_relayInfo[i].online = 1;
			}

			if (g_relayPri[i].isSend)
			{
				//lhm: g_relayPri[i].sw---PDU模块i的继电器组sw的数据，i + 1是PDU模块的非零编号，用于广播发送后的接收过滤
				//lhm: sw是一个uint8_t数组，每个字节通过0 / 1存储每个PDU板子12个继电器控制IO口的断开 / 闭合命令
				RelayCtrlFrame(g_relayPri[i].sw, i+1, &sendFrame);
				ProtDataSend(sendFrame, sizeof(struct can_frame));

				if (sendTick[i] == 0)		// 发送开始计时//lhm: sendTick[i]会一直记录发送时刻，直到循环检测到超时
				{
					sendTick[i] = get_timetick();
					g_relayInfo[i].ctrlBack = RELAY_BACK_WAIT;
				}
				else
				{
					/* 长时间反馈失败，可能是粘连了 */
					if (abs(get_timetick() - sendTick[i]) > 5000)
					{
						g_relayPri[i].isSend = 0;
						g_relayInfo[i].ctrlBack = RELAY_BACK_ERR;//lhm: g_relayInfo用的时候发现有问题会立马采取相应措施（重发或者直接把错误反馈到上面去），之后再反馈成功是被无视的
					}
				}
			}
			else
			{
				sendTick[i] = 0;
			}

			if (abs(get_timetick() - heartTick[i]) > 1000+i*10)
			{
				heartTick[i] = get_timetick();
				HeartFrame(i+1, &sendFrame);
				ProtDataSend(sendFrame, sizeof(struct can_frame));
			}
			usleep(5000);
		}

		if (abs(get_timetick() - canTick) > 5000)//lhm: 间隔一段时间检测PDU是否“全部”掉线，如果是则重启can（接收）线程
		{
			canTick = get_timetick();
			
			for (i = 0; i < PDU_MAX_NUM; i++)
			{
				if (g_relayInfo[i].online == 1)
				{
					break;
				}
			}

			if (i == PDU_MAX_NUM)//lhm: 如果PDU全部断线
			{
				can_pthread_unInit(CAN_NO);
				usleep(100*1000);
				can_pthread_init(CAN_NO, &ProtDataCallBack);
			}
		}

		usleep(50000);
	}

	return NULL;
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
	int i = 0;
	for (i = 0; i < PDU_MAX_NUM; i++)
	{
		g_relayInfo[i].online = 1;
		g_relayPri[i].recvTick = get_timetick();
	}

	can_pthread_init(CAN_NO, &ProtDataCallBack);
	pthread_create(&dealPid, 0, RelayDeal, NULL);
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
	can_pthread_unInit(CAN_NO);

	/* 等待线程退出 */
	pthread_cancel(dealPid);
	pthread_join(dealPid, NULL);
}

RELAY_FUNCTIONS_STRUCT	Relay_SCII_Functions =
{
	.Init					= Init,
	.Uninit					= Uninit,
	.SetSwitch				= SetSwitch,
	.GetInfo				= GetInfo,
};

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: Get_RelaySCII_Functions
* ABSTRACT	: 获取函数指针
* FUNCTION	: 
* ARGUMENT	: RELAY_FUNCTIONS_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
RELAY_FUNCTIONS_STRUCT *Get_RelaySCII_Functions(void)
{
	return &Relay_SCII_Functions;
}