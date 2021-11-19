#ifndef DLOG_H_
#define DLOG_H_

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define __DEBUG
#ifdef __DEBUG
FILE		*dmail_debug;

/* 默认黑色 */
#define DEBUG(format, ...) 							\
do													\
{													\
	time_t					seconds;				\
	struct tm				*tmp = NULL;			\
	seconds = time(NULL);							\
	tmp = gmtime(&seconds);							\
	fprintf(dmail_debug, "\r[\033[0mDEBUG\033[0m][%d-%02d-%02d %02d:%02d:%02d.000Z] "format" [%s:%d]\r\n", 	\
		tmp->tm_year + 1900,									\
		tmp->tm_mon + 1,										\
		tmp->tm_mday,											\
		tmp->tm_hour,											\
		tmp->tm_min,											\
		tmp->tm_sec, 											\
		##__VA_ARGS__,											\
		__FILE__, 												\
		__LINE__												\
		);														\
} while(0)

/* 默认黑色 */
#define DLOG_DBG(format, ...) 						\
do													\
{													\
	time_t					seconds;				\
	struct tm				*tmp = NULL;			\
	seconds = time(NULL);							\
	tmp = gmtime(&seconds);							\
	fprintf(dmail_debug, "\r[\033[0mDEBUG\033[0m][%d-%02d-%02d %02d:%02d:%02d.000Z] "format" [%s:%d]\r\n", 	\
		tmp->tm_year + 1900,									\
		tmp->tm_mon + 1,										\
		tmp->tm_mday,											\
		tmp->tm_hour,											\
		tmp->tm_min,											\
		tmp->tm_sec, 											\
		##__VA_ARGS__,											\
		__FILE__, 												\
		__LINE__												\
		);														\
} while(0)

/* 蓝色 */
#define DLOG_INFO(format, ...) 						\
do													\
{													\
	time_t					seconds;				\
	struct tm				*tmp = NULL;			\
	seconds = time(NULL);							\
	tmp = gmtime(&seconds);							\
	fprintf(dmail_debug, "\r[\033[34mINFO \033[0m][%d-%02d-%02d %02d:%02d:%02d.000Z] "format" [%s:%d]\r\n", 	\
		tmp->tm_year + 1900,									\
		tmp->tm_mon + 1,										\
		tmp->tm_mday,											\
		tmp->tm_hour,											\
		tmp->tm_min,											\
		tmp->tm_sec, 											\
		##__VA_ARGS__,											\
		__FILE__, 												\
		__LINE__												\
		);														\
} while(0)

/* 黄色 */
#define DLOG_WARN(format, ...) 						\
do													\
{													\
	time_t					seconds;				\
	struct tm				*tmp = NULL;			\
	seconds = time(NULL);							\
	tmp = gmtime(&seconds);							\
	fprintf(dmail_debug, "\r[\033[33mWARN \033[0m][%d-%02d-%02d %02d:%02d:%02d.000Z] "format" [%s:%d]\r\n", 	\
		tmp->tm_year + 1900,									\
		tmp->tm_mon + 1,										\
		tmp->tm_mday,											\
		tmp->tm_hour,											\
		tmp->tm_min,											\
		tmp->tm_sec, 											\
		##__VA_ARGS__,											\
		__FILE__, 												\
		__LINE__												\
		);														\
} while(0)

/* 红色 */
#define DLOG_ERR(format, ...)						\
do													\
{													\
	time_t					seconds;				\
	struct tm				*tmp = NULL;			\
	seconds = time(NULL);							\
	tmp = gmtime(&seconds);							\
	fprintf(dmail_debug, "\r[\033[31mERR  \033[0m][%d-%02d-%02d %02d:%02d:%02d.000Z] "format" [%s:%d]\r\n", 	\
		tmp->tm_year + 1900,									\
		tmp->tm_mon + 1,										\
		tmp->tm_mday,											\
		tmp->tm_hour,											\
		tmp->tm_min,											\
		tmp->tm_sec, 											\
		##__VA_ARGS__,											\
		__FILE__, 												\
		__LINE__												\
		);														\
} while(0)


#else
#define	DEBUG(...)
#define DLOG_DBG(...)
#define DLOG_INFO(...)
#define DLOG_WARN(...)
#define DLOG_ERR(...)
#endif


//#define LOCALDEBUG			1
#if LOCALDEBUG
#define LOCATDEBUGSTART \
	{ \
		FILE *dout; \
		dout = fopen("/tmp/mdl.log", "a"); \

#define LOCATDEBUGEND \
		fclose(dout); \
	}
#else
#define LOCATDEBUGSTART
#define LOCATDEBUGEND
#endif

//#define DBUSOPTDEBUG			1
#if DBUSOPTDEBUG
#define DBUSDEBUGSTART \
	{ \
		FILE *dout; \
		dout = fopen("/tmp/dbus.log", "a"); \

#define DBUSDEBUGEND \
		fclose(dout); \
	}
#else
#define DBUSDEBUGSTART
#define DBUSDEBUGEND
#endif

//#define POLICYDEBUG			1
#if POLICYDEBUG
#define POLICYDEBUGSTART \
	{ \
		FILE *dout; \
		dout = fopen("/tmp/policy.log", "a"); \

#define POLICYDEBUGEND \
		fclose(dout); \
	}
#else
#define POLICYDEBUGSTART
#define POLICYDEBUGEND
#endif

//#define PWRSTATUSDEBUG			1
#if PWRSTATUSDEBUG
#define PWRSTATUSDEBUGSTART \
	{ \
		FILE *dout; \
		dout = fopen("/tmp/pwrstatus.log", "a"); \

#define PWRSTATUSDEBUGEND \
		fclose(dout); \
	}
#else
#define PWRSTATUSDEBUGSTART
#define PWRSTATUSDEBUGEND
#endif

/*************************接口函数**********************************************/
int InitDlog(char *path);

/*
	name不超过14个字符
*/
void PrintHexBuf(char *name, unsigned char *buf, int buflen);

#endif
