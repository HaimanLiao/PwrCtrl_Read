#ifndef _TIMER_H
#define _TIMER_H

#include <time.h>
#include "option.h"

#pragma pack(1)

typedef struct
{
	int index;
	int intervalMS;
	CallBackFun pCallBack;
}_TIMER_FUNC;

#pragma pack()

unsigned long get_timetick();
void get_localtime(char *t);
void set_localtime(char *t);

int diff_timetick(unsigned long *cur_tick, unsigned long interval_tick);

void timer_addTimer(_TIMER_FUNC task);
void timer_killTimer(_TIMER_FUNC task);

void timer_pthread_init();
void timer_pthread_unInit();


#endif