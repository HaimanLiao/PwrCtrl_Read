/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: cnfJson.c
* SYSTEM NAME	: 整流柜配置文件的读取和设置
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

#include "cnfJson.h"
#include "cJSON.h"

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
const char *pwrCnfPath = "/ubi/conf/pwrctrl/pwrCnf.json";

static char g_pwrCnfBuffer[MAX_BUFFER_LEN] = {0};	// 从配置文件中读取数据

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
	fp = fopen(pwrCnfPath, "r");
	if (fp == NULL)
	{
		return;
	}
	fread(g_pwrCnfBuffer, MAX_BUFFER_LEN, 1, fp);
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
* MODULE	: CNF_SetPwrCnf
* ABSTRACT	: 设置用户配置信息，并保存
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: int
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int CNF_SetPwrCnf(PWR_CNF_STRUCT cnf)
{
	char *string = NULL;
	cJSON *mod = NULL;
	cJSON *sensor = NULL;

	int i = 0;
	int ret = RESULT_OK;

	cJSON *json = cJSON_CreateObject();

	if (cJSON_AddNumberToObject(json, "PowerType", cnf.powerType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PowerCount", cnf.powerCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PowerMax", cnf.powerMax) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PowerUnitCount", cnf.powerUnitNum) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduType", cnf.pduType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduCount", cnf.pduCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduRelayCount", cnf.pduRelayCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduTempLevel", cnf.pduTempLevel) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "PduType", cnf.pduType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "ModType", cnf.mdlType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "ModCount", cnf.mdlCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "ModOverVol", cnf.mdlOverVol) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "ModUnderVol", cnf.mdlUnderVol) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "FanCount", cnf.fanCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "FanType", cnf.fanType) == NULL)
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

	for (i = 0; i < cnf.mdlCount; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddStringToObject(tmp, "SN", (const char *)cnf.mdl[i].SN) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "GroupId", cnf.mdl[i].groupId) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		cJSON_AddItemToArray(mod, tmp);
	}

	sensor = cJSON_AddArrayToObject(json, "Sensors");
	if (sensor == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	for (i = 0; i < 1; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(tmp, "tempEn", cnf.sensors.tempEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "humiEn", cnf.sensors.humiEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "liqEn", cnf.sensors.liqEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "atmoEn", cnf.sensors.atmoEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "gyroEn", cnf.sensors.gyroEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "doorEn", cnf.sensors.doorEn) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "tempLevel", cnf.sensors.tempLevel) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "humiLevel", cnf.sensors.humiLevel) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "atmoLevel", cnf.sensors.atmoLevel) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		cJSON_AddItemToArray(sensor, tmp);
	}

	if (cJSON_AddNumberToObject(json, "OTAStep", cnf.OTAStep) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	string = cJSON_Print(json);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(g_pwrCnfBuffer, string, strlen(string));
	g_pwrCnfBuffer[strlen(string)] = '\0';

	FILE *fp = fopen(pwrCnfPath, "w");
	if (fp == NULL)
	{
		goto end;
	}
	fwrite(g_pwrCnfBuffer, strlen(g_pwrCnfBuffer), 1, fp);
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
* MODULE	: GetPwrCnf
* ABSTRACT	: 获取用户配置文件信息，保存了用户配置的信息
* FUNCTION	:
* ARGUMENT	:
* NOTE		:
* RETURN	: int
* CREATE	:
*			: V0.01
* UPDATE	:
* *******************************************************************************
*/
int CNF_GetPwrCnf(PWR_CNF_STRUCT *pcnf)
{
	const cJSON *mod = NULL;
	const cJSON *sensor = NULL;
	const cJSON *tmp = NULL;
	int ret = RESULT_OK;
	int i = 0;

	cJSON *json = cJSON_Parse(g_pwrCnfBuffer);
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

	mod = cJSON_GetObjectItemCaseSensitive(json, "PowerType");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->powerType = mod->valueint;
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "PowerCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->powerCount = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "PowerMax");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->powerMax = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "PowerUnitCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->powerUnitNum = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "PduCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->pduCount = mod->valueint;
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "PduRelayCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->pduRelayCount = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "PduTempLevel");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->pduTempLevel = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "PduType");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->pduType = mod->valueint;


	mod = cJSON_GetObjectItemCaseSensitive(json, "ModType");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->mdlType = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "ModCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->mdlCount = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "ModOverVol");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->mdlOverVol = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "ModUnderVol");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->mdlUnderVol = mod->valueint;
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "FanCount");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->fanCount = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "FanType");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->fanType = mod->valueint;

	i = 0;
	mod = cJSON_GetObjectItemCaseSensitive(json, "Mod");
	cJSON_ArrayForEach(tmp, mod)
	{
		cJSON *sn = cJSON_GetObjectItemCaseSensitive(tmp, "SN");
		cJSON *num = cJSON_GetObjectItemCaseSensitive(tmp, "GroupId");

		if (!cJSON_IsNumber(num) || !cJSON_IsString(sn) 
							|| (sn->valuestring == NULL))
		{
			ret = RESULT_ERR;
			goto end;
		}

		pcnf->mdl[i].groupId = num->valueint;
		memcpy(pcnf->mdl[i].SN, sn->valuestring, 18);
		pcnf->mdl[i].SN[18] = '\0';
		i++;
	}

	if (i < MDL_MAX_NUM)
	{
		memset(&pcnf->mdl[i], 0, sizeof(MDL_CNF_STRUCT)*(MDL_MAX_NUM-i));
	}

	sensor = cJSON_GetObjectItemCaseSensitive(json, "Sensors");
	cJSON_ArrayForEach(tmp, sensor)
	{
		cJSON *tempEn = cJSON_GetObjectItemCaseSensitive(tmp, "tempEn");
		cJSON *humiEn = cJSON_GetObjectItemCaseSensitive(tmp, "humiEn");
		cJSON *liqEn = cJSON_GetObjectItemCaseSensitive(tmp, "liqEn");
		cJSON *atmoEn = cJSON_GetObjectItemCaseSensitive(tmp, "atmoEn");
		cJSON *gyroEn = cJSON_GetObjectItemCaseSensitive(tmp, "gyroEn");
		cJSON *doorEn = cJSON_GetObjectItemCaseSensitive(tmp, "doorEn");
		cJSON *tempLevel = cJSON_GetObjectItemCaseSensitive(tmp, "tempLevel");
		cJSON *humiLevel = cJSON_GetObjectItemCaseSensitive(tmp, "humiLevel");
		cJSON *atmoLevel = cJSON_GetObjectItemCaseSensitive(tmp, "atmoLevel");

		if (!cJSON_IsNumber(tempEn) || !cJSON_IsNumber(humiEn) || !cJSON_IsNumber(liqEn) || !cJSON_IsNumber(atmoEn)
			|| !cJSON_IsNumber(gyroEn) || !cJSON_IsNumber(doorEn) || !cJSON_IsNumber(tempLevel) || !cJSON_IsNumber(humiLevel)
			|| !cJSON_IsNumber(atmoLevel))
		{
			ret = RESULT_ERR;
			goto end;
		}
		pcnf->sensors.tempEn = tempEn->valueint;
		pcnf->sensors.humiEn = humiEn->valueint;
		pcnf->sensors.liqEn = liqEn->valueint;
		pcnf->sensors.atmoEn = atmoEn->valueint;
		pcnf->sensors.gyroEn = gyroEn->valueint;
		pcnf->sensors.doorEn = doorEn->valueint;
		pcnf->sensors.tempLevel = tempLevel->valueint;
		pcnf->sensors.humiLevel = humiLevel->valueint;
		pcnf->sensors.atmoLevel = atmoLevel->valueint;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "OTAStep");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	pcnf->OTAStep = mod->valueint;

end:
	cJSON_Delete(json);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: CNF_Init
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
int CNF_Init(void)
{
	PWR_CNF_STRUCT cnf;

	LoadJsonFile();

	if (CNF_GetPwrCnf(&cnf) < 0)
	{
		DLOG_WARN("CNF_Load Error, try to fix");
		log_send("CNF_Load Error, try to fix");
		system(CMD_CNF_COPY_JSON);
		sleep(2);
		LoadJsonFile();
		if (CNF_GetPwrCnf(&cnf) < 0)
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
* MODULE	: CNF_Uninit
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
void CNF_Uninit(void)
{
}
