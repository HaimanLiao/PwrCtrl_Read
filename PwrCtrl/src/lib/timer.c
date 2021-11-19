#include "timer.h"

#define TIMER_FUNC_MAX	8

typedef struct
{
	pthread_t pid;
	int tn;
	int isEnable;

	_TIMER_FUNC fun;
}_HANDLE_FUNC;

pthread_t g_pid;
static _HANDLE_FUNC g_func[TIMER_FUNC_MAX];

unsigned long get_timetick()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec/1000000);
}

void get_localtime(char *t)
{
	time_t tm;
    struct tm *pTM;
    
    time(&tm);
    pTM = localtime(&tm);

    /* 系统日期和时间,格式: yyyymmddHHMMSS */
    sprintf(t, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
}

void set_localtime(char *t)
{
	char timeBuf[64] = {0};
	sprintf(timeBuf, "date -s \"%s\"", t);
	system(timeBuf);
}

int CheckExeConditionEx(int curTickCount,int levelTickCount,int interval)
{
	int intervalMS = interval;

	if(curTickCount<0xFFFFFF &&levelTickCount>0xFFFFFFF)
	{
		//					CUR
		//|----------------------|
		//  LEVEL

		if((curTickCount + (0xFFFFFFFF - levelTickCount)) >=intervalMS)
		{
			return RESULT_OK;
		}
	}
	else if(levelTickCount<0xFFFFFF && curTickCount>0xFFFFFFF)
	{

		//					LEVEL
		//|----------------------|
		//  CUR

		if((levelTickCount + (0xFFFFFFFF - curTickCount)) >=intervalMS)
		{
			return RESULT_OK;
		}
	}
	else
	{
		if(levelTickCount>=curTickCount)
		{
			return RESULT_ERR;
		}
		if((curTickCount-levelTickCount)>=intervalMS)
		{
			return RESULT_OK;
		}

	}
	return RESULT_ERR;
}

int diff_timetick(unsigned long *cur_tick, unsigned long interval_tick)
{
	unsigned long tick = get_timetick();
	if (abs(tick-(*cur_tick)) > interval_tick)
	{
		(*cur_tick) = tick;
		return RESULT_OK;
	}
	else
	{
		return RESULT_ERR;
	}
}

void timer_addTimer(_TIMER_FUNC task)
{
	if ((task.index>=0)&&(task.index<TIMER_FUNC_MAX))
	{
		g_func[task.index].fun.intervalMS = task.intervalMS;
		g_func[task.index].fun.pCallBack = task.pCallBack;
		g_func[task.index].tn = 0;
		g_func[task.index].isEnable = 1;
	}
}

void timer_killTimer(_TIMER_FUNC task)
{
	if ((task.index>=0)&&(task.index<TIMER_FUNC_MAX))
	{
		g_func[task.index].isEnable = 0;
	}
}

void *tiemr_deal(void *p)
{
	int index = 0;
	while (1)
	{
		for (index=0; index < TIMER_FUNC_MAX; index++)
		{
			if (g_func[index].isEnable)
			{
				if (CheckExeConditionEx(get_timetick(), g_func[index].tn, g_func[index].fun.intervalMS))
				{
					g_func[index].tn = get_timetick();

					g_func[index].fun.pCallBack(NULL, 0);
				}
			}
		}

		usleep(1000);
	}
}

void timer_pthread_init()
{
	int index = 0;
	for(; index < TIMER_FUNC_MAX; index++)
	{
		g_func[index].isEnable = 0;
	}
	pthread_create(&g_pid, 0, tiemr_deal, NULL);
}

void timer_pthread_unInit()
{
	int index = 0;
	for(; index<TIMER_FUNC_MAX; index++)
	{
		g_func[index].isEnable = 0;
	}
	pthread_cancel(g_pid);
	pthread_join(g_pid, NULL);
}