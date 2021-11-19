/*
* *******************************************************************************
*	@(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME		: 
* SYSTEM NAME	: 
* BLOCK NAME	: 
* PROGRAM NAME	: 
* MODULE FORM	: 
* -------------------------------------------------------------------------------
* AUTHOR		: 
* DEPARTMENT	: 
* DATE			: 
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "zmqProt.h"
#include "zmq_api.h"
#include "cJSON.h"
#include "crc.h"
#include "mbedtls/aes.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"

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

#define 	SUB_IPC 			"tcp://127.0.0.1:6668"
#define 	PUB_IPC 			"tcp://127.0.0.1:6667"

#define 	SUB_TER_THEME		"sc/os/terminal"
#define 	PUB_UNIT_THEME		"sc/os/pwrunit"

#define 	SUB_WEB_THEME		"sc/os/web"
#define 	PUB_GW_THEME		"sc/os/gateway"

#define 	TMP_BUF_LEN			MAX_BUFFER_LEN

static 		HANDLE_STRUCT *g_pHandle = NULL;

/*
* *******************************************************************************
*                                  Private function prototypes
* *******************************************************************************
*/

static int FrameDataPackFrame(FRAME_DATA_STRUCT frame, cJSON *json);
static int FrameDataUnpackFrame(unsigned char *pframe);
/*
* *******************************************************************************
*                                  Private functions
* *******************************************************************************
*/

/****************************WEB或HMI的数据包处理*********************************/

/*
* *******************************************************************************
* MODULE	: SetPwrReqUnpackFrame
* ABSTRACT	: setPwrReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetPwrReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	cJSON *child = NULL;
	cJSON *mod = NULL;
	int i = 0;

	child = cJSON_GetObjectItemCaseSensitive(json, "pwr");
	if (cJSON_IsObject(child))
	{
		WEB_SET_STRUCT *pbody = (WEB_SET_STRUCT *)(frame.body);

		pbody->pwrSetEn = 1;

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_type");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pwrType = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_max");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pwrMax = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_num");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pwrNum = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_unit_num");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pwrUnitNum = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_relay_num");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pduRlyNum = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_type");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pduType = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_num");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.pduNum = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_fan_type");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwr.fanType = mod->valueint;
		}
	}

	child = cJSON_GetObjectItemCaseSensitive(json, "mdl");
	if (cJSON_IsObject(child))
	{
		WEB_SET_STRUCT *pbody = (WEB_SET_STRUCT *)(frame.body);

		pbody->mdlSetEn = 1;

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_mdl_num");
		if (cJSON_IsNumber(mod))
		{
			pbody->mdl.mdlNum = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_mdl_type");
		if (cJSON_IsNumber(mod))
		{
			pbody->mdl.mdlType = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_mdl_undervol");
		if (cJSON_IsNumber(mod))
		{
			pbody->mdl.mdlUnderVol = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_mdl_overvol");
		if (cJSON_IsNumber(mod))
		{
			pbody->mdl.mdlOverVol = mod->valueint;
		}

		i = 0;
		child = cJSON_GetObjectItemCaseSensitive(child, "array");
		cJSON_ArrayForEach(mod, child)
		{
			cJSON *sn = cJSON_GetObjectItemCaseSensitive(mod, "id_mdl_sn");
			cJSON *num = cJSON_GetObjectItemCaseSensitive(mod, "id_mdl_grpid");

			pbody->mdl.sn[i].groupId = num->valueint;
			memcpy(pbody->mdl.sn[i].SN, sn->valuestring, 18);
			pbody->mdl.sn[i].SN[18] = '\0';
			i++;
		}
	}

	child = cJSON_GetObjectItemCaseSensitive(json, "sensor");
	if (cJSON_IsObject(child))
	{
		WEB_SET_STRUCT *pbody = (WEB_SET_STRUCT *)(frame.body);

		pbody->sensorSetEn = 1;

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_temp_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.tempEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_temp_thre");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.tempLevel = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_liq_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.liqEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_door_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.doorEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_atmo_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.atmoEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_gyro_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.gyroEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_hum_en");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.humiEn = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_hum_thre");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.humiLevel = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_atmo_thre");
		if (cJSON_IsNumber(mod))
		{
			pbody->sensor.atmoLevel = mod->valueint;
		}
	}

	child = cJSON_GetObjectItemCaseSensitive(json, "hw");
	if (cJSON_IsObject(child))
	{
		WEB_SET_STRUCT *pbody = (WEB_SET_STRUCT *)(frame.body);

		pbody->hwSetEn = 1;

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_warn_led");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.warnLed = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_err_led");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.errLed = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_ac_relay");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.acRelay = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_fan_relay");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.fanRelay = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_fan_speed");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.fanSpeed = mod->valueint;
		}
		else
		{
			pbody->hw.fanSpeed = 0xFF;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_id");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.pduId = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_relay_id");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.pduRelayId = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pdu_relay");
		if (cJSON_IsNumber(mod))
		{
			pbody->hw.pduRelay = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_check");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwrCheck = mod->valueint;

			pbody->checkSetEn = 1;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_id");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwrGunId = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_vol");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwrVol = mod->valueint;
		}

		mod = cJSON_GetObjectItemCaseSensitive(child, "id_pwr_cur");
		if (cJSON_IsNumber(mod))
		{
			pbody->pwrCur = mod->valueint;
		}
	}

	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: SetPwrResPackFrame
* ABSTRACT	: setPwrRes打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int SetPwrResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;
	int ret = RESULT_OK;

	WEB_SET_RESULT_STRUCT *pbody = (WEB_SET_RESULT_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	if (cJSON_AddNumberToObject(body, "id_result", pbody->result) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}
	
	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: StatReqUnpackFrame
* ABSTRACT	: statReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int StatReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: StatResPackFrame
* ABSTRACT	: statRes打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int StatResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;
	cJSON *child = NULL;
	int i = 0;
	int ret = RESULT_OK;

	WEB_SMARTOPS_STRUCT *pbody = (WEB_SMARTOPS_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	child = cJSON_AddObjectToObject(body, "mdl");
	if (cJSON_AddNumberToObject(child, "id_mdl_num", pbody->mdlCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}
	child = cJSON_AddArrayToObject(child, "array");
	for (i = 0; i < pbody->mdlCount; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(tmp, "id_mdl_id", pbody->mdl[i].groupId) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddStringToObject(tmp, "id_mdl_sn", (char *)pbody->mdl[i].SN) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_mdl_time", pbody->mdl[i].workTime) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		cJSON_AddItemToArray(child, tmp);
	}

	child = cJSON_AddObjectToObject(body, "pdu");
	if (cJSON_AddNumberToObject(child, "id_pdu_num", pbody->pduCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	child = cJSON_AddArrayToObject(child, "array");
	for (i = 0; i < pbody->pduCount; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(tmp, "id_pdu_id", pbody->pdu[i].no) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly1_times", pbody->pdu[i].relayTimes[0]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly2_times", pbody->pdu[i].relayTimes[1]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly3_times", pbody->pdu[i].relayTimes[2]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly4_times", pbody->pdu[i].relayTimes[3]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly5_times", pbody->pdu[i].relayTimes[4]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}

		if (cJSON_AddNumberToObject(tmp, "id_rly6_times", pbody->pdu[i].relayTimes[5]) == NULL)
		{
			ret = RESULT_ERR;
			goto end;
		}
		
		cJSON_AddItemToArray(child, tmp);
	}

	child = cJSON_AddObjectToObject(body, "other");
	if (cJSON_AddNumberToObject(child, "id_fan_time", pbody->fanTime) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: GetPwrReqUnpackFrame
* ABSTRACT	: getPwrReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetPwrReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));
	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: GetPwrResPackFrame
* ABSTRACT	: getPwrRes打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int GetPwrResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;
	cJSON *child = NULL;
	int i = 0;
	int ret = RESULT_OK;
	char tempBuf[128] = {0};
	static int sendLoop = 0;		// 因为数据包太大，部分数据包循环发送

	WEB_POWER_STATUS_STRUCT *pbody = (WEB_POWER_STATUS_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	child = cJSON_AddObjectToObject(body, "pwr");
	if (cJSON_AddNumberToObject(child, "id_pwr_type", pbody->pwr.pwrType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pwr_max", pbody->pwr.pwrMax) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pwr_num", pbody->pwr.pwrNum) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pwr_unit_num", pbody->pwr.pwrUnitNum) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pdu_relay_num", pbody->pwr.pduRlyNum) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pdu_type", pbody->pwr.pduType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_pdu_num", pbody->pwr.pduNum) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_fan_type", pbody->pwr.fanType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	child = cJSON_AddObjectToObject(body, "status");
	if (cJSON_AddNumberToObject(child, "id_estop_sta", pbody->status.estop) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_sd_sta", pbody->status.sd) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_fan_sta", pbody->status.fan) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_output_power", pbody->status.outPower) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_fan_speed", pbody->status.fanSpeed) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	sendLoop++;
			
	child = cJSON_AddObjectToObject(body, "mdl");
	if (cJSON_AddNumberToObject(child, "id_mdl_undervol", pbody->mdlUnderVol) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_mdl_overvol", pbody->mdlOverVol) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_mdl_num", pbody->mdlCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_mdl_type", pbody->mdlType) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if ((sendLoop % 2) == 0)
	{
		child = cJSON_AddArrayToObject(child, "array");
		for (i = 0; i < pbody->mdlCount; i++)
		{
			cJSON *tmp = cJSON_CreateObject();
			if (cJSON_AddNumberToObject(tmp, "id_mdl_id", pbody->mdl[i].id) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_grpid", pbody->mdl[i].groupId) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddStringToObject(tmp, "id_mdl_sn", (char *)pbody->mdl[i].SN) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_reqvol", pbody->mdl[i].volNeed) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_reqcur", pbody->mdl[i].curNeed) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_realvol", pbody->mdl[i].volOut) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_realcur", pbody->mdl[i].curOut) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_temp", pbody->mdl[i].temp) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_status", pbody->mdl[i].fault) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_mdl_sts", pbody->mdl[i].sts) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			cJSON_AddItemToArray(child, tmp);
		}
	}

	child = cJSON_AddObjectToObject(body, "pdu");
	if (cJSON_AddNumberToObject(child, "id_pdu_num", pbody->pduCount) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if ((sendLoop % 2) == 1)
	{
		child = cJSON_AddArrayToObject(child, "array");
		for (i = 0; i < pbody->pduCount; i++)
		{
			cJSON *tmp = cJSON_CreateObject();

			if (cJSON_AddNumberToObject(tmp, "id_pdu_id", pbody->pdu[i].id) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			if (cJSON_AddNumberToObject(tmp, "id_pdu_temp", pbody->pdu[i].temp[0]) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}

			sprintf(tempBuf, "%02x-%02x-%02x-%02x-%02x-%02x", pbody->pdu[i].sw[0], pbody->pdu[i].sw[1],
				pbody->pdu[i].sw[2], pbody->pdu[i].sw[3], pbody->pdu[i].sw[4], pbody->pdu[i].sw[5]);
			if (cJSON_AddStringToObject(tmp, "id_pdu_rlyBack", tempBuf) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}
			
			cJSON_AddItemToArray(child, tmp);
		}
	}

	child = cJSON_AddObjectToObject(body, "sensor");
	if (cJSON_AddNumberToObject(child, "id_temp_en", pbody->sensor.tempEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_hum_en", pbody->sensor.humiEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_liq_en", pbody->sensor.liqEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_atmo_en", pbody->sensor.atmoEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_gyro_en", pbody->sensor.gyroEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_door_en", pbody->sensor.doorEn) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_temp_thre", pbody->sensor.tempLevel) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_atmo_thre", pbody->sensor.atmoLevel) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_hum_thre", pbody->sensor.humiLevel) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_temp_val", pbody->senInfo.tempVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_hum_val", pbody->senInfo.humVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_liq_val", pbody->senInfo.liqVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_atmo_val", pbody->senInfo.atmoVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_gyro_val", pbody->senInfo.gyroVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_door_val", pbody->senInfo.doorVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_cb_val", pbody->senInfo.CBVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(child, "id_surging_val", pbody->senInfo.surgingVal) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/****************************充电主题的数据包处理*********************************/

/*
* *******************************************************************************
* MODULE	: heartReqUnpackFrame
* ABSTRACT	: heartReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int heartReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	int ret = 0;
	cJSON *mod = NULL;

	FRAME_DATA_HEART_STRUCT *pbody = (FRAME_DATA_HEART_STRUCT *)(frame.body);

	mod = cJSON_GetObjectItemCaseSensitive(json, "time");
	if (cJSON_IsString(mod))
	{
		memcpy(pbody->time, mod->valuestring, strlen(mod->valuestring));
	}

	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));

	return ret;
}

/*
* *******************************************************************************
* MODULE	: HeartResFrame
* ABSTRACT	: heartRes打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int HeartResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;

	int ret = RESULT_OK;

	FRAME_DATA_HEART_STRUCT *pbody = (FRAME_DATA_HEART_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();

	body = cJSON_AddObjectToObject(json, "body");

	if (cJSON_AddStringToObject(body, "time", pbody->time) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: statusUnpackFrame
* ABSTRACT	: status解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int statusUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	int ret = 0;
	cJSON *mod = NULL;

	FRAME_DATA_STATUS_REQ_STRUCT *pbody = (FRAME_DATA_STATUS_REQ_STRUCT *)(frame.body);

	mod = cJSON_GetObjectItemCaseSensitive(json, "chgSta");
	if (cJSON_IsNumber(mod))
	{
		pbody->chgSta = mod->valueint;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "workMode");
	if (cJSON_IsNumber(mod))
	{
		pbody->workMode = mod->valueint;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "cardSta");
	if (cJSON_IsNumber(mod))
	{
		pbody->cardSta = mod->valueint;
	}

	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));

	return ret;
}

/*
* *******************************************************************************
* MODULE	: StatusFrame
* ABSTRACT	: status打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int StatusPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;
	cJSON *mod = NULL;
	int i = 0;
	int ret = RESULT_OK;

	FRAME_DATA_STATUS_STRUCT *pbody = (FRAME_DATA_STATUS_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	mod = cJSON_AddArrayToObject(body, "pwrErr");
	if (mod == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	for (i = 0; i < DTC_MAX_NUM; i++)
	{
		cJSON *tmp = cJSON_CreateObject();

		if (pbody->pwrErr[i])
		{
			if (cJSON_AddNumberToObject(tmp, "dtc", pbody->pwrErr[i]) == NULL)
			{
				ret = RESULT_ERR;
				goto end;
			}
			cJSON_AddItemToArray(mod, tmp);
		}
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);
	DEBUG("pwrStatus = %s", string);
	log_send("pwrStatus = %s", string);
end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: PwrReqUnpackFrame
* ABSTRACT	: PwrReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int PwrReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	int ret = 0;
	cJSON *mod = NULL;

	FRAME_DATA_PWR_REQ_STRUCT *pbody = (FRAME_DATA_PWR_REQ_STRUCT *)(frame.body);

	mod = cJSON_GetObjectItemCaseSensitive(json, "chgSta");
	if (cJSON_IsNumber(mod))
	{
		pbody->chgSta = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "chgStep");
	if (cJSON_IsNumber(mod))
	{
		pbody->chgStep = mod->valueint;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "EVPwrMax");
	if (cJSON_IsNumber(mod))
	{
		pbody->EVPwrMax = mod->valueint;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "GunPwrMax");
	if (cJSON_IsNumber(mod))
	{
		pbody->GunPwrMax = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "EVVolMax");
	if (cJSON_IsNumber(mod))
	{
		pbody->EVVolMax = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "EVCurMax");
	if (cJSON_IsNumber(mod))
	{
		pbody->EVCurMax = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "reqVol");
	if (cJSON_IsNumber(mod))
	{
		pbody->reqVol = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "reqCur");
	if (cJSON_IsNumber(mod))
	{
		pbody->reqCur = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "stopReason");
	if (cJSON_IsNumber(mod))
	{
		pbody->stopReason = mod->valueint;
	}
	
	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));

	return ret;
}

/*
* *******************************************************************************
* MODULE	: PwrResFrame
* ABSTRACT	: pwrRes打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int PwrResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;

	int ret = RESULT_OK;

	FRAME_DATA_PWR_RES_STRUCT *pbody = (FRAME_DATA_PWR_RES_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	if (cJSON_AddNumberToObject(body, "pwrSta", pbody->pwrSta) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "pwrStep", pbody->pwrStep) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "EVSEPwrMax", pbody->EVSEPwrMax) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "EVSEVolMax", pbody->EVSEVolMax) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "EVSEVolMin", pbody->EVSEVolMin) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "EVSECurMax", pbody->EVSECurMax) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "realVol", pbody->realVol) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}
	if (cJSON_AddNumberToObject(body, "realCur", pbody->realCur) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "stopReason", pbody->stopReason) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	DEBUG("pwrRes = %s", string);
	log_send("pwrRes = %s", string);

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: OTAReqUnpackFrame
* ABSTRACT	: OTAReq解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int OTAReqUnpackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	int ret = 0;
	cJSON *mod = NULL;

	FRAME_DATA_OTA_REQ_STRUCT *pbody = (FRAME_DATA_OTA_REQ_STRUCT *)(frame.body);

	mod = cJSON_GetObjectItemCaseSensitive(json, "OTAEnable");
	if (cJSON_IsNumber(mod))
	{
		pbody->OTAEnable = mod->valueint;
	}
	
	mod = cJSON_GetObjectItemCaseSensitive(json, "OTAPath");
	if (cJSON_IsString(mod) && strlen(mod->valuestring))
	{
		memcpy(pbody->OTAPath, mod->valuestring, strlen(mod->valuestring));
	}
	
	g_pHandle->eventHandle((unsigned char *)&frame, sizeof(FRAME_DATA_STRUCT));

	return ret;
}

/*
* *******************************************************************************
* MODULE	: CheckReqPackFrame
* ABSTRACT	: checkReq打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int OTAResPackFrame(FRAME_DATA_STRUCT frame, 
							unsigned char *dstFrame, int *dstLen)
{
	char *string = NULL;
	cJSON *json = NULL;
	cJSON *body = NULL;

	int ret = RESULT_OK;

	FRAME_DATA_OTA_RES_STRUCT *pbody = (FRAME_DATA_OTA_RES_STRUCT *)(frame.body);
	
	json = cJSON_CreateObject();
	body = cJSON_AddObjectToObject(json, "body");

	if (cJSON_AddNumberToObject(body, "OTAStep", pbody->OTAStep) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(body, "failReason", pbody->failReason) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	FrameDataPackFrame(frame, json);

	string = cJSON_Print(json);
	cJSON_Minify(string);
	if (string == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	memcpy(dstFrame, string, strlen(string));
	(*dstLen) = strlen(string);

end:
	cJSON_Delete(json);
	free(string);
	return ret;
}

/****************************协议帧格式组包解包***********************************/

/*
* *******************************************************************************
* MODULE	: FrameDataPackFrame
* ABSTRACT	: 数据内容打包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int FrameDataPackFrame(FRAME_DATA_STRUCT frame, cJSON *json)
{
	int ret = 0;

	if (cJSON_AddNumberToObject(json, "requestId", frame.requestId) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddStringToObject(json, "method", frame.method) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "pwrUnitId", frame.pwrUnitId) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "terminalId", frame.terminalId) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (cJSON_AddNumberToObject(json, "connId", frame.connId) == NULL)
	{
		ret = RESULT_ERR;
		goto end;
	}

end:
	return ret;
}

/*
* *******************************************************************************
* MODULE	: FrameDataUnpackFrame
* ABSTRACT	: 数据内容解包函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int FrameDataUnpackFrame(unsigned char *pframe)
{
	int ret = 0;
	cJSON *mod = NULL;
	FRAME_DATA_STRUCT frameData = {0};

	cJSON *json = cJSON_Parse((char *)pframe);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			DEBUG("pframe = %s", pframe);
			DLOG_ERR("cJSON_GetErrorPtr");
		}
		ret = RESULT_ERR;
		goto end;
	}

	mod = cJSON_GetObjectItemCaseSensitive(json, "requestId");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	frameData.requestId = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "method");
	if (!cJSON_IsString(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	memcpy(frameData.method, mod->valuestring, strlen(mod->valuestring));

	mod = cJSON_GetObjectItemCaseSensitive(json, "pwrUnitId");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	frameData.pwrUnitId = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "terminalId");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	frameData.terminalId = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "connId");
	if (!cJSON_IsNumber(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}
	frameData.connId = mod->valueint;

	mod = cJSON_GetObjectItemCaseSensitive(json, "body");
	if (!cJSON_IsObject(mod))
	{
		ret = RESULT_ERR;
		goto end;
	}

	if (!strcmp(frameData.method, MTD_HEART_REQ))
	{
//		DEBUG("heart = %s", pframe);
		heartReqUnpackFrame(frameData, mod);		
	}
	else if (!strcmp(frameData.method, MTD_STATUS))
	{
		DEBUG("chgStatus = %s", pframe);
		log_send("chgStatus = %s", pframe);
		statusUnpackFrame(frameData, mod);		
	}
	else if (!strcmp(frameData.method, MTD_PWR_REQ))
	{
		DEBUG("pwrReq = %s", pframe);
		log_send("pwrReq = %s", pframe);
		PwrReqUnpackFrame(frameData, mod);
	}
	else if (!strcmp(frameData.method, MTD_OTA_REQ))
	{
		OTAReqUnpackFrame(frameData, mod);
	}
	else if (!strcmp(frameData.method, MTD_SET_PWR_REQ))
	{
		SetPwrReqUnpackFrame(frameData, mod);
	}
	else if (!strcmp(frameData.method, MTD_GET_PWR_REQ))
	{
		GetPwrReqUnpackFrame(frameData, mod);
	}
	else if (!strcmp(frameData.method, MTD_STAT_REQ))
	{
		StatReqUnpackFrame(frameData, mod);
	}
	
end:
	cJSON_Delete(json);
	return ret;
}

/*
* *******************************************************************************
* MODULE	: AES
* ABSTRACT	: AES加解密
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int AES(unsigned char *srcBuf, int srcLen,
						unsigned char *dstBuf, int *dstLen, int mode)
{
	mbedtls_aes_context aes_ctx;
	int i = 0;
	int length = 0;
	unsigned char key[16] = {'c', 'b', 'c', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1', '2', '3', '4'};
	unsigned char iv[16];

	mbedtls_aes_init(&aes_ctx);

	for(i = 0; i < 16; i++)
    {
        iv[i] = 0x01;
    }

	if (mode == MBEDTLS_AES_ENCRYPT)
	{
		length = srcLen % 16 ? (((srcLen/16)+1)*16) : srcLen;
		mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
		mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, length, iv, srcBuf, dstBuf);
		(*dstLen) = length;
	}
	else if (mode == MBEDTLS_AES_DECRYPT)
	{
		mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
		mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, srcLen, iv, srcBuf, dstBuf);
		(*dstLen) = strlen((char *)dstBuf);		// 偷懒，协议是json字符串所以用strlen，如果是十六进制则不能这么计算
	}

	mbedtls_aes_free(&aes_ctx);
    
	return 0;
}

/*
* *******************************************************************************
* MODULE	: FilterBuffer
* ABSTRACT	: 帧过滤，判断帧格式
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int FilterBuffer(unsigned char *pframe, int len)
{
	/* 过滤主题消息 */
	if (memcmp(pframe, "sc/", 3) == 0)
	{
		return -1;
	}

	/* 帧头(3B) + 长度(2B) + 数据(nB) + 校验(2B)*/
	if (len < 7)	// 帧最短7字节
	{
		DEBUG("len < 7");
		return -1;
	}

	if ((pframe[0] != 0xFF) || (pframe[1] != 0x6A) || (pframe[2] != 0xA6))		// 帧头异常
	{
		DEBUG("[0] [1] [2] = %02x %02x %02x", pframe[0], pframe[1], pframe[2]);
		return -1;
	}

	int length = pframe[3] * 256 + pframe[4];
	if ((length + 5) != (len))		// 帧长度不匹配
	{
		DEBUG("len = %d %d", length, len);
		return -1;
	}

	unsigned char crcH = CRC16(pframe, len-2, Hi);
	unsigned char crcL = CRC16(pframe, len-2, Lo);

	if ((crcH != pframe[len-2]) || (crcL != pframe[len-1]))		// 校验失败
	{
		DEBUG("crc = %x %x, %x %x", pframe[len-2], pframe[len-1], crcH, crcL);
		return -1;
	}

	return len;
}

/*
* *******************************************************************************
* MODULE	: PackFrame
* ABSTRACT	: 帧格式组包
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int PackFrame(unsigned char *srcFrame, int srcLen,
						unsigned char *dstFrame, int *dstLen)
{
	/* 帧头(3B) + 长度(2B) + 数据(nB) + 校验(2B) */
	dstFrame[0] = 0xFF;
	dstFrame[1] = 0x6A;
	dstFrame[2] = 0xA6;

	dstFrame[3] = (srcLen + 2) / 256;
	dstFrame[4] = (srcLen + 2) % 256;

	memcpy(dstFrame+5, srcFrame, srcLen);
	
	dstFrame[srcLen + 5] = CRC16(dstFrame, srcLen+5, Hi);
	dstFrame[srcLen + 6] = CRC16(dstFrame, srcLen+5, Lo);

	(*dstLen) = srcLen + 7;

	return 0;
}

/*
* *******************************************************************************
* MODULE	: UnpackFrame
* ABSTRACT	: 获取帧数据内容
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int UnpackFrame(unsigned char *srcFrame, unsigned char *dstFrame, int *dstLen)
{
	/* 帧头(3B) + 长度(2B) + 数据(nB) + 校验(2B) */

	int length = srcFrame[3] * 256 + srcFrame[4] - 2;
	memcpy(dstFrame, srcFrame+5, length);

	(*dstLen) = length;

	return 0;
}


/*
* *******************************************************************************
* MODULE	: ProtDataCallBack
* ABSTRACT	: 数据接收回调函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static int ProtDataCallBack(unsigned char *pframe, int len)
{
	ZMQ_MSG_STRUCT *recvMsg = (ZMQ_MSG_STRUCT *)pframe;

	unsigned char srcFrame[TMP_BUF_LEN] = {0};
	int srcLen = 0;
	unsigned char dstFrame[TMP_BUF_LEN] = {0};
	int dstLen = 0;

	if (len != sizeof(ZMQ_MSG_STRUCT))
	{
		DEBUG("recvMsg error");
		return RESULT_ERR;
	}
 
	if (FilterBuffer(recvMsg->msg, recvMsg->length) == (-1))
	{
//		DEBUG("FilterBuffer error");
		return RESULT_ERR;
	}

	UnpackFrame(recvMsg->msg, srcFrame, &srcLen);

	AES(srcFrame, srcLen, dstFrame, &dstLen, MBEDTLS_AES_DECRYPT);

	// json数据包解析
	FrameDataUnpackFrame(dstFrame);
	// 用回调传到外层进行处理

	return RESULT_OK;
}

/*
* *******************************************************************************
* MODULE	: ProtDataSend
* ABSTRACT	: 数据打包发送函数
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
static void ProtDataSend(char *pubTheme, unsigned char *pframe, int len)
{
	unsigned char srcFrame[TMP_BUF_LEN] = {0};
	int srcLen = 0;
	ZMQ_MSG_STRUCT sendMsg = {0};

	if (len > (TMP_BUF_LEN-7-15))		// 考虑预留帧头7B，加密补位最多15B
	{
		return;
	}

	AES(pframe, len, srcFrame, &srcLen, MBEDTLS_AES_ENCRYPT);

	PackFrame(srcFrame, srcLen, sendMsg.msg, &sendMsg.length);
	strcpy(sendMsg.pubTheme, pubTheme);
	Zmq_Publish(sendMsg);
}

/*
* *******************************************************************************
*                                  Public functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendSetPwrResFrame
* ABSTRACT	: setPwrRes
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendSetPwrResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;
	SetPwrResPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_GW_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendGetPwrResFrame
* ABSTRACT	: getPwrRes
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendGetPwrResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;
	GetPwrResPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_GW_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendStatResFrame
* ABSTRACT	: statRes发送函数
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendStatResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;
	StatResPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_GW_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendHeartResFrame
* ABSTRACT	: heartRes发送函数
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendHeartResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;

	HeartResPackFrame(frame, sendFrame, &sendLen);

	ProtDataSend(PUB_UNIT_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendStatusFrame
* ABSTRACT	: status发送函数
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendStatusFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;

	StatusPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_UNIT_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendPwrResFrame
* ABSTRACT	: pwrRes发送函数
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendPwrResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;
	PwrResPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_UNIT_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_SendOTAResFrame
* ABSTRACT	: OTARes发送函数
* FUNCTION	: 
* ARGUMENT	: FRAME_DATA_STRUCT
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_SendOTAResFrame(FRAME_DATA_STRUCT frame)
{
	unsigned char sendFrame[TMP_BUF_LEN] = {0};
	int sendLen = 0;
	OTAResPackFrame(frame, sendFrame, &sendLen);
	ProtDataSend(PUB_UNIT_THEME, sendFrame, sendLen);
}

/*
* *******************************************************************************
* MODULE	: ZmqProt_Init
* ABSTRACT	: 初始化
* FUNCTION	: 
* ARGUMENT	: void
* NOTE		: 
* RETURN	: 
* CREATE	: 
*			: V0.01
* UPDATE	: 
* *******************************************************************************
*/
void ZmqProt_Init(HANDLE_STRUCT *pHandle)
{
	ZMQ_SETINFO_STRUCT zmqInfo = {0};

	zmqInfo.subIPC = SUB_IPC;
	zmqInfo.pubIPC = PUB_IPC;
	zmqInfo.proxyEn = 1;
	strcpy(zmqInfo.subTheme[0], SUB_TER_THEME);
	strcpy(zmqInfo.subTheme[1], SUB_WEB_THEME);
	zmqInfo.pCallBack = &ProtDataCallBack;

	g_pHandle = pHandle;

	Zmq_Init(zmqInfo);
}