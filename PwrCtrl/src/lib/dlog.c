#include "dlog.h"

/*
	path = NULL, 那么定位到标准输出
*/
int InitDlog(char *path)
{
#ifdef		__DEBUG
	if (path == NULL) {
		dmail_debug = stdout;
	} else {
		if ((dmail_debug = fopen(path, "a")) == NULL) {
			printf("open log path = %s error!\n", path);
			return -1;
		}
	}

	if (setvbuf(dmail_debug, NULL, _IONBF, 0) != 0) {
		printf("setvbuf error!\n");
		return 0;
	}
#endif

	return 0;
}

/* dbus 专用*/
#define DLOG_NAME_LEN			17
void PrintHexBuf(char *name, unsigned char *buf, int buflen)
{
#ifdef DBUSOPTDEBUG

	char			nameTmp[DLOG_NAME_LEN];
	char			strbuf[512];
	int				len = 0;
	struct tm		*tmp = NULL;
	time_t			seconds;
	int				i;
	
	seconds = time(NULL);
	tmp = gmtime(&seconds);

	snprintf(strbuf, sizeof(strbuf), "[%d-%02d-%02d %02d:%02d:%02d.000Z] ",
		tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour,											
		tmp->tm_min, tmp->tm_sec);

	snprintf(nameTmp, sizeof(nameTmp), "%s", name);
	if (strlen(nameTmp) < (DLOG_NAME_LEN - 1))
	{
		for (i = strlen(nameTmp); i < DLOG_NAME_LEN - 1; i++)
		{
			nameTmp[i] = ' ';
		}
		nameTmp[DLOG_NAME_LEN - 1] = '\0';
	}
	
	strcat(strbuf, nameTmp);

	len = strlen(strbuf);
	snprintf(strbuf + len, sizeof(strbuf) - len, "len=%02d [", buflen);
	
	if (buflen > sizeof(strbuf) - 50)
	{
		buflen = sizeof(strbuf) - 50;
	}
	
	if (buflen > 0 && buf != NULL)
	{
		for (i = 0; i < buflen; i++)
		{
			if (i == 7)
			{
				len = strlen(strbuf);
				snprintf(strbuf + len, sizeof(strbuf) - len, "\033[34m");
			}
			len = strlen(strbuf);
			snprintf(strbuf + len, sizeof(strbuf) - len, "%02X ", buf[i]);
			if (i == 8)
			{
				len = strlen(strbuf);
				snprintf(strbuf + len - 1, sizeof(strbuf) - len + 1, "\033[0m ");
			}
		}
		len = strlen(strbuf);
		snprintf(strbuf + len, sizeof(strbuf) - len, "\b]");
	}
	else
	{
		len = strlen(strbuf);
		snprintf(strbuf + len, sizeof(strbuf) - len, "NULL]");
	}
	DBUSDEBUGSTART
	fprintf(dout, "%s\r\n", strbuf);
	DBUSDEBUGEND
#endif
}
