/*
	模组控制层
	模块控制分为3层：模组控制层、组内模块控制层、单个模块控制驱动

	模组控制层 只调用 组内模块控制层 中的函数（除了初始化以外）

	组内模块控制层 只调用 单个模块控制驱动 中的函数（除了初始化以外）
*/
#ifndef __GROUPSCTRL_H__
#define __GROUPSCTRL_H__

#include "modCtrl.h"

#pragma pack(1)

/*
	组内控制提供的函数
*/
typedef struct
{
	int				(*SetGroupUI)(int groupId, int vol, int cur);			// lhm: 设置模组电压和电流
	int				(*SetGroupMode)(int groupId, int mode);					// 设置模块高低压模式
	int				(*GetGroupUI)(int groupId, int *vol, int *cur);
	int				(*GetGroupPwrByVol)(int groupId, int vol);				// 获取在当前电压下，模块能够输出的功率，可能还要考虑温度，
	int				(*GetGroupEnvMaxPwr)(int groupId);						// 获取当前环境下，模块能够输出的最大功率(用于通知车端功率变化)
	int				(*GetGroupMaxCurByVol)(int groupId, int vol);			// 获取当前电压下，模块能够输出的最大电流
	unsigned long	(*GetGroupWorktime)(int groupId);						// 获取模组的平均工作时间
	int 			(*GetGroupInfo)(GROUP_INFO_STRUCT *group);				// 获取该组模块信息
	int 			(*GetGroupOnline)(int groupId);							// 获得该组模块是否全部在线
	void			(*SetGroupChgSign)(int sign);							// sign 1-使能 0-未使能
} SINGLE_GROUP_FUNCTINS;

#pragma pack()

SINGLE_GROUP_FUNCTINS *GetSingleGroupFun(void);

#endif
