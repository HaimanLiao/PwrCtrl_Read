/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME			: zmq_api.c
* SYSTEM NAME		: ZMQ收发
* BLOCK NAME		:
* PROGRAM NAME		:
* MDLULE FORM		:
* -------------------------------------------------------------------------------
* AUTHOR			: wenlong wan
* DEPARTMENT		:
* DATE				: 20200219
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "zmq_api.h"

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


static 	ZMQ_SETINFO_STRUCT g_zmqSetInfo = {0};
static 	void *g_publisher = NULL;
static 	void *g_subscriber = NULL;
static 	void *g_subContext = NULL;
static 	void *g_pubContext = NULL;

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
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void *ProxyDeal(void *p)
{
	void* pContext = zmq_ctx_new();
	void* pXsubSocket = zmq_socket(pContext, ZMQ_XSUB);
	
//	int subhighWaterLevel = 65535;				// 设置接收与发送的高水位
//	zmq_setsockopt(pXsubSocket, ZMQ_RCVHWM, &subhighWaterLevel, sizeof(subhighWaterLevel));
	zmq_bind(pXsubSocket, "tcp://*:6667");		// 前端绑定
	
	zmq_setsockopt(pXsubSocket, ZMQ_SUBSCRIBE, 0, 0);	// 定阅所有消息

	void* pXpubSocket = zmq_socket(pContext, ZMQ_XPUB);
	
//	int pubhighWaterLevel = 65535;				// 设置接收与发送的高水位
//	zmq_setsockopt(pXpubSocket, ZMQ_SNDHWM, &pubhighWaterLevel, sizeof(pubhighWaterLevel));
	zmq_bind(pXpubSocket, "tcp://*:6668");		// 后端绑定

//	printf("start proxy...");
	zmq_proxy(pXsubSocket, pXpubSocket, NULL);	// 循环代理工作
	printf("zmq_proxy error = %s\n", zmq_strerror(errno));

	zmq_close(pXpubSocket);
	zmq_close(pXsubSocket);
	zmq_ctx_destroy(pContext);
	return NULL;
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ProxyInit()
{
	pthread_t pid;
	pthread_create(&pid, 0, ProxyDeal, NULL);
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void *SubDeal(void *p)
{
	int len = 0;
	ZMQ_MSG_STRUCT recvMsg = {0};

	while (1)
	{
//		len = zmq_recv(g_subscriber, recvMsg.subTheme, ZMQ_BUFFER_LEN, 0);		// 接收主题
//		recvMsg.subTheme[len] = '\0';
		len = zmq_recv(g_subscriber, recvMsg.msg, ZMQ_BUFFER_LEN, 0);			// 接收数据
		if (len == -1)
		{
			printf("zmq_recv error, errno = %s\n\r", zmq_strerror(errno));
			continue;
		}
		else if (len > 0)
		{
//			printf("msg = %x\n\r", recvMsg.msg[0]);
			recvMsg.length = len;
			g_zmqSetInfo.pCallBack((unsigned char *)&recvMsg, sizeof(ZMQ_MSG_STRUCT));
		}
	}

	return NULL;
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void SubInit()
{
	int i = 0;
	g_subContext = zmq_ctx_new();
	g_subscriber = zmq_socket(g_subContext, ZMQ_SUB);
	
/*	int subhighWaterLevel = 65535;
	zmq_setsockopt(g_subscriber, ZMQ_RCVHWM, &subhighWaterLevel, sizeof(subhighWaterLevel));*/
	int ret = zmq_connect(g_subscriber, g_zmqSetInfo.subIPC);
	if (ret == -1)
	{
		printf("sub connect error, errno = %s\n\r", zmq_strerror(errno));
	}
	else if (ret == 0)
	{
//		printf("sub connect success\n\r");
	}

	//主题设置
	for (i = 0; i < THEME_MAX_NUM; i++)
	{
		if (strlen(g_zmqSetInfo.subTheme[i]))
		{
			zmq_setsockopt(g_subscriber, ZMQ_SUBSCRIBE, g_zmqSetInfo.subTheme[i], strlen(g_zmqSetInfo.subTheme[i]));
		}
	}
	
	pthread_t pid;
	pthread_create(&pid, 0, SubDeal, NULL);
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void PubInit()
{
	g_pubContext = zmq_ctx_new();
	g_publisher = zmq_socket(g_pubContext, ZMQ_PUB);
	
/*	int pubhighWaterLevel = 65535;
	zmq_setsockopt(pubContext, ZMQ_SNDHWM, &pubhighWaterLevel, sizeof(pubhighWaterLevel));*/
	int linger = 0;
	zmq_setsockopt(g_pubContext, ZMQ_LINGER, &linger, sizeof(linger));
	int ret = zmq_connect(g_publisher, g_zmqSetInfo.pubIPC);
	if (ret == -1)
	{
		printf("pub connect error, errno = %s\n\r", zmq_strerror(errno));
	}
	else if (ret == 0)
	{
//		printf("pub connect success\n\r");
	}
}


/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int Zmq_Publish(ZMQ_MSG_STRUCT sendMsg)
{
	if (!strlen(sendMsg.pubTheme) || (!sendMsg.msg) || (!sendMsg.length))
	{
		return RESULT_ERR;
	}

	zmq_send(g_publisher, sendMsg.pubTheme, strlen(sendMsg.pubTheme), ZMQ_SNDMORE);
	int len = zmq_send(g_publisher, sendMsg.msg, sendMsg.length, 0);
	if (len != sendMsg.length)
	{
		printf("zmq_send error, errno = %s\n\r", zmq_strerror(errno));
		return RESULT_ERR;
	}

	

	return len;
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
int Zmq_Init(ZMQ_SETINFO_STRUCT info)
{
	if (info.pCallBack == NULL)
	{
		return RESULT_ERR;
	}

	int i = 0;
	for (i = 0; i < THEME_MAX_NUM; i++)
	{
		if (strlen(info.subTheme[i]))
		{
			strcpy(g_zmqSetInfo.subTheme[i], info.subTheme[i]);
		}
	}
	
	g_zmqSetInfo.subIPC = info.subIPC;
	g_zmqSetInfo.pubIPC = info.pubIPC;
	g_zmqSetInfo.proxyEn = info.proxyEn;
	g_zmqSetInfo.pCallBack = info.pCallBack;

	if (g_zmqSetInfo.proxyEn == 1)
	{
		ProxyInit();		// 代理，根据实际需求打开
	}
	
	SubInit();

	PubInit();

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: 
* ABSTRACT	: 
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void Zmq_Uninit()
{
	zmq_close(g_subscriber);
	zmq_close(g_publisher);
	zmq_ctx_destroy(g_pubContext);
	zmq_ctx_destroy(g_subContext);
}