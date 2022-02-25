/***
 * ����: ����ϵͳ��ʱ����������
 * ��ʱ����ÿ���Сʱִ��һ��
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"
#include <net/if.h>
#include <sys/ioctl.h>

static	STR szServiceName[MAX_STR_LEN];
static	STR szDbName[MAX_STR_LEN];
static	STR szUser[MAX_STR_LEN];
static	STR szPwd[MAX_STR_LEN];


static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(��ʱ�������)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName,pszProgName,pszProgName);
}

char* GetLocalIp()  
{        
    int MAXINTERFACES=16;  
    char *ip = NULL;  
    int fd, intrface, retn = 0;    
    struct ifreq buf[MAXINTERFACES];    
    struct ifconf ifc;    

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)    
    {    
        ifc.ifc_len = sizeof(buf);    
        ifc.ifc_buf = (caddr_t)buf;    
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))    
        {    
            intrface = ifc.ifc_len / sizeof(struct ifreq);    

            while (intrface-- > 0)    
            {    
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))    
                {    
                    ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));    
                    break;  
                }                        
            }  
        }    
        close (fd);    
    }
    return ip;  
}

RESULT SaveAlarmLog(int nAlarmType, PSTR pszTitle, PSTR pszContent)
{
    char szSql[MAX_BUFFER_LEN];
    char szHostName[100];
    
    gethostname(szHostName,sizeof(szHostName));
      
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "insert into alm_systemlog (alm_typeid, alm_typename, alm_ip, alm_computername, alm_time, alm_info)"
    " values (%d, '%s', '%s', '%s',to_date( '%s','yyyy-mm-dd hh24:mi:ss'),  '%s')", 
     nAlarmType, pszTitle, GetLocalIp(), szHostName, GetSysDateTime(), pszContent);
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	
	return NORMAL; 
}


BOOL ExistAlarmLog(int nAlarmId)
{
	CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	INT nCount;
	
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select count(alg_NeId) as v_count from alm_AlarmLog where alg_AlarmId= %d and alg_alarmstatusid = 1",  nAlarmId);
	//PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
	    FreeCursor(&struCursor);
	    PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    				  szSql, GetSQLErrorMessage());
	    return EXCEPTION;

	}
	nCount = atoi(GetTableFieldValue(&struCursor, "v_count"));
	FreeCursor(&struCursor);
	if (nCount > 0)
	    return BOOLTRUE;
	else
	    return BOOLFALSE;
}


static RESULT ProcessCpuAlarmWork()
{
	FILE *fp;
    int  ret, nCount = 0;
    int nIde=0, nAlarmIdeValue;
    char szBuffer[100];
    STR szArgName[100], szTemp[100], szHostName[100];
    char szSql[MAX_BUFFER_LEN];
    
    if (GetCfgItem("systemalarm.cfg","ALARM", "CpuIde",szTemp) != NORMAL)
    {
    	PrintErrorLog(DBG_HERE,"���ø澯��������\n");
		return EXCEPTION;
    }
    nAlarmIdeValue= atoi(szTemp);
    
    memset(szBuffer, 0, sizeof(szBuffer));
	sprintf(szBuffer, "vmstat 1 10|awk '{print $15}'");
                                                                            
    if ((fp = popen(szBuffer, "r")) == NULL) 
    {
		fprintf(stderr, "clearprg error!\n");
		exit(1);
    }
    sleep(10);
    while(!feof(fp))
    {
        memset(szBuffer, 0, sizeof(szBuffer));
 
        fgets(szBuffer,sizeof(szBuffer),fp);
        
		if (strcmp(szBuffer, "id") != 0)
		{
			nIde = nIde + atoi(szBuffer);
            nCount ++;
        }
       
    }
    pclose(fp);
    
    int nAverage = nIde/nCount;
    if (nAverage < nAlarmIdeValue)//���ڸ澯ֵ,cpu�澯
    {
    	gethostname(szHostName,sizeof(szHostName));
    	sprintf(szBuffer, "%s,%s,cpu used %d %%", szHostName, GetLocalIp(),  100-nAlarmIdeValue);
    	
    	SaveAlarmLog(1, "cpu alarm", szBuffer);
    	
    }
    /*
    else if (ExistAlarmLog() == NORMAL)
    {
    	DeleAlarmLog();
    }*/
    
    return NORMAL;
}

int IsNumber(char *str)
{
	int i;
	for ( i = 0 ; i < strlen(str) ; i ++)
		if( !isdigit(str[i]))
			return -1;
	return 0;
}


static RESULT ProcessDiskAlarmWork()
{
	FILE *fp;
    int  ret, nCount = 0;
    int nIde=0, nDiskAlarmValue;
    char szBuffer[100];
    int nSeperateNum;
    PSTR pszParamStr[5];
    STR szArgName[100], szTemp[100], szHostName[100];
    char szSql[MAX_BUFFER_LEN];
    
    if (GetCfgItem("systemalarm.cfg","ALARM", "DiskUse",szTemp) != NORMAL)
    {
    	PrintErrorLog(DBG_HERE,"���ø澯��������\n");
		return EXCEPTION;
    }
    nDiskAlarmValue= atoi(szTemp);
    
    memset(szBuffer, 0, sizeof(szBuffer));
	sprintf(szBuffer, "df |awk '{print $5$6}'");
                                                                            
    if ((fp = popen(szBuffer, "r")) == NULL) 
    {
		fprintf(stderr, "clearprg error!\n");
		exit(1);
    }
    sleep(10);
    while(!feof(fp))
    {
        memset(szBuffer, 0, sizeof(szBuffer));
 
        fgets(szBuffer,sizeof(szBuffer),fp);
        
        nSeperateNum = SeperateString(szBuffer,  '%', pszParamStr, 5);
        if (IsNumber(pszParamStr[0])==0)
		{
			if (atoi(pszParamStr[0]) > nDiskAlarmValue)//���̸澯
			{
		    	
    			
    			gethostname(szHostName,sizeof(szHostName));
		    	sprintf(szBuffer, "%s, %s,disk used %d%%", szHostName, GetLocalIp(), atoi(pszParamStr[0]));
		    	
		    	SaveAlarmLog(2, "disk alarm", szBuffer);
		    	
		    	
		    }
		    /*
		    else if (ExistAlarmLog() == NORMAL)
		    {
		    	DeleAlarmLog();
		    }*/
        }
        break;
       
    }
    pclose(fp);
    
    return NORMAL;
}


static RESULT ProcessDatabaseAlarmWork()
{
	STR szServiceName[MAX_STR_LEN];
	STR szDbName[MAX_STR_LEN];
	STR szUser[MAX_STR_LEN];
	STR szPwd[MAX_STR_LEN];
    
    if(GetDatabaseArg(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		strcpy(szServiceName,"omc");
		strcpy(szDbName,"omc");
		strcpy(szUser,"omc");
		strcpy(szPwd,"omc");
	}
	
	if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"�����ݿ���� [%s]\n",GetSQLErrorMessage());
		
		//�����������ӳ���澯
		
		return EXCEPTION;
	}
	
	
	
	CloseDatabase();
    
    return NORMAL;
}

/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	
	fprintf(stderr,"\t��ӭʹ������ϵͳ(��ʱ�������)\n");

	if(argc!=2)
	{
		Usage(argv[0]);
		return EXCEPTION;
	}
	if(strcmp(argv[1],"stop")==0)
	{
	    sprintf(szTemp, "clearprg %s", argv[0]);
		system(szTemp);
		//StopPrg(argv[0]);
		return NORMAL;
	}
	if(strcmp(argv[1],"start")!=0)
	{
		Usage(argv[0]);
		return NORMAL;
	}

	if(TestPrgStat(argv[0])==NORMAL)
	{
		fprintf(stderr,"��ʱ�����������Ѿ�����\n");
		return EXCEPTION;
	}
	
	if(GetDatabaseArg(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		strcpy(szServiceName,"omc");
		strcpy(szDbName,"omc");
		strcpy(szUser,"omc");
		strcpy(szPwd,"omc");
	}
	
	if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"�����ݿ���� [%s]\n",GetSQLErrorMessage());
		
		return EXCEPTION;
	}
	
	ProcessCpuAlarmWork();
	ProcessDiskAlarmWork();
	//ProcessDatabaseAlarmWork();
	
	CloseDatabase();
	
	fprintf(stderr,"���н���[%s]\n", GetSysDateTime());
	
	return NORMAL;
}

