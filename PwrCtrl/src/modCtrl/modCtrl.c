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
#include "modCtrl.h"
#include "groupsCtrl.h"
#include "./plug/UR_100030SW.h"

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
static 		MDL_FUNCTIONS_STRUCT 	*g_pMdlFun = NULL;
static 		GROUP_INFO_STRUCT 		g_group[GROUP_MAX_NUM] = {0};
static 		MDL_INFO_STRUCT			g_mdl[MDL_MAX_NUM] = {0};
static 		MDL_PARA_STRUCT			g_mdlPara = {0};
static		unsigned char			MdlQueneSign[GROUP_MAX_NUM] = {0};

#pragma pack(1)

/*
	代表一个模块
	主要用于策略分配
*/
typedef struct
{
	int				addr;				//模块地址
	/*
		用 KWH 来评判模块寿命，用4字节保存，180KW * 24h * 365天 * 10年 = 15768000
		比四字节的上限还小很多，可以使用
	*/
	unsigned long 	kwh;				//已经输出的千瓦时功率//lhm:从部署开始算起，累计的输出
	int				volOut;				//当前输出电压
	int				curOut;				//当前输出电流
	int				volNeed;			//单个模块需要输出的电压
	int				curNeed;			//单个模块需要输出的电流
	int				mdlOnoff;			//模块是否在运行，因为台达测试的后端电压，所以只能从状态判断是否在运行
} STRUCT_MDL_CELL;

typedef struct
{
	/* 策略计算需要的信息 */
	int					groupId;
	int					mdlPwr;								//每个模块能够提供的最大功率, 单位KW，分辨率0.1
	int 				numTotal;							//某个组的所有模块数量
	STRUCT_MDL_CELL		mdlList[GROUP_MDL_NUM];

	/* 下面是策略计算的结果 */
	STRUCT_MDL_CELL		*mdlPoint[GROUP_MDL_NUM];			//指向策略计算的结果
} STRUCT_MDL_POLICY;

#pragma pack(0)

STRUCT_MDL_POLICY			MdlPolicy[GROUP_MDL_NUM] = {0};

#define	HIGH_PWR_THRESHOLD_VALUE				0		//如果偏差值超过这个值，就添加一个模块,单位(KW, 分辨率0.1)
#define LOW_PWR_THRESHOLD_VALUE					-5		//如果偏差值小于这个值，就减少一个模块,单位(KW, 分辨率0.1)

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
	根据当前“组”需求电压和电流来策略分配
	调用者应该提供模块的基本信息，包括addr, volOut, curOut等，这些会影响策略分配
	groupVolNeed 分辨率0.1V
	groupCurNeed 分辨率将0.01A

	功率分辨率0.1KW

	策略计算并不充分，还需要考虑模块本身的特性，比如温度变化影响最大功率
	比如模块电压输出范围，以及受电压影响，最大功率的变化
*/
static int MDL_Policy(STRUCT_MDL_POLICY *mdlPolicy, int groupVolNeed, int groupCurNeed)
{
	int						i;
	int						inUseMdlNum;			//正在使用的模块数量
	int						targetNum;				//目标数量
	int						changeNum;
	int						diff;

	if (mdlPolicy->numTotal == 0)
	{
		return -1;
	}

	/* 如果需求电压为0，那么就将所有组内模块的需求电压电流设置为0 */
	if (groupVolNeed == 0)
	{
		for (i = 0; i < mdlPolicy->numTotal; i++)
		{
			mdlPolicy->mdlList[i].volNeed = 0;
			mdlPolicy->mdlList[i].curNeed = 0;
		}
		return 0;
	}

	/* 先按照消耗时间排序一次 */
	/*
	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		for (j = 0; j < mdlPolicy->numTotal - 1; j++)
		{
			if (mdlPolicy->mdlPoint[j]->kwh > mdlPolicy->mdlPoint[j + 1]->kwh)
			{
				STRUCT_MDL_CELL			*tmp;

				tmp = mdlPolicy->mdlPoint[j];
				mdlPolicy->mdlPoint[j] = mdlPolicy->mdlPoint[j + 1];
				mdlPolicy->mdlPoint[j + 1] = tmp;
			}
		}
	}
	*/
	/*
		再按照是否正在使用再次排序
		正在使用的排在前面
	*/
	/*
	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		for (j = 0; j < mdlPolicy->numTotal - 1; j++)
		{
			if (mdlPolicy->mdlPoint[j]->curOut < mdlPolicy->mdlPoint[j + 1]->curOut)
			{
				STRUCT_MDL_CELL			*tmp;

				tmp = mdlPolicy->mdlPoint[j];
				mdlPolicy->mdlPoint[j] = mdlPolicy->mdlPoint[j + 1];
				mdlPolicy->mdlPoint[j + 1] = tmp;
			}
		}
	}
	*/

	inUseMdlNum = 0;
	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		if (mdlPolicy->mdlPoint[i]->volOut >= 500)			//只要模块输出电压大于50V，就认为模块正在工作
		{
			inUseMdlNum ++;
		}
	}

	/*
		计算需求功率和当前已使用的模块能够提供的功率之间的差值
		mdlPolicy->mdlPwr的分辨率应该是0.1KW
		diff同时需要处理负数

		这部分的目的是“消抖”，需求电压只有在高于某个阀值才会增加模块
		只有低于某个阀值才会减少模块
	*/
	diff = groupVolNeed / 10 * groupCurNeed / 100 / 100 - inUseMdlNum * mdlPolicy->mdlPwr;

	targetNum = inUseMdlNum;
	if (diff <= HIGH_PWR_THRESHOLD_VALUE && diff >= ( - (mdlPolicy->mdlPwr - LOW_PWR_THRESHOLD_VALUE)))
	{
		;
	}
	else if (diff > 0)
	{
		changeNum = (diff - HIGH_PWR_THRESHOLD_VALUE) / mdlPolicy->mdlPwr + 1;
		targetNum += changeNum;
	}
	else if (diff < 0)
	{
		changeNum = (abs(diff) + LOW_PWR_THRESHOLD_VALUE) / mdlPolicy->mdlPwr;
		targetNum -= changeNum;
	}

	/* 数据合理化 */
	if (targetNum < 1)
	{
		targetNum = 1;
	}
	else if (targetNum > mdlPolicy->numTotal)
	{
		targetNum = mdlPolicy->numTotal;
	}

	/*
		到这里已经计算出需要使用的模块了，优先级也已经排好
		可以计算每个模块需要输出的电压电流了
		原则：排在前面的模块尽可能输出多的功率
	*/
	int				maxCurForThisVol = mdlPolicy->mdlPwr * 100 * 1000 / groupVolNeed;
	int				curTemp = groupCurNeed;
	for (i = 0; i < targetNum; i++)
	{
		mdlPolicy->mdlPoint[i]->volNeed = groupVolNeed;
		if (curTemp <= 0)
		{
			mdlPolicy->mdlPoint[i]->curNeed = 0;		//有电压需求，但是没有电流需求
			//mdlPolicy->mdlPoint[i]->volNeed = 0;
		}
		else if (curTemp < maxCurForThisVol)
		{
			mdlPolicy->mdlPoint[i]->curNeed = curTemp;
			curTemp = 0;
		}
		else
		{
			mdlPolicy->mdlPoint[i]->curNeed = maxCurForThisVol;
			curTemp -= maxCurForThisVol;
		}
	}

	for (i = targetNum; i < mdlPolicy->numTotal; i++)
	{
		mdlPolicy->mdlPoint[i]->volNeed = 0;
		mdlPolicy->mdlPoint[i]->curNeed = 0;
	}

	return 0;
}

/*
	根据组号填充MDL_TIME_STRUCT需要的信息
*/
static int MDL_FillMdlPolicy(GROUP_INFO_STRUCT group, STRUCT_MDL_POLICY		*mdlPolicy)
{
	int						totalNum = g_pMdlFun->GetMdlTotalNum();		//所有模块个数
	int						i, j;

	if (totalNum == 0)
	{
		DEBUG("Total Module Num = 0!");
		return -1;
	}

	//mdlPolicy->mdlPwr = g_pMdlFun->GetSingleMdlMaxPower();
	/* 认为一组模块在某个电压下能够输出的功率都是一样的 */
	mdlPolicy->mdlPwr = g_pMdlFun->GetSingleMdlPowerByVol(group.id, group.volNeed);
	mdlPolicy->numTotal = 0;
	mdlPolicy->groupId = group.id;


	for (i = 0; i < totalNum; i++)
	{
		if (g_pMdlFun->GetGroupIdByAddr(i + 1) == group.id)
		{
			mdlPolicy->mdlList[mdlPolicy->numTotal].addr = i + 1;
			mdlPolicy->mdlList[mdlPolicy->numTotal].kwh = g_pMdlFun->GetKwhByAddr(i + 1);
			mdlPolicy->mdlList[mdlPolicy->numTotal].volOut = g_pMdlFun->GetMdlVolByAddr(i + 1);
			mdlPolicy->mdlList[mdlPolicy->numTotal].curOut = g_pMdlFun->GetMdlCurByAddr(i + 1);
			mdlPolicy->numTotal ++;
		}
	}

	/* 把结果指针预先一一指向对应的模块,后面再调整优先顺序 */
	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		mdlPolicy->mdlPoint[i] = &(mdlPolicy->mdlList[i]);
	}

	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		for (j = 0; j < mdlPolicy->numTotal - 1; j++)
		{
			if (mdlPolicy->mdlPoint[j]->kwh > mdlPolicy->mdlPoint[j + 1]->kwh)
			{
				STRUCT_MDL_CELL			*tmp;

				tmp = mdlPolicy->mdlPoint[j];
				mdlPolicy->mdlPoint[j] = mdlPolicy->mdlPoint[j + 1];
				mdlPolicy->mdlPoint[j + 1] = tmp;
			}
		}
	}
	return 0;
}

/*
	根据组号填充MDL_TIME_STRUCT需要的信息
*/
static int MDL_RefreshMdlPolicy(GROUP_INFO_STRUCT group, STRUCT_MDL_POLICY		*mdlPolicy)
{
	int						i, addr;

	//mdlPolicy->mdlPwr = g_pMdlFun->GetSingleMdlMaxPower();
	/* 认为一组模块在某个电压下能够输出的功率都是一样的 */
	mdlPolicy->mdlPwr = g_pMdlFun->GetSingleMdlPowerByVol(group.id, group.volNeed);

	for (i = 0; i < mdlPolicy->numTotal; i++)
	{
		addr = mdlPolicy->mdlList[i].addr;
		mdlPolicy->mdlList[i].kwh = g_pMdlFun->GetKwhByAddr(addr);
		mdlPolicy->mdlList[i].volOut = g_pMdlFun->GetMdlVolByAddr(addr);
		mdlPolicy->mdlList[i].curOut = g_pMdlFun->GetMdlCurByAddr(addr);
	}

	return 0;
} 
/*
	还需要完善
	暂时只考虑30kw的台达模块(策略计算是通用的)
*/
static int MDL_SetGroupUI(GROUP_INFO_STRUCT group)
{
	int								i;
	MDL_INFO_STRUCT					mdl;
	int								groupId = group.id;

	if (groupId < 1)
	{
		return RESULT_ERR;
	}

	if (MdlQueneSign[groupId - 1] == 1)
	{
		MdlQueneSign[groupId - 1] = 0;
		MDL_FillMdlPolicy(group, &MdlPolicy[groupId - 1]);
	}
	else
	{
		MDL_RefreshMdlPolicy(group, &MdlPolicy[groupId - 1]);
	}

	MDL_Policy(&MdlPolicy[groupId-1], group.volNeed, group.curNeed);

	for (i = 0; i < MdlPolicy[groupId-1].numTotal; i++)
	{
		mdl.id = MdlPolicy[groupId-1].mdlList[i].addr;
		mdl.groupId = group.id;
		mdl.volNeed = MdlPolicy[groupId-1].mdlList[i].volNeed;
		mdl.curNeed = MdlPolicy[groupId-1].mdlList[i].curNeed;
#ifdef POLICYDEBUG
		POLICYDEBUGSTART
			fprintf(dout, "Module id = %d, Group id = %d, volNeed = %d, curNeed = %d\n",
																	mdl.id, mdl.groupId, mdl.volNeed, mdl.curNeed);
		POLICYDEBUGEND
#endif
//		log_send("Module id = %d, Group id = %d, volNeed = %d, curNeed = %d\n",
//																	mdl.id, mdl.groupId, mdl.volNeed, mdl.curNeed);
		g_pMdlFun->SetMdlUI(mdl);
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: GetGroupInfo
* ABSTRACT	: 以组的方式获得组数据
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static int GetGroupInfo(GROUP_INFO_STRUCT *group)
{
	if ((group->id > GROUP_MAX_NUM) || (group->id == 0))
	{
		DLOG_ERR("GetGroupInfo, group id = %d", group->id);
		return RESULT_ERR;
	}

	int i, flag, modeFlag;
	int mdlnum = 0;
	int vol = 0;
	g_pMdlFun->GetMdlInfo(g_mdl);

	g_group[group->id-1].mdlNum = 0;
	g_group[group->id-1].mdlOnlineNum = 0;
	g_group[group->id-1].curOut = 0;
	g_group[group->id-1].mode = NORMAL_MODE;

	flag = 0;	// 能否找到匹配的模块数据
	modeFlag = 0;
	for (i = 0; i < MDL_MAX_NUM; i++)
	{
		if (g_mdl[i].groupId == group->id)
		{
			g_group[group->id-1].id = group->id;
			mdlnum = g_group[group->id-1].mdlNum;
			memcpy(&g_group[group->id-1].mdl[mdlnum], &g_mdl[i], sizeof(MDL_INFO_STRUCT));
			g_group[group->id-1].mdlNum++;

			vol = vol < g_mdl[i].volOut ? g_mdl[i].volOut : vol;
			g_group[group->id-1].volOut = vol;
			g_group[group->id-1].curOut += g_mdl[i].curOut;

			if (g_mdl[i].fault <= MDL_FAULT_OUTPUT)	// 模块状态正常
			{
				g_group[group->id-1].mdlOnlineNum++;
			}

			if (g_mdl[i].sts)	// 模块状态正常
			{
				g_group[group->id-1].sts = g_mdl[i].sts;
			}
			else
			{
				g_group[group->id-1].sts = 0;
			}

			if (modeFlag == 0)		// 模组高低压赋值，1个模组内的模块必须在同一模式下
			{
				modeFlag = g_mdl[i].mode;
			}
			else
			{
				if (modeFlag != g_mdl[i].mode)
				{
					modeFlag = -1;
				}
			}

			flag = 1;
		}
	}

	if (modeFlag > 0)
	{
		g_group[group->id-1].mode = modeFlag;
	}

	if (!flag)
	{
		/* 没有模块对某个组,那么认为改组电压电流为0 */
		//DLOG_WARN("None Module in group %d!", group->id);
		group->volOut = 0;
		group->curOut = 0;
		group->mdlNum = 0;
		return RESULT_OK;
	}

	memcpy(group, &g_group[group->id-1], sizeof(GROUP_INFO_STRUCT));
	return RESULT_OK;
}

/* 以下部分接口为模组控制，并不是单个模块控制*/
static int SetGroupUI(int groupId, int vol, int cur)
{
	if ((groupId > GROUP_MAX_NUM) || (groupId == 0))
	{
		DLOG_ERR("SetGroupUI, group id = %d", groupId);
		return RESULT_ERR;
	}
	
	GROUP_INFO_STRUCT			group;

	group.id = groupId;
	group.volNeed = vol;
	group.curNeed = cur ? cur : cur+1;	// 这里+1是为了不出现0的情况，因为下层函数里面这个参数作为了除数

	if (MDL_SetGroupUI(group) != RESULT_OK)
	{
		return -1;
	}

	return 0;
}

static int SetGroupMode(int groupId, int mode)
{
	g_pMdlFun->SetGroupChgMode(groupId, mode);

	return 0;
}

static int GetGroupUI(int groupId, int *vol, int *cur)
{
	GROUP_INFO_STRUCT			group;

	group.id = groupId;

	if (GetGroupInfo(&group) != RESULT_OK)
	{
		return -1;
	}

	*vol = group.volOut;
	*cur = group.curOut;

	return 0;
}

/* 
	注意，返回的功率分辨率是0.1KW
*/
static int GetGroupPwrByVol(int groupId, int vol)
{
	return (g_pMdlFun->GetGroupPwrByVol(groupId, vol));
}

static int GetGroupEnvMaxPwr(int groupId)
{
	return (g_pMdlFun->GetGroupEnvMaxPwr(groupId));
}

static int GetGroupMaxCurByVol(int groupId, int vol)
{
	return (g_pMdlFun->GetGroupMaxCurByVolPwr(groupId, vol, 300));
}

static unsigned long GetGroupWorktime(int groupId)
{
	return (g_pMdlFun->GetGroupKwhByAverage(groupId));
}

static int GetGroupOnline(int groupId)
{
	if ((groupId > GROUP_MAX_NUM) || (groupId == 0))
	{
		DLOG_ERR("GetGroupOnline, group id = %d", groupId);
		return RESULT_ERR;
	}

	GROUP_INFO_STRUCT group;
	group.id = groupId;
	GetGroupInfo(&group);

	groupId -= 1;

	if ((g_group[groupId].mdlNum == g_group[groupId].mdlOnlineNum) && (g_group[groupId].mdlNum))
	{
//		DEBUG("mdlNum = %d, mdlOnlineNum = %d", g_group[groupId].mdlNum, g_group[groupId].mdlOnlineNum);
		return RESULT_OK;
	}
	
	return RESULT_ERR;
}

static void SetGroupChgSign(int sign)
{
	int i = 0;
	for (i = 0; i < GROUP_MAX_NUM; i++)
	{
		g_pMdlFun->SetGroupChgSign(i+1, sign);
	}
}

static SINGLE_GROUP_FUNCTINS SingleGroupFunctions =
{
	.SetGroupUI = SetGroupUI,
	.SetGroupMode = SetGroupMode,
	.GetGroupUI = GetGroupUI,
	.GetGroupPwrByVol = GetGroupPwrByVol,			//单位0.1KW
	.GetGroupEnvMaxPwr = GetGroupEnvMaxPwr,			//单位0.1KW
	.GetGroupMaxCurByVol = GetGroupMaxCurByVol,
	.GetGroupWorktime = GetGroupWorktime,			//时间1s
	.GetGroupInfo = GetGroupInfo,
	.GetGroupOnline = GetGroupOnline,
	.SetGroupChgSign = SetGroupChgSign,
};

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

SINGLE_GROUP_FUNCTINS *GetSingleGroupFun(void)
{
	return (&SingleGroupFunctions);
}

int Mdl_SetQueneSign(int grpId)
{
	if ((grpId > GROUP_MAX_NUM) || (grpId == 0))
	{
		return RESULT_ERR;
	}

	MdlQueneSign[grpId - 1] = 1;

	return 0;
}

int Mdl_GetGrpInfo(GROUP_INFO_STRUCT *pGrp, int grpId)
{
	pGrp->id = grpId;

	if (GetGroupInfo(pGrp) != RESULT_OK)
	{
		return RESULT_ERR;
	}

	return RESULT_OK;
}

int Mdl_GetMdlInfo(MDL_INFO_STRUCT *pMdl, int mdlId)
{
	if ((mdlId == 0) || (mdlId > MDL_MAX_NUM))
	{
		return RESULT_ERR;
	}

	int i = 0;
	MDL_INFO_STRUCT mdl[MDL_MAX_NUM] = {0};

	g_pMdlFun->GetMdlInfo(mdl);
	for (i = 0; i < MDL_MAX_NUM; i++)
	{
		if (mdlId == mdl[i].id)
		{
			memcpy(pMdl, &mdl[i], sizeof(MDL_INFO_STRUCT));
//			DEBUG("id = %d, fault= %d", mdlId, mdl[i].fault);
			return RESULT_OK;
		}
	}

	if (i == MDL_MAX_NUM)	// 找不到这个id
	{
		return RESULT_ERR;
	}

	return RESULT_OK;
}

void Mdl_GetMdlAllow(int type, MDL_ALLOW_STRUCT *pMdl)
{
	switch (type)
	{
		case DCM_303AA:
		case DCM_123AA:
		case UR_50060E:
		case UR_75040E:
				pMdl->pwrMax = 0;
				pMdl->volMax = 0;
				pMdl->volMinH = 0;
				pMdl->volMinL = 0;
				pMdl->curMax = 0;
				break;
		case UR_100030SW:
				pMdl->pwrMax = 300;
				pMdl->volMax = 10000;
				pMdl->volMinH = 2000;
				pMdl->volMinL = 1500;
				pMdl->curMax = 10000;
				break;
		default:
				pMdl->pwrMax = 300;
				pMdl->volMax = 10000;
				pMdl->volMinH = 2000;
				pMdl->volMinL = 1500;
				pMdl->curMax = 10000;
				break;
	}
}

void Mdl_SetPara(MDL_PARA_STRUCT para)
{
	int i = 0;

	if (g_pMdlFun)		// 暂时先不判断
	{
		g_pMdlFun->DisConnectMdl();			//需要先断开所有连接
		g_pMdlFun->Uninit();
	}

	switch (para.mdlType)
	{
		case DCM_303AA:
		case DCM_123AA:
		case UR_50060E:
		case UR_75040E:
				g_pMdlFun = NULL;
				break;
		case UR_100030SW:
				g_pMdlFun = Get_UR_100030SW_Functions();
				system("ip link set can0 down");
				usleep(100 * 1000);
				system("ip link set can0 type can bitrate 125000");
				usleep(100 * 1000);
				system("ip link set can0 up");
				usleep(100 * 1000);
				break;
		default:
				g_pMdlFun = Get_UR_100030SW_Functions();
				system("ip link set can0 down");
				usleep(100 * 1000);
				system("ip link set can0 type can bitrate 125000");
				usleep(100 * 1000);
				system("ip link set can0 up");
				usleep(100 * 1000);
				break;
	}

	if (g_pMdlFun == NULL)
	{
		return;
	}

	g_pMdlFun->Init();
	g_pMdlFun->SetMdlInfo(para.mdlCount);
	g_pMdlFun->SetACVoltageLimit(para.mdlUnderVol, para.mdlOverVol);
	g_pMdlFun->DisConnectMdl();
	for (i = 0; i < para.mdlCount; i++)
	{
		/* 配置模块SN号,已经充电的时间为0，由smartOps来累加 */
		g_pMdlFun->SetSN(i+1, para.mdlSN[i].SN, para.mdlSN[i].groupId, para.mdlSN[i].kwh);
//		g_pMdlFun->SetGroupChgSign(i+1, 1);
	}

	memcpy(&g_mdlPara, &para, sizeof(MDL_PARA_STRUCT));
}

void Mdl_GetPara(MDL_PARA_STRUCT *pMdl)
{
	memcpy(pMdl, &g_mdlPara, sizeof(MDL_PARA_STRUCT));
}

void Mdl_Init(int (*ModuleInitParaCallback)(void))
{
	int i = 0;
	for (i = 0; i < GROUP_MAX_NUM; i++)
	{
		Mdl_SetQueneSign(i+1);
	}
}

void Mdl_Uninit()
{
	if (g_pMdlFun)
	{
		g_pMdlFun->Uninit();
	}
}
