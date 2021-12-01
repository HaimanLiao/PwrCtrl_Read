/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: 
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: 
* DEPARTMENT	: 
* DATE			: 
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "matrix.h"
#include "modCtrl.h"
#include "pwrCtrl.h"

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

//#define 	PRINT
static 		CHG_POLICY_STRUCT 		g_oldResAry[GUN_DC_MAX_NUM] = {0};
static 		CHG_POLICY_STRUCT 		g_resAry[GUN_DC_MAX_NUM] = {0};

static 		GUN_STRUCT				g_gun[GUN_DC_MAX_NUM] = {0};
static 		GROUP_STRUCT			g_group[GROUP_MAX_NUM] = {0};
static		MATRIX_INIT_STRUCT 		g_init = {0};

typedef struct
{
	int gunId;				// 枪ID
	int startAddr;			// 可以使用的模组ID，起始地址
	int endAddr;			// 可以使用的模组ID，结束地址
}ADDR_STRUCT;

static 		ADDR_STRUCT				g_matrixAddr[12] = {		//lhm: 最多支持6把枪 + 对称系数 = 2
	// gunId 	// startAddr 	// endAddr						//lhm: 该策略用在360kw双枪分体机

	/* 1-6枪180单体柜 */
	{1,			1,				12},
	{2,			1,				12},
	{3,			1,				12},
	{4,			1,				12},
	{5,			1,				12},
	{6,			1,				12},
};

static 		ADDR_STRUCT				g_matrixDoubleAddr[12] = {	//lhm: 两个MATRIX---如果枪号是7~12则从模块7~12中分；如果枪号是1~6则从模块1~6中分
	// gunId 	// startAddr 	// endAddr						//lhm: 该策略用在180kw双枪一体机（2枪 + 对称系数 = 1）
	/* 1-6枪180单体柜 */										 //lhm: 实际也是最多支持6把枪 + 对称系数 = 1（否则策略代码运行出错）
	{1,			1,				6},
	{2,			1,				6},
	{3,			1,				6},
	{4,			1,				6},
	{5,			1,				6},
	{6,			1,				6},

	/* 7-12枪180单体柜 */
	{7,			7,				12},
	{8,			7,				12},
	{9,			7,				12},
	{10,		7,				12},
	{11,		7,				12},
	{12,		7,				12},
};

static 		ADDR_STRUCT			*g_addr = NULL;

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
* MODULE	: PolicyStart
* ABSTRACT	: 
* FUNCTION	: 策略：1、有枪进来 2、某枪功率改变
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void PolicyStart(CHG_POLICY_NEED_STRUCT chg)//lhm: 针对一把枪的策略（该枪的行为是：1、新的发起充电的枪；2、正在充电的枪，但功率需求发生改变）
{
	int i, j, k;
	int flag[GUN_DC_MAX_NUM] = {0};

	GUN_STRUCT 				gunTmp[GUN_DC_MAX_NUM];
	GUN_STRUCT 				tmpbuf;
	unsigned int 			gunNum = 0;
	int 					gunid = chg.gunId - 1;
	unsigned int 			groupNum = g_addr[gunid].endAddr - g_addr[gunid].startAddr + 1;
	int 					grpid;

	/* 计算实际可用模块数量，故障模块不进行分配 */
	//lhm: 相当于把“所有正在使用的枪（guntmp[]）”当前的分配情况推倒然后重新分配
	//lhm: 每把枪的id是不一样的，根据各自的id，每把枪在g_gun[]和guntmp[]占据一个位置
	int tmpCount = 0;
	for (i = 0; i < groupNum; i++)	//lhm: 初始时GROUP_MAX_NUM（12）个模块sta全为0
	{								//lhm: 在Init的时候，如果是MATRIX2HAND，只会在1-6出现sta为1（故障模块数不会超过6，代码仍行得通）
		if (g_group[i].sta == 1)	//lhm: 1表示模块故障
		{
			tmpCount++;//lhm: 故障模块总数
		}
	}
	groupNum = groupNum - tmpCount * g_init.grpSync;

	memcpy(g_oldResAry, g_resAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);		// 备份上次的分配结果
	for (i = 0; i < g_init.gunNum; i++)
	{
		//lhm: 下面if本意是初始化和gunid处于同一个MATRIX下的枪的策略
		//lhm: 但是对应双MATRIX，当对称系数 > 1时，模块起始地址到末地址还是不对应同MATRIX下的枪id
		//lhm: 所以对于MATRIX2MATIRX策略，实际最多支持6把枪 + 对称系数 = 1（否则代码运行出错）
		if ((i >= (g_addr[gunid].startAddr-1)) && (i < g_addr[gunid].endAddr))
		{
			g_resAry[i].gunId = i + 1;
			g_resAry[i].gunRes = CHANGE_NO;
			if (i != gunid)	//lhm: 每把枪的id是不一样的，根据各自的id，每把枪在g_gun[]和guntmp[]占据一个位置
			{				//lhm: 感觉这个if没什么意义，因为for循环结束后g_resAry[gunid]的这些成员照样赋值0
				g_resAry[i].groupNum = 0;
				g_resAry[i].relayNum = 0;
				g_resAry[i].addGrpNum = 0;
				g_resAry[i].freeGrpNum = 0;
			}
		}
		else
		{
			g_resAry[i].gunRes = CHANGE_NO;
		}
	}

	g_gun[gunid].gun.pwrNeed = chg.pwrNeed;
	g_gun[gunid].gun.startTime = chg.startTime;

	g_gun[gunid].gunUsed = 1;
	g_resAry[gunid].groupNum = 0;
	g_resAry[gunid].relayNum = 0;
	g_resAry[gunid].freeGrpNum = 0;
	g_resAry[gunid].addGrpNum = 0;

	/* 重新策略分配 
	*  1、先进行排序，按照充电起始时间
	*  2、开始分配，计算分配的个数
	*  3、根据现有情况，分配实际的模组和继电器
	*/

	/* step one */
	/* 根据枪实际所在位置，把相关的枪弄进来 */
	gunNum = 0;
//	lhm: 下面这个for循环本意应该是遍历同MATRIX下的枪，但当对称系数 > 1时，模块起始地址到末地址不对应同MATRIX下的枪id
//	for (i = g_addr[gunid].startAddr-1; i < g_addr[gunid].endAddr; i++)	//lhm: 这行也不能代表同一个MATRIX下的枪id
	for (i = 0; i < g_init.gunNum; i++)									//lhm: g_gun[]固定6个元素（初始化为0，所以初始时g_gun[i].gunUsed均为0）
	{																	//lhm: g_gun[]固定6个元素，所以g_init.gunNum不能超过6（MATRIX2MATRIX那个结构数组定义7~12枪无意义）
		if (g_gun[i].gunUsed == 1)										//lhm: g_gun[i].gunId = i + 1（固定在g_gun[]的6个元素里，g_gun是本c文件的全局变量）
		{
			memcpy(&gunTmp[gunNum], &g_gun[i], sizeof(GUN_STRUCT));	//lhm: gumTmp[i].gunId不一定等于i
			gunNum++;
		}
	}

	if (!gunNum)
	{
		goto RETURN;
	}

	/* 根据充电开始时间排序，从先到后 */
	//lhm: 冒泡排序
	//lhm: 第一次外循环，把剩下的元素（0~n）最值推到[0]；第二次外循环，把剩下元素（1~n）的最值推到[1]；···
	//lhm: 经过这一轮排序后，gunTmp[]的gunId不一定等于其索引 + 1
	for (i = 0; i < gunNum-1; i++)
	{
		for (j = gunNum-2; j >= i; j--)
		{
			if (gunTmp[j].gun.startTime > gunTmp[j+1].gun.startTime)  
			{
				memcpy(&tmpbuf, &gunTmp[j], sizeof(GUN_STRUCT));
				memcpy(&gunTmp[j], &gunTmp[j+1], sizeof(GUN_STRUCT));
				memcpy(&gunTmp[j+1], &tmpbuf, sizeof(GUN_STRUCT));
			}
		}
	}
	//lhm: gunTmp[]保存正在使用的枪的policy相关参数（最大提供功率 + 枪的需求 + 枪的开始充电时间）

    /* step two */
	int grpPwrMax = g_init.groupPwr * g_init.grpSync;	// 计算模组功率//lhm: 一个模组指的是兄弟模块组（共切入共切出）
	j = 0;
	while ((j < gunNum) && (groupNum))//lhm: 经过“groupNum = groupNum - tmpCount * g_init.grpSync”后，groupNum指的是模组数
	{
		//lhm: gunTmp[]保存“正在使用的枪（新进来的枪 + 发生功率改变的枪）”的policy（最大提供功率 + 枪的需求 + 枪的开始充电时间）并按枪的开始充电时间从先到后排序
		//lhm: （重新分配）只决定分配到多少个模块，而具体是连接到那几个模块则不管
		for (i = 0; i < gunNum; i++)
		{
			gunid = gunTmp[i].gunId - 1;
			if (gunTmp[i].gun.pwrNeed >= grpPwrMax)		// 枪功率大于一个模组功率
    		{
    			gunTmp[i].gun.pwrNeed -= grpPwrMax;
    			g_resAry[gunid].groupNum += g_init.grpSync;//lhm: 对称系数是grpSync个模块一起分配的意思（兄弟模块，共切入共切出）
    			groupNum -= g_init.grpSync;
    		}
    		else if ((gunTmp[i].gun.pwrNeed < grpPwrMax) 
    					&& (g_resAry[gunid].groupNum == 0))	// 枪功率小于一个模组功率，但是还未分配到一个
    		{
    			gunTmp[i].gun.pwrNeed = 0;
    			g_resAry[gunid].groupNum += g_init.grpSync;
    			groupNum -= g_init.grpSync;
    		}
    		else if (gunTmp[i].gun.pwrNeed < grpPwrMax)		// 枪剩余功率小于一个模组功率，不过已经分配到了//lhm: 每把枪（已经分到过至少一个模组）剩余功率的分配留到下一轮
    		{
    			if (flag[i] == 0)
    			{
    				j++;
    				flag[i] = 1;//lhm: 用flag变量表示对应的枪已经分到了模块
    			}
    		}

    		if (groupNum == 0)
    		{
    			break;
    		}
    	}
    }

	/* 这是最后一轮分配，哪把枪剩的最大，就先给谁 */
    while (groupNum)
    {
    	for (i = 0; i < gunNum; i++)
    	{
    		/* 先进行一轮排序，找到需求最大的那个 */
    		k = 0;
    		for (j = 0; j < gunNum; j++)
    		{
    			if ((gunTmp[j].gun.pwrNeed < grpPwrMax) 
    					&& (gunTmp[j].gun.pwrNeed != 0))
	    		{
	    			if (gunTmp[k].gun.pwrNeed < gunTmp[j].gun.pwrNeed)
	    			{
	    				k = j;
	    			}
	    		}
    		}
    		
    		if (gunTmp[k].gun.pwrNeed != 0)
    		{
    			gunTmp[k].gun.pwrNeed = 0;
    			gunid = gunTmp[k].gunId-1;
	    		g_resAry[gunid].groupNum += g_init.grpSync;
    			groupNum -= g_init.grpSync;
    		}

    		if (!groupNum)
    		{
    			break;
    		}
    	}
    	break;
    }

    /* step three */
    for (i = 0; i < g_init.gunNum; i++)
    {
    	if (g_oldResAry[i].groupNum > g_resAry[i].groupNum)		// free group
    	{
    		for (j = 0; j < g_oldResAry[i].groupNum; j++)
    		{
    			if (j >= g_resAry[i].groupNum)
    			{
    				g_group[g_oldResAry[i].groupId[j]-1].groupUsed = 0;
    				g_resAry[i].freeGrpId[g_resAry[i].freeGrpNum] = g_oldResAry[i].groupId[j];
    				g_resAry[i].freeGrpNum++;
    			}
    			else
    			{
    				g_resAry[i].groupId[j] = g_oldResAry[i].groupId[j];
    			}
    		}
    	}
    }

    for (i = 0; i < g_init.gunNum; i++)							//lhm: g_init.gunNum包含了所有的枪号而不仅是正在使用的枪号
    {
    	if (g_oldResAry[i].groupNum < g_resAry[i].groupNum) 	// add group//lhm: 如果是未使用的枪号，那么这两者是相等的
    	{
    		int pos = g_oldResAry[i].groupNum;					//lhm: 这个pos作为g_resAry[i].groupId[pos]参数（新模块入组，这里groupId的group指的是模块）
    		for (j = 0; j < g_init.groupNum; j++)
    		{
    			/* 根据实际情况，进行模组的分配，各用各的，还是可以互拉 */
    			if ((j < (g_addr[i].startAddr-1)) || (j > (g_addr[i].endAddr-1)))//lhm: 是该枪连接的group（模块）才参与分配（先free再add）
    			{
    				continue;
    			}

    			if ((g_group[j].groupUsed == 0) && (g_group[j].sta == 0))	// 该组模块没有使用且状态正常//lhm: “没有使用”即没有分配给任何一把枪（闲置）
    			{
    				gunid = g_resAry[i].gunId;
    
					for (k = 0; k < g_init.grpSync; k++)					//lhm: 把该组的所有模块添加（枪i需要分配的模块）
					{
						grpid = g_group[j].groupId + 6*k;					//lhm: g_group[]应该是模组数组，所以g_group[j].groupId应该是定值
						g_resAry[i].groupId[pos] = grpid;
						pos++;
						g_group[grpid-1].groupUsed = 1;

						g_resAry[i].addGrpId[g_resAry[i].addGrpNum] = grpid;
						g_resAry[i].addGrpNum++;
					}
    			
    				if (pos == g_resAry[i].groupNum)						//lhm: 把得到的模组数（groupNum）具体分配完就退出
    				{
    					break;
    				}
    			}
    		}
    	}
    }
//lhm: 经过上面的步骤，每把枪具体分配得到那几个模块均可知---g_resAry[i].groupId[]
    for (i = 0; i < g_init.gunNum; i++)		// 对继电器进行赋值//lhm: 继电器id应该是要对应模块id（PDU的6个继电器对应6个模块）
	{
		//lhm: 外循环（依次对每把枪进行操作）
		g_resAry[i].relayNum = g_init.grpSync;//lhm: relayNum是需要操作的PDU数量
		memset(g_resAry[i].relaySW, 0, 6*PDU_MAX_NUM);			//lhm: 这里是把枪连接的所有继电器都断开（后面再根据分配到的模块合上对应的继电器）
		for (k = 0; k < g_init.grpSync; k++)					//lhm: g_resAry[i].gunId = i + 1;//lhm: k表示第k组（每6个模块为1组），每一组根据对称系数不同会连接多个PDU
		{
			//lhm: 内循环（依次对枪的每个PDU进行操作）
			g_resAry[i].relayId[k] = g_resAry[i].gunId + 6*k;	//lhm: PDU编号1，7，13，···；2，8，14，···，；···（枪id及其对应的PDU编号应该在桩搭好之后确定了，如果要改枪id，那么需重新接线）

			for (j = 0; j < g_resAry[i].groupNum; j++)
			{
				if (((g_resAry[i].groupId[j]-1)/6) == k)//lhm: 从这里也可以看出MATRIX2MATRIX最多也是支持6把枪（虽然它本意是支持12把枪）
				//lhm: relaySW是“6个继电器”的数组：在组装PDU控制can帧时，PDU地址从relayId[k]中获取，对应的6个继电器通断情况（can的数据段）从relaySW[k][]中获取
				{
					g_resAry[i].relaySW[k][g_resAry[i].groupId[j] - 6*k - 1] = 1;//lhm: 1应该是闭合的意思；“g_resAry[i].groupId[j] - 6*k - 1”是把模块Id映射到0~5（对应连接的6个继电器）
				}
			}
		}

		g_resAry[i].pwrMax = g_resAry[i].groupNum * g_init.groupPwr;//lhm: pwrMax就是最终提供给该枪的功率
	}

RETURN:
	for (i = 0; i < g_init.gunNum; i++)
	{
		if ((g_resAry[i].freeGrpNum == 0) && (g_resAry[i].addGrpNum == 0))
		{
			g_resAry[i].gunRes = CHANGE_NO;
		}
		else if ((g_resAry[i].freeGrpNum != 0) && (g_resAry[i].addGrpNum == 0))
		{
			g_resAry[i].gunRes = CHANGE_OUT;//lhm: 模块切出去
		}
		else  if ((g_resAry[i].freeGrpNum == 0) && (g_resAry[i].addGrpNum != 0))
		{
			g_resAry[i].gunRes = CHANGE_IN;//lhm: 模块切进来
		}
	}
	return;
}

/*
* *******************************************************************************
* MODULE	: PolicyStop
* ABSTRACT	: 
* FUNCTION	: 策略：枪停止,释放掉该枪占用的模组
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void PolicyStop(CHG_POLICY_NEED_STRUCT chg)
{
	int gunid = chg.gunId - 1;
	int i = 0;

	memcpy(g_oldResAry, g_resAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);
	
	g_gun[gunid].gunUsed = 0;

	for (i = 0; i < g_init.gunNum; i++)
	{
		g_resAry[i].gunRes = CHANGE_NO;//lhm: 其他枪不变（g_resAry[gunid].gunRes = CHANGE_OUT）
		g_resAry[i].addGrpNum = 0;
		g_resAry[i].freeGrpNum = 0;
	}

	for (i = 0; i < g_resAry[gunid].groupNum; i++)
	{
		g_group[g_resAry[gunid].groupId[i]-1].groupUsed = 0;
	}

	g_resAry[gunid].gunRes = CHANGE_OUT;
	g_resAry[gunid].pwrMax = 0;
	g_resAry[gunid].groupNum = 0;
	g_resAry[gunid].relayNum = g_oldResAry[gunid].relayNum;
	memcpy(g_resAry[gunid].relayId, g_oldResAry[gunid].relayId, sizeof(unsigned int)*PDU_MAX_NUM);
	g_resAry[gunid].addGrpNum = 0;
	g_resAry[gunid].freeGrpNum = g_oldResAry[gunid].groupNum;
	memcpy(g_resAry[gunid].freeGrpId, g_oldResAry[gunid].groupId, sizeof(unsigned int)*GROUP_MAX_NUM);
	g_gun[gunid].gun.pwrNeed = 0;
}

/*
* *******************************************************************************
* MODULE	: HandStart
* ABSTRACT	: 
* FUNCTION	:
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
//lhm: 手拉手方式可分四种情况，带两把枪，每一把枪的groupId[GROUP_MAX_NUM]要么用一半，要么全部用
//lhm: 即在add / free groupId的时候要么add / free一半，要么add / free所有（不会add / free某半边的其中若干个）
//lhm: 两把枪，一开始默认每把各占一半（“startAddr = 0 & endAddr = groupNum /2” + “startAddr = 0 & endAddr = groupNum”）
//lhm: （gunId + 1) % 2是另一把枪的意思，如果gunid = 1(3, 5, 7, ...)，则另一把枪的gunid = 0（反之则反过来）
static void HandStart(CHG_POLICY_NEED_STRUCT chg)
{
	int i, j, k;
	int 					gunid = chg.gunId - 1;
	unsigned int 			groupNum = g_init.groupNum;
	int 					startAddr, endAddr;

	memcpy(g_oldResAry, g_resAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);		// 备份上次的分配结果
	for (i = 0; i < g_init.gunNum; i++)//lhm: 所有枪的策略置零重整（g_init.gunNum不能超过2）
	{
		g_resAry[i].gunId = i + 1;
		g_resAry[i].gunRes = CHANGE_NO;
		g_resAry[i].relayNum = 0;
		g_resAry[i].addGrpNum = 0;
		g_resAry[i].freeGrpNum = 0;
	}
	//lhm: 该策略只支持2把枪
	//lhm: 两把枪根据情况使用自己半边的模块或者使用所有的模块，由中间继电器控制
	if (gunid == 0)//lhm: gunid = 0占半边模块
	{
		startAddr = 0;
		endAddr = groupNum / 2;
	}
	else//lhm: gunid = 1 (3, 5, 7, ...)占另外半边模块
	{
		startAddr = groupNum / 2;
		endAddr = groupNum;
	}

	if ((g_resAry[gunid].groupNum == 0) && (g_resAry[(gunid+1)%2].groupNum == 0))//lhm: 另一把枪空闲，gunId发起充电请求（gunId之前也空闲）
	{
		if (chg.pwrNeed > groupNum/2 * g_init.groupPwr)	// 一定会用到另外半边模块
		{
			startAddr = 0;
			endAddr = groupNum;

			g_resAry[gunid].relayNum = 1;
			g_resAry[gunid].relayId[0] = 1;
			g_resAry[gunid].relaySW[0][0] = 1;//lhm: 只有一个继电器
		}

		j = 0;
		for (i = startAddr; i < endAddr; i++)
		{
			if (g_group[i].sta == 0)
			{
				g_resAry[gunid].groupNum++;
				g_resAry[gunid].groupId[j] = i+1;
				g_resAry[gunid].addGrpNum++;
				g_resAry[gunid].addGrpId[j] = i+1;
				j++;
			}
		}

		g_resAry[gunid].pwrMax = g_resAry[gunid].groupNum * g_init.groupPwr;
	}
	else
	{
		if ((g_resAry[gunid].groupNum > 0) && (g_resAry[(gunid+1)%2].groupNum == 0))//lhm: 另一把枪空闲，gunId充电需求发生改变
		{
			if (chg.pwrNeed > groupNum/2 * g_init.groupPwr)	// 一定会用到另外半边模块
			{
				startAddr = 0;
				endAddr = groupNum;//lhm: 一定会用到另外半边模块

				g_resAry[gunid].relayNum = 1;
				g_resAry[gunid].relayId[0] = 1;
				g_resAry[gunid].relaySW[0][0] = 1;
			}

			j = g_resAry[gunid].groupNum;
			for (i = startAddr; i < endAddr; i++)	//lhm: 轮询模块，检查其是否是原来的（groupNum），没有则添加进去
													//lhm: 应该不用这么麻烦，可以通过判断endAddr大小来决定是否添加另外半边的模块Id
			{
				if (g_group[i].sta == 0)//lhm: 0代表正常，1代表失败
				{
					for (k = 0; k < g_resAry[gunid].groupNum; k++)
					{
						if ((i+1) == g_resAry[gunid].groupId[k])	// 说明已经存在
						{
							break;
						}
					}

					DEBUG("k = %d, groupNum = %d, i = %d", k, g_resAry[gunid].groupNum, i);
					if (k == g_resAry[gunid].groupNum)		// 不存在，需要添加进去
					{
						g_resAry[gunid].groupNum++;
						g_resAry[gunid].groupId[j] = i+1;
						k = g_resAry[gunid].addGrpNum++;
						g_resAry[gunid].addGrpId[k] = i+1;
						j++;
					}
				}
			}

			g_resAry[gunid].pwrMax = g_resAry[gunid].groupNum * g_init.groupPwr;
		}
		else
		{
			if (g_resAry[(gunid+1)%2].pwrMax <= groupNum/2 * g_init.groupPwr)	// 说明中间继电器未闭合
			//lhm: 另一把枪不空闲，但只用到自己那半边的模块
			{
				j = 0;
				for (i = startAddr; i < endAddr; i++)//lhm: gunId只能用自己半边的模块
				{
					if (g_group[i].sta == 0)
					{
						g_resAry[gunid].groupNum++;
						g_resAry[gunid].groupId[j] = i+1;
						g_resAry[gunid].addGrpNum++;
						g_resAry[gunid].addGrpId[j] = i+1;
						j++;
					}
				}

				g_resAry[gunid].pwrMax = g_resAry[gunid].groupNum * g_init.groupPwr;
			}
			else//lhm: 另一把枪不空闲，且用了所有的模块
			{
				j = 0;
				k = 0;
				for (i = 0; i < groupNum; i++)
				{
					if (g_group[i].sta == 0)
					{
						if ((i >= startAddr) && (i < endAddr))//lhm: 把“被另外一把枪占用的，属于自己”的半边拿回来
						{
							g_resAry[gunid].groupNum++;
							g_resAry[gunid].groupId[j] = i+1;
							g_resAry[gunid].addGrpNum++;
							g_resAry[gunid].addGrpId[j] = i+1;
							g_resAry[(gunid+1)%2].groupNum--;
							g_resAry[(gunid+1)%2].freeGrpNum++;
							g_resAry[(gunid+1)%2].freeGrpId[j] = i+1;
							j++;
						}
						else
						{
							g_resAry[(gunid+1)%2].groupId[k] = i+1;//lhm: “属于另一把枪”的半边保持不变
							k++;
						}
					}
				}

				g_resAry[gunid].pwrMax = g_resAry[gunid].groupNum * g_init.groupPwr;//lhm: 自己边的最大功率
				
				gunid= (gunid+1) % 2;
				g_resAry[gunid].pwrMax = g_resAry[gunid].groupNum * g_init.groupPwr;//lhm: 另外半边的最大功率
				g_resAry[gunid].relayNum = 1;
				g_resAry[gunid].relayId[0] = 1;
				g_resAry[gunid].relaySW[0][0] = 0;//lhm: 中间继电器断开
			}
		}
	}

	for (i = 0; i < g_init.gunNum; i++)
	{
		if ((g_resAry[i].freeGrpNum == 0) && (g_resAry[i].addGrpNum == 0))
		{
			g_resAry[i].gunRes = CHANGE_NO;
		}
		else if ((g_resAry[i].freeGrpNum != 0) && (g_resAry[i].addGrpNum == 0))
		{
			g_resAry[i].gunRes = CHANGE_OUT;
		}
		else  if ((g_resAry[i].freeGrpNum == 0) && (g_resAry[i].addGrpNum != 0))
		{
			g_resAry[i].gunRes = CHANGE_IN;
		}
	}

	return;
}


/*
* *******************************************************************************
* MODULE	: HandStop
* ABSTRACT	: 
* FUNCTION	: 策略：枪停止,释放掉该枪占用的模组
* ARGUMENT	: 
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void HandStop(CHG_POLICY_NEED_STRUCT chg)
{
	int gunid = chg.gunId - 1;
	int i = 0;

	memcpy(g_oldResAry, g_resAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);
	
	g_gun[gunid].gunUsed = 0;

	for (i = 0; i < g_init.gunNum; i++)
	{
		g_resAry[i].gunRes = CHANGE_NO;
		g_resAry[i].addGrpNum = 0;
		g_resAry[i].freeGrpNum = 0;
	}

	for (i = 0; i < g_resAry[gunid].groupNum; i++)
	{
		g_group[g_resAry[gunid].groupId[i]-1].groupUsed = 0;
	}

	g_resAry[gunid].gunRes = CHANGE_OUT;
	g_resAry[gunid].pwrMax = 0;
	g_resAry[gunid].groupNum = 0;
	g_resAry[gunid].relayNum = g_oldResAry[gunid].relayNum;
	memcpy(g_resAry[gunid].relayId, g_oldResAry[gunid].relayId, sizeof(unsigned int)*PDU_MAX_NUM);
	g_resAry[gunid].addGrpNum = 0;
	g_resAry[gunid].freeGrpNum = g_oldResAry[gunid].groupNum;
	memcpy(g_resAry[gunid].freeGrpId, g_oldResAry[gunid].groupId, sizeof(unsigned int)*GROUP_MAX_NUM);
	g_gun[gunid].gun.pwrNeed = 0;
}


/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: Matrix_Policy
* ABSTRACT	: 矩阵式分配策略算法入口
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
CHG_POLICY_RES_STRUCT Matrix_Policy(CHG_POLICY_NEED_STRUCT chg)
{
	CHG_POLICY_RES_STRUCT rt;

	if ((chg.gunId > g_init.gunNum) && (chg.gunId == 0))//lhm: 枪id的合法性检查
	{
		rt.result = 0;//lhm: POLICY_OK
		return rt;
	}

	int grpPwrMax = g_init.groupPwr * g_init.grpSync;	// 计算模组功率
	int addPwr = grpPwrMax - 150;

	printf("grpPwrMax = %d, chg.pwrNeed = %d\n", grpPwrMax, chg.pwrNeed);
	if (chg.pwrNeed == 0)
	{
		rt.result = POLICY_OK;

		if (g_init.policyType == HAND2HAND)
		{

			HandStop(chg);
		}
		else
		{
			PolicyStop(chg);
		}
		
	}
	/* pwr变动不大，维持现状 */
	else if ((((chg.pwrNeed + addPwr) / grpPwrMax)
			== (PwrCtrl_GetEVSEPwrMax(chg.gunId - 1) / grpPwrMax))
			&& (g_gun[chg.gunId-1].gun.pwrNeed != 0))
	{
		printf("pwrNeed = %d\n", g_gun[chg.gunId-1].gun.pwrNeed);
		rt.result = POLICY_NOCHANGE;
	}
	else
	{
		rt.result = POLICY_OK;
		if (g_init.policyType == HAND2HAND)
		{
			HandStart(chg);
		}
		else
		{
			PolicyStart(chg);
		}

		/* 算法成功了，但是没有分到1个模块 */
		//lhm: 可能原因是模块故障（否则按照现行策略，“6把枪 + 6个以上模块”以及“12把枪 + 12个以上模块”可以保证每把枪至少得到一个模块）
		if (g_resAry[chg.gunId-1].groupNum == 0)
		{
			rt.result = POLICY_ERR;
		}
	}

	memcpy(rt.oldResAry, g_oldResAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);
	memcpy(rt.resAry, g_resAry, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);

	return rt;
}

/*
* *******************************************************************************
* MODULE	: Matrix_Init
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
void Matrix_Init(MATRIX_INIT_STRUCT init)
{
	g_init.gunNum = init.gunNum > GUN_DC_MAX_NUM ? GUN_DC_MAX_NUM : init.gunNum;
	g_init.groupNum = init.groupNum > GROUP_MAX_NUM ? GROUP_MAX_NUM : init.groupNum;
	g_init.relayNum = init.relayNum > PDU_MAX_NUM ? PDU_MAX_NUM : init.relayNum;
	g_init.groupPwr = init.groupPwr;
	g_init.grpSync = init.grpSync;
	g_init.policyType = init.policyType;

	if (init.policyType == MATRIX2HAND)
	{
		g_addr = g_matrixAddr;
	}
	else if (init.policyType == MATRIX2MATRIX)
	{
		g_addr = g_matrixDoubleAddr;
	}

	memset(g_oldResAry, 0, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);
	memset(g_resAry, 0, sizeof(CHG_POLICY_STRUCT)*GUN_DC_MAX_NUM);

	int i;
	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		g_gun[i].gunId = i + 1;
		g_gun[i].gunUsed = 0;
		g_gun[i].gun.gunId = i + 1;
		g_gun[i].gun.pwrNeed = 0;
	}

	for (i = 0; i < GROUP_MAX_NUM; i++)
	{
		g_group[i].groupId = i + 1;
		g_group[i].groupUsed = 0;
		g_group[i].sta = 0;
	}

	for (i = 0; i < GUN_DC_MAX_NUM; i++)
	{
		g_resAry[i].gunId = i + 1;
		g_resAry[i].gunRes = 0;
	}
}

/*
* *******************************************************************************
* MODULE	: Matrix_SetGroupSta
* ABSTRACT	: 设置模块状态，假如模块异常，则不能再参与分配，直到恢复正常
* FUNCTION	: 
* ARGUMENT	: index:组号; sta:0-正常 1-异常
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int Matrix_SetGroupSta(int grpId, int sta)//lhm: 提供操作变量g_group的接口（用法应该是调用modCtrl接口得到模块情况，然后设置g_group[i].sta参数）
{
	if ((grpId <= 0) || (grpId > GROUP_MAX_NUM))
	{
		return RESULT_ERR;
	}

	if (g_init.policyType == MATRIX2HAND)//lhm: 模组中的某个模块不能用会导致整个模组不能用
	{
		grpId = (grpId-1) % 6;//lhm: 只需用到g_group的前6个模块
		g_group[grpId].sta = sta;//lhm: grpId是模块id（各个功率模块都有一个编号），g_group是模组数组
	}
	else
	{
		grpId = grpId - 1;//lhm: 从这里可以看出，当策略模式是MATRIX2MATRIX时，对称系数只能是1（12个模块）
		g_group[grpId].sta = sta;
	}

	return RESULT_OK;
}

void MatrixPrint(CHG_POLICY_RES_STRUCT res, char *string)
{
	char tempStr[1024] = {0};

	if (res.result == POLICY_OK)
	{
		strcat(string, "POLICY_OK\n\r");
	}
	else if (res.result == POLICY_NOCHANGE)
	{
		strcat(string, "POLICY_NOCHANGE\n\r");
	}
	else if (res.result == POLICY_ERR)
	{
		strcat(string, "POLICY_ERR\n\r");
	}

	int i, j;
	int gunId, pwrMax, grpNum, rlyNum, freeGrpNum, addGrpNum;
	int grpId, rlyId, gunRes;
	unsigned char sw[2][6] = {0};

	for (i = 0; i < g_init.gunNum; i++)
	{
		gunId = res.resAry[i].gunId;
		pwrMax = res.resAry[i].pwrMax;
		grpNum = res.resAry[i].groupNum;
		rlyNum = res.resAry[i].relayNum;
		freeGrpNum = res.resAry[i].freeGrpNum;
		addGrpNum = res.resAry[i].addGrpNum;
		gunRes = res.resAry[i].gunRes;

		sprintf(tempStr, "gunId=%d, pwrMax=%d, gunRes=%d, grpId : ", gunId, pwrMax, gunRes);
		strcat(string, tempStr);
		for (j = 0; j < grpNum; j++)
		{
			grpId = res.resAry[i].groupId[j];
			sprintf(tempStr, " %d", grpId);
			strcat(string, tempStr);
		}

		sprintf(tempStr, ", rlyId : ");
		strcat(string, tempStr);
		for (j = 0; j < rlyNum; j++)
		{
			rlyId = res.resAry[i].relayId[j];
			memcpy(sw, res.resAry[i].relaySW, 6*2);
			sprintf(tempStr, " %d=%d%d%d%d%d%d", rlyId, sw[j][0], sw[j][1], sw[j][2], sw[j][3], sw[j][4], sw[j][5]);
			strcat(string, tempStr);
		}

		sprintf(tempStr, ", freeGrpId :");
		strcat(string, tempStr);
		for (j = 0; j < freeGrpNum; j++)
		{
			grpId = res.resAry[i].freeGrpId[j];
			sprintf(tempStr, " %d", grpId);
			strcat(string, tempStr);
		}

		sprintf(tempStr, ", addGrpId :");
		strcat(string, tempStr);
		for (j = 0; j < addGrpNum; j++)
		{
			grpId = res.resAry[i].addGrpId[j];
			sprintf(tempStr, " %d", grpId);
			strcat(string, tempStr);
		}
		strcat(string, "\n\r");
	}
}
