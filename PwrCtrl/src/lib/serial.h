#ifndef _SERIAL_H
#define _SERIAL_H

#include "option.h"


#define 	TTYMXC1				0	// "/dev/ttymxc1"
#define 	TTYMXC2				1	// "/dev/ttymxc2"
#define 	TTYMXC3				2	// "/dev/ttymxc3"
#define 	TTYMXC4				3	// "/dev/ttymxc4"
#define 	TTYMXC5				4	// "/dev/ttymxc5"
#define 	TTYMXC6				5	// "/dev/ttymxc6"


int serial_open(int dev);
void serial_close(int dev);

void serial_set_com(int dev, int baudrate, int databit, const char *stopbit, char parity);
//带超时读写   
int serial_recv(int dev, void *data, int len);
int serial_send(int dev, char *data, int len);
void serial_clearwriteport (int dev);
void serial_clearreadport (int dev);

int serial_pthread_open(int dev, CallBackFun pCallBack);
void serial_pthread_close(int dev);

#endif /* serial.c */

