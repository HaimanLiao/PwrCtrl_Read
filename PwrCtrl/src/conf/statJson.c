/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: statJson.c
* SYSTEM NAME	: 整流柜统计数据文件的读取和设置
* BLOCK NAME	:
* PROGRAM NAME	:
* MODULE FORM	:
* -------------------------------------------------------------------------------
* AUTHOR		: wenlong wan
* DEPARTMENT	:
* DATE			:
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "statJson.h"
#include "cJSON.h"
#include "dlog.h"
#include "log_sys.h"

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
const char *statPath = "/ubi/conf/pwrctrl/smartOps.json";

static char g_statBuffer[MAX_BUFFER_LEN] = {0};	// 从统计文件中读取数据

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
* MODULE	: LoadJsonFile
* ABSTRACT	: 读取json文件内容到buffer
* FUNCTION	:
* ARGUMENT	: void
* NOTE		:
* RETURN	:
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
static void LoadJsonFile()
{
	FILE *fp = NULL;
	fp = fopen(statPath, "r");
	if (fp == NULL)
	{
		return;
	}
	fread(g_statBuffer, MAX_BUFFER_LEN, 1, fp);
	fclose(fp);

	return;
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: Stat_GetStatInfo
* ABSTRACT	: 获取统计信息
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: int
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int Stat_GetStatInfo(STAT_STRUCT *pstat)
{
	const cJSON *mod = NULL;
	const cJSON *tmp = NULL;
	int ret = RESULT_OK;
	int i = 0;

	cJSON *json = cJSON_Parse(g_statBuffer);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			DLOG_ERR("cJSON_GetErrorPtr");
			log_send("cJSON_GetErrorPtr");
		}
		ret = RESULT_ERR;
		goto end;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "FanTime");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pstat->fanTime = mod->valueint;

	i = 0;
	mod = cJSON_GetObjectItemCaseSensitive(json, "PDU");
	cJSON_ArrayForEach(tmp, mod)
	{
		cJSON *no = cJSON_GetObjectItemCaseSensitive(tmp, "No");
		cJSON *relay1Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay1Times");
		cJSON *relay2Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay2Times");
		cJSON *relay3Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay3Times");
		cJSON *relay4Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay4Times");
		cJSON *relay5Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay5Times");
		cJSON *relay6Times = cJSON_GetObjectItemCaseSensitive(tmp, "Relay6Times");

		if (!cJSON_IsNumber(no) || !cJSON_IsNumber(relay1Times) 
			|| !cJSON_IsNumber(relay2Times) || !cJSON_IsNumber(relay3Times)
			|| !cJSON_IsNumber(relay4Times) || !cJSON_IsNumber(relay5Times)
			|| !cJSON_IsNumber(relay6Times))
		{
			ret = RESULT_ERR;
			goto end;
		}

		pstat->pdu[i].no = no->valueint;
		pstat->pdu[i].relayTimes[0] = relay1Times->valueint;
		pstat->pdu[i].relayTimes[1] = relay2Times->valueint;
		pstat->pdu[i].relayTimes[2] = relay3Times->valueint;
		pstat->pdu[i].relayTimes[3] = relay4Times->valueint;
		pstat->pdu[i].relayTimes[4] = relay5Times->valueint;
		pstat->pdu[i].relayTimes[5] = relay6Times->valueint;

		i++;
	}

	if (i < PDU_MAX_NUM)
	{
		memset(&pstat->pdu[i], 0, sizeof(PDU_STAT_STRUCT)*(PDU_MAX_NUM-i));
	}

	i = 0;
	mod = cJSON_GetObjectItemCaseSensitive(json, "Mod");
	cJSON_ArrayForEach(tmp, mod)
	{
		cJSON *sn = cJSON_GetObjectItemCaseSensitive(tmp, "SN");
		cJSON *num = cJSON_GetObjectItemCaseSensitive(tmp, "GroupId");
		cJSON *workTime = cJSON_GetObjectItemCaseSensitive(tmp, "WorkTime");

		if (!cJSON_IsNumber(num) || !cJSON_IsString(sn) 
			|| !cJSON_IsNumber(workTime) || (sn->valuestring == NULL))
		{
			ret = RESULT_ERR;
			goto end;
		}

		pstat->mdl[i].groupId = num->valueint;
		pstat->mdl[i].workTime = workTime->valueint;
		memcpy(pstat->mdl[i].SN, sn->valuestring, 18);
		pstat->mdl[i].SN[18] = '\0';
		i++;
	}

	if(i < MDL_MAX_NUM)
	{
		memset(&pstat->mdl[i], 0, sizeof(MDL_STAT_STRUCT)*(MDL_MAX_NUM-i));
	}

end:
	cJSON_Delete(json);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: Stat_SetStatInfo
* ABSTRACT	: 设置统计数据，并保存
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: int
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int Stat_SetStatInfo(STAT_STRUCT stat)
{
	char *string = NULL;
	cJSON *mod = NULL;
	cJSON *pdu = NULL;

	int i = 0;
	int ret = RESULT_OK;

	cJSON *json = cJSON_CreateObject();

	if (cJSON_AddNumberToObject(json, "FanTime", stat.fanTime) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "ModCount", stat.mdlCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduCount", stat.pduCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	mod = cJSON_AddArrayToObject(json, "Mod");
	if (mod == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (stat.mdlCount > MDL_MAX_NUM)
	{
		ret = RESULT_ERR;
		goto end;
	}

	for (i = 0; i < stat.mdlCount; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddStringToObject(tmp, "SN", (const char *)stat.mdl[i].SN) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "GroupId", stat.mdl[i].groupId) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "WorkTime", stat.mdl[i].workTime) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		cJSON_AddItemToArray(mod, tmp);
	}

	if (stat.pduCount > PDU_MAX_NUM)
	{
		ret = RESULT_ERR;
		goto end;
	}

	pdu = cJSON_AddArrayToObject(json, "PDU");
	if (pdu == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	for (i = 0; i < stat.pduCount; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(tmp, "No", stat.pdu[i].no) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay1Times", stat.pdu[i].relayTimes[0]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay2Times", stat.pdu[i].relayTimes[1]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay3Times", stat.pdu[i].relayTimes[2]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay4Times", stat.pdu[i].relayTimes[3]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay5Times", stat.pdu[i].relayTimes[4]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "Relay6Times", stat.pdu[i].relayTimes[5]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		cJSON_AddItemToArray(pdu, tmp);
	}

	string = cJSON_Print(json);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(g_statBuffer, string, strlen(string));
	g_statBuffer[strlen(string)] = '\0';

	FILE *fp = fopen(statPath, "w");
	if (fp == NULL)
	{
		goto end;
	}
	fwrite(g_statBuffer, strlen(g_statBuffer), 1, fp);
	fclose(fp);

	/* 将系统缓存同步到flash中 */
	sync();
	
	usleep(100 * 1000);
//	sleep(1);
end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: Stat_Init
* ABSTRACT	: 初始化
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: int
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int Stat_Init(void)
{
	STAT_STRUCT stat;

	LoadJsonFile();

	if (Stat_GetStatInfo(&stat) < 0)
	{
		DLOG_WARN("CNF_Load Error, try to fix");
		log_send("CNF_Load Error, try to fix");
		system(CMD_STAT_COPY_JSON);
		sleep(2);
		LoadJsonFile();
		if (Stat_GetStatInfo(&stat) < 0)
		{
			DLOG_ERR("CNF_Load");
			log_send("CNF_Load");
			return RESULT_ERR;
		}
	}

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: Stat_Uninit
* ABSTRACT	: 注销
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: void
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
void Stat_Uninit(void)
{

}
