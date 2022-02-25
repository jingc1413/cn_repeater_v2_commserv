/***
 * ����: ����ϵͳ�澯��ʱ����������
 * ��ʱ����ÿ��ִ��һ��
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"
#include <hiredis/hiredis.h>
#include "cJSON.h"


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


static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(ÿ�ո澯��ʱ����)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName,pszProgName,pszProgName);
}

RESULT ConnectRedis()
{
	 redisReply *reply;
	 struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	 
     redisconn = redisConnectWithTimeout(szRedisIp, nRedisPort, timeout);
     if (redisconn == NULL || redisconn->err) {
		if (redisconn) {
             PrintErrorLog(DBG_HERE, "Connection error: %s\n", redisconn->errstr);
             //redisFree(redisconn);
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


RESULT GetRedisDeviceIpInfo(int nRepeaterId, int nDeviceId,PSTR pszDeviceIp, int *pPort)
{

	char szMessage[8000];
	redisReply *reply;
	cJSON* cjson_root = NULL;
	cJSON* cjson_item = NULL;
	char* pMessageBoby = NULL;
	
	cjson_root = cJSON_CreateObject();
	cJSON_AddStringToObject(cjson_root, "qs_deviceip", "192.168.4.2");
	cJSON_AddNumberToObject(cjson_root, "qs_port",  	777);
	cJSON_AddStringToObject(cjson_root, "qs_eventtime", GetSysDateTime());
	pMessageBoby = cJSON_PrintUnformatted(cjson_root);
  	cJSON_Delete(cjson_root);
  	
	//reply = redisCommand(redisconn,"HSET ne_deviceip %d_%d %s", nRepeaterId, nDeviceId,
  	//		pMessageBoby);
  			
    //PrintDebugLog(DBG_HERE, "HSET ne_deviceip %d_%d %s\n", nRepeaterId, nDeviceId,
  	//		pMessageBoby);
	reply = redisCommand(redisconn,"HSET ne_deviceip %d_%d {\"qs_deviceip\":\"%s\",\"qs_port\":%d,\"qs_eventtime\":\"%s\"}", nRepeaterId, nDeviceId,
  			"192.168.4.2", 999, GetSysDateTime());
  			
	PrintDebugLog(DBG_HERE, "HSET ne_deviceip %d_%d {\"qs_deviceip\":\"%s\",\"qs_port\":%d,\"qs_eventtime\":\"%s\"}\n", nRepeaterId, nDeviceId,
  			"192.168.4.2", 999, GetSysDateTime());
	freeReplyObject(reply);
	
	reply = redisCommand(redisconn,"HGET ne_deviceip %d_%d", nRepeaterId, nDeviceId);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HGET man_eleqrylog error: %s\n", redisconn->errstr);
		return -1;
	}
	PrintDebugLog(DBG_HERE, "HGET ne_deviceip %d_%d %d %s\n", nRepeaterId,
			nDeviceId, 	reply->type,reply->str);
	if(reply->type == REDIS_REPLY_NIL){
		freeReplyObject(reply);
        return -1;
	}
	strcpy(szMessage, reply->str);
	freeReplyObject(reply);
	
	cjson_root = cJSON_Parse(szMessage);
    if(cjson_root == NULL)
    {
        PrintErrorLog(DBG_HERE, "parse man_eleqrylog fail.\n");
        return -1;
    }
    cjson_item = cJSON_GetObjectItem(cjson_root, "qs_deviceip");
	strcpy(pszDeviceIp, cjson_item->valuestring);
	PrintDebugLog(DBG_HERE,"%s\n", pszDeviceIp);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qs_eventtime");
	PrintDebugLog(DBG_HERE,"%s\n", cjson_item->valuestring);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qs_port");
    *pPort = cjson_item->valueint;
	PrintDebugLog(DBG_HERE,"%d\n", *pPort);
	cJSON_Delete(cjson_root);

	return NORMAL;
}

RESULT ProcessAlarmYJ()
{
	STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	STR szSql2[MAX_SQL_LEN];
	CURSORSTRU struCursor2;
	INT nAlarmLogId, nNeId;

	/*
	 * ��������Ӧ�澯
	 */
	memset(szSql, 0, sizeof(szSql));
	
	sprintf(szSql, "INSERT INTO tt_elementparam (epm_neid, epm_objid, epm_curvalue) VALUES (105639,'0427','888'),(105639,'0028','��־��test'),(105639,'0028','�������������'),(105637,'006F','�ȥݥ��`�ޥåװ�')");
	
	PrintDebugLog(DBG_HERE, "Execute SQL[%s]\n", szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		fprintf(stderr,"%s[%s]", szSql, GetSQLErrorMessage());
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	
	

	    
	return NORMAL;
}


/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	STR szDeviceIp[30];
	int nDevicePort;
	//fprintf(stderr,"\t��ӭʹ������ϵͳ(ÿ�ո澯��ʱ����)\n");

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
		fprintf(stderr,"ÿ�ո澯��ʱ��������Ѿ�����\n");
		return EXCEPTION;
	}
	
	if(GetDatabaseArg(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		strcpy(szServiceName,"omc");
		strcpy(szDbName,"omc");
		strcpy(szUser,"omc");
		strcpy(szPwd,"omc");
	}
	/*
	if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"�����ݿ���� [%s]\n",GetSQLErrorMessage());
		return EXCEPTION;
	}
	*/
	//ProcessAlarmYJ();
	
	if (InitRedisMQ_cfg() != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"Failed to InitRedisMQ_cfg()\n");
	        return EXCEPTION;
	    }
		if (ConnectRedis() !=NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "Connect Redis Error \n");
	        return EXCEPTION;
	    }
	    GetRedisDeviceIpInfo(3, 0, szDeviceIp, &nDevicePort);
	    
	    FreeRedisConn();
	
	//CloseDatabase();
	
	//fprintf(stderr,"���н���[%s]\n", GetSysDateTime());
	
	return NORMAL;
}


