/* Usage:
 * FTP download:
 *    CurlDownloadFile("ftp://101.132.135.29/ini.h",
 *                "ftpuser:sx200788",
 *                 "/tmp/md_ini.h",
 *                 3,  30, 1);
 * SFTP download
 *    CurlDownloadFile("sftp://101.132.135.29/~/ini1.h",
 *                "testme:111111",
 *                "/tmp/sft_ini.h",
 *                 3,  30, 1);
 *
 * HTTP(s) download:
 *   CurlDownloadFile("http(s)://www.xxx.com/category.php?id=360",
 *				    "",
 *					"/tmp/http.h",
 *					 3,  30, 1);
 *
 * SFTP upload:
 *   CurlUploadFile("sftp://101.132.135.29/~/ini1.h", "testme:111111", "./ini.h", 3, 30, 1);
 *
 * FTP upload:
 *   CurlUploadFile("ftp://101.132.135.29/ini1.h", "ftpuser:sx200788",  "./ini.h", 3, 30, 1);
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include "log_sys.h"

/* <DESC>
 * Get a single file from a server.
 * </DESC>
 */
static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	size_t writeSize;

	writeSize = fwrite(buffer, size, nmemb, stream);
	log_send("writeSize = %d", writeSize);
	return writeSize;
}

int  CurlDownloadFile(char *szCurlUrl,
                      char *pUsernamePassword,
                      char * fpDownloadFile,
                      int  nConnectTimeout, int nTimeout, char isPassiveMode)
{
	CURL * curl;
	FILE *outfile;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (!curl)
	{
		printf("curl error!\r\n");
		return -1;
	}

	curl_easy_reset(curl);
	outfile = fopen(fpDownloadFile, "wb");
	if (NULL == outfile)
	{
		curl_easy_cleanup(curl);
		return -1;
	}

	//szCurlUrl需满足格式如：scheme://host:port/path。
	curl_easy_setopt(curl, CURLOPT_URL, szCurlUrl);
	//curl_easy_setopt(curl, CURLOPT_PORT, usPort);

	/*user & pwd*/
	if (strlen(pUsernamePassword))
	{
		curl_easy_setopt(curl, CURLOPT_USERPWD, pUsernamePassword);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);//暂时不对服务器传过来的证书进行验证

	/*
	如果是sft，并且无密码，则使用验证公钥登陆，如果账号，需设置账号/私钥放在/ubi/conf/ocpp/sftp_rsa_key
	 */
	if (strstr(szCurlUrl, "sftp") != NULL && strstr(pUsernamePassword, ":") == NULL)
	{
		if (strlen(pUsernamePassword))
		{
			curl_easy_setopt(curl, CURLOPT_USERNAME, pUsernamePassword);
		}
		curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, "/ubi/conf/ocpp/sftp_rsa_key");
		curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
	}


	if (!isPassiveMode)
	{
		curl_easy_setopt(curl, CURLOPT_FTPPORT, "-"); /* disable passive mode */
	}

	CURLcode res = curl_easy_perform(curl);

	fclose(outfile);
	/* always cleanup */
	curl_easy_cleanup(curl);

	/* Check for errors */
	if (res != CURLE_OK)
	{
		log_send("curl get error:%s \r\n", curl_easy_strerror(res));
		return -1;
	}

	return 0;
}

#undef DISABLE_SSH_AGENT

size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	// curl_off_t nread;
	size_t retcode = fread(ptr, size, nmemb, (FILE*)(stream));
	// nread = (curl_off_t)retcode;

	return retcode;
}

int  CurlUploadFile(char *szCurlUrl,
                    char *pUsernamePassword,
                    char *fpLocalFile,
                    int   nConnectTimeout, int nTimeout,
                    int isPassiveMode)
{
	CURL *curl;
	CURLcode res;
	char ret = 0;
	FILE* pSendFile = fopen(fpLocalFile, "rb");
	if (!pSendFile)
	{
		return -1;
	}

	fseek(pSendFile, 0L, SEEK_END);
	size_t iFileSize = ftell(pSendFile);
	fseek(pSendFile, 0L, SEEK_SET);

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, szCurlUrl);
		if (strlen(pUsernamePassword))
		{
			if (strstr(pUsernamePassword, ":") == NULL)
			{
				curl_easy_setopt(curl, CURLOPT_USERNAME, pUsernamePassword);
			}
			else
			{
				curl_easy_setopt(curl, CURLOPT_USERPWD, pUsernamePassword);
			}
		}
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, pSendFile);
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, iFileSize);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnectTimeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);//暂时不对服务器传过来的证书进行验证
		/*
		如果是sft，并且无密码，则使用验证公钥登陆，如有账号，需设置账号/私钥放在/ubi/conf/ocpp/sftp_rsa_key
		 */
		if (strstr(szCurlUrl, "sftp") != NULL && strstr(pUsernamePassword, ":") == NULL)
		{
			if (strlen(pUsernamePassword))
			{
				curl_easy_setopt(curl, CURLOPT_USERNAME, pUsernamePassword);
			}
			curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, "/ubi/conf/ocpp/sftp_rsa_key");
			curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
		}

		if (!isPassiveMode)
		{
			curl_easy_setopt(curl, CURLOPT_FTPPORT, "-"); /* disable passive mode */
		}

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);

		if (CURLE_OK != res)
		{
			log_send("curl told us %s\n", curl_easy_strerror(res));
			ret = -1;
		}
	}

	fclose(pSendFile);
	curl_global_cleanup();
	return ret;
}

