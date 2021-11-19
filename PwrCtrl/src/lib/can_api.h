/*
 * can_api.h
 *
 *  Created on: 2018-4-18
 *      Author: root
 */

#ifndef CAN_API_H_
#define CAN_API_H_

#include "option.h"


#define		CAN0					0
#define 	CAN1					1

int can_init(const int dev);
int can_send(const int dev, struct can_frame sndcan_frame);
int can_recv(const int dev, struct can_frame *canrev_return);
void can_unInit(const int dev);

int can_pthread_init(const int dev, CallBackFun pCallBack);
int can_pthread_unInit(const int dev);

#endif /* CAN_API_H_ */
