/*
 * Program:     serial.c
 * Author:      wujj
 * Version:      
 * Date:        20170630
 * Description: 串口应用层驱动
 *               
*/

#include <termios.h>            /* tcgetattr, tcsetattr */
#include <stdio.h>              /* perror, printf, puts, fprintf, fputs */
#include <unistd.h>             /* read, write, close */
#include <fcntl.h>              /* open */
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>             /* bzero, memcpy */
#include <limits.h>             /* CHAR_MAX */
#include "serial.h"
#include "msg.h"

#define SERIAL_MAX_CLIENT      8
#define MSG_READ_LEN           MAX_BUFFER_LEN

typedef struct
{
    pthread_t   pid;
    int         serialFd;
    int         msgFd;
    CallBackFun pCallBack;
}CAN_PRI_STRUCT;

static CAN_PRI_STRUCT g_serialPri[SERIAL_MAX_CLIENT] = {0};
static char g_devName[SERIAL_MAX_CLIENT][16] = {
    {"/dev/ttymxc1"}, {"/dev/ttymxc2"}, {"/dev/ttymxc3"},
    {"/dev/ttymxc4"}, {"/dev/ttymxc5"}, {"/dev/ttymxc6"},
};


void *serial_deal(void *p);

/**
 * 根据波特率的值获取波特率标志  115200 -> B115200
 */
static int baudrate2Bxx (int baudrate)
{
    switch (baudrate) {
    case 0:
        return (B0);
    case 50:
        return (B50);
    case 75:
        return (B75);
    case 110:
        return (B110);
    case 134:
        return (B134);
    case 150:
        return (B150);
    case 200:
        return (B200);
    case 300:
        return (B300);
    case 600:
        return (B600);
    case 1200:
        return (B1200);
    case 2400:
        return (B2400);
    case 9600:
        return (B9600);
    case 19200:
        return (B19200);
    case 38400:
        return (B38400);
    case 57600:
        return (B57600);
    case 115200:
        return (B115200);
    default:
        return (B9600);
    }
}

/**
 * 获取波特率的值  115200
 */
static void set_baudrate(struct termios *ptr, int baudrate)
{
    ptr->c_cflag = baudrate2Bxx (baudrate);  /* set baudrate */
}

/**
 * 设置数据位
 */
static void set_data_bit (struct termios *ptr ,int databit)
{
    ptr->c_cflag &= ~CSIZE;
    switch (databit) {
    case 8:
        ptr->c_cflag |= CS8;
        break;
    case 7:
        ptr->c_cflag |= CS7;
        break;
    case 6:
        ptr->c_cflag |= CS6;
        break;
    case 5:
        ptr->c_cflag |= CS5;
        break;
    default:
        ptr->c_cflag |= CS8;
        break;
    }
}

/**
 * 设置停止位
 */
static void set_stopbit (struct termios *ptr ,const char *stopbit)
{
    if (0 == strcmp (stopbit, "1")) {
        ptr->c_cflag &= ~CSTOPB; /* 1 stop bit */
    }
    else if (0 == strcmp (stopbit, "1.5")) {
        ptr->c_cflag &= ~CSTOPB; /* 1.5 stop bits */
    }
    else if (0 == strcmp (stopbit, "2")) {
        ptr->c_cflag |= CSTOPB;  /* 2 stop bits */
    }
    else {
        ptr->c_cflag &= ~CSTOPB; /* 1 stop bit */
    }
}

/**
 * 设置奇偶校验
 */
static void set_parity (struct termios *ptr ,char parity)
{
    switch (parity) {
    case 'N':                  /* no parity check */
        ptr->c_cflag &= ~PARENB;
        break;
    case 'E':                  /* even */
        ptr->c_cflag |= PARENB;
        ptr->c_cflag &= ~PARODD;
        break;
    case 'O':                  /* odd */
        ptr->c_cflag |= PARENB;
        ptr->c_cflag |= ~PARODD;
        break;
    default:                   /* no parity check */
        ptr->c_cflag &= ~PARENB;
        break;
    }
}

/*
*功能：打开串口，获取描述符
*参数：设备文件路径
*返回：
*-1：打开设备失败；fd：设备文件描述符
*/
int serial_open(int dev)
{
    int ret = RESULT_OK;
	int fd = open(g_devName[dev], O_RDWR | O_NOCTTY);    //阻塞模式    
	if (fd < 3)
	{  
		ret = RESULT_ERR;
	}
    else
    {
        g_serialPri[dev].serialFd = fd;
    }

    return ret;	
}

void serial_close(int dev)
{
    if (g_serialPri[dev].serialFd > 2)
    {
        close(g_serialPri[dev].serialFd);
    }
}

int serial_pthread_open(int dev, CallBackFun pCallBack)
{
    int ret = RESULT_OK;

    ret = serial_open(dev);

    pthread_create(&g_serialPri[dev].pid, 0, serial_deal, (void *)&dev);

    g_serialPri[dev].pCallBack = pCallBack;
    g_serialPri[dev].msgFd = msg_init(MSG_TYPE_RING);

    usleep(100*1000);   // 等待创建线程传递参数完成
    return ret;

}

void serial_pthread_close(int dev)
{
    /* 等待线程退出 */
    pthread_cancel(g_serialPri[dev].pid);
    pthread_join(g_serialPri[dev].pid, NULL);

    /* 清空链表 */
    msg_unInit(g_serialPri[dev].msgFd);

    serial_close(dev);
}

/*
*功能：串口设置
*
*参数：
*fd:设备文件描述符
*baudrate: 1200 2400 4800 9600 .. 115200
*databit:  5, 6, 7, 8
*stopbit:  "1", "1.5", "2"
*parity:   N(o), O(dd), E(ven)
*
*无返回
*/
void serial_set_com(int dev, int baudrate, int databit, const char *stopbit, char parity)           
{
	struct termios options;
    int fd = g_serialPri[dev].serialFd;

    bzero(&options, sizeof (options));
	cfmakeraw (&options);
	
    options.c_cflag |= CLOCAL | CREAD;     
	set_baudrate(&options,baudrate);
	set_data_bit(&options,databit);
    set_parity(&options,parity);
    set_stopbit(&options,stopbit);
    options.c_oflag = 0;
    options.c_lflag |= 0;
    options.c_oflag &= ~OPOST;
    options.c_cc[VTIME] = 0;        /* unit: 1/10 second. */
    options.c_cc[VMIN] = 1;	    /* minimal characters for reading */
	
    tcflush (fd, TCIOFLUSH);// TCIOFLUSH刷清输入/输出队列
	tcsetattr (fd, TCSANOW, &options);
}

/*
*功能:读取串口数据，带超时退出
*参数：
 -fd:设备文件描述符
 -data:读取到数据存放的地址
 -len：需要读取的长度
 -timeout1：读取超时时间1，单位s
 -timeout2：读取超时时间2，单位us
 -超时时间 = timeout1 + timeout2
*返回：
*-1：读取出错；0：超时退出 ；> 0 读取的字节数
*/ 
int serial_recv(int dev, void *data, int len)
{
    int retval = 0;
    struct timeval tv_timeout;
	fd_set fs_read;
    int fd = g_serialPri[dev].serialFd;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);
    tv_timeout.tv_sec = 0;
    tv_timeout.tv_usec = 5000;
    retval = select(fd + 1, &fs_read, NULL, NULL, &tv_timeout);
    if (retval > 0) 
	{
		retval = read(fd, data, len);
        return retval;
    }
    else if (0 == retval) 
	{
		return RESULT_TOUT;
	}
	else
	{ 
        return RESULT_ERR;      
    }
}

/*
*功能:向串口写入数据，带超时退出
*参数：
 -fd:设备文件描述符
 -data:需要写入数据的地址
 -datalength：需要写入的长度
 -timeout1：写入超时时间1，单位s
 -timeout2：写入超时时间2，单位us
 -超时时间 = timeout1 + timeout2
*返回：
*-1：写入出错；0：超时退出 ；> 0 写入的字节数
*/
int serial_send(int dev, char *data, int len)
{
    int  retval;
	struct timeval tv_timeout;
	fd_set  fs_write;
    int fd = g_serialPri[dev].serialFd;

    FD_ZERO (&fs_write);
    FD_SET (fd, &fs_write);
    tv_timeout.tv_sec = 0;
    tv_timeout.tv_usec = 5000;

    retval = select(fd + 1, NULL, &fs_write, NULL, &tv_timeout);
    if(retval > 0) 
    {
        retval = write(fd, data, len);
        if (retval == len)
        {
            return RESULT_OK;
        }
        else
        {
            return RESULT_ERR;
        }
    }
    else if (0 == retval)
    {
        tcflush (fd, TCOFLUSH);     /* flush all output data */
        return RESULT_ERR;
    }
    else
    {   
        tcflush (fd, TCOFLUSH);    
        return RESULT_ERR;
    }

    return RESULT_OK;
}

void serial_clearwriteport (int dev)
{
	tcflush(g_serialPri[dev].serialFd, TCOFLUSH);
}

void serial_clearreadport (int dev)
{
	tcflush(g_serialPri[dev].serialFd, TCIFLUSH);
}

void *serial_deal(void *p)
{
    unsigned char recvbuf[MSG_READ_LEN] = {0};
    int length = 0;
    int msgSize = 0;
 //   int dev = (int)(*((int *)p));
    int dev = *((int *)p);
    int msgFd = g_serialPri[dev].msgFd;
    unsigned char msgFrame[MSG_READ_LEN];

    while (1)
    {
        length = serial_recv(dev, recvbuf, MSG_READ_LEN);
        if (length > 0)
        {
//            printf("length = %d\n\r",length );
            msg_write(msgFd, recvbuf, length);
        }
        else if (length == RESULT_TOUT)
        {
            
        }
        else if (length == RESULT_ERR)
        {
            sleep(1);
        }

        msgSize = msg_size(msgFd);
        if (msgSize > 0)
        {
            length = msg_read(msgFd, msgFrame, MSG_READ_LEN);
            if (length > 0)
            {
                length = g_serialPri[dev].pCallBack(msgFrame, length);
                msg_free(msgFd, length);
            }
            
        }
    }
}