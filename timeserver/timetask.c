/***
 * ����: ����ϵͳ��ʱ����������
 *
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"
#include "timeserver.h"

static	STR szServiceName[MAX_STR_LEN];
static	STR szDbName[MAX_STR_LEN];
static	STR szUser[MAX_STR_LEN];
static	STR szPwd[MAX_STR_LEN];

static int nApplPort;

static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(��ʱ�������)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName,pszProgName,pszProgName);
}

static PSTR GetSysDateTime2(VOID)
{
	static STR szNowDateTime[19 + 1];
	time_t struTimeNow;
	struct tm struTmNow;

	if(time(&struTimeNow)==(time_t)(-1))
	{
		PrintErrorLog(DBG_HERE,"�õ�ϵͳʱ����� %s\n",strerror(errno));
		return NULL;
	}

	memset(szNowDateTime,0,sizeof(szNowDateTime));
	struTmNow=*localtime(&struTimeNow);
	snprintf(szNowDateTime,sizeof(szNowDateTime),"%04d-%02d-%02d %02d:%02d",
		struTmNow.tm_year+1900,struTmNow.tm_mon+1,struTmNow.tm_mday, struTmNow.tm_hour,struTmNow.tm_min);

	return szNowDateTime; 
}

static PSTR GetNextTime(PSTR pszType, PSTR pszPeriod, PSTR pszTime)
{
	static STR szTime[21];
	struct tm struTmNow;
	time_t nTime;
	

	nTime =(int)time(NULL);
	struTmNow = *localtime(&nTime);
	
	if (strcmp(pszType, "1") == 0)
	    struTmNow.tm_mday  = struTmNow.tm_mday + 1;
    else if (strcmp(pszType, "2") == 0)
        struTmNow.tm_mday  = struTmNow.tm_mday + 7;
    else if (strcmp(pszType, "3") == 0)
        struTmNow.tm_mon = struTmNow.tm_mon + 1;
    else if (strcmp(pszType, "0") == 0)
    	struTmNow.tm_hour = struTmNow.tm_hour + 1 * atoi(pszPeriod);
    
    nTime=mktime(&struTmNow);
    
    struTmNow=*localtime(&nTime);
    
    if (strcmp(pszType, "0") == 0)
		snprintf(szTime, sizeof(szTime), "%04d-%02d-%02d %02d:%02d", 
		    struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, struTmNow.tm_hour, struTmNow.tm_min);
    else
    	snprintf(szTime, sizeof(szTime), "%04d-%02d-%02d %s", 
		    struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, pszTime);
   

	return szTime;
}

static time_t MakeITimeFromLastTime(PSTR pszLastTime)
{
	time_t nTime;
	struct tm struTmNow;

	STR szTemp[5];

	if(strlen(pszLastTime)!=19)
		return((time_t)(-1));	

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime,4);
	struTmNow.tm_year=atoi(szTemp)-1900;

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime+5,2);
	struTmNow.tm_mon=atoi(szTemp)-1;

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime+8,2);
	struTmNow.tm_mday=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime+11,2);
	struTmNow.tm_hour=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime+14,2);
	struTmNow.tm_min=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszLastTime+17,2);
	struTmNow.tm_sec=atoi(szTemp);

	nTime=mktime(&struTmNow);

	return nTime;
}

/**	
 * ��Ӧ�÷������XML���ݽ���
 */
static RESULT ExchDataApplSvr(PXMLSTRU pXmlReq)
{
	STR szTransBuffer[MAX_BUFFER_LEN];		/*	���ݽ�������	*/
	INT nConnectFd;

	/*
	 *	��������
	 */
	if((nConnectFd = CreateConnectSocket("127.0.0.1", nApplPort, 60)) < 0)
	{
		PrintErrorLog(DBG_HERE, \
			"ͬӦ�÷�����������Ӵ���,��ȷ��applserv�Ѿ�����\n");
		return EXCEPTION;
	}
	
	/*
	 *	XML���ݵ���
	 */	
	memset(szTransBuffer, 0, sizeof(szTransBuffer));
	ExportXml(pXmlReq, szTransBuffer, sizeof(szTransBuffer));
	PrintDebugLog(DBG_HERE,"���͵�Ӧ�÷�����[%s][%d]\n",
		szTransBuffer, strlen(szTransBuffer));
	
	/*
	 *	�������ݵ���Ϣ���������
	 */
	if(SendSocketWithSync(nConnectFd, szTransBuffer, strlen(szTransBuffer), 60) < 0)
	{
		PrintErrorLog(DBG_HERE, "�������ݵ�Ӧ�÷������\n");
		close(nConnectFd);
		return EXCEPTION;
	}
	
	/*
	 *	���շ�������Ӧ��
	 */
	memset(szTransBuffer, 0, sizeof(szTransBuffer));
	if(RecvSocketWithSync(nConnectFd, szTransBuffer, sizeof(szTransBuffer), 60) < 0)
	{
		PrintErrorLog(DBG_HERE, "��������Ӧ�÷����Ӧ���Ĵ���\n");
		close(nConnectFd);
		return EXCEPTION;
	}
	
	close(nConnectFd);
	
	if (memcmp(szTransBuffer, "0000", 4) !=0)
	{
	    PrintErrorLog(DBG_HERE,"���յ�Ӧ�÷���Ӧ����ʧ�ܣ�������[%s]\n", szTransBuffer);
		return EXCEPTION;
	}

	return NORMAL;	
}

//��������Ϊ��ѵ�������
static RESULT SetTaskStopUsing(int nTaskId, int nTskStyle)
{
    char szSql[MAX_BUFFER_LEN];
     
    //if (nTskStyle != 215)
    {
	    memset(szSql, 0, sizeof(szSql));
	    snprintf(szSql, sizeof(szSql), "update man_Task set tsk_state = %d  where tsk_Taskid= %d", 
	            NORMAL_STATE, nTaskId);
	     
	    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
	        return EXCEPTION;
		}
		CommitTransaction();
	}
	
	//����������־����ʱ��
	memset(szSql, 0, sizeof(szSql));
	/*
    snprintf(szSql, sizeof(szSql), "update man_TaskLog set TKL_ENDTIME = to_date( '%s','yyyy-mm-dd hh24:mi:ss')  where tkl_Taskid= %d and to_char(tkl_begintime,'yyyymmdd')  ='%s'", 
        GetSysDateTime(), nTaskId, GetSystemDate());
	*/
	//ͬһ����ͬ��������־�Ľ���ʱ�佫����Ϊͬһʱ��(BUG) mod by wwj at 2010.07.27
	snprintf(szSql, sizeof(szSql), 
			" update man_TaskLog set TKL_ENDTIME = to_date( '%s','yyyy-mm-dd hh24:mi:ss')"
			" where tkl_Taskid= %d and tkl_begintime = (select max(tkl_begintime) from man_TaskLog where tkl_taskid= %d)", 
        	GetSysDateTime(), nTaskId, nTaskId);
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	/*
	if (nTskStyle == 215)
	{
		memset(szSql, 0, sizeof(szSql));
    	snprintf(szSql, sizeof(szSql), "delete from man_Task where tsk_Taskid= %d", nTaskId);
    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
    	    return EXCEPTION;
		}
		CommitTransaction();
		
		memset(szSql, 0, sizeof(szSql));
    	snprintf(szSql, sizeof(szSql), "delete from man_taskdetail where tkd_Taskid= %d", nTaskId);
    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
    	    return EXCEPTION;
		}
		CommitTransaction();
	}
	*/
	if (nTaskId == 4 || nTaskId == 32)
	{
		memset(szSql, 0, sizeof(szSql));
    	snprintf(szSql, sizeof(szSql), "delete from ne_msgqueue where qs_taskid = %d and qs_msgstat = '0'", nTaskId);
    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
    	    return EXCEPTION;
		}
		CommitTransaction();
	}
	return NORMAL; 
}

//��������Ϊ������ѯ״̬���´���ѵʱ��
static RESULT SetTaskUsingAndTime(int nTaskId, PSTR pszPeriodTime, int nTskStyle)
{
    char szSql[MAX_BUFFER_LEN];
    char szTime[10], szType[10];
    char szPeriod[10];
    STR *pszField[10];
    INT nFieldNum;
    
    nFieldNum = SeperateString( pszPeriodTime, ',', pszField, 10);
    strcpy(szType, pszField[0]);
    strcpy(szPeriod, pszField[1]);
    strcpy(szTime, pszField[2]);
    memset(szSql, 0, sizeof(szSql));
    
    //if (nTskStyle != 215)
    {
	    snprintf(szSql, sizeof(szSql), "update man_Task set tsk_state = %d, tsk_lasttime = '%s', tsk_nexttime = '%s' where tsk_Taskid= %d", 
	            TASKRUNING_STATE, GetSysDateTime(), GetNextTime(szType, szPeriod, szTime), nTaskId);
	     
	    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
	        return EXCEPTION;
		}
		CommitTransaction();
	}
	return NORMAL; 
}

static RESULT ProcessTimeTaskWork()
{
    STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	XMLSTRU struReqXml;
	PXMLSTRU pstruReqXml=&struReqXml;
	STR szLastTime[30];
	STR szNextTime[30];
	STR szPeriodTime[30];
	INT nTskState, nTskStyle;
	INT nTaskId=-1, nFailTimes;
	INT nNextStep=0;
	

    //while(TRUE)
	{
	    sprintf(szSql,"select tsk_taskid,tsk_style, tsk_state, tsk_period,tsk_lasttime, tsk_nexttime, tsk_failtimes from man_Task where tsk_isuse = 0 ");
	    PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	//sleep(60);
	    	//CloseDatabase();
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        nTskState = atoi(GetTableFieldValue(&struCursor, "tsk_state"));
	        nTaskId = atoi(GetTableFieldValue(&struCursor, "tsk_taskid"));
	        nTskStyle = atoi(GetTableFieldValue(&struCursor, "tsk_style"));
	        strcpy(szPeriodTime, TrimAllSpace(GetTableFieldValue(&struCursor,"tsk_period")));
	        strcpy(szLastTime, TrimAllSpace(GetTableFieldValue(&struCursor,"tsk_lasttime")));
	        strcpy(szNextTime, TrimAllSpace(GetTableFieldValue(&struCursor,"tsk_nexttime")));
	        //������ʱ����
            nFailTimes = atoi(GetTableFieldValue(&struCursor, "tsk_failtimes"));
	        //����ִ��ʱ�䣬״̬����
	        if (nTskState == NORMAL_STATE)
	        {
	    PrintDebugLog(DBG_HERE,"[%s][%s]\n",szNextTime, GetSysDateTime2());
	            //�Ƚ��Ƿ�ִ��ʱ��
	            if (strcmp(szNextTime, GetSysDateTime2()) == 0)
	            {
	                /*
	                 *	��������XML
                     */
                    memset(pstruReqXml, 0, sizeof(struReqXml));
	                CreateXml(pstruReqXml, FALSE, OMC_ROOT_PATH, NULL);
	                if (nTskStyle == 215)
	                	InsertInXmlExt(pstruReqXml, "<omc>/<packcd>", "6003",
	                					MODE_AUTOGROW|MODE_UNIQUENAME);
	                else
	                	InsertInXmlExt(pstruReqXml, "<omc>/<packcd>", "6002",
	                					MODE_AUTOGROW|MODE_UNIQUENAME);
	                						
	                InsertInXmlExt(pstruReqXml, "<omc>/<taskid>", 
	                					GetTableFieldValue(&struCursor, "tsk_taskid"),
	                					MODE_AUTOGROW|MODE_UNIQUENAME);
	                InsertInXmlExt(pstruReqXml, "<omc>/<style>", 
	                					GetTableFieldValue(&struCursor, "tsk_style"),
	                					MODE_AUTOGROW|MODE_UNIQUENAME);
	                					
	                //�����񷢵�Ӧ�÷�����
                    if (ExchDataApplSvr(pstruReqXml) == NORMAL)
                        //��������Ϊ������ѯ״̬���´�ִ��ʱ��
                    {
                    	DeleteXml(pstruReqXml);
                        nNextStep = 1;
                        //break;
                        sleep(1);
	               		SetTaskUsingAndTime(nTaskId, szPeriodTime, nTskStyle); 
                    }
                    DeleteXml(pstruReqXml);
                }
	        }

	        //������������
	        if (nTskState == TASKRUNING_STATE) 
	        {
	            if ((int)time(NULL) - (int)MakeITimeFromLastTime(szLastTime) - nFailTimes * 60 >= 0)//�ж���������ʱ���Ƿ�
	            {
	                nNextStep = 2;
	                //break;
	                sleep(1);
                    SetTaskStopUsing(nTaskId, nTskStyle);
	            }    
	        }
            
	    }
	    FreeCursor(&struCursor);
    	/*
    	sprintf(szSql,"select tsk_taskid from man_task  where tsk_style = 215 and tsk_eventtime < sysdate - 1/12");
    	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
    	{
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	//sleep(60);
	    	//CloseDatabase();
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	   	{
	   		nTaskId = atoi(GetTableFieldValue(&struCursor, "tsk_taskid"));
	   		
	   		memset(szSql, 0, sizeof(szSql));
	    	snprintf(szSql, sizeof(szSql), "delete from man_Task where tsk_Taskid= %d", nTaskId);
	    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
			if(ExecuteSQL(szSql) != NORMAL)
			{
				PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
					szSql, GetSQLErrorMessage());
	    	    return EXCEPTION;
			}
						
			memset(szSql, 0, sizeof(szSql));
	    	snprintf(szSql, sizeof(szSql), "delete from man_taskdetail where tkd_Taskid= %d", nTaskId);
	    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
			if(ExecuteSQL(szSql) != NORMAL)
			{
				PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
					szSql, GetSQLErrorMessage());
	    	    return EXCEPTION;
			}
	   	}
	   	FreeCursor(&struCursor);
	   	
	   	CommitTransaction();
	   	*/
	}
	return NORMAL;
}


/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	
	fprintf(stderr,"���п�ʼ[%s]\n", GetSysDateTime());

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
	if(DaemonStart()!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"��ʱ���������̽����̨���д���\n");
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
	
//	if (GetCfgItem("applserv.cfg","APPLSERV","ListenPort1�1",szTemp) != NORMAL)
  //      return EXCEPTION;
    //nApplPort=atoi(szTemp);
    nApplPort=8801;
	
	ProcessTimeTaskWork();
	
	sleep(1);
	
	CloseDatabase();
	
	fprintf(stderr,"����[%s]\n", GetSysDateTime());
	
	return NORMAL;
}

