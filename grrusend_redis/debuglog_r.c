/*
 * ����: ������־������������
 * 
 * �޸ļ�¼:
 * 2008-08-20 - ��־�� ����
 */
#include "ebdgdl.h"
//#include <stdio.h>
//#include <error.h>
#include <pthread.h>
#include "log.h"

/* 
 * ������¼������Ϣ�ļ� 
 */
static FILE *pstruDebugLogFile = NULL;
static pthread_mutex_t struDebugLogMutex = PTHREAD_MUTEX_INITIALIZER; /* ��������־ */

/* 
 * ������־��׼̨ͷ��־
 */
static BOOL nDebugLogHead = BOOLTRUE;

/*	
 * �õ�������Ϣ������س�������	
 */
static PSTR GetDebugProgName()
{
	static STR szProgname[MAX_PATH_LEN];
	static BOOL nInitFlag = BOOLFALSE;

	if(nInitFlag == BOOLTRUE)
		return szProgname;
		
	snprintf(szProgname, MAX_PATH_LEN, "%s/log/debug/%s", getenv("HOME"),
		GetProgName());
	nInitFlag = BOOLTRUE;
	
	return szProgname;
}

/*	
 * ���ɵ�����־��Ϣͷ 
 */
static PCSTR CreateDebugLogHead()
{
    struct tm struTmNow;
	time_t struTimeNow;
	static STR szHead[MAX_STR_LEN];

	struct timeval struTmVal;

	if(time(&struTimeNow)==(time_t)(-1))
		return NULL;
	gettimeofday(&struTmVal,NULL);

	memset(szHead,0,sizeof(szHead));
	struTmNow=*localtime(&struTimeNow);
	snprintf(szHead,MAX_STR_LEN,"[%04d-%02d-%02d][%02d:%02d:%02d.%06d][%ld]",
		struTmNow.tm_year+1900,struTmNow.tm_mon+1,struTmNow.tm_mday,struTmNow.tm_hour,
		struTmNow.tm_min,struTmNow.tm_sec,(INT)struTmVal.tv_usec, (LONG)getpid());

	return(szHead);

}

/*	
 * �õ�������־�ļ������� 
 */
static PCSTR GetDebugLogFileName()
{
	struct tm struTmNow;
	time_t struTimeNow;
	static STR szFileName[MAX_PATH_LEN];

	if(time(&struTimeNow) == (time_t)(-1))
		return NULL;
	
	memset(szFileName, 0, sizeof(szFileName));
	struTmNow = *localtime(&struTimeNow);
	snprintf(szFileName, MAX_PATH_LEN, "%s-%04d%02d%02d.dbg",
		GetProgName(),  struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, \
		struTmNow.tm_mday);
		
	return szFileName;
}

static INT GetFileSize(PSTR pszFileName)
{
	struct stat struFileStat;

	if(stat(pszFileName,&struFileStat)!=0)
		return -1;

	return struFileStat.st_size;
}

/*	
 * ��ʼ���ĵ�����־�ļ�
 */
static BOOL InitDebugLogFile()
{	
	static STR szFileName[MAX_PATH_LEN];
	STR szShellCmd[MAX_PATH_LEN];
	
   	if((pstruDebugLogFile != NULL) 
   		&& (access(szFileName, F_OK|W_OK) == 0)
   		&& (GetFileSize(szFileName)<LOG_FILE_MAX_SIZE)
		&& (strcmp(szFileName, GetDebugLogFileName()) == 0))
		return BOOLTRUE;

	memset(szFileName, 0, sizeof(szFileName));
	if(getenv("HOME"))
	{
		snprintf(szFileName, MAX_PATH_LEN, "%s/log/debug", getenv("HOME"));
		if((mkdir(szFileName, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
			&& (errno != EEXIST))
		{
			return BOOLFALSE;
		}		
		snprintf(szFileName, MAX_PATH_LEN, "%s/log/debug/%s", \
			getenv("HOME"), GetDebugLogFileName());
	}
	else
	{
		snprintf(szFileName, MAX_PATH_LEN, "%s/debug","/tmp");
		if((mkdir(szFileName, S_IRUSR|S_IWUSR|S_IXUSR) == -1) 
			&& (errno != EEXIST))
		{
			return BOOLFALSE;
		}				
		snprintf(szFileName, MAX_PATH_LEN, "%s/debug/%s","/tmp", \
			GetDebugLogFileName());	
	}
	
	if(strstr(szFileName, "other-"))
		return BOOLFALSE;
		
	if(pstruDebugLogFile != NULL) 
		fclose(pstruDebugLogFile);
	
	if((access(szFileName,F_OK|W_OK)==0)&&
		(GetFileSize(szFileName) >=LOG_FILE_MAX_SIZE))
    {
    	memset(szShellCmd,0,sizeof(szShellCmd));
    	srand((unsigned)time(NULL));
		snprintf(szShellCmd,sizeof(szShellCmd),"mv %s %s.%d", szFileName, szFileName, rand()%100+1);
 		system(szShellCmd);
    }
    	
	pstruDebugLogFile = fopen(szFileName,"a");
	if(pstruDebugLogFile == NULL)
	{
		return BOOLFALSE;
	}
	if(chmod(szFileName,S_IRUSR|S_IWUSR)==-1)
	{
		return BOOLFALSE;
	}	
	return BOOLTRUE;
}

/**
 * PrintDebugLog() ���̰߳�ȫ�棬������
 * ��ӡ������־��Ϣ�������ļ�
 *
 * pszDebugStr	Ҫ��ӡ��ͷ���ļ�����Ϣ
 * pszFormatStr Ҫ��ӡ���ַ����ĸ�ʽ��Ϣ
 *
 * Returns �޷���ֵ
 */
VOID PrintDebugLogR(PCSTR pszDebugStr, PCSTR pszFormatStr, ...)
{
	va_list listArg;

	pthread_mutex_lock(&struDebugLogMutex);
	if(access(GetDebugProgName(), F_OK)==0)
	{
		if(InitDebugLogFile() == BOOLTRUE)
		{
			va_start(listArg, pszFormatStr);
			if(nDebugLogHead)
			{
				fprintf(pstruDebugLogFile, "%s%s\n\t", CreateDebugLogHead(), \
					pszDebugStr);
			}
			
			vfprintf(pstruDebugLogFile, pszFormatStr, listArg);
			va_end(listArg);
			fflush(pstruDebugLogFile);
		}
	}
	pthread_mutex_unlock(&struDebugLogMutex);
}

/**
 * PrintHexDebugLog() ���̰߳�ȫ�汾��������
 * ��16��չ��ӡ������־��Ϣ�������ļ�
 *
 * pszDebugStr	Ҫ��ӡ��ͷ���ļ�����Ϣ
 * pszPrintBuf Ҫ��ӡ������
 * nPrintLen ���ݵĳ���
 *
 * Returns �޷���ֵ
 */
VOID PrintHexDebugLogR(PCSTR pszDebugStr, PCSTR pszPrintBuf, UINT nPrintLen)
{
	register int i,j;
	register int nRowNo;
	UCHAR cTemp;

	pthread_mutex_lock(&struDebugLogMutex);
	if((access(GetDebugProgName(), F_OK) !=0) 
		||(InitDebugLogFile() != BOOLTRUE))
	{
		pthread_mutex_unlock(&struDebugLogMutex);
		return;
	}
		
	if(nDebugLogHead)
	{
		fprintf(pstruDebugLogFile, "%s%s\n", \
			CreateDebugLogHead(), pszDebugStr);
	}
		
	nRowNo = nPrintLen / 16;
	for(i = 0; i < nRowNo; i ++)
	{
		fprintf(pstruDebugLogFile, "%08X | ", i * 16);		
		for(j = 0; j < 16; j ++)
		{
			fprintf(pstruDebugLogFile, "%02X ",
				*(PUCHAR)(pszPrintBuf + i * 16 + j));
		}
		fprintf(pstruDebugLogFile,"| ");
		for(j=0;j<16;j++)
		{
			cTemp = *(PUCHAR)(pszPrintBuf + i * 16 + j);
			cTemp = cTemp > 32 ? cTemp : '.';
			fprintf(pstruDebugLogFile, "%c", cTemp);
		}
		fprintf(pstruDebugLogFile,"\n");
	}
	
	nRowNo = nPrintLen % 16;
	if(nRowNo)
	{
		fprintf(pstruDebugLogFile, "%08X | ", i * 16);
		for(j = 0; j < nRowNo; j ++)
		{
			fprintf(pstruDebugLogFile, "%02X ",
				*(PUCHAR)(pszPrintBuf + i * 16 + j));
		}
		while(j ++ < 16)
			fprintf(pstruDebugLogFile,"   ");
		fprintf(pstruDebugLogFile,"| ");
		for(j = 0; j < nRowNo; j ++)
		{
			cTemp = *(PUCHAR)(pszPrintBuf + i * 16 + j);
			cTemp = cTemp > 32 ? cTemp : '.';
			fprintf(pstruDebugLogFile, "%c", cTemp);
			fflush(pstruDebugLogFile);
		}
		while(j ++ < 16)
			fprintf(pstruDebugLogFile, " ");
		fprintf(pstruDebugLogFile, "\n");
	}	
	fflush(pstruDebugLogFile);	

	pthread_mutex_unlock(&struDebugLogMutex);
}

/**
 * SetDebugLogHead
 * �����Ƿ��ӡ������־��ͷ����Ϣ
 *
 * nHeadFlag	1Ҫ���ӡͷ����Ϣ 0��Ҫ���ӡͷ����Ϣ
 *
 * Returns �޷���ֵ
 */
VOID SetDebugLogHead(int nHeadFlag)
{
	if(nHeadFlag)
		nDebugLogHead = BOOLTRUE;
	else
		nDebugLogHead = BOOLFALSE;
}

/**
 * GetProgName
 * ��õ�ǰ���н��̵�����
 *
 *
 * Returns ���̵�����
 */
#ifdef SYSTEM_AIX
PSTR GetProgName()
{
    struct procsinfo    stProcsinfo;
    INT                 n;
    static STR szProgname[MAX_PATH_LEN];
    static BOOL         nInitFlag = FALSE;
    pid_t               pid;
    if(nInitFlag == TRUE)
            return szProgname;
    pid = getpid();
    n=0;
    while (getprocs(&stProcsinfo,sizeof(stProcsinfo),NULL,0,&n,1) > 0)
    {
        if (stProcsinfo.pi_pid == pid)
        {
            nInitFlag = TRUE;
            memset(szProgname,0,sizeof(szProgname));
            strcpy(szProgname,stProcsinfo.pi_comm);
            return szProgname;
        }
    }
    return "other";
}
#else
PSTR GetProgName()
{
	STR szShellCmd[MAX_STR_LEN];
	static STR szProgname[MAX_PATH_LEN];
	static BOOL nInitFlag = FALSE;
	FILE *pstruPipeFile;

	if(nInitFlag == BOOLTRUE)
		return szProgname;
	snprintf(szShellCmd, MAX_STR_LEN, \
		"ps -el | awk ' $4 == \"%d\" { print $14 }'", (INT )getpid());
	pstruPipeFile = popen(szShellCmd, "r");
	if(pstruPipeFile == NULL)
	{
		fprintf(stderr,"popen(%s)\n",szShellCmd);
		return "other";
	}

	memset(szProgname, 0, sizeof(szProgname));
	fgets(szProgname, MAX_PATH_LEN, pstruPipeFile);
	szProgname[strlen(szProgname) - 1] = '\0';
	pclose(pstruPipeFile);
	
	if(strlen(szProgname) < 1)
	{
		fprintf(stderr,"strlen(szProgname)��(%s)(%s) < 1\n",szProgname,szShellCmd);
		strcpy(szProgname, "other");
	}
	else
		nInitFlag = BOOLTRUE;
		
	return szProgname;
}
#endif

