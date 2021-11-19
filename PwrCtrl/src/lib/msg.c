/*
* *******************************************************************************
*   @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME         : msg.c
* SYSTEM NAME       : list、ringbuffer等方式的集合
* BLOCK NAME        :
* PROGRAM NAME      :
* MDLULE FORM       :
* -------------------------------------------------------------------------------
* AUTHOR            : wenlong wan
* DEPARTMENT        :
* DATE              : 20200228
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "msg.h"
#include "sclist.h"
#include "ring_buffer.h"

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

#define     FD_MAX_NUM      16      // 最大支持fd个数

#pragma pack(1)
typedef struct
{
    int len;
    unsigned char* outbuf;
}OUTBUF;

typedef struct
{
    int msgType;            
    int fd;                 // 分配的虚拟fd
    pthread_mutex_t mutex;  // 锁

    LinkList *pList;        // 指向list的指针
    ring_buffer_t *pRing;   // 指向ringbuffer的指针
}MSG_STRUCT;
#pragma pack()

static MSG_STRUCT g_msg[FD_MAX_NUM] = {0};
static int g_fd[FD_MAX_NUM] = {0};

static LinkList g_list[FD_MAX_NUM] = {0};
static ring_buffer_t g_ring[FD_MAX_NUM] = {0};

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
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void list_push(int idx, unsigned char *in, int len)
{ 
    OUTBUF* buf = (OUTBUF *)malloc(sizeof(OUTBUF));
    buf->outbuf = (unsigned char *)malloc(len);
    memcpy(buf->outbuf, in, len);
    buf->len = len;

    PushBack_LinkList(g_msg[idx].pList, buf);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static int list_pop(int idx, unsigned char *pframe, int len)
{
    int length = 0;
    OUTBUF *out = NULL;

    out = RemoveByPos_LinkList(g_msg[idx].pList, 0);

    if (out == NULL)
    {
        return RESULT_ERR;
    }

    length = out->len;

    if (length <= len)
    {
        memcpy(pframe, out->outbuf, length);
    }
    else
    {
        length = RESULT_ERR;
    }

    free(out->outbuf);
    free(out);

    return length;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static int list_size(int idx)
{
    return Size_LinkList(g_msg[idx].pList);
}


/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void list_init(int idx)
{
    g_msg[idx].pList = Init_LinkList();
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void list_unInit(int idx)
{
    int i = 0;
    int len = list_size(idx);
    
    for (i = 0; i < len; i++)
    {
        OUTBUF* ob = Get_LinkList(g_msg[idx].pList, i);
        if (ob)
        {
            if (ob->outbuf)
                free(ob->outbuf);
            free(ob);
        }
    }
    Destroy_LinkList(g_msg[idx].pList);
    g_msg[idx].pList = NULL;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static int ring_push(int idx, unsigned char *pframe, int len)
{
    return ring_buffer_write(g_msg[idx].pRing, pframe, len);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static int ring_pop(int idx, unsigned char *pframe, int len)
{
    return ring_buffer_read(g_msg[idx].pRing, pframe, len);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void ring_free(int idx, int len)
{
    ring_buffer_free(g_msg[idx].pRing, len);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static int ring_size(int idx)
{
    return ring_buffer_bytes_available(g_msg[idx].pRing);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void ring_init(int idx)
{
    unsigned char *storage = NULL;

    storage = (unsigned char *)malloc(MAX_BUFFER_LEN);
    ring_buffer_init(g_msg[idx].pRing, storage, MAX_BUFFER_LEN);
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
static void ring_unInit(int idx)
{
    ring_buffer_free(g_msg[idx].pRing, MAX_BUFFER_LEN);
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_read(int fd, unsigned char *pframe, int len)
{
    if ((fd > FD_MAX_NUM) || (fd <= 0))
    {
        return RESULT_ERR;
    }

    int idx = fd - 1;
    int ret = RESULT_OK;

    pthread_mutex_lock(&g_msg[idx].mutex);

    if (g_msg[idx].msgType == MSG_TYPE_LIST)
    {
        ret = list_pop(idx, pframe, len);
    }
    else if (g_msg[idx].msgType == MSG_TYPE_RING)
    {
        ret = ring_pop(idx, pframe, len);
    }

    pthread_mutex_unlock(&g_msg[idx].mutex);

    return ret;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_write(int fd, unsigned char *pframe, int len)
{
    if ((fd > FD_MAX_NUM) || (fd <= 0))
    {
        return RESULT_ERR;
    }

    int idx = fd - 1;
    int ret = RESULT_OK;

    pthread_mutex_lock(&g_msg[idx].mutex);

    if (g_msg[idx].msgType == MSG_TYPE_LIST)
    {
        list_push(idx, pframe, len);
    }
    else if (g_msg[idx].msgType == MSG_TYPE_RING)
    {
        ret = ring_push(idx, pframe, len);
    }

    pthread_mutex_unlock(&g_msg[idx].mutex);

    return ret;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_size(int fd)
{
    if ((fd > FD_MAX_NUM) || (fd <= 0))
    {
        return RESULT_ERR;
    }

    int idx = fd - 1;
    int ret = RESULT_OK;

    pthread_mutex_lock(&g_msg[idx].mutex);

    if (g_msg[idx].msgType == MSG_TYPE_LIST)
    {
        ret = list_size(idx);
    }
    else if (g_msg[idx].msgType == MSG_TYPE_RING)
    {
        ret = ring_size(idx);
    }

    pthread_mutex_unlock(&g_msg[idx].mutex);

    return ret;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_free(int fd, int len)
{
    if ((fd > FD_MAX_NUM) || (fd <= 0))
    {
        return RESULT_ERR;
    }

    int idx = fd - 1;
    int ret = RESULT_OK;

    pthread_mutex_lock(&g_msg[idx].mutex);

    if (g_msg[idx].msgType == MSG_TYPE_LIST)
    {

    }
    else if (g_msg[idx].msgType == MSG_TYPE_RING)
    {
        ring_free(idx, len);
    }

    pthread_mutex_unlock(&g_msg[idx].mutex);

    return ret;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_init(int msgType)
{
    int i = 0;
    int fd = 0;

    for (i = 0; i < FD_MAX_NUM; i++)
    {
        if (g_fd[i] == 0)
        {
            fd = i + 1;
            g_fd[i] = 1;
            break;
        }
    }

    if (i == FD_MAX_NUM)    // 资源全部被使用，不予分配
    {
        return RESULT_ERR;
    }

    g_msg[i].fd = fd;
    g_msg[i].msgType = msgType;
    pthread_mutex_init(&g_msg[i].mutex, NULL);

    if (g_msg[i].msgType == MSG_TYPE_LIST)
    {
        g_msg[i].pList = &g_list[i];
        list_init(i);
    }
    else if (g_msg[i].msgType == MSG_TYPE_RING)
    {
        g_msg[i].pRing = &g_ring[i];
        ring_init(i);
    }

    return fd;
}

/*
* *******************************************************************************
* MODULE    : 
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
int msg_unInit(int fd)
{
    if ((fd > FD_MAX_NUM) || (fd <= 0))
    {
        return RESULT_ERR;
    }

    int idx = fd - 1;
    g_fd[idx] = 0;
    g_msg[idx].fd = 0;

    pthread_mutex_destroy(&g_msg[idx].mutex);

    if (g_msg[idx].msgType == MSG_TYPE_LIST)
    {
        list_unInit(idx);
    }
    else if (g_msg[idx].msgType == MSG_TYPE_RING)
    {
        ring_unInit(idx);
    }

    return RESULT_OK;
}