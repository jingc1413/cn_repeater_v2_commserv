/***
 * ����: ����ϵͳ��ʱ����������
 *
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"
#include "timeserver.h"
#include "cJSON.h"
#include <hiredis/hiredis.h>
/*
 * ȫ�ֱ�������
 */
static INT nYiYangPort;						/* ����������˿� */
static STR szYiYangIpAddr[MAX_STR_LEN];		/* �����������ַ */
static INT nYiConnectFd=-1;
static int nApplPort;

static	STR szServiceName[MAX_STR_LEN];
static	STR szDbName[MAX_STR_LEN];
static	STR szUser[MAX_STR_LEN];
static	STR szPwd[MAX_STR_LEN];

static  char szRedisIp[20];
static  int nRedisPort;
static  char szAuthPwd[20];

static  char szRabbitIp[20];
static  int nRabbitPort;
static  char szExchange[100];
static  char szRoutingKey[20];
static  char szRabbitUser[20];
static  char szRabbitPwd[20];
static	redisContext *redisconn=NULL;


RESULT InitRedisMQ_cfg()
{
	char szTemp[10];
	
	if (GetCfgItem("redisMQ.cfg","redis", "RedisIp", szRedisIp) != NORMAL)
        strcpy(szRedisIp, "127.0.0.1");
    if (GetCfgItem("redisMQ.cfg","redis", "RedisPort", szTemp) != NORMAL)
        strcpy(szTemp, "6379");
    nRedisPort=atoi(szTemp);
    if (GetCfgItem("redisMQ.cfg","redis", "AuthPwd", szAuthPwd) != NORMAL)
        strcpy(szAuthPwd, "sunwave123");
    
	if (GetCfgItem("redisMQ.cfg","rabbitMQ", "RabbitIp", szRabbitIp) != NORMAL)
        strcpy(szRabbitIp, "127.0.0.1");
    if (GetCfgItem("redisMQ.cfg","rabbitMQ", "RabbitPort", szTemp) != NORMAL)
        strcpy(szTemp, "5672");
    nRabbitPort=atoi(szTemp);
    if (GetCfgItem("redisMQ.cfg","rabbitMQ", "RabbitUser", szRabbitUser) != NORMAL)
        strcpy(szRabbitUser, "guest");
    if (GetCfgItem("redisMQ.cfg","rabbitMQ", "RabbitPwd", szRabbitPwd) != NORMAL)
        strcpy(szRabbitPwd, "guest");
      
    if (GetCfgItem("redisMQ.cfg","rabbitMQ", "exchange", szExchange) != NORMAL)
        strcpy(szExchange, "amq.direct");
    if (GetCfgItem("redisMQ.cfg","rabbitMQ", "routingkey", szRoutingKey) != NORMAL)
        strcpy(szRoutingKey, "man_eleqrylog");
    
   
    
	return NORMAL;
}

int RedisPingInterval(int timeout)
{
    static time_t last = 0;
    time_t nowtime;
	redisReply *reply;
	
    if (timeout <= 0)
        return 0;

    nowtime = time(NULL);
    if (nowtime - last < timeout)
        return 0;

    last = nowtime;
    reply = redisCommand(redisconn,"ping");
	//printf("ping:%d %s\n", reply->type, reply->str);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis ping error: %s\n", redisconn->errstr);
		return -1;
	}
	
	if (reply->type==REDIS_REPLY_STATUS && strcmp(reply->str, "PONG")==0)
	{
		freeReplyObject(reply);
		return 0;
	}
    else 
    {
    	PrintErrorLog(DBG_HERE, "Redis ping error: %d,%s\n", reply->type,reply->str);
    	freeReplyObject(reply);
		return -1;
    }
	
    return 0;

}

RESULT ConnectRedis()
{
	 redisReply *reply;
	 struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	 
     redisconn = redisConnectWithTimeout(szRedisIp, nRedisPort, timeout);
     if (redisconn == NULL || redisconn->err) {
		if (redisconn) {
             PrintErrorLog(DBG_HERE, "Connection error: %s\n", redisconn->errstr);
             redisFree(redisconn);
		} else {
             PrintErrorLog(DBG_HERE, "Connection error: can't allocate redis context\n");
		}
		return EXCEPTION;
     }
     reply = redisCommand(redisconn, "AUTH %s", szAuthPwd);
     PrintDebugLog(DBG_HERE, "AUTH: %s\n", reply->str);
     freeReplyObject(reply);
     
	 return NORMAL;
}

RESULT FreeRedisConn()
{
	if (redisconn!=NULL) //10.25
		redisFree(redisconn);
	return NORMAL;
}



static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(��ʱ�������)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName,pszProgName,pszProgName);
}


// �˳�����
static VOID SigHandle(INT nSigNo) 
{
	if(nSigNo!=SIGTERM)
		return;
	
    CloseDatabase();
    //close(nYiConnectFd);
    FreeRedisConn();
    PrintDebugLog(DBG_HERE,"�ر����ݿ�ɹ�\n");
	exit(0);
}

PSTR TrimRightOneChar(PSTR pszInputString,CHAR cChar)
{
	INT i;

	if(pszInputString==NULL)
		return NULL;

	for(i=strlen(pszInputString)-1;(i>=0)&&(pszInputString[i]==cChar);i--)
	{
		pszInputString[i]=0;
		break;
	}

	return pszInputString;
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
	{
	    struTmNow.tm_mday  = struTmNow.tm_mday + 1;
	    //if (struTmNow.tm_wday == 5)
	    	//struTmNow.tm_mday  = struTmNow.tm_mday + 2;
	}
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


static RESULT InitYiYangInfo(VOID)
{
	STR szTemp[MAX_STR_LEN];
	STR szFile[MAX_PATH_LEN];
	PFILE pFile;

	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;

	sprintf(szFile,"%s/etc/yiyang.cfg",getenv("HOME"));
	pFile=fopen(szFile,"r");
	if(pFile==NULL)
	{
		PrintErrorLog(DBG_HERE,"��Mas Ftp�����ļ�[%s]ʧ�� [%s]\n",szFile,strerror(errno));
		return EXCEPTION;
	}

	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXmlFromFile(pstruXml,FALSE,pFile)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"���ļ�[%s]����������Ϣʧ��\n",szFile);
		DeleteXml(pstruXml);
		fclose(pFile);
		return EXCEPTION;
	}

	fclose(pFile);

	if(DemandInXmlExt(pstruXml,"<����>/<�����ַ>",szYiYangIpAddr,sizeof(szYiYangIpAddr))!=NORMAL)
	{

		PrintErrorLog(DBG_HERE,"����ǰת�����ַ���ô���\n");
		DeleteXml(pstruXml);
		return EXCEPTION;
	}

	if((DemandInXmlExt(pstruXml,"<����>/<����˿�>",szTemp,sizeof(szTemp))!=NORMAL)
	    || ((nYiYangPort=atoi(szTemp))<=0))
	{
		PrintErrorLog(DBG_HERE,"����ǰת����˿����ô���\n");
		DeleteXml(pstruXml);
		return EXCEPTION;
	}
	
    DeleteXml(pstruXml);
	return NORMAL;
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
	PrintTransLog(DBG_HERE,"���͵�Ӧ�÷�����[%s][%d]\n",
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
    CURSORSTRU struCursor;
	int nTaskLogId, nRxCount, nEleCount;
	char szSysDateTime[30];
    
    strcpy(szSysDateTime, GetSysDateTime());
    
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
	
	memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "SELECT tkl_tasklogid FROM  man_TaskLog  WHERE tkl_Taskid= %d  AND tkl_begintime >= (SELECT tsk_lasttime FROM man_task WHERE tsk_taskid = %d) ORDER BY tkl_tasklogid DESC LIMIT 1", 
       	nTaskId, nTaskId);
    
    PrintDebugLog(DBG_HERE,"Execute SQL[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"Execute SQL[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	if (FetchCursor(&struCursor) == NORMAL)
	{
        nTaskLogId= atoi(GetTableFieldValue(&struCursor,"tkl_tasklogid"));
	}
	FreeCursor(&struCursor);
	
	//У����������
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "SELECT COUNT(*) AS nrxcount FROM man_eleqrylog WHERE qry_tasklogid=%d AND qry_IsSuccess=1", 
         nTaskLogId);
    PrintDebugLog(DBG_HERE,"Execute SQL[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"Execute SQL[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	if (FetchCursor(&struCursor) == NORMAL)
	{
        nRxCount= atoi(GetTableFieldValue(&struCursor,"nrxcount"));
	}
	FreeCursor(&struCursor);
	
	//У���ɹ�վ������
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "SELECT COUNT(*) AS nelecount FROM (SELECT COUNT(qry_eleid) FROM man_eleqrylog WHERE qry_tasklogid=%d AND qry_IsSuccess=1 GROUP BY qry_eleid) b", 
         nTaskLogId);
    PrintDebugLog(DBG_HERE,"Execute SQL[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"Execute SQL[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	if (FetchCursor(&struCursor) == NORMAL)
	{
        nEleCount= atoi(GetTableFieldValue(&struCursor,"nelecount"));
	}
	FreeCursor(&struCursor);
	
	snprintf(szSql, sizeof (szSql), "update man_tasklog SET TKL_ENDTIME = to_date('%s','yyyy-mm-dd hh24:mi:ss'), tkl_RxPackCount=%d, tkl_EleSuccessCount=%d "
                "where tkl_tasklogid=%d",  szSysDateTime, nRxCount, nEleCount, nTaskLogId);
    
    PrintDebugLog(DBG_HERE, "Execute SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	
	//����������־����ʱ��
	/*
	memset(szSql, 0, sizeof(szSql));
	//2020 ���ϵ���mysql���ݿ��޸�
    snprintf(szSql, sizeof(szSql), "update man_TaskLog set TKL_ENDTIME = to_date( '%s','yyyy-mm-dd hh24:mi:ss')  where tkl_Taskid= %d  AND tkl_begintime >= (SELECT tsk_lasttime FROM man_task WHERE tsk_taskid = %d) ORDER BY tkl_tasklogid DESC LIMIT 1", 
        GetSysDateTime(), nTaskId, nTaskId);
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	*/
	
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
	}
	*/
	
	return NORMAL; 
}

//��������Ϊ������ѯ״̬���´���ѵʱ��
static RESULT SetTaskUsingAndTime(int nTaskId, PSTR pszPeriodTime)
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
	return NORMAL; 
}

static RESULT ReplaceAlarmIdTitle(int nNeId, PSTR pszOrgAlarmId, PSTR pszAlarmName)
{
	STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	TBINDVARSTRU struBindVar;
	
	memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_INT; //SQLT_INT
	struBindVar.struBind[0].VarValue.nValueInt = nNeId;
	struBindVar.nVarCount++;

	struBindVar.struBind[1].nVarType = SQLT_INT; //SQLT_INT
	struBindVar.struBind[1].VarValue.nValueInt = atoi(pszOrgAlarmId);
	struBindVar.nVarCount++;
	
	sprintf(szSql,"select a.eas_name, a.eas_alarmoraid from alm_extalarmset t, alm_extalarmset_info a where t.eas_extalarmid = a.eas_extalarmid and  t.eas_neid = :v_0 and t.eas_alarmid = :v_1");
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%d][%d]\n", szSql, nNeId, atoi(pszOrgAlarmId));
    if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	{
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	return EXCEPTION;
	}
	if (FetchCursor(&struCursor) == NORMAL)
	{
		strcpy(pszOrgAlarmId, GetTableFieldValue(&struCursor, "eas_alarmoraid"));
		strcpy(pszAlarmName, GetTableFieldValue(&struCursor, "eas_name"));
	}
	FreeCursor(&struCursor);
	
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
	
	if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"�����ݿ���� [%s]\n",GetSQLErrorMessage());
		sleep(10);
		return EXCEPTION;
	}

    while(TRUE)
	{
	    sprintf(szSql,"select tsk_taskid,tsk_style, tsk_state, tsk_period,tsk_lasttime, tsk_nexttime, tsk_failtimes, tsk_areaid from man_Task where tsk_isuse = 0 ");
	    //PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	sleep(60);
	    	CloseDatabase();
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        nTskState = atoi(GetTableFieldValue(&struCursor, "tsk_state"));
	        nTaskId = atoi(GetTableFieldValue(&struCursor, "tsk_taskid"));
	        nTskStyle = atoi(GetTableFieldValue(&struCursor, "tsk_style"));
	        strcpy(szPeriodTime, GetTableFieldValue(&struCursor,"tsk_period"));
	        strcpy(szLastTime, GetTableFieldValue(&struCursor,"tsk_lasttime"));
	        strcpy(szNextTime, GetTableFieldValue(&struCursor,"tsk_nexttime"));
	        //������ʱ����
            nFailTimes = atoi(GetTableFieldValue(&struCursor, "tsk_failtimes"));
	        //����ִ��ʱ�䣬״̬����
	        if (nTskState == NORMAL_STATE)
	        {
	            //�Ƚ��Ƿ�ִ��ʱ��
	            //PrintDebugLog(DBG_HERE,"szNextTime[%s]\n",szNextTime);
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
	                
	                InsertInXmlExt(pstruReqXml, "<omc>/<areaid>", 
	                					GetTableFieldValue(&struCursor, "tsk_areaid"),
	                					MODE_AUTOGROW|MODE_UNIQUENAME);					
	                //�����񷢵�Ӧ�÷�����
                    if (ExchDataApplSvr(pstruReqXml) == NORMAL)
                        //��������Ϊ������ѯ״̬���´�ִ��ʱ��
                    {
                    	DeleteXml(pstruReqXml);
                        nNextStep = 1;
                        break;
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
	                break;
	            }    
	        }
            
	    }
	    FreeCursor(&struCursor);
	    
	    //ִ�����֮����������������ѵ������´�����ʱ��
	    if (nNextStep == 1)
	    {
	        SetTaskUsingAndTime(nTaskId, szPeriodTime); 
	        nNextStep = 0;
	    }
	    //���񵽽���ʱ��,����ֹͣʹ��
	    if (nNextStep == 2)
	    {
             SetTaskStopUsing(nTaskId, nTskStyle);
             nNextStep = 0;
	    }
	    sleep(10);
	}
	return NORMAL;
}



RESULT TransferToYiYang(PSTR pszSendBuffer)
{
    INT nConnectFd;
    
    /*
	 *	��������
	 */
	if((nConnectFd = CreateConnectSocket(szYiYangIpAddr, nYiYangPort, 60)) < 0)
	{
		PrintErrorLog(DBG_HERE, \
			"ͬ�����������[%s][%d]�������Ӵ���,����ϵ����Աȷ���Ѿ�����\n", szYiYangIpAddr, nYiYangPort);
		return EXCEPTION;
	}
	
	/*
	 *	�������ݵ���Ϣ���������
	 */
	if(SendSocketWithSync(nConnectFd, pszSendBuffer, strlen(pszSendBuffer), 60) < 0)
	{
		PrintErrorLog(DBG_HERE, "�������ݵ���������������\n");
		close(nConnectFd);
		return EXCEPTION;
	}
	PrintTransLog(DBG_HERE,"���͵���������[%s]\n",pszSendBuffer);
	//�ر�����
	close(nConnectFd);
    return NORMAL;
}

RESULT SetUploadFlag(PSTR pszAlarmLogId, BOOL isNewAlarm)
{
    char szSql[MAX_BUFFER_LEN];
 
    memset(szSql, 0, sizeof(szSql));
    if (isNewAlarm == BOOLTRUE)
         snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_AlarmLogId='%s' and alg_AlarmStatusId = 0", pszAlarmLogId);
    else
         snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_AlarmLogId='%s' and alg_AlarmStatusId >= 4", pszAlarmLogId);
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	
	return NORMAL; 
}

RESULT UpdateAlarmLog(PSTR pszAlarmLogId)
{
	char szSql[MAX_BUFFER_LEN];
	STR *pszField[10];
    STR szAlarmLogId[100];
    INT nFieldNum;
       
	//���ø澯ǰת��־��ʱ��
	nFieldNum = SeperateString( pszAlarmLogId, '-', pszField, 10);
    strcpy(szAlarmLogId, pszField[1]);
    
	memset(szSql, 0, sizeof(szSql));
	snprintf(szSql, sizeof(szSql), "update alm_AlarmLog set alg_isuploadalarm=1, alg_uploaddate=sysdate where alg_AlarmLogId=%d", atoi(szAlarmLogId));
	 PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	
	return NORMAL;
}

RESULT SetTriggerStatus(PSTR pszAlarmLogId)
{
    char szSql[MAX_BUFFER_LEN];
     
    snprintf(szSql, sizeof(szSql), "update tf_ALarmLog_Trigger set alg_AlarmStatusId = '1' where alg_AlarmLogId='%s' and alg_AlarmStatusId = 0", pszAlarmLogId);
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}

RESULT DeleteAlarmTrigger(PSTR pszAlarmLogId)
{
    char szSql[MAX_BUFFER_LEN];
     
    snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_AlarmLogId='%s' and alg_AlarmStatusId = 1", pszAlarmLogId);
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}


RESULT DeleteHisAlarmTrigger()
{
    char szSql[MAX_BUFFER_LEN];
     
    snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_alarmtime< sysdate-2 ");
     
    //PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}

RESULT DeleteAlarmBackTrigger(PSTR pszAlarmLogId)
{
    char szSql[MAX_BUFFER_LEN];
     
    snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_AlarmLogId <= '%s' and alg_AlarmStatusId = 7", pszAlarmLogId);
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}

/*
 * �õ��Ƿ��и澯�ָ�����
 */
RESULT GetAlarmBack(PSTR pszItemName)
{
	STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	STR szAlarmLogId[20];
	
	memset(szAlarmLogId, 0, sizeof(szAlarmLogId));
	memcpy(szAlarmLogId, pszItemName+9, strlen(pszItemName)-9);
	sprintf(szSql,"select alg_alarmlogid from alm_alarmlog where alg_alarmlogid= %d and alg_AlarmStatusId = 7", atoi(szAlarmLogId));
	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(SelectTableRecord(szSql,&struCursor)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ȡ�������к�ʱ���� SQL���[%s]������Ϣ=[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor)!=NORMAL)
	{
		FreeCursor(&struCursor);
		return EXCEPTION;
    }
    FreeCursor(&struCursor);
	return NORMAL;
}

/*
 * ���ָ澯ǰת��־
 */
RESULT SaveTransLog(YIYANGSTRU *pstruYiYang)
{
    char szSql[MAX_BUFFER_LEN];
     
    memset(szSql, 0, sizeof(szSql));
    if (pstruYiYang->IsNewAlarm == BOOLTRUE)
         snprintf(szSql, sizeof(szSql), "INSERT INTO TF_ALARMTRANSFERLOG(NEID, SENDTIME, UPLOADRESULT, ALARMLOGID, ORIALARMID, "
            "ALARMTITLE, ALARMCREATETIME, NETYPE, NENAME, NEVENDOR, "
            "ALARMLEVEL, ALARMTYPE, ALARMREDEFLEVEL, ALARMREDEFTYPE, ALARMLOCATION, "
            "ALARMDETAIL, ALARMREGION, EXTENDINFO, SUCCEED) VALUES( "
            " %d, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s', to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s',  '%s', '%s', '%s', '%s',"
            " '%s',  '%s', '%s', %d)",
            pstruYiYang->NeId, GetSysDateTime(), "�ɹ�", pstruYiYang->szAlarmId, pstruYiYang->szOriAlarmId,
            pstruYiYang->szAlarmTitle, pstruYiYang->szAlarmCreateTime, pstruYiYang->szNeType, pstruYiYang->szNeName, pstruYiYang->szNeVendor,
            pstruYiYang->szAlarmLevel, pstruYiYang->szAlarmType, pstruYiYang->szAlarmRedefLevel, pstruYiYang->szAlarmRedefType, pstruYiYang->szAlarmLocation,
            pstruYiYang->szAlarmDetail, pstruYiYang->szAlarmRegion, pstruYiYang->szExtendInfo, 0
            );
    else
         snprintf(szSql, sizeof(szSql), "INSERT INTO TF_ALARMTRANSFERLOG(NEID, SENDTIME, UPLOADRESULT, ALARMLOGID, ALARMSTATUS, STATUSTIME, SUCCEED) VALUES ("
             " %d, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', %s, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), %d)",
             pstruYiYang->NeId, GetSysDateTime(), "�ɹ�", pstruYiYang->szAlarmId,
             pstruYiYang->szAlarmStatus, pstruYiYang->szStatusTime, 0
             );
     
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}


/*
 * ������Ϣ���з��Ͷ���
 */
RESULT SaveToMsgQueue(INT nNeId, PSTR pszMobile, PSTR pszSpNumber, PSTR pszMsgCont)
{
	char szSql[MAX_BUFFER_LEN];
	INT qry_EleQryLogId;		//��ȡ���
	STR pszChinese[10][141];					/* �������� */
	INT nChineseNum, i;							/* ���ĸ��� */
	
	
	memset(pszChinese, 0, sizeof(pszChinese));
	nChineseNum = SeperateChineseString(pszMsgCont, pszChinese, 10);
	for(i = 0 ; i < nChineseNum ; i ++)
	{
		if (strcmp(getenv("DATABASE"), "mysql") == 0)
		{
			if(GetMySqlSequence(&qry_EleQryLogId, "ne_msgqueue")!=NORMAL)
			{
				PrintErrorLog(DBG_HERE,"��ȡ��ѯ������ˮ�Ŵ���\n");
				return EXCEPTION;
			}
		}
		else
		{
			if(GetDbSequence(&qry_EleQryLogId, "SEQ_MSGQUEUE")!=NORMAL)
			{
				PrintErrorLog(DBG_HERE,"��ȡ��ѯ������ˮ�Ŵ���\n");
				return EXCEPTION;
			}
		}
		
		snprintf(szSql, sizeof(szSql),
			"INSERT INTO NE_MSGQUEUE (QS_ID, QS_NEID, QS_TELEPHONENUM, QS_CONTENT, "
			" QS_SERVERTELNUM,QS_TYPE, QS_EVENTTIME, QS_LEVEL, "
			"QS_LASTTIME, QS_RETRYTIMES, QS_FIRSTID, QS_SECONDID, QS_MSGSTAT) VALUES ( "
			" %d,  %d,   '%s',   '%s',"
			" '%s', '0', to_date( '%s','yyyy-mm-dd hh24:mi:ss'), 1, "
			" '0', 0, 0, 0, '0')",
			
			qry_EleQryLogId, nNeId, pszMobile, pszChinese[i],
			
			pszSpNumber, GetSysDateTime()
		);
		
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

/*
 * ������Ÿ澯ǰת
 */
RESULT SendSmsMessage()
{
	STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	STR szSpNumber[20];
	STR szMobile[20], szMsgCont[500];
	INT nNeId;
	INT nSleepTime = 0;
	INT nTransferlogId;

	
	if (GetCfgItem("gsmmodem.cfg", "GSM����", "������ĺ���",szSpNumber) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ȡCMPP�������ʧ��\n");
        return EXCEPTION;
    }
	
	while(TRUE)
	{
		memset(szSql, 0, sizeof(szSql));
		if (strcmp(getenv("DATABASE"), "mysql") == 0)
			sprintf(szSql,"select atl_transferlogid, atl_moble, atl_interface, atl_neid from alm_alarmtransferlog where atl_alarmstate = 0 limit 50");
		else
			sprintf(szSql,"select atl_transferlogid, atl_moble, atl_interface, atl_neid from alm_alarmtransferlog where atl_alarmstate = 0 and rownum <=50");
			
		//PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	CloseDatabase();
	    	close(nYiConnectFd);
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    /*
	    if (FetchCursor(&struCursor) != NORMAL)
	    {
	    	FreeCursor(&struCursor);
	    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
	    	sleep(10);
	    	continue;
	    }*/
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	    	nTransferlogId = atoi(GetTableFieldValue(&struCursor, "atl_transferlogid"));
	    	strcpy(szMobile, TrimAllSpace(GetTableFieldValue(&struCursor, "atl_moble")));
	    	strcpy(szMsgCont, TrimAllSpace(GetTableFieldValue(&struCursor, "atl_interface")));
	    	nNeId = atoi(GetTableFieldValue(&struCursor, "atl_neid"));
	    	
	    	if (SaveToMsgQueue(nNeId, szMobile, szSpNumber, szMsgCont) != NORMAL)
	    	{
	    		break;
	    	}
	    	
	    	memset(szSql, 0, sizeof(szSql));
    		snprintf(szSql, sizeof(szSql), "update alm_alarmtransferlog set atl_alarmstate=1 where atl_transferlogid = %d",
    		     nTransferlogId);
    		 
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
	    
	    if (nSleepTime ++ > 720)
	    {
	    	memset(szSql, 0, sizeof(szSql));
    		snprintf(szSql, sizeof(szSql), "delete from  alm_alarmtransferlog where atl_time < sysdate - 30");
    		PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
			if(ExecuteSQL(szSql) != NORMAL)
			{
				PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
					szSql, GetSQLErrorMessage());
    		    return EXCEPTION;
			}
    		CommitTransaction();
	    }
	    sleep(10);
	}
	
	return NORMAL;
}

/*
 * ����澯����
 */
RESULT ProcessAlarmTransfer()
{
    STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	YIYANGSTRU struYiYangData;
	STR szArea[20];
	STR szSiteId[10];
	STR szSendBuffer[MAX_BUFFER_LEN];
	STR szAlarmBegin[20], szAlarmEnd[20];
	STR szAlarmBeginDate[20], szAlarmEndDate[20];
	INT nSiteLevelId;
	STR szAlarmLogId[100];
	/*
	 *	��������
	 */
	if((nYiConnectFd = CreateConnectSocket(szYiYangIpAddr, nYiYangPort, 60)) < 0)
	{
		PrintErrorLog(DBG_HERE, \
			"ͬ�����������[%s][%d]�������Ӵ���,����ϵ����Աȷ���Ѿ�����\n", szYiYangIpAddr, nYiYangPort);
		CloseDatabase();
		sleep(60);
		return EXCEPTION;
	} 
	
	
    while(TRUE)
	{
	    /*
	 	 *	����澯����
	 	 */
	    sprintf(szSql,"SELECT alg_AlarmLogId, alg_alarmId, alg_NeId, to_char(alg_AlarmTime,'yyyy-mm-dd hh24:mi:ss') as alg_AlarmTime,"
	    						"alm_Alarm.alm_Name AS alg_AlarmName," 
								"n.ne_RepeaterId ," 
								"n.ne_DeviceId,"
								"n.ne_AlarmBegin, n.ne_AlarmEnd,"       
								"n.ne_Name, n.ne_NeTelNum," 
								"n.ne_sitelevelid, n.ne_systemnumbers," 
								"n.ne_systemname, n.ne_protocoldevicetypeid,"
								"n.ne_CellId, "
								
								"p.are_Code AS alg_areaCode, ne_Company.co_Name AS alg_CompanyName," 
								"ne_Company.co_CompanyId AS alg_CompanyId, "
								"alm_Alarm.alm_LevelId AS alg_LevelId, "
								"alm_AlarmLevel.alv_Name AS alg_LevelName," 
								"ne_DeviceType.dtp_Name AS alg_DeviceTypeName, "
								"ne_DeviceType.dtp_DeviceTypeId AS alg_DeviceTypeId, "
								"case p.are_Grade when 1 then p.are_Name when 2 then (select are_Name from pre_Area where pre_Area.are_Code = p.are_Code) end ne_Area ,"
								"b.bs_SiteId, b.bs_SiteName, b.bs_lac "
							"FROM tf_alarmlog_trigger t "
								"LEFT JOIN  alm_Alarm ON alm_Alarm.alm_AlarmId = t.alg_AlarmId " 
								"JOIN  ne_Element n ON n.ne_NeId = t.alg_NeId "
								"LEFT JOIN  pre_Area p on n.ne_AreaId = p.are_AreaId  "
								"LEFT JOIN  ne_Company ON ne_Company.co_CompanyId = n.ne_CompanyId  "
								"LEFT JOIN  alm_AlarmLevel ON alm_AlarmLevel.alv_AlarmLevelId = alm_Alarm.alm_LevelId "
								"LEFT JOIN  ne_DeviceType ON ne_DeviceType.dtp_DeviceTypeId = n.ne_DeviceTypeId "
								"LEFT JOIN  bs_gsmbasestation b ON n.ne_CellId = b.bs_CellId  and substr(b.bs_areaCode, 0, 4) = substr(p.are_Code, 0, 4) "
                          "WHERE t.alg_AlarmStatusId = 0 ");
	    //PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	CloseDatabase();
	    	close(nYiConnectFd);
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        PrintDebugLog(DBG_HERE,"��ʼ����澯ǰת================\n");
	        memset(&struYiYangData, 0, sizeof(YIYANGSTRU));
	        struYiYangData.IsNewAlarm = BOOLTRUE;
			struYiYangData.NeId = atoi(GetTableFieldValue(&struCursor, "alg_NeId"));
			strcpy(struYiYangData.szSystemVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName"));//������
			strcpy(struYiYangData.szAlarmId,  GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
			strcpy(struYiYangData.szOriAlarmId,  GetTableFieldValue(&struCursor, "alg_alarmId"));
			strcpy(struYiYangData.szAlarmTitle,  GetTableFieldValue(&struCursor, "alg_AlarmName"));
			
			if (atoi(GetTableFieldValue(&struCursor, "ne_protocoldevicetypeid")) == 35) 
			{
				if (strcmp(struYiYangData.szOriAlarmId, "22") == 0 || strcmp(struYiYangData.szOriAlarmId, "23") == 0
				    || strcmp(struYiYangData.szOriAlarmId, "24") == 0 || strcmp(struYiYangData.szOriAlarmId, "25") == 0)
				{
					PrintDebugLog(DBG_HERE,"�ⲿ�澯�滻ǰ[%s][%s]\n",struYiYangData.szOriAlarmId, struYiYangData.szAlarmTitle);
				 	ReplaceAlarmIdTitle(struYiYangData.NeId, struYiYangData.szOriAlarmId, struYiYangData.szAlarmTitle);
				 	PrintDebugLog(DBG_HERE,"�ⲿ�澯�滻��[%s][%s]\n",struYiYangData.szOriAlarmId, struYiYangData.szAlarmTitle);
				}
			}
			
            strcpy(struYiYangData.szAlarmCreateTime,  GetTableFieldValue(&struCursor, "alg_AlarmTime"));
            strcpy(struYiYangData.szNeType,  GetTableFieldValue(&struCursor, "alg_DeviceTypeName"));
            
            strcpy(szSiteId, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_CellId")));
            //��Ԫ����
            sprintf(struYiYangData.szNeName, "%s", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_Name")));
            
            strcpy(struYiYangData.szNeVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName")); //��Ԫ����
            strcpy(struYiYangData.szAlarmLevel,  GetTableFieldValue(&struCursor, "alg_LevelName")); //�澯����
            strcpy(struYiYangData.szAlarmType, "���߸澯");  //�澯���ͣ����߸澯
            
            sprintf(struYiYangData.szAlarmLocation, "%08X", atoi(GetTableFieldValue(&struCursor, "ne_RepeaterId"))); //  �澯��λ��վ����,�豸��ţ��Զ��ŷָ���վ����Ϊ8λ16���������豸���Ϊ2λ16��������
            sprintf(struYiYangData.szAlarmDeviceId, "%02X", atoi(GetTableFieldValue(&struCursor, "ne_DeviceId")));
            strcpy(struYiYangData.szAlarmTelnum,  GetTableFieldValue(&struCursor, "ne_NeTelNum")); 
            
            //�澯�������� 
			strcpy(szArea, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_Area")));
			if (strcmp(szArea, "") == 0)
			    strcpy(szArea, "����");
			if (strstr("����,����,����,����,����,����,��,����,̨��,��ˮ,��ɽ", szArea) != NULL)
			    strcat(szArea, "��");
		    strcpy(struYiYangData.szAlarmRegion, szArea);
		    
		    nSiteLevelId = atoi(GetTableFieldValue(&struCursor, "ne_sitelevelid"));
		    if (nSiteLevelId == 1)
		    	strcpy(struYiYangData.szSystemLevel, "��ͨ");
		    else if (nSiteLevelId == 2)
		    	strcpy(struYiYangData.szSystemLevel, "VIP");
		    else if (nSiteLevelId == 3)
		    	strcpy(struYiYangData.szSystemLevel, "VVIP");
		    else if (nSiteLevelId == 4)
		    	strcpy(struYiYangData.szSystemLevel, "����");
		    
		    strcpy(struYiYangData.szSystemNo, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_systemnumbers")));
		    strcpy(struYiYangData.szSystemTitle, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_systemname")));
		    
		    strcpy(struYiYangData.szSiteId, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_SiteId")));
		    strcpy(struYiYangData.szSiteName, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_sitename")));
		    //2011.11.21 add
		    strcpy(struYiYangData.szCellId, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_CellId")));
		    strcpy(struYiYangData.szLac, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_Lac")));
		    //�ж��Ƿ��и澯ʱ��
            strcpy(szAlarmBegin, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmBegin")));
            strcpy(szAlarmEnd, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmEnd")));
            if (strlen(szAlarmBegin) == 5 && strlen(szAlarmEnd) == 5) 
            {
 				time_t struTimeNow;
				struct tm struTmNow;

			    time(&struTimeNow);
			    struTmNow=*localtime(&struTimeNow);
			    
            	snprintf(szAlarmBeginDate, sizeof(szAlarmBeginDate), "%04d-%02d-%02d %s:00", 
		    		struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szAlarmBegin);
		    	snprintf(szAlarmEndDate, sizeof(szAlarmEndDate), "%04d-%02d-%02d %s:00", 
		    		struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szAlarmEnd);
		    	
		    	PrintDebugLog(DBG_HERE,"�澯��ʼʱ��[%s],�澯����ʱ��[%s],�澯ʱ��[%s]\n", szAlarmBeginDate, szAlarmEndDate, struYiYangData.szAlarmCreateTime);
            	if ((int)MakeITimeFromLastTime(struYiYangData.szAlarmCreateTime) < (int)MakeITimeFromLastTime(szAlarmBeginDate) || 
            		(int)MakeITimeFromLastTime(struYiYangData.szAlarmCreateTime) > (int)MakeITimeFromLastTime(szAlarmEndDate))
            	{
            		//PrintDebugLog(DBG_HERE,"ִ�е�����\n");
            		//���������Ѵ���
            		SetTriggerStatus(struYiYangData.szAlarmId);
            		continue;
            	}
            }
 
			bufclr(szSendBuffer);
			snprintf(szSendBuffer, sizeof(szSendBuffer), "\n%s\nsystemName:%s\nsystemVendor:%s\nalarmId:%s\noriAlarmId:%s\n"
                "alarmTitle:%s\nalarmCreateTime:%s\nneType:%s\nneName:%s\nneVendor:%s\n"
                "alarmLevel:%s\nalarmType:%s\nalarmRedefLevel:%s\nalarmRedefType:%s\nalarmLocation:%s\n"
                "alarmDeviceid:%s\nalarmTelnum:%s\nalarmRegion:%s\nsystemLevel:%s\n"
                "systemNo:%s\nsystemTitle:%s\nsiteId:%s\nsiteName:%s\n"
                "cellId:%s\nlac:%s\nextendInfo:%s\nalarmStatus:0\n%s\n",
                
                "<ALARMSTART>", "���������ҷ�ά��ƽ̨", "��άͨ��", struYiYangData.szAlarmId, struYiYangData.szOriAlarmId,
                struYiYangData.szAlarmTitle, struYiYangData.szAlarmCreateTime, struYiYangData.szNeType, struYiYangData.szNeName, struYiYangData.szNeVendor,
                struYiYangData.szAlarmLevel, struYiYangData.szAlarmType, struYiYangData.szAlarmRedefLevel, struYiYangData.szAlarmRedefType, struYiYangData.szAlarmLocation,
                struYiYangData.szAlarmDeviceId, struYiYangData.szAlarmTelnum, struYiYangData.szAlarmRegion, struYiYangData.szSystemLevel,
                struYiYangData.szSystemNo, struYiYangData.szSystemTitle, struYiYangData.szSiteId, struYiYangData.szSiteName, 
                struYiYangData.szCellId, struYiYangData.szLac, struYiYangData.szExtendInfo,  "<ALARMEND>");
            PrintTransLog(DBG_HERE,"���͸澯ǰת����[%s]\n",szSendBuffer);  
            
            /*
			 *	�������ݵ���Ϣ���������
			 */
			if(SendSocketNoSync(nYiConnectFd, szSendBuffer, strlen(szSendBuffer), 60) < 0)
			{
				FreeCursor(&struCursor);
				PrintErrorLog(DBG_HERE, "�������ݵ���������������\n");
				close(nYiConnectFd);
				CloseDatabase();
				return EXCEPTION;
			}
			
			
	        SetUploadFlag(struYiYangData.szAlarmId, struYiYangData.IsNewAlarm);
		    SaveTransLog(&struYiYangData);
		    UpdateAlarmLog(struYiYangData.szAlarmId);


	    }
	    FreeCursor(&struCursor);
	    
	    
	    
	    
	    //�����ɵ�ʱ��֮��澯
	    
	    sprintf(szSql,"SELECT alg_AlarmLogId, alg_alarmId, alg_NeId, to_char(alg_AlarmTime,'yyyy-mm-dd hh24:mi:ss') as alg_AlarmTime,"
	    						"alm_Alarm.alm_Name AS alg_AlarmName," 
								"n.ne_RepeaterId ," 
								"n.ne_DeviceId,"
								"n.ne_AlarmBegin, n.ne_AlarmEnd,"       
								"n.ne_Name, n.ne_NeTelNum," 
								"n.ne_sitelevelid, n.ne_systemnumbers," 
								"n.ne_systemname,"
								"n.ne_CellId, "
								
								"p.are_Code AS alg_areaCode, ne_Company.co_Name AS alg_CompanyName," 
								"ne_Company.co_CompanyId AS alg_CompanyId, "
								"alm_Alarm.alm_LevelId AS alg_LevelId, "
								"alm_AlarmLevel.alv_Name AS alg_LevelName," 
								"ne_DeviceType.dtp_Name AS alg_DeviceTypeName, "
								"ne_DeviceType.dtp_DeviceTypeId AS alg_DeviceTypeId, "
								"case p.are_Grade when 1 then p.are_Name when 2 then (select are_Name from pre_Area where pre_Area.are_Code = p.are_Code) end ne_Area ,"
								"b.bs_SiteId, b.bs_SiteName,bs_lac "
							"FROM tf_alarmlog_trigger t "
								"LEFT JOIN  alm_Alarm ON alm_Alarm.alm_AlarmId = t.alg_AlarmId " 
								"JOIN  ne_Element n ON n.ne_NeId = t.alg_NeId "
								"LEFT JOIN  pre_Area p on n.ne_AreaId = p.are_AreaId  "
								"LEFT JOIN  ne_Company ON ne_Company.co_CompanyId = n.ne_CompanyId  "
								"LEFT JOIN  alm_AlarmLevel ON alm_AlarmLevel.alv_AlarmLevelId = alm_Alarm.alm_LevelId "
								"LEFT JOIN  ne_DeviceType ON ne_DeviceType.dtp_DeviceTypeId = n.ne_DeviceTypeId "
								"LEFT JOIN  bs_gsmbasestation b ON n.ne_CellId = b.bs_CellId  and substr(b.bs_areaCode, 0, 4) = substr(p.are_Code, 0, 4) "
                          "WHERE t.alg_AlarmStatusId = 1 ");
	    //PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	CloseDatabase();
	    	close(nYiConnectFd);
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        PrintDebugLog(DBG_HERE,"��ʼ����澯ǰת================\n");
	        memset(&struYiYangData, 0, sizeof(YIYANGSTRU));
	        struYiYangData.IsNewAlarm = BOOLTRUE;
			struYiYangData.NeId = atoi(GetTableFieldValue(&struCursor, "alg_NeId"));
			strcpy(struYiYangData.szSystemVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName"));//������
			strcpy(struYiYangData.szAlarmId,  GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
			strcpy(struYiYangData.szOriAlarmId,  GetTableFieldValue(&struCursor, "alg_alarmId"));
			strcpy(struYiYangData.szAlarmTitle,  GetTableFieldValue(&struCursor, "alg_AlarmName"));
			
            strcpy(struYiYangData.szAlarmCreateTime,  GetTableFieldValue(&struCursor, "alg_AlarmTime"));
            strcpy(struYiYangData.szNeType,  GetTableFieldValue(&struCursor, "alg_DeviceTypeName"));
            
            strcpy(szSiteId, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_CellId")));
            //��Ԫ����
            sprintf(struYiYangData.szNeName, "%s", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_Name")));
            
            strcpy(struYiYangData.szNeVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName")); //��Ԫ����
            strcpy(struYiYangData.szAlarmLevel,  GetTableFieldValue(&struCursor, "alg_LevelName")); //�澯����
            strcpy(struYiYangData.szAlarmType, "���߸澯");  //�澯���ͣ����߸澯
            
            sprintf(struYiYangData.szAlarmLocation, "%08X", atoi(GetTableFieldValue(&struCursor, "ne_RepeaterId"))); //  �澯��λ��վ����,�豸��ţ��Զ��ŷָ���վ����Ϊ8λ16���������豸���Ϊ2λ16��������
            sprintf(struYiYangData.szAlarmDeviceId, "%02X", atoi(GetTableFieldValue(&struCursor, "ne_DeviceId")));
            strcpy(struYiYangData.szAlarmTelnum,  GetTableFieldValue(&struCursor, "ne_NeTelNum")); 
            
            //�澯�������� 
			strcpy(szArea, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_Area")));
			if (strcmp(szArea, "") == 0)
			    strcpy(szArea, "����");
			if (strstr("����,����,����,����,����,����,��,����,̨��,��ˮ,��ɽ", szArea) != NULL)
			    strcat(szArea, "��");
		    strcpy(struYiYangData.szAlarmRegion, szArea);
		    
		    nSiteLevelId = atoi(GetTableFieldValue(&struCursor, "ne_sitelevelid"));
		    if (nSiteLevelId == 1)
		    	strcpy(struYiYangData.szSystemLevel, "��ͨ");
		    else if (nSiteLevelId == 2)
		    	strcpy(struYiYangData.szSystemLevel, "VIP");
		    else if (nSiteLevelId == 3)
		    	strcpy(struYiYangData.szSystemLevel, "VVIP");
		    else if (nSiteLevelId == 4)
		    	strcpy(struYiYangData.szSystemLevel, "����");
		    
		    strcpy(struYiYangData.szSystemNo, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_systemnumbers")));
		    strcpy(struYiYangData.szSystemTitle, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_systemname")));
		    
		    strcpy(struYiYangData.szSiteId, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_SiteId")));
		    strcpy(struYiYangData.szSiteName, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_sitename")));
		    
		     //2011.11.21 add
		    strcpy(struYiYangData.szCellId, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_CellId")));
		    strcpy(struYiYangData.szLac, TrimAllSpace(GetTableFieldValue(&struCursor, "bs_Lac")));
		    
		    //�ж��Ƿ��и澯ʱ��
            strcpy(szAlarmBegin, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmBegin")));
            strcpy(szAlarmEnd, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmEnd")));
            
            if (strlen(szAlarmBegin) == 5 && strlen(szAlarmEnd) == 5)
            {
 				time_t struTimeNow;
				struct tm struTmNow;

			    time(&struTimeNow);
			    struTmNow=*localtime(&struTimeNow);
			    
            	snprintf(szAlarmBeginDate, sizeof(szAlarmBeginDate), "%04d-%02d-%02d %s:00", 
		    		struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szAlarmBegin);
		    	snprintf(szAlarmEndDate, sizeof(szAlarmEndDate), "%04d-%02d-%02d %s:00", 
		    		struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szAlarmEnd);
		    	
		    	
		    	PrintDebugLog(DBG_HERE,"�澯�ָ���ʼʱ��[%s],�澯�ָ�����ʱ��[%s]\n", szAlarmBeginDate, szAlarmEndDate);
            	if ((int)time(NULL)-600 > (int)MakeITimeFromLastTime(szAlarmBeginDate) && 
            		(int)time(NULL) < (int)MakeITimeFromLastTime(szAlarmEndDate))
            	{
               		//�����Ƿ��и澯�ָ����и澯�ָ���ɾ���ɵ�
               		if (GetAlarmBack(struYiYangData.szAlarmId) == NORMAL)
               		{
            			DeleteAlarmTrigger(struYiYangData.szAlarmId);
            			continue;
            		}
            	}
            	else
            		continue;
            }
            
 
			bufclr(szSendBuffer);
			snprintf(szSendBuffer, sizeof(szSendBuffer), "\n%s\nsystemName:%s\nsystemVendor:%s\nalarmId:%s\noriAlarmId:%s\n"
                "alarmTitle:%s\nalarmCreateTime:%s\nneType:%s\nneName:%s\nneVendor:%s\n"
                "alarmLevel:%s\nalarmType:%s\nalarmRedefLevel:%s\nalarmRedefType:%s\nalarmLocation:%s\n"
                "alarmDeviceid:%s\nalarmTelnum:%s\nalarmRegion:%s\nsystemLevel:%s\n"
                "systemNo:%s\nsystemTitle:%s\nsiteId:%s\nsiteName:%s\n"
                "cellId:%s\nlac:%s\nextendInfo:%s\nalarmStatus:0\n%s\n",
                
                "<ALARMSTART>", "���������ҷ�ά��ƽ̨", "��άͨ��", struYiYangData.szAlarmId, struYiYangData.szOriAlarmId,
                struYiYangData.szAlarmTitle, struYiYangData.szAlarmCreateTime, struYiYangData.szNeType, struYiYangData.szNeName, struYiYangData.szNeVendor,
                struYiYangData.szAlarmLevel, struYiYangData.szAlarmType, struYiYangData.szAlarmRedefLevel, struYiYangData.szAlarmRedefType, struYiYangData.szAlarmLocation,
                struYiYangData.szAlarmDeviceId, struYiYangData.szAlarmTelnum, struYiYangData.szAlarmRegion, struYiYangData.szSystemLevel,
                struYiYangData.szSystemNo, struYiYangData.szSystemTitle, struYiYangData.szSiteId, struYiYangData.szSiteName, 
                struYiYangData.szCellId, struYiYangData.szLac, struYiYangData.szExtendInfo,  "<ALARMEND>");
            PrintTransLog(DBG_HERE,"���͸澯ǰת����[%s]\n",szSendBuffer);  
            
            /*
			 *	�������ݵ���Ϣ���������
			 */
			if(SendSocketNoSync(nYiConnectFd, szSendBuffer, strlen(szSendBuffer), 60) < 0)
			{
				FreeCursor(&struCursor);
				PrintErrorLog(DBG_HERE, "�������ݵ���������������\n");
				close(nYiConnectFd);
				CloseDatabase();
				return EXCEPTION;
			}
			
			DeleteAlarmTrigger(struYiYangData.szAlarmId);
	        //SetUploadFlag(struYiYangData.szAlarmId, struYiYangData.IsNewAlarm);
		    SaveTransLog(&struYiYangData);

	    }
	    FreeCursor(&struCursor);
	    
	    /*
	     * ����澯�ָ�����
	     */
	    sprintf(szSql,"SELECT alg_AlarmLogId, to_char(alg_AlarmTime,'yyyy-mm-dd hh24:mi:ss') as alg_AlarmTime, to_char(alg_ClearTime,'yyyy-mm-dd hh24:mi:ss') as alg_ClearTime, alg_AlarmStatusId, alg_NeId, "
							"p.are_Code AS alg_areaCode, "
							"case p.are_Grade when 1 then p.are_Name when 2 then (select are_Name from pre_Area where pre_Area.are_Code = p.are_Code) end ne_Area  "
							"FROM tf_alarmlog_trigger "
							"JOIN ne_Element ON ne_Element.ne_NeId = tf_alarmlog_trigger.alg_NeId  "
							"left join  pre_Area p on ne_Element.ne_AreaId = p.are_AreaId "
							"where tf_alarmlog_trigger.alg_AlarmStatusId>=4");
	    //PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	CloseDatabase();
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	close(nYiConnectFd);
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        PrintDebugLog(DBG_HERE,"��ʼ����澯���================\n");
	        memset(&struYiYangData, 0, sizeof(YIYANGSTRU));
	        struYiYangData.IsNewAlarm = BOOLFALSE;
			struYiYangData.NeId = atoi(GetTableFieldValue(&struCursor, "alg_NeId"));
			strcpy(struYiYangData.szAlarmId,  GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
			strcpy(struYiYangData.szAlarmCreateTime,  GetTableFieldValue(&struCursor, "alg_AlarmTime"));
			
			if (atoi(GetTableFieldValue(&struCursor, "alg_AlarmStatusId")) == 7)
			    strcpy(struYiYangData.szAlarmStatus, "1");
			else
			    strcpy(struYiYangData.szAlarmStatus, "2");
			strcpy(struYiYangData.szStatusTime,  GetTableFieldValue(&struCursor, "alg_ClearTime"));
			
			strcpy(szArea, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_Area")));
			if (strcmp(szArea, "") == 0)
			    strcpy(szArea, "����");
			if (strstr("����,����,����,����,����,����,��,����,̨��,��ˮ,��ɽ", szArea) != NULL)
			    strcat(szArea, "��");
		    strcpy(struYiYangData.szAlarmRegion, "szArea");
		    
		    bufclr(szSendBuffer);
		    snprintf(szSendBuffer, sizeof(szSendBuffer), "\n%s\nsystemName:%s\nsystemVendor:%s\nalarmId:%s\nalarmStatus:%s\nstatusTime:%s\n%s\n",
		        "<ALARMSTART>", "���������ҷ�ά��ƽ̨", "��άͨ��", struYiYangData.szAlarmId, struYiYangData.szAlarmStatus, struYiYangData.szStatusTime, "<ALARMEND>");
		    PrintTransLog(DBG_HERE,"���͸澯�������[%s]\n",szSendBuffer);
		    
		    /*
			 *	�������ݵ���Ϣ���������
			 */
			if(SendSocketNoSync(nYiConnectFd, szSendBuffer, strlen(szSendBuffer), 60) < 0)
			{
				FreeCursor(&struCursor);
				PrintErrorLog(DBG_HERE, "�������ݵ���������������\n");
				CloseDatabase();
				close(nYiConnectFd);
				return EXCEPTION;
			}
			
	        SetUploadFlag(struYiYangData.szAlarmId, struYiYangData.IsNewAlarm);
	        strcpy(szAlarmLogId, struYiYangData.szAlarmId);
		    SaveTransLog(&struYiYangData);
		    
		    //UpdateAlarmLog(struYiYangData.szAlarmId);
	    }
	    FreeCursor(&struCursor);
	    
	    sleep(2);
	    
	    DeleteHisAlarmTrigger();
	    
	    sleep(30);
 
		
	}
	return NORMAL;
}

RESULT ClearRedisEleqrylog()
{
	INT nQB, j, nRepeaterId, nDeviceId;
	char szMessage[8192], szBeginTime[100], szFieldKey[100];
	redisReply *reply, *reply2, *reply3;
	cJSON* cjson_root = NULL;
	cJSON* cjson_name = NULL;
	static time_t last = 0;
    time_t nowtime;
    

	//while(TRUE)
	{    

		nowtime = time(NULL);
	    if (nowtime - last < 10*60)
	        return 0;
	    
	    last = nowtime;
	        
		reply = redisCommand(redisconn,"HGETALL man_eleqrylog");
		
		PrintDebugLog(DBG_HERE, "HGETALL man_eleqrylog\n");
		if (reply == NULL || redisconn->err) {   //10.25
			PrintErrorLog(DBG_HERE, "Redis HGETALL man_eleqrylog error: %s\n", redisconn->errstr);
			return -1;
		}
		if (reply->type == REDIS_REPLY_ARRAY && reply->elements>=2) {
			for (j = 0; j < reply->elements/2; j++) {
				
				strcpy(szFieldKey, reply->element[j*2]->str);
			    //nQryLogId2=atoi(reply->element[j*2]->str);
			        
		        strcpy(szMessage, reply->element[j*2+1]->str);
				//PrintDebugLog(DBG_HERE, "%s, %s\n", szFieldKey, szMessage);
				
				cjson_root = cJSON_Parse(szMessage);
			    if(cjson_root == NULL)
			    {
			        PrintErrorLog(DBG_HERE, "parse man_eleqrylog fail.\n");
			        return -1;
			    }
			    			    
			    cjson_name = cJSON_GetObjectItem(cjson_root, "qs_netflag");
			    nQB = cjson_name->valueint;
			    
			    cjson_name = cJSON_GetObjectItem(cjson_root, "ne_repeaterid");
				nRepeaterId = cjson_name->valueint;
				
				cjson_name = cJSON_GetObjectItem(cjson_root, "ne_deviceid");
				nDeviceId = cjson_name->valueint;
							    
				cjson_name = cJSON_GetObjectItem(cjson_root, "qry_begintime");
				strcpy(szBeginTime, cjson_name->valuestring);

				//PrintDebugLog(DBG_HERE, "man_eleqrylog: %s, %d,%d\n", szBeginTime, (int)time(NULL)-43200, (int)MakeITimeFromLastTime(szBeginTime));
				
				if ((int)time(NULL) > (int)MakeITimeFromLastTime(szBeginTime)+43200)
				{
		        	reply2 = redisCommand(redisconn,"HDEL man_eleqrylog %d_%d_%d", nRepeaterId, nDeviceId, nQB);
				    PrintDebugLog(DBG_HERE, "HDEL man_eleqrylog: %d_%d_%d, %s\n", nRepeaterId, nDeviceId, nQB, szBeginTime);
				    freeReplyObject(reply2);
		        }
				cJSON_Delete(cjson_root);
			}
		}
		
		freeReplyObject(reply);
		
		reply = redisCommand(redisconn,"keys Queue*");
		//PrintDebugLog(DBG_HERE, "keys :  %d, %d\n", reply->type,reply->elements);
		
		if (reply->type == REDIS_REPLY_ARRAY && reply->elements>0) {
		    for (j = 0; j < reply->elements; j++) {
		        //PrintDebugLog(DBG_HERE,"key %d) %s\n", j, reply->element[j]->str);
		        strcpy(szFieldKey, reply->element[j]->str);
		        
		        reply2 = redisCommand(redisconn,"llen %s", szFieldKey);
	        	if (reply2->type == REDIS_REPLY_INTEGER && reply2->integer>100){
	        		
	        		reply3 = redisCommand(redisconn,"DEL %s", szFieldKey);
	        		PrintDebugLog(DBG_HERE, "DEL %s\n", szFieldKey);
	        		freeReplyObject(reply3);
	        	}
	        	freeReplyObject(reply2);
		    }
		}
	    freeReplyObject(reply);
	}
	
	
	return NORMAL;
}


RESULT ProcRedisHeartBeat()
{
	char szRepeaterIds[4096];
	static time_t lasttime = 0;
    time_t nowtime;
	STR szSql[MAX_SQL_LEN];
 	UINT nTimes=0;
 	redisReply *reply;
 	int i;
	
	if (InitRedisMQ_cfg() != NORMAL)
    {
    	PrintErrorLog(DBG_HERE,"Failed to InitRedisMQ_cfg()\n");
        return EXCEPTION;
    }
    if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"�����ݿ���� [%s]\n",GetSQLErrorMessage());
		sleep(10);
		return EXCEPTION;
	}
    if (ConnectRedis() !=NORMAL)
    {
        PrintErrorLog(DBG_HERE, "Connect Redis Error \n");
        return EXCEPTION;
    }
	while(TRUE)
	{   
		nTimes=0;
		memset(szRepeaterIds, 0, sizeof(szRepeaterIds)); 
		for(i=0; i< 50; i++)
	    {
		    reply = redisCommand(redisconn,"BRPOP HeartBeatQueue 5");
		    if (reply == NULL || redisconn->err) {   //10.25
				PrintErrorLog(DBG_HERE, "Redis BRPOP ElementSQLQueue error: %s\n", redisconn->errstr);
				sleep(10);
				break;
			}
			
			if (!(reply->type == REDIS_REPLY_ARRAY && reply->elements==2)) {
			    if(reply->type == REDIS_REPLY_NIL){
			    	nowtime = time(NULL);
    				if (nowtime - lasttime >= 60){
    					PrintDebugLog(DBG_HERE, "Redis BRPOP HeartBeatQueue is Null, No Task\n");
    					lasttime = nowtime;
    				}
    				freeReplyObject(reply);
			    }
			    else{
			    	PrintErrorLog(DBG_HERE, "Redis BRPOP HeartBeatQueue Error Type: %d, %s\n", reply->type, reply->str);
			    	freeReplyObject(reply);
			    	FreeRedisConn();
			    	sleep(1);
			    	if (ConnectRedis() !=NORMAL)
				    {
				        PrintErrorLog(DBG_HERE, "Connect Redis Error BRPOP ElementSQLQueue After\n");
				        return EXCEPTION;
				    }
			    }
			    break;
			}

		    if (reply->type == REDIS_REPLY_ARRAY && reply->elements==2) {
		    	nTimes++;
		        //PrintDebugLog(DBG_HERE, "RPOP [%s]\n",  reply->element[1]->str);
			    sprintf(szRepeaterIds, "%s%d,", szRepeaterIds, atoi(reply->element[1]->str));
		    }
		    freeReplyObject(reply);
		}
		if (nTimes>0)
		{
			TrimRightOneChar(szRepeaterIds, ',');
			
			snprintf(szSql, sizeof (szSql), "update man_linklog set mnt_lastupdatetime=NOW() where mnt_repeaterid in (%s)", szRepeaterIds);
	        PrintDebugLog(DBG_HERE,"Execute SQL[%s]\n",szSql);
			if(ExecuteSQL(szSql)!=NORMAL)
	        {
	            PrintErrorLog(DBG_HERE,"ִ�� SQL���[%s]������Ϣ=[%s]\n",szSql,GetSQLErrorMessage());
	            return EXCEPTION;
	        }
	        CommitTransaction();
		}
		
		ClearRedisEleqrylog();
	}
	FreeRedisConn();
	
	return NORMAL;
}

RESULT TestTimeServPidStat(int nCmppPid)
{
    STR szShellCmd[MAX_STR_LEN];
    STR szFileName[100];
	INT nTempLen;
	FILE *pstruFile;
	
    memset(szShellCmd,0,sizeof(szShellCmd));
	snprintf(szShellCmd,sizeof(szShellCmd),"ps -e|awk '$1 == %d {print $4}'", nCmppPid);
    
	if ((pstruFile=popen(szShellCmd,"r")) == NULL)
	{
	    fprintf(stderr,"popen�д���\n");
		return EXCEPTION;
    }
    memset(szFileName, 0, sizeof(szFileName));
	while(fgets(szFileName, 10, pstruFile) != NULL)
	{
	    nTempLen=strlen(szFileName);
	    szFileName[nTempLen]=0;
	    if(strncmp(szFileName, "timeserv", 8)==0)
	    {
	        pclose(pstruFile);
		    return NORMAL;
		}
    }
	pclose(pstruFile);

	return EXCEPTION;
}

RESULT TimeServChildProcessWork(int nPid)
{
    struct sigaction struSigAction;
    STR szTemp[MAX_STR_LEN];
    STR szCfgSeg[100];
	
	//����CMPP���̺�
	sprintf(szCfgSeg, "TIMESERVPROC%d", nPid);
	sprintf(szTemp, "%05d\n", (int)getpid());
	ModifyCfgItem("timeserv.cfg", szCfgSeg, "TIMESERVPID",szTemp);
	
	if (GetCfgItem("applserv.cfg","APPLSERV","ListenPort1",szTemp) != NORMAL)
        return EXCEPTION;
    nApplPort=atoi(szTemp);
	
	struSigAction.sa_handler=SigHandle;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags=0;
	if(sigaction(SIGTERM,&struSigAction,NULL)==-1)
	{
		PrintErrorLog(DBG_HERE,"��װ��������������.�������[%d] ������Ϣ[%s]\n",errno,strerror(errno));
		return EXCEPTION;
	}
    
	struSigAction.sa_handler=SIG_IGN;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags=0;
	if(sigaction(SIGPIPE,&struSigAction,NULL)==-1)
	{
		PrintErrorLog(DBG_HERE,"��װ��������������.�������[%d] ������Ϣ[%s]\n",errno,strerror(errno));
		return EXCEPTION;
	}
	
	
    
    if (nPid == 1)//��ʱ����
       ProcessTimeTaskWork();
    else if (nPid == 2)//����澯����
    {
    	ProcRedisHeartBeat();
       /*
       GetCfgItem("timeserv.cfg", szCfgSeg, "ISTRANSFER",szTemp);
       if (atoi(szTemp) == 1)
       	   SendSmsMessage();//ת������
       else
       	   ProcessAlarmTransfer();//�����ӿ�
       */
	}
	else if (nPid == 3)
	   ProcessMsgQueue();
	   
	
    return NORMAL;
}


RESULT ParentForkWork(int nProcCount)
{
    STR szTemp[MAX_STR_LEN];
    STR szCfgSeg[100], szOpen[10];
    INT nPid;
    
    while(BOOLTRUE)
	{
	    //����ӽ���
	    for(nPid=1; nPid< nProcCount+1; nPid++)
	    {   
            sprintf(szCfgSeg, "TIMESERVPROC%d", nPid);
	        //CMPP���̺�
	        if (GetCfgItem("timeserv.cfg", szCfgSeg, "TIMESERVPID",szTemp) != NORMAL)
                return EXCEPTION;
           
            memset(szOpen, 0, sizeof(szOpen)); 
            GetCfgItem("timeserv.cfg", szCfgSeg, "ISRUN",szOpen);
            
            if (atoi(szOpen) == 1 && TestTimeServPidStat(atoi(szTemp)) == EXCEPTION)
            {
                switch (fork())
                {
		            case -1:
		            	PrintErrorLog(DBG_HERE, "�����ӽ���ʧ��\n");
		            	break;
		            case 0:
		            {
                        TimeServChildProcessWork(nPid);
		            	exit(0);
		            }
		            default:
		            	break;
                }
            }
            sleep(2);
        }
        sleep(10); 
	}
}

/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	INT nProcCount;
	
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

	if(DaemonStart()!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"��ʱ���������̽����̨���д���\n");
		return EXCEPTION;
	}
	if(CreateIdFile(argv[0])!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"��������ID�ļ�����\n");
		return EXCEPTION;
	}
	
	//��ʼ��������������Ϣ
	//InitYiYangInfo();

	if(GetDatabaseArg(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		strcpy(szServiceName,"omc");
		strcpy(szDbName,"omc");
		strcpy(szUser,"omc");
		strcpy(szPwd,"omc");
	}
	
	if (GetCfgItem("timeserv.cfg","TIMESERV","MaxProcesses",szTemp) != NORMAL)
        return EXCEPTION;
    nProcCount = atoi(szTemp);

	fprintf(stderr,"���!����ʼ����\n");
	
	ParentForkWork(nProcCount);
	
		
	return NORMAL;
}

