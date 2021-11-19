/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: fanEBM.c
* SYSTEM NAME	: EBM风扇，MODBUS协议
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

#include "fanEBM.h"
#include "serial.h"
#include "crc.h"
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

/* 命令号 */
#define		CMD_READ_REG				0x04		// 读风机寄存器
#define		CMD_SET_REG					0x06		// 写风机寄存器

/* 风扇地址 */
#define		FAN1_ADDR					0x01		// 风扇1的地址
#define		FAN2_ADDR					0x02		// 风扇2的地址

/* 风扇写寄存器地址 */
#define		REGADDR_SPEED_SET			0xD001		// 设置风机转速
#define		REGADDR_FANADDR_SET			0xD100		// 设置风机地址
#define		REGADDR_CTRSTYLE_SET		0xD101		// 设置风机信号控制方式 （0：模拟量 / 1：RS485）
#define		REGADDR_PARA_SET			0xD105		// 设置风机参数表 （0：参数表1 / 1：参数表2）
#define		REGADDR_CTRMODE_SET			0xD106		// 设置风机控制模式 （0：闭环控制 / 1：传感器控制）

/* 风扇读寄存器地址 */
#define		REGADDR_FANADDR_READ		0xD010		// 读取风机转速
#define		REGADDR_FANTEMP_READ		0xD015		// 读取风机电源温度
#define		REGADDR_FANSTA_READ			0xD011		// 读取风机运行状态
#define		REGADDR_FANWAR_READ			0xD012		// 读取风机预警状态

/* 各数据在一帧数据里的位置信息 */
#define		ADDR_SITE					0			// 风机地址在帧的第1个字节上
#define		CMD_SITE					1			// 读写指令在帧的第2个字节上

/* 发送命令帧 */
#define		TREGHO_SITE					2			// 寄存器地址高位在帧的第3个字节上
#define		TREGLO_SITE					3			// 寄存器地址低位在帧的第4个字节上
#define		TDATAHO_SITE				4			// 数据高位在帧的第5个字节上
#define		TDATALO_SITE				5			// 数据低位在帧的第6个字节上
#define		TCRCLO_SITE					6			// 检验高位在帧的第7个字节上
#define		TCRCHO_SITE					7			// 检验低位在帧的第8个字节上

/* 返回命令帧 */
#define		RDATACOUNT_SITE				2			// 返回帧第三字节是返回数据的长度，读一个寄存器一般是两位
#define		RDATAHO_SITE				3			// 数据高位在帧的第4个字节上
#define		RDATALO_SITE				4			// 数据低位在帧的第5个字节上
#define		RCRCLO_SITE					5			// 检验高位在帧的第6个字节上
#define		RCRCHO_SITE					6			// 检验低位在帧的第7个字节上

#define		FAN_MAX_SPEED				64000		// 风扇最大转速
#define		FAN_MAX_REAL_SPEED			4000		// 风扇实际最大转速 4000rpm
#define		FAN_STOP_SPEED				0			// 风扇停止

#define 	REGADDR_NUM					7			// 需要发送的寄存器个数

#define		FAN_MAX_NUM					2			// 风扇数量

#define 	UART_NO 					TTYMXC3
#define 	UART_N1						TTYMXC4

#pragma pack(1)

typedef struct
{
	int reg;
	int regAddr;
	int data;
}FAN_CMD_STRUCT;

#pragma pack()

static 	FAN_INFO_STRUCT			g_fanInfo[FAN_MAX_NUM] = { 0 };			// 风扇信息结构体
static 	unsigned long 			g_fanCommTick[FAN_MAX_NUM] = { 0 };		// 风扇通信超时
static 	pthread_t  				dealPid = 0;

static  FAN_CMD_STRUCT 			g_fanCMD[REGADDR_NUM] = {
//	reg 				regAddr 				data
//	{CMD_SET_REG, 		REGADDR_FANADDR_SET, 	1},
	{CMD_SET_REG, 		REGADDR_CTRSTYLE_SET, 	1},
	{CMD_SET_REG, 		REGADDR_PARA_SET, 		0},
	{CMD_SET_REG, 		REGADDR_CTRMODE_SET, 	0},
	{CMD_SET_REG, 		REGADDR_SPEED_SET, 		0},
	{CMD_READ_REG, 		REGADDR_FANADDR_READ, 	1},
//	{CMD_READ_REG, 		REGADDR_FANTEMP_READ, 	1},
	{CMD_READ_REG, 		REGADDR_FANSTA_READ, 	1},
	{CMD_READ_REG, 		REGADDR_FANWAR_READ, 	1}
};

static int g_fanWriteOK[FAN_MAX_NUM][REGADDR_NUM] = {0};

/*
* *******************************************************************************
*                                  Private function prototypes
* *******************************************************************************
*/

static void ProtDataSend(unsigned char *pframe, int len);

/*
* *******************************************************************************
*                                  Private functions
* *******************************************************************************
*/

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
static int FilterBuffer(unsigned char *pframe, int len)
{
	/* 读命令返回帧格式：风机地址（1B）+ 命令号（1B）+ 数据字节数（1B）+ 数据（2B）+ 校验（2B） */
	/* 写命令返回帧格式：风机地址（1B）+ 命令号（1B）+ 寄存器地址（2B）+ 数据（2B）+ 校验（2B） */
	if (len < (RCRCHO_SITE + 1))	// 返回帧一般为7或8个字节
	{
		return -1;
	}

	if ((pframe[CMD_SITE] != CMD_SET_REG) && (pframe[CMD_SITE] != CMD_READ_REG))
	{
		return -1;
	}

	if ((len != (TCRCHO_SITE + 1)) && (len != (RCRCHO_SITE + 1)))
	{
		return -1;
	}

	if (len == (TCRCHO_SITE + 1))                      //写命令返回帧一般为8位
	{
		unsigned char crcH = CRC16(pframe, TCRCLO_SITE, Hi);
		unsigned char crcL = CRC16(pframe, TCRCLO_SITE, Lo);

		if ((crcH != pframe[TCRCHO_SITE]) || (crcL != pframe[TCRCLO_SITE]))		// 校验失败
		{
			return -1;
		}
		else
		{
			len = TCRCHO_SITE + 1;
			return len;
		}
	}

	if (len == (RCRCHO_SITE + 1))                       //读命令返回帧一般为7位
	{
		unsigned char crcH = CRC16(pframe, RCRCLO_SITE, Hi);
		unsigned char crcL = CRC16(pframe, RCRCLO_SITE, Lo);

		if ((crcH != pframe[RCRCHO_SITE]) || (crcL != pframe[RCRCLO_SITE]))		// 校验失败
		{
			return -1;
		}
		else
		{
			len = RCRCHO_SITE + 1;
			return len;
		}
	}

	return len;
}

/*
* *******************************************************************************
* MODULE	: PackFrame
* ABSTRACT	: 帧格式组包发送
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void PackFrameSend(int addr, int cmd, int regaddr, int data)
{
	/* 读或写命令发送帧格式：风机地址（1B）+ 命令号（1B）+ 寄存器地址（2B）+ 数据（2B）+ 校验（2B） */
	unsigned char sendFrame[TCRCHO_SITE + 1] = {0};

	sendFrame[ADDR_SITE] = addr;
	sendFrame[CMD_SITE] = (cmd == CMD_READ_REG ? CMD_READ_REG : CMD_SET_REG);
	sendFrame[TREGHO_SITE] = regaddr >> 8;
	sendFrame[TREGLO_SITE] = regaddr;
	sendFrame[TDATAHO_SITE] = data >> 8;
	sendFrame[TDATALO_SITE] = data;

	unsigned char crcH = CRC16((unsigned char *)&sendFrame, TCRCLO_SITE, Hi);
	unsigned char crcL = CRC16((unsigned char *)&sendFrame, TCRCLO_SITE, Lo);

	sendFrame[TCRCHO_SITE] = crcH;
	sendFrame[TCRCLO_SITE] = crcL;

	ProtDataSend((unsigned char *)&sendFrame, TCRCHO_SITE + 1);

	if (cmd == CMD_READ_REG)
	{
		g_fanInfo[addr-1].currentcmd = regaddr;
		g_fanInfo[addr].currentcmd = regaddr;
	}
}

/*
* *******************************************************************************
* MODULE	: UnpackFrame1
* ABSTRACT	: 获取风扇1帧数据内容
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int UnpackFrame1(unsigned char *pframe, int len, int fanAddr)
{
	int addr = fanAddr;
	if (addr != 1)
	{
		return;
	}
	int type = pframe[CMD_SITE];
	int reg = (pframe[TREGHO_SITE] << 8) | pframe[TREGLO_SITE];
	int data = (pframe[RDATAHO_SITE] << 8) | pframe[RDATALO_SITE];

	addr = addr - 1;
	
	if (type == CMD_READ_REG)
	{
		switch (g_fanInfo[addr].currentcmd)
		{
			case REGADDR_FANADDR_READ:
				g_fanInfo[addr].speed = data  * 100 / FAN_MAX_SPEED;
//				DEBUG("addr = %d, speed = %d", addr, g_fanInfo[addr].speed);
				break;
			case REGADDR_FANTEMP_READ:
//				g_fanInfo[addr].temp = data;
//				DEBUG("addr = %d, temp = %d", addr, g_fanInfo[addr].temp);
				break;
			case REGADDR_FANSTA_READ:
				g_fanInfo[addr].error = data;
//				DEBUG("error = %x", g_fanInfo[addr].error);
				break;
			case REGADDR_FANWAR_READ:
				g_fanInfo[addr].warning = data;
//				DEBUG("warning = %x", g_fanInfo[addr].warning);
				break;
			default:
				break;
		}

		g_fanInfo[addr].currentcmd = 0;
	}
	else if (type == CMD_SET_REG)
	{
		data = (pframe[TDATAHO_SITE] << 8) | pframe[TDATALO_SITE];
//		DEBUG("reg = %x", reg);
		int i = 0;
		for (i = 0; i < REGADDR_NUM; i++)
		{
			if ((g_fanCMD[i].regAddr == reg) && (g_fanCMD[i].data == data) 
				&& (reg != REGADDR_SPEED_SET))
			{
//				DEBUG("i = %d", i);
				g_fanWriteOK[addr][i] = 1;
			}
			else
			{
				g_fanWriteOK[addr][i] = 0;
			}
		}
	}

	return 0;
}

/*
* *******************************************************************************
* MODULE	: UnpackFrame2
* ABSTRACT	: 获取风扇2帧数据内容
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int UnpackFrame2(unsigned char* pframe, int len, int fanAddr)
{
	int addr = fanAddr;
	if (addr != 2)
	{
		return;
	}
	int type = pframe[CMD_SITE];
	int reg = (pframe[TREGHO_SITE] << 8) | pframe[TREGLO_SITE];
	int data = (pframe[RDATAHO_SITE] << 8) | pframe[RDATALO_SITE];

	addr = addr - 1;

	if (type == CMD_READ_REG)
	{
		DEBUG("g_fanInfo[%d].currentcmd = %x", addr, g_fanInfo[addr].currentcmd);
		switch (g_fanInfo[addr].currentcmd)
		{
		case REGADDR_FANADDR_READ:
			g_fanInfo[addr].speed = data * 100 / FAN_MAX_SPEED;
//			DEBUG("addr = %d, speed = %d", addr, g_fanInfo[addr].speed);
			break;
		case REGADDR_FANTEMP_READ:
//			g_fanInfo[addr].temp = data;
//			DEBUG("addr = %d, temp = %d", addr, g_fanInfo[addr].temp);
			break;
		case REGADDR_FANSTA_READ:
			g_fanInfo[addr].error = data;
//			DEBUG("addr = %d, error = %x", addr, g_fanInfo[addr].error);
			break;
		case REGADDR_FANWAR_READ:
			g_fanInfo[addr].warning = data;
//			DEBUG("addr = %d, warning = %x", addr, g_fanInfo[addr].warning);
			break;
		default:
			break;
		}

		g_fanInfo[addr].currentcmd = 0;
	}
	else if (type == CMD_SET_REG)
	{
		data = (pframe[TDATAHO_SITE] << 8) | pframe[TDATALO_SITE];
//		DEBUG("reg = %x", reg);
		int i = 0;
		for (i = 0; i < REGADDR_NUM; i++)
		{
			if ((g_fanCMD[i].regAddr == reg) && (g_fanCMD[i].data == data)
				&& (reg != REGADDR_SPEED_SET))
			{
//				DEBUG("i = %d", i);
				g_fanWriteOK[addr][i] = 1;
			}
			else
			{
				g_fanWriteOK[addr][i] = 0;
			}
		}
	}

	return 0;
}

/*
* *******************************************************************************
* MODULE	: ProtDataCallBack1
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
static int ProtDataCallBack1(unsigned char *pframe, int len)
{
/*	char string[1024] = {0};

	hex2string(pframe, len, string);
	DEBUG("address1 = %s", string);*/

	int ret = 0;
	unsigned char recvFrame[len];

	memcpy(&recvFrame, pframe, len);
	ret = FilterBuffer(pframe, len);

	if (ret > 0)
	{
		int addr = recvFrame[ADDR_SITE];
		g_fanCommTick[addr - 1] = get_timetick();

		UnpackFrame1(recvFrame, ret, addr);
	}

	return len;
}

/*
* *******************************************************************************
* MODULE	: ProtDataCallBack2
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
static int ProtDataCallBack2(unsigned char* pframe, int len)
{
	/*char string[1024] = {0};

	hex2string(pframe, len, string);
	DEBUG("address2= %s", string);*/

	int ret = 0;
	unsigned char recvFrame[len];

	memcpy(&recvFrame, pframe, len);
	ret = FilterBuffer(pframe, len);

	if (ret > 0)
	{
		int addr = recvFrame[ADDR_SITE];
		addr = addr + 1;
		g_fanCommTick[addr - 1] = get_timetick();

		UnpackFrame2(recvFrame, ret, addr);
	}

	return len;
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
static void ProtDataSend(unsigned char *pframe, int len)
{
/*	char string[1024] = {0};

	hex2string(pframe, len, string);
	DEBUG("%s", string);*/

	serial_send(UART_NO, (char *)pframe, len);
	serial_send(UART_N1, (char*)pframe, len);
}

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
//	g_fanInfo[addr].runTime = 0;
	g_fanInfo[addr].error = 0;
	g_fanInfo[addr].warning = 0;
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
	speed = speed * FAN_MAX_SPEED / 100;

	if ((speed <= FAN_MAX_SPEED) && (speed >= 0))
	{
		g_fanCMD[3].data = speed;
	}

	return 0;
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
	memcpy(pInfo, &g_fanInfo[addr-1], sizeof(FAN_INFO_STRUCT));
	return 0;
}

/*
* *******************************************************************************
* MODULE	: FanDeal
* ABSTRACT	: 风扇进程
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void *FanDeal(void *p)
{
	int i = 0;
	int addr = 0;
	int regAddr = 0;
	unsigned long fanDealTick = 0 ;
	unsigned long fanRunTick[FAN_MAX_NUM] = {0};
	int realAddr, fanAddr;

	while (1)
	{
		for (i = 0; i < FAN_MAX_NUM; i++)
		{
			addr = i == 0 ? FAN1_ADDR : FAN2_ADDR;
			addr -= 1;

			if (abs(get_timetick() - g_fanCommTick[addr]) > 6000)
			{
				g_fanInfo[addr].online = 0;
				ClearInfo(addr);
//				DEBUG("addr = %d", addr);
			}
			else
			{
				g_fanInfo[addr].online = 1;
			}

			if (g_fanInfo[addr].speed != 0)		// 说明风扇在运行，需要计算时间
			{
				if (abs(get_timetick() - fanRunTick[addr]) > 1000)
				{
					fanRunTick[addr] = get_timetick();
					g_fanInfo[addr].runTime += 1;
				}
			}
			else
			{
				fanRunTick[addr] = get_timetick();
			}
		}

		if (abs(get_timetick() - fanDealTick) > 500)
		{
			fanDealTick = get_timetick();

			fanAddr = FAN1_ADDR;
			regAddr++;


			if (regAddr > REGADDR_NUM)
			{
				regAddr = 1;
			}

			/*for (i = 0; i < REGADDR_NUM; i++)
			{
				if (regAddr > REGADDR_NUM)
				{
					regAddr = 1;
				}
				else
				{
					regAddr++;
				}

				if (regAddr <= REGADDR_NUM)
				{
					realAddr = regAddr - 1;
					fanAddr = FAN1_ADDR;
				}
				else if (regAddr <= REGADDR_NUM*2)
				{
					realAddr = regAddr - REGADDR_NUM - 1;
					fanAddr = FAN2_ADDR;
				}
				else
				{
					realAddr = 1;
					fanAddr = FAN1_ADDR;
				}

				if (g_fanWriteOK[fanAddr - 1][realAddr] == 0)
				{
					PackFrameSend(fanAddr, g_fanCMD[realAddr].reg, g_fanCMD[realAddr].regAddr, g_fanCMD[realAddr].data);
					break;
				}
				else
				{
					continue;
				}
			}*/
			realAddr = regAddr - 1;			//暂时做成所有命令轮着发送
			//DEBUG("realAddr = %d", realAddr);
			PackFrameSend(fanAddr, g_fanCMD[realAddr].reg, g_fanCMD[realAddr].regAddr, g_fanCMD[realAddr].data);
		}

		usleep(100*1000);
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
	serial_pthread_open(UART_NO, &ProtDataCallBack1);
	serial_pthread_open(UART_N1, &ProtDataCallBack2);
	serial_set_com(UART_NO, 19200, 8, "1", 'E');
	serial_set_com(UART_N1, 19200, 8, "1", 'E');
	pthread_create(&dealPid, 0, FanDeal, NULL);
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
	serial_pthread_close(UART_NO);
	serial_pthread_close(UART_N1);
}

FAN_FUNCTIONS_STRUCT	FAN_EBM_Functions =
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
* MODULE	: Get_FanEBM_Functions
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
FAN_FUNCTIONS_STRUCT *Get_FanEBM_Functions(void)
{
	return &FAN_EBM_Functions;
}