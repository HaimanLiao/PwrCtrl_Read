/*
 * can_api.c
 *
 *  Created on: 2018-4-18
 *      Author: root
 */
#include "can_api.h"
#include "msg.h"

#define CAN0_DEV		"can0"
#define CAN1_DEV		"can1"

#define CAN_MAX_CLIENT		2
#define MSG_READ_LEN		(sizeof(struct can_frame))

typedef struct
{
	pthread_t 	pid;
	int 		canFd;			//lhm: can套接字文件描述符
	int 		msgFd;
	CallBackFun pCallBack;
}CAN_PRI_STRUCT;

static CAN_PRI_STRUCT g_canPri[CAN_MAX_CLIENT] = {0};

void *can_deal(void *p);

int can_init(const int dev)//lhm: 根据文件名初始化can套接字fd
{
	int attr = 0;
	int temp = -1;
	struct ifreq ifr;
	struct sockaddr_can addr;

	char *can;
	if (dev == CAN0)
		can = CAN0_DEV;
	else if (dev == CAN1)
		can = CAN1_DEV;
	else
		return RESULT_ERR;

	if (g_canPri[dev].canFd > 2)
		return RESULT_OK;

	g_canPri[dev].canFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(g_canPri[dev].canFd < 0)
	{
		perror("socket_fd");
		return RESULT_ERR;
	}

	attr = fcntl(g_canPri[dev].canFd, F_GETFL, 0);
	fcntl(g_canPri[dev].canFd, F_SETFL,attr | O_NONBLOCK);

	strcpy(ifr.ifr_name, can);
	ioctl(g_canPri[dev].canFd, SIOCGIFINDEX, &ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	temp = bind(g_canPri[dev].canFd, (struct sockaddr *)&addr, sizeof(addr));
	if(temp < 0)
	{
		perror("bind");
		return RESULT_ERR;
	}
	return RESULT_OK;
}

void can_unInit(const int dev)//lhm: 关闭can套接字fd
{
	if ((dev == CAN0) || (dev == CAN1))
	{
		if (g_canPri[dev].canFd > 2)
		{
			close(g_canPri[dev].canFd);
			g_canPri[dev].canFd = -1;
		}
	}
}

/*
	之前
	int d = dev;
	pthread_create(&g_func[dev].pid, 0, can_deal, &d);

	之前使用局部变量给can_del传递参数，因为是局部变量，所以函数退出之后就会释放
	如果can_deal很长一段时间没有运行，那么&d指向的内存就可能会被又被分配出去，然后被修改了
	然后can_deal开始运行，就取到了奇怪的数值，然后就可能会出问题
*/
static int			dev_can0 = 0;
static int			dev_can1 = 1;

int can_pthread_init(const int dev, CallBackFun pCallBack)//lhm: 开启can_deal线程（pCallBack是can线程调用的接收回调函数）
{
	if (dev != CAN0 && dev != CAN1)
	{
		return RESULT_ERR;
	}

	if (can_init(dev) == RESULT_ERR)
	{
		return RESULT_ERR;
	}

	printf("CAN %d init success\n\r", dev);
	g_canPri[dev].pCallBack = pCallBack;
	g_canPri[dev].msgFd = msg_init(MSG_TYPE_LIST);

	if (dev == CAN0)
	{
		pthread_create(&g_canPri[dev].pid, 0, can_deal, (void *)(&dev_can0));//lhm: 最后一个参数是线程运行函数的参数
	}
	else if (dev == CAN1)
	{
		pthread_create(&g_canPri[dev].pid, 0, can_deal, (void *)(&dev_can1));
	}

	usleep(100*1000);
	return RESULT_OK;
}

int can_pthread_unInit(const int dev)
{
	if ((dev == CAN0) || (dev == CAN1))
	{
		/* 等待线程退出 */
		pthread_cancel(g_canPri[dev].pid);
		pthread_join(g_canPri[dev].pid, NULL);

		/* 清空链表 */
		msg_unInit(g_canPri[dev].msgFd);

		/* 关闭句柄 */
		can_unInit(dev);
	}
	else
	{
		return RESULT_ERR;
	}

	printf("CAN %d uninit success\n\r", dev);

	return RESULT_OK;
}

int can_send(const int dev, struct can_frame sndcan_frame)
{
	int send_bytes = 0;
	fd_set m_writefds;

	struct timeval tm_out;
	int iset = 0;
	int fd = g_canPri[dev].canFd;
	if (fd < 3)
	{
		return RESULT_ERR;
	}

	FD_ZERO(&m_writefds);
	FD_SET(fd, &m_writefds);
	tm_out.tv_sec = 0;
	tm_out.tv_usec = 20;

	iset = select(fd+1, NULL, &m_writefds, NULL, &tm_out);
	if (iset<=0)
	{
		return RESULT_ERR;
	}

	if (FD_ISSET(fd, &m_writefds))
	{
		sndcan_frame.can_id |= CAN_EFF_FLAG;
		send_bytes = send(fd, (char*)&sndcan_frame, sizeof(struct can_frame), 0);
		if(send_bytes == -1)
		{
//			perror("can send error");
			return RESULT_ERR;
		}
	}
	return RESULT_OK;
}

int can_recv(const int dev, struct can_frame *canrev_return)
{
	struct timeval waitd;          // the max wait time for an event
	fd_set m_readfds;
	fd_set m_exceptfds;
	int stat = 0;
	struct can_frame canrev_frame;
	int fd = g_canPri[dev].canFd;

	if ((canrev_return == NULL) || (fd <= 2))
	{
		return RESULT_ERR;
	}

	waitd.tv_sec = 0;
	waitd.tv_usec = 5*1000;

	FD_ZERO(&m_readfds);
	FD_ZERO(&m_exceptfds);
	FD_SET(fd, &m_exceptfds);
	FD_SET(fd, &m_readfds);

	stat = select(fd+1, &m_readfds, NULL, &m_exceptfds, &waitd);

	if (stat < 0)
	{
		//must be dead so you must close it.
		return RESULT_ERR;
	}
	else if (stat == 0)
	{
		return RESULT_TOUT;
	}
	else if (stat > 0)
	{
		if (FD_ISSET(fd, &m_readfds))
		{
			FD_CLR(fd, &m_readfds);

			if (read(fd, &canrev_frame, sizeof(struct can_frame)) <= 0)
			{
				return RESULT_ERR;
			}
			canrev_frame.can_id &= 0x7FFFFFFF;
			memset(canrev_return, 0, sizeof(struct can_frame));
			memcpy(canrev_return, &canrev_frame, sizeof(struct can_frame));

			return RESULT_OK;
		}
	}

	return RESULT_ERR;
}

void *can_deal(void *p)//lhm: 不断接收can帧并调用can接口结构体的成员————接收回调函数pCallBack处理can帧
{
	int length = 0;
	int msgSize = 0;
	int dev = (int)(*((int *)p));
	int msgFd = g_canPri[dev].msgFd;
	struct can_frame recvframe;
	unsigned char msgFrame[MSG_READ_LEN];

	while (1)
	{
		length = can_recv(dev, &recvframe);
		if (length == RESULT_OK)
		{
			msg_write(msgFd, (unsigned char *)&recvframe, sizeof(recvframe));
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
				length = g_canPri[dev].pCallBack(msgFrame, length);
				msg_free(msgFd, length);
			}
			
		}
	}

	return NULL;
}
