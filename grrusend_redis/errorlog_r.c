/*
 * ����: ������־������������ - �������
 * 
 * �޸ļ�¼:
 * 2008-08-20 - ��־�� ����
 * 2015-01-04 - �½��� �޸ĳɿ������
 */
#include "ebdgdl.h"
//#include <stdio.h>
//#include <error.h>
#include <pthread.h>
#include "log.h"

/* 
 * ������¼������Ϣ�ļ�
 */
static FILE *pstruErrorLogFile = NULL;
static pthread_mutex_t struErrorLogMutex = PTHREAD_MUTEX_INITIALIZER; /* ��������־ */

/*
 * �õ���־ͷ 
 * ����������'R'��ʾ������(re-entrant)
 * @para[out] pszHead: ������־ͷ
 * @para[in] nLen: ָ����־ͷ����
 * @return: �ɹ� - ����0��ʧ�� - ����-1
 */
static INT CreateErrorLogHeadR(PSTR pszHead, INT nLen)
{
    struct tm struTmNow;
	time_t struTimeNow;
	STR szHead[MAX_STR_LEN] = {0};

	if (!pszHead)
		return -1;

	struct timeval struTmVal;

	if(time(&struTimeNow) == (time_t)(-1))
		return -1;
	gettimeofday(&struTmVal, NULL);

	localtime_r(&struTimeNow, &struTmNow);
	snprintf(szHead, sizeof(szHead), "[%04d-%02d-%02d][%02d:%02d:%02d.%06d][%ld]",
		struTmNow.tm_year+1900,struTmNow.tm_mon+1,struTmNow.tm_mday,struTmNow.tm_hour,
		struTmNow.tm_min,struTmNow.tm_sec,(INT)struTmVal.tv_usec,(LONG)getpid());

	strncpy(pszHead, szHead, nLen);
	return 0;
}

static INT GetProgNameR(PSTR pszProgName, INT nLen)
{
	INT nFindIt = 0;
	STR szProgName[MAX_PATH_LEN] = {0};
	STR szTmp[MAX_PATH_LEN] = {0};
	FILE *pstruFile;

	snprintf(szTmp, sizeof (szTmp), "/proc/%d/status", (INT)getpid());

	pstruFile = fopen(szTmp, "r");
	if (!pstruFile)
	{
		fprintf(stderr, "���ļ�ʧ��[%s][%s]\n",szTmp, strerror(errno));
		return -1;
	}

	while (fgets(szTmp, sizeof (szTmp), pstruFile))
	{
		szTmp[strlen(szTmp)-1] = '\0';
		if (strstr(szTmp, "Name:"))
		{
			nFindIt = 1;
			break;
		}
	}
	fclose(pstruFile);

	if (!nFindIt)
		return -1;

	sscanf(szTmp, "%*s%s", szProgName);
	strncpy(pszProgName, szProgName, nLen);
	return 0;
}

/*	
 * �õ���־�ļ������� 
 */
static INT GetErrorLogFileNameR(PSTR pszFileName, INT nLen)
{
	struct tm struTmNow;
	time_t struTimeNow;
	STR szFileName[MAX_PATH_LEN] = {0};
	STR szProgName[MAX_PATH_LEN] = {0};
	INT nRet;

	if (!pszFileName)
		return -1;

	nRet = GetProgNameR(szProgName, sizeof (szProgName));
	if (nRet < 0)
		return -1;

	if(time(&struTimeNow) == (time_t)(-1))
		return -1;
	
	localtime_r(&struTimeNow, &struTmNow);
	snprintf(szFileName, sizeof(szFileName), "%s-%04d%02d%02d.err",
		szProgName, struTmNow.tm_year + 1900,
		struTmNow.tm_mon + 1, struTmNow.tm_mday);			

	strncpy(pszFileName, szFileName, nLen);
	return 0;
}

static INT GetFileSizeR(PSTR pszFileName)
{
	struct stat struFileStat;

	if(stat(pszFileName,&struFileStat)!=0)
	{
		fprintf(stderr, "����ļ���Ϣ����[%s][%s]\n",pszFileName,strerror(errno));
		return -1;
	}

	return struFileStat.st_size;
}

/*
 * ���ɳ�ʼ������־�ļ�	
 */
static INT InitErrorLogFileR()
{
	STR szFileName[MAX_PATH_LEN];
	STR szPathName[MAX_PATH_LEN];
	STR szTmp[MAX_PATH_LEN];
	INT nRet;

	nRet = GetErrorLogFileNameR(szFileName, sizeof (szFileName));
	if (nRet < 0)
	{
		fprintf(stderr, "�޷��õ�������־���ļ�����\n");
		return -1;
	}
	if(getenv("HOME"))
		snprintf(szPathName, MAX_PATH_LEN, "%s/log/error/%s", getenv("HOME"), szFileName);
	else
		snprintf(szPathName, MAX_PATH_LEN, "/tmp/err/%s", szFileName);	

	//pthread_mutex_lock(&struErrorLogMutex);
   	if((pstruErrorLogFile != NULL)
   		&& (access(szPathName, F_OK|W_OK) == 0)
   		&& (GetFileSizeR(szPathName) < LOG_FILE_MAX_SIZE))
	{ /* һ������ */
		//pthread_mutex_unlock(&struErrorLogMutex);
		return 0;
	}

	/* ���쳣���ȹر��ļ������� */
	if(pstruErrorLogFile != NULL) 
	{
		fclose(pstruErrorLogFile);
		pstruErrorLogFile = NULL;
	}

	if((access(szPathName,F_OK|W_OK)==0)&&
		(GetFileSizeR(szPathName) >= LOG_FILE_MAX_SIZE))
    { /* ��־�ļ����������� */
		snprintf(szTmp, sizeof (szTmp), "%s.%d", szPathName, getpid());
		if (rename(szPathName, szTmp) != 0)
		{
			fprintf(stderr, "������־�ļ�����������[%s]", strerror(errno));
			//pthread_mutex_unlock(&struErrorLogMutex);
			return -1;
		}
    }

	/* ��ʽ����������־�ļ� */
	pstruErrorLogFile = fopen(szPathName, "a");
	if(pstruErrorLogFile == NULL)
	{
		fprintf(stderr, "����fopen����[%s]\n", strerror(errno));
		//pthread_mutex_unlock(&struErrorLogMutex);
		return -1;
	}

	if(chmod(szPathName, S_IRUSR|S_IWUSR)==-1)
	{
		fprintf(stderr, "����chmod����[%s]\n", strerror(errno));
		//pthread_mutex_unlock(&struErrorLogMutex);
		return -1;
	}
	//pthread_mutex_unlock(&struErrorLogMutex);

	return 0;
}

/**
 * PrintErrorLog() �Ŀ�����汾
 * ��ӡ������־��Ϣ�������ļ�
 *
 * pszDebugStr	Ҫ��ӡ��ͷ���ļ�����Ϣ
 * pszFormatStr Ҫ��ӡ���ַ����ĸ�ʽ��Ϣ
 *
 * Returns �޷���ֵ
 */
VOID PrintErrorLogR(PCSTR pszDebugStr, PCSTR pszFormatStr, ...)
{
	va_list listArg;
	STR szHead[MAX_STR_LEN] = {0};
	
	pthread_mutex_lock(&struErrorLogMutex);
	if(InitErrorLogFileR() == 0)
	{
		CreateErrorLogHeadR(szHead, sizeof (szHead));
		va_start(listArg, pszFormatStr);
		fprintf(pstruErrorLogFile, "%s%s\n\t", szHead, pszDebugStr);
		vfprintf(pstruErrorLogFile, pszFormatStr, listArg);
		va_end(listArg);
		fflush(pstruErrorLogFile);
	}
	pthread_mutex_unlock(&struErrorLogMutex);
}

/**
 * SetDebugLogHead() �Ŀ�����汾
 * �����Ƿ��ӡ������־��ͷ����Ϣ
 *
 * nHeadFlag 1Ҫ���ӡͷ����Ϣ 0��Ҫ���ӡͷ����Ϣ
 *
 * Returns �޷���ֵ
 */
VOID SetErrorLogHeadR(int nHeadFlag)
{
	return;
}
