/***
 * ����: ����ϵͳGRRU������Ϣ����
 *
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"
#include "grrusend.h"
#include <mobile2g.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "log_r.h"
#include "utils.h"
#include "doubly_linked_list.h"

#include <hiredis/hiredis.h>
#include "cJSON.h"


#define VERSION "2.1.20"

#define SOCK_TIMEOUT 20 /* ���ó��� socket ��ʱʱ��Ϊ20�� */
#define DEBUG_VERY_VERBOSE 1

/* task node */
typedef struct tagTASKPARASTRU {
	BYTEARRAY struPack;

	STR szDeviceIp[20]; /* ne's ip address */
	INT nPort; /* ne's listen port */
	INT nDcsId; /* ne_gprsqueue.qs_id */
    int nTaskLogId; /* for debugging */
    int nResendTimes; /* nResendTime is 1 if nTaskLogId is 0, otherwise 2*/
    struct list_head list;
} TASKPARASTRU;

/* ���߳�ά���� qs_id �б����ڼ�¼���߳��Ƿ�ɹ�������� */
typedef struct tagTASKSTATESTRU {
	INT nDcsId; /* ne_gprsqueue.qs_id */
    /* 0 - to be sent to ne;
     * 1 - recv from ne;
     * 2 - update into ne_gprsqueue;
     * -1 - failed to recv from ne */
	INT nState;

    struct list_head list;
} TASKSTATESTRU;

struct task_state_list {
    int nCount; /* current count of items in the list */
    int nMaxCount; /* capacity of the list */

    pthread_mutex_t lock;
    struct list_head tasks;
};

struct task_state_list *gTaskStateList;

static STR szServiceName[MAX_STR_LEN];
static STR szDbName[MAX_STR_LEN];
static STR szUser[MAX_STR_LEN];
static STR szPwd[MAX_STR_LEN];
static int nGprsServPort=-1;
static STR szGprsServIp[MAX_STR_LEN];
static int nMasterPort=-1;
static STR szMasterIp[MAX_STR_LEN];
static STR szHostState[100];
/* ������ָ����ĳ�� IP ��ַ��grrusend ��ʹ�������ַ�������ݰ� */
static STR szLocalIpAddr[32] = {0};
static INT gMaxThrNum = 20;  //10.25 Ĭ��50��Ϊ20����ֹ��ȡ����ʧ�ܹ���

struct list_head g_packets_ne; /* packets to be sent to ne */
struct list_head g_packets_gprsserv; /* packets to be sent to gprsserv */
pthread_mutex_t g_packets_lock; /* lock for g_packets_ne and g_packets_gprsserv */
pthread_mutex_t g_EleResp_lock;
int nCfgResendTimes = 5;



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
    	PrintErrorLogR(DBG_HERE, "Redis ping error: %d,%s\n", reply->type,reply->str);
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
             PrintErrorLogR(DBG_HERE, "Connection error: %s\n", redisconn->errstr);
             redisFree(redisconn);
		} else {
             PrintErrorLogR(DBG_HERE, "Connection error: can't allocate redis context\n");
		}
		return EXCEPTION;
     }
     reply = redisCommand(redisconn, "AUTH %s", szAuthPwd);
     PrintDebugLogR(DBG_HERE, "AUTH: %s\n", reply->str);
     freeReplyObject(reply);
     
	 return NORMAL;
}

RESULT FreeRedisConn()
{
	if (redisconn!=NULL) //10.25
		redisFree(redisconn);
	return NORMAL;
}

RESULT PushEleResp(int nLogId, PSTR pszRespBuffer)
{
	redisReply *reply;
	char szBuffer[MAX_BUFFER_LEN];
	
	memset(szBuffer, 0, sizeof(szBuffer));
	sprintf(szBuffer, "%d,%s", nLogId, pszRespBuffer);
	
	//pthread_mutex_lock(&g_EleResp_lock);
	reply = redisCommand(redisconn,"LPUSH ElementRespQueue %s", szBuffer);
	PrintDebugLogR(DBG_HERE, "LPUSH ElementRespQueue: %d, %d\n", nLogId,reply->type);
    freeReplyObject(reply);
    //pthread_mutex_unlock(&g_EleResp_lock);
    
	return NORMAL;
}


RESULT PublishToRedis(PXMLSTRU  pstruXml, int nReqFlag)
{
  cJSON* cjson_mq = NULL;
  char* pMessageBoby = NULL;
  redisReply *reply;
  
  	cjson_mq = cJSON_CreateObject();
    cJSON_AddNumberToObject(cjson_mq, "qry_eleqrylogid", atoi(DemandStrInXmlExt( pstruXml, "<omc>/<��ˮ��>")));
    cJSON_AddNumberToObject(cjson_mq, "qry_eleid", atoi(DemandStrInXmlExt( pstruXml, "<omc>/<��Ԫ���>")));
    cJSON_AddStringToObject(cjson_mq, "qry_property",	DemandStrInXmlExt( pstruXml, "<omc>/<��ض���>"));
	cJSON_AddNumberToObject(cjson_mq, "qry_style",	atoi(DemandStrInXmlExt( pstruXml, "<omc>/<�����>")));
			
	cJSON_AddNumberToObject(cjson_mq, "qry_commtype",  atoi(DemandStrInXmlExt( pstruXml, "<omc>/<ͨ�ŷ�ʽ>")));
	cJSON_AddNumberToObject(cjson_mq, "qry_taskid", 	 atoi(DemandStrInXmlExt( pstruXml, "<omc>/<�����>")));
	cJSON_AddStringToObject(cjson_mq, "qry_begintime", GetSysDateTime());
	cJSON_AddStringToObject(cjson_mq, "qry_number",	DemandStrInXmlExt( pstruXml, "<omc>/<��ˮ��>"));
			
	//cJSON_AddStringToObject(cjson_mq, "qrt_eventtime", GetSysDateTime());
    cJSON_AddNumberToObject(cjson_mq, "qry_tasklogid",  atoi(DemandStrInXmlExt( pstruXml, "<omc>/<������־��>")));
	cJSON_AddStringToObject(cjson_mq, "qry_windowlogid", DemandStrInXmlExt( pstruXml,"<omc>/<������ˮ��>"));
    if (nReqFlag==0) //����
    {
    	cJSON_AddStringToObject(cjson_mq, "qry_packstatus", "Sent");
    }
    else if (nReqFlag==1)
    {
    	cJSON_AddStringToObject(cjson_mq, "qry_content",	DemandStrInXmlExt( pstruXml, "<omc>/<Content>"));
    	cJSON_AddStringToObject(cjson_mq, "qry_endtime",	DemandStrInXmlExt( pstruXml, "<omc>/<����ʱ��>"));
    	cJSON_AddStringToObject(cjson_mq, "qry_failcontent",DemandStrInXmlExt( pstruXml, "<omc>/<FailContent>"));
    	cJSON_AddStringToObject(cjson_mq, "qry_issuccess",	"1");
    	cJSON_AddStringToObject(cjson_mq, "qry_packstatus", "Received");
    }
    
  
  	pMessageBoby = cJSON_PrintUnformatted(cjson_mq);
  	cJSON_Delete(cjson_mq);
  	PrintDebugLogR(DBG_HERE, "Publish MQ[%s]\n", pMessageBoby);
 	
	reply = redisCommand(redisconn,"PUBLISH mq.man_eleqrylog %s", pMessageBoby);
	
    PrintDebugLogR(DBG_HERE, "PUBLISH mq.man_eleqrylog: %d\n", reply->integer);
    freeReplyObject(reply);
 
  	
		
  	return NORMAL;
}

RESULT GetRedisPackageInfo(int nQryLogId, PXMLSTRU  pstruXml)
{
	char szMessage[8192], szLogId[20];
	redisReply *reply;
	cJSON* cjson_root = NULL;
	cJSON* cjson_name = NULL;
	
	reply = redisCommand(redisconn,"HGET man_eleqrylog %d", nQryLogId);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HGET man_eleqrylog error: %s\n", redisconn->errstr);
		return -1;
	}
	PrintDebugLogR(DBG_HERE, "HGET: %d, %s\n", nQryLogId,reply->str);
	if(reply->type == REDIS_REPLY_NIL){
		freeReplyObject(reply);
        return -1;
	}
	
	strcpy(szMessage, reply->str);
	freeReplyObject(reply);
	
	cjson_root = cJSON_Parse(szMessage);
    if(cjson_root == NULL)
    {
        PrintErrorLogR(DBG_HERE, "parse man_eleqrylog fail.\n");
        reply = redisCommand(redisconn,"HDEL man_eleqrylog %d", nQryLogId);
	    PrintDebugLogR(DBG_HERE, "HDEL: %d, %d\n", nQryLogId, reply->type);
	    freeReplyObject(reply);
        return -1;
    }
    
    sprintf(szLogId, "%d", nQryLogId);
	InsertInXmlExt(pstruXml,"<omc>/<��ˮ��>",  szLogId, MODE_AUTOGROW|MODE_UNIQUENAME);
	
    cjson_name = cJSON_GetObjectItem(cjson_root, "qry_eleid");
	InsertInXmlExt(pstruXml,"<omc>/<��Ԫ���>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_property");
	InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_begintime");
	InsertInXmlExt(pstruXml,"<omc>/<��ʼʱ��>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	InsertInXmlExt(pstruXml,"<omc>/<����ʱ��>",  GetSysDateTime(), MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_style");
	InsertInXmlExt(pstruXml,"<omc>/<�����>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_commtype");
	InsertInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
    cjson_name = cJSON_GetObjectItem(cjson_root, "ne_protocoltypeid");
	InsertInXmlExt(pstruXml,"<omc>/<Э������>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);

	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_taskid");
	InsertInXmlExt(pstruXml,"<omc>/<�����>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_tasklogid");
	InsertInXmlExt(pstruXml,"<omc>/<������־��>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_name = cJSON_GetObjectItem(cjson_root, "qry_windowlogid");
	InsertInXmlExt(pstruXml,"<omc>/<������ˮ��>",  cjson_name->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);

	InsertInXmlExt(pstruXml,"<omc>/<Content>", "", MODE_AUTOGROW|MODE_UNIQUENAME);	
	InsertInXmlExt(pstruXml,"<omc>/<AlarmName>", "", MODE_AUTOGROW|MODE_UNIQUENAME);	
	InsertInXmlExt(pstruXml,"<omc>/<FailContent>", "", MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cJSON_Delete(cjson_root);

	//�����б�ɾ��
	{
		reply = redisCommand(redisconn,"HDEL man_eleqrylog %d", nQryLogId);
	    PrintDebugLogR(DBG_HERE, "HDEL: %d, %d\n", nQryLogId, reply->type);
	    freeReplyObject(reply);
	}
	return NORMAL;
}

time_t MakeITimeFromSTime2(PSTR pszTime)
{
	time_t nTime;
	struct tm struTmNow;

	STR szTemp[5];

	if(strlen(pszTime)!=19)
		return((time_t)(-1));	
	

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime,4);
	struTmNow.tm_year=atoi(szTemp)-1900;

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime+5,2);
	struTmNow.tm_mon=atoi(szTemp)-1;

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime+8,2);
	struTmNow.tm_mday=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime+11,2);
	struTmNow.tm_hour=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime+14,2);
	struTmNow.tm_min=atoi(szTemp);

	memset(szTemp,0,sizeof(szTemp));
	memcpy(szTemp,pszTime+17,2);
	struTmNow.tm_sec=atoi(szTemp);

	nTime=mktime(&struTmNow);

	return nTime;
}

RESULT ClearRedisEleqrylog(int gTaskLogid)
{
	INT nQryLogId, j, nQryLogId2, nTaskLogId;
	char szMessage[8192], szBeginTime[30];
	redisReply *reply, *reply2;
	cJSON* cjson_root = NULL;
	cJSON* cjson_name = NULL;
	
	reply = redisCommand(redisconn,"HGETALL man_eleqrylog");
	
	PrintDebugLogR(DBG_HERE, "HGETALL man_eleqrylog\n");
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HGETALL man_eleqrylog error: %s\n", redisconn->errstr);
		return -1;
	}
	if (reply->type == REDIS_REPLY_ARRAY && reply->elements>=2) {
		for (j = 0; j < reply->elements/2; j++) {
			
			//strcpy(szTaskLog, HmReply->element[j*2]->str);
		    nQryLogId2=atoi(reply->element[j*2]->str);
		        
	        strcpy(szMessage, reply->element[j*2+1]->str);

			cjson_root = cJSON_Parse(szMessage);
		    if(cjson_root == NULL)
		    {
		        PrintErrorLog(DBG_HERE, "parse man_eleqrylog fail.\n");
		        return -1;
		    }
		    
		    cjson_name = cJSON_GetObjectItem(cjson_root, "qry_eleqrylogid");
		    nQryLogId = atoi(cjson_name->valuestring);
			cjson_name = cJSON_GetObjectItem(cjson_root, "qry_begintime");
			strcpy(szBeginTime, cjson_name->valuestring);
			
			cjson_name = cJSON_GetObjectItem(cjson_root, "qry_tasklogid");
			nTaskLogId = atoi(cjson_name->valuestring);
			
			//PrintDebugLogR(DBG_HERE, "man_eleqrylog: %d, %s, %d,%d\n", nQryLogId, szBeginTime, (int)time(NULL), (int)MakeITimeFromSTime2(szBeginTime));
			
			if ((int)time(NULL) > (int)MakeITimeFromSTime2(szBeginTime)+43200 
				|| gTaskLogid==nTaskLogId)
			{
	        	reply2 = redisCommand(redisconn,"HDEL man_eleqrylog %d", nQryLogId);
			    PrintDebugLogR(DBG_HERE, "HDEL man_eleqrylog: %d, %s\n", nQryLogId, szBeginTime);
			    freeReplyObject(reply2);
	        }
			cJSON_Delete(cjson_root);
		}
	}
	
	freeReplyObject(reply);
	
	return NORMAL;
}

RESULT InsertEleQryLog(PXMLSTRU pstruXml)
{
	STR szSql[MAX_SQL_LEN];
	int nIsAlarm, nType;
	
   	nIsAlarm=0; 
   	if (atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�����>"))==1)//��ѯ
    	nType=0;
    else //����
		nType=1;
    //����man_eleqrylog��
    snprintf(szSql, sizeof(szSql),
		    " insert into man_eleqrylog( qry_EleId, qry_Property,qry_Content, qry_Style,"
		    " qry_Commtype, qry_TaskId, qry_User, qry_BeginTime, qry_Number,"
		    " qry_endtime, qry_FailContent, qry_AlarmName, qry_IsSuccess, qry_IsAlarm,"
		    " qry_TxPackCount, qry_RxPackCount, qry_TaskLogId,  qry_windowlogid, qry_packstatus) values("
		    " %d,  '%s',   '%s', %d,"
			" %d, %d, '%s', '%s', '%s', "
			" '%s', '%s', '%s', %d, %d, "
			" %d,  %d, %d, '%s', '%s') ",
			//DemandStrInXmlExt(pstruXml, "<omc>/<��־��>"),

			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<��Ԫ���>")),
			DemandStrInXmlExt(pstruXml, "<omc>/<��ض���>"),
			DemandStrInXmlExt(pstruXml, "<omc>/<Content>"),  //��ض�������
			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�����>")),
			
			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<ͨ�ŷ�ʽ>")),
			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�����>")),
			DemandStrInXmlExt(pstruXml, "<omc>/<�û�>"), 
			DemandStrInXmlExt(pstruXml, "<omc>/<��ʼʱ��>"),
			DemandStrInXmlExt(pstruXml, "<omc>/<��ˮ��>"),
			
			GetSysDateTime(),  //����ʱ��
			DemandStrInXmlExt(pstruXml, "<omc>/<FailContent>"),
			DemandStrInXmlExt(pstruXml, "<omc>/<AlarmName>"),
			0,   												//qry_IsSuccess
			nIsAlarm,   										//qry_IsAlarm
			
			1,													//qry_TxPackCount
			0,													//qry_RxPackCount
    	    atoi(DemandStrInXmlExt(pstruXml, "<omc>/<������־��>")),
			DemandStrInXmlExt(pstruXml,"<omc>/<������ˮ��>"),
			"Timeout"										//qry_packstatus
		);

    PrintDebugLogR(DBG_HERE, "Execute SQL[%s]\n", szSql);
    if(ExecuteSQL(szSql)!=NORMAL)
    {
        PrintErrorLogR(DBG_HERE,"Excute SQL[%s] Info=[%s]\n",szSql, GetSQLErrorMessage());
        return EXCEPTION;
    }
    CommitTransaction();
	
	return NORMAL;
}


/******************************************
 *GetCfgItem() �Ŀ�����汾
 *ȡinit�ļ����������ֵ 
 *cfg_seg   ����
 *cfg_item  ����������
 *value     �������ֵ
 *������    0: �ɹ�
 *         -1: �����ļ�������
 *         -2: δ�ҵ����ý�
 *         -3: δ�ҵ�������
 *******************************************/

/******************************************
 ��ȥע������
 �� libpublic/utils.c �ļ��и��ƹ���
******************************************/

void FreeTaskPara(TASKPARASTRU *pstruTaskPara)
{
    if (pstruTaskPara == NULL)
        return;

    free(pstruTaskPara->struPack.pPack);
    free(pstruTaskPara);
}

TASKPARASTRU *CreateTaskPara()
{
    TASKPARASTRU *pstruTaskPara;

    pstruTaskPara = (TASKPARASTRU *)calloc(1, sizeof (TASKPARASTRU));
    if (pstruTaskPara == NULL) {
        PrintErrorLogR(DBG_HERE, "Failed to allocate memory for task\n");
        return NULL;
    }

    /* pPack point a buffer that is used to send/recv msg to/from peer,
     * so it needs enough space to restore data */
    pstruTaskPara->struPack.pPack = (BYTE*)calloc(1, MAX_BUFFER_LEN);
    if (pstruTaskPara->struPack.pPack == NULL) {
        PrintErrorLogR(DBG_HERE, "Failed to allocate memory for task's buffer\n");
        FreeTaskPara(pstruTaskPara);
        return NULL;
    }

    return pstruTaskPara;
}

void FreeTaskState(TASKSTATESTRU *pstruTaskState)
{
    if (pstruTaskState == NULL)
        return;

    free(pstruTaskState);
}

TASKSTATESTRU *CreateTaskState(void)
{
    TASKSTATESTRU *pstruTaskState;

    pstruTaskState = (TASKSTATESTRU *)calloc(1, sizeof (TASKSTATESTRU));
    if (pstruTaskState == NULL) {
        PrintDebugLogR(DBG_HERE, "Failed to allocate memory for task state\n");
        return NULL;
    }

    return pstruTaskState;
}

/**
 * return: 0 - succeed; -1 - failed.
 */
int AddTaskState(int nDcsId)
{
    TASKSTATESTRU *pstruTaskStateNew, *pstruTaskState, *pstruTaskStateSafe;
    int nNeedDel = 0;
    int nActualDel = 0;
    int ret = -1;

    if (nDcsId < 0)
        return ret;

    pstruTaskStateNew = CreateTaskState();
    if (pstruTaskStateNew == NULL)
        return ret;

    pstruTaskStateNew->nDcsId = nDcsId;

    pthread_mutex_lock(&gTaskStateList->lock);
    /* check if need to delete a outdated item */
    if (gTaskStateList->nCount >= gTaskStateList->nMaxCount) {
        nNeedDel = 1;
        list_for_each_entry_safe(pstruTaskState, pstruTaskStateSafe,
                &gTaskStateList->tasks, list) {
            if (pstruTaskState->nState == 2) {
                /* this task is already updated into gprsqueue,
                 * so delete it */
                list_del(&pstruTaskState->list);
                gTaskStateList->nCount--;
                FreeTaskState(pstruTaskState);
                nActualDel = 1;
                break;
            }
        }
    }

    if (nNeedDel == 0 || (nNeedDel == 1 && nActualDel == 1)) {
        list_add_tail(&pstruTaskStateNew->list, &gTaskStateList->tasks);
        gTaskStateList->nCount++;
        ret = 0;
    } else {
        FreeTaskState(pstruTaskStateNew);
        ret = -1;
    }
    pthread_mutex_unlock(&gTaskStateList->lock);

    return ret;
}

static char * FilterRemark(char *src, char *dest)
{
    char *ptr;
    ptr=strtok(src, "#");
    strcpy(dest, ptr);
    return dest;
}

/*************************
 ���˻س���
 �� libpublic/utils.c �ļ��и��ƹ���
**************************/
static char *FilterCR(char *str)
{
    char *ptr;
    
    ptr = strchr(str, '\n');
    if (ptr != NULL)
        *ptr = '\0';
    return str;
}

static RESULT GetCfgItemR(char *cfg_filename, char *cfg_seg, char *cfg_item, char *value)
{
    FILE *fp;
    char item[256], buffer[256], buf[51], *ptr;
    int flag;

    sprintf(buf, "%s/etc/%s", getenv("HOME"), cfg_filename);
    if ((fp = fopen(buf, "r")) == NULL)
    {
        PrintDebugLogR(DBG_HERE, "���ļ� [%s] �������� ������ļ��Ƿ��ж�ȡȨ��.\n",buf);
	    return EXCEPTION;
	}

    flag = 0;
    sprintf(buf, "[%s]", cfg_seg);
    while (fgets(buffer, 256, fp) != NULL)
    {
        FilterRemark(buffer, item);
        if (strstr(item, buf) != NULL)
        {
            flag = 1;
            break;
        }
    }
    if (flag != 1)
    {
        PrintDebugLogR(DBG_HERE, "δ�ҵ����ö��� [%s] .\n", cfg_seg );
        fclose(fp);
        return EXCEPTION;
    }

    sprintf(buf, "%s=", cfg_item);
    flag = 0;
    while(fgets(buffer, 256, fp) != 0)
    {
        FilterRemark(buffer, item);
        FilterCR(item);
        if (strstr(item, buf) != NULL)
        {
            ptr = strchr(item, '=');
            TrimAllSpace(ptr+1);
            strcpy(value, ptr+1);
            flag = 1;
            break;
        }
        else
        if (strchr(item, '[') != NULL && strchr(item, ']') != NULL)
        {
            flag = 2;
            break;
        }
    } 
    fclose(fp);
    if (flag != 1)
        return EXCEPTION;
    else
    {
    	/* ȡ�����ɹ� */
       	return NORMAL;
    }
}

/**
 * SendUdp() �Ŀ�����汾
 * ����UDP����
 *	
 *	nConnectFd 	���ӵ��ļ�������
 *	pszSendBuffer	���ͻ�����
 *	nSendBufferLenth	Ҫ���͵ı��ĳ���
 *
 * Returns �ɹ�����NORMAL ʧ�ܷ���EXCEPTION
 */
INT SendUdpR(INT nConnectFd,PSTR pszSendBuffer,UINT nSendBufferLenth, struct sockaddr *pClientAddr, INT nCliLen, UINT nTimeout)
{
	INT nWritenLenth;
	PSTR pszSendBufferTemp;
	struct timeval struTimeval;

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;

	if(setsockopt(nConnectFd, SOL_SOCKET, SO_SNDTIMEO, &struTimeval, sizeof(struTimeval)) < 0){
		PrintDebugLogR(DBG_HERE,"���÷��ͳ�ʱʱ�����[%s]\n",strerror(errno));
		return EXCEPTION;
	}
	pszSendBufferTemp=pszSendBuffer;

    nWritenLenth=sendto(nConnectFd,pszSendBufferTemp,nSendBufferLenth, 0, pClientAddr, nCliLen);
    if(nWritenLenth < 0)
    {
        PrintDebugLogR(DBG_HERE,"�������ݵ� UDPSERV ����[%s]\n", strerror(errno));
        return EXCEPTION;
    }

	return NORMAL;
}

/**
 * RecvUdp() �Ŀ�����汾
 * ���տͻ��˵�UDP����
 *	
 * nServerUDPFd	ʹ��CreateListenUdp�������׽���
 * pszRecvBuffer	���ջ�����
 * nRecvBufferLen	Ҫ���յ���󳤶�
 * Returns 	�ɹ����ؽ��յ����ֽ���Ŀ ʧ�ܷ���-1
 */
static INT RecvUdpR(INT nServerSock,PSTR pszRecvBuffer,INT nRecvBufferLen, struct sockaddr *pClientAddr, PINT pnCliLen, UINT nTimeout)
{
	INT nReadLenth;
	PSTR pszRecvBufferTemp;
	struct timeval struTimeval;

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;

	if(setsockopt(nServerSock, SOL_SOCKET, SO_RCVTIMEO, &struTimeval, sizeof(struTimeval)) < 0)
	{
		PrintDebugLogR(DBG_HERE,"���ý��ճ�ʱʱ�����[%s]\n",strerror(errno));
		return EXCEPTION;
	}
	pszRecvBufferTemp=pszRecvBuffer;

    *pnCliLen = sizeof (struct sockaddr);
    nReadLenth=recvfrom(nServerSock,pszRecvBufferTemp,nRecvBufferLen,
        0, pClientAddr, (socklen_t *)pnCliLen);
    if(nReadLenth < 0)
    {
        PrintDebugLogR(DBG_HERE,"RecvUdpR ��ȡ���ݳ�ʱ[%s]\n",
            strerror(errno));
        return EXCEPTION;
    }

    return nReadLenth;
}

static VOID Usage(PSTR pszProg)
{
    fprintf(stderr, "network management system(grrusend) V%s\n", VERSION);
    fprintf(stderr, "%s start (startup program)\n"
            "%s stop  (close program) \n" , pszProg, pszProg);
}


// �˳�����
static VOID SigHandle(INT nSigNo) 
{
	if(nSigNo!=SIGTERM)
		return;
	FreeRedisConn();
	CloseDatabase();
	PrintDebugLogR(DBG_HERE, "Free Redis Conn \n");
	exit(0);
}

//ASCII���ִ���(ASCIIר�̳�HEX)
// �����󳤶Ȼ���
BOOL AscEsc(BYTEARRAY *Pack)
{
 	//�Գ���ʼ��־�ͽ�����־�����������ת��
	int len = (Pack->Len-2)*2+2;
	BYTE pdu[len+1];		 //ת��
	BYTE m_Pdu[Pack->Len+1]; //ԭ��
	
	memset(m_Pdu, 0, Pack->Len);
	memcpy(m_Pdu, Pack->pPack, Pack->Len);
	
	memset(pdu, 0, len+1);
	   
	BYTE Hivalue,Lovalue;
	int i, j;
	for(i=1, j=1; i<Pack->Len-1; i++)
	{
		Hivalue=m_Pdu[i]/16;
		Lovalue=m_Pdu[i]%16;
		Hivalue=Hivalue<10?Hivalue+48:Hivalue+55;
		Lovalue=Lovalue<10?Lovalue+48:Lovalue+55;
		pdu[j++]=Hivalue;
		pdu[j++]=Lovalue;
	}
	pdu[0] = 0x7E;
	pdu[j] = 0x7E;
	
	//����ݽ���������
	memset(Pack->pPack, 0, len+1);
	Pack->Len = len;
	memcpy(Pack->pPack, pdu, len);
	return TRUE;
}

//����ASCII���ִ���
//Pack ������������
//m_Pdu Ϊ�����Ľ��
// �����󳤶Ȼ��С
BOOL AscUnEsc(BYTEARRAY *Pack)
{
	int Hivalue,Lovalue,temp;
	int i, j;
	char m_Pdu[MAX_BUFFER_LEN];
	
	bufclr(m_Pdu);
	m_Pdu[0]=0x7E;
	for(i=1,j=1; i<Pack->Len-1; i++)
	{
		temp=Pack->pPack[i];
		Hivalue=(temp>=48&&temp<=57)?temp-48:temp-55;
		temp=Pack->pPack[i+1];
	 	Lovalue=(temp>=48&&temp<=57)?temp-48:temp-55;
		m_Pdu[j] = (Hivalue*16+Lovalue);
		j++;
		i++;
	}
	m_Pdu[j] = 0x7E;
	j++;
	
	memset(Pack->pPack, 0, j+1);
	Pack->Len = j;
	memcpy(Pack->pPack, m_Pdu, j);

	return TRUE;
}

/**
 * CreateConnectSocket() �Ŀ�����汾
 * ������������������
 *
 * pszHostString	Ҫ���ӵ���������
 * nPort		Ҫ���ӵ������˿�
 * nTimeout	��ʱ������.Ϊ0�͹�ó�ʱʱ���?60
 *
 * �ɹ����� ���Ӻõ��׽��� ʧ�ܷ��� -1
 */
INT CreateConnectSocketR(PCSTR pszHostString,UINT nPort, UINT nTimeout)
{
	INT nConnectFd;
	struct sockaddr_in struInAddr;
	struct hostent * pstruHost;
	struct timeval struTimeval;
    pthread_t ThreadId = pthread_self();

	memset(&struInAddr,0,sizeof(struInAddr));
	if((inet_aton(pszHostString,(struct in_addr *)&struInAddr.sin_addr))==0)
	{
		if((pstruHost=gethostbyname(pszHostString))==NULL)
		{
			PrintDebugLogR(DBG_HERE,"Failed to get host ip [%s]\n",
				hstrerror(h_errno));
			return -1;
		}
		struInAddr.sin_addr=*(struct in_addr *)pstruHost->h_addr_list[0];
	}
	struInAddr.sin_family=AF_INET;
	struInAddr.sin_port=htons(nPort);

	nConnectFd=socket(AF_INET,SOCK_STREAM,0);
	if(nConnectFd==-1)
	{
		PrintDebugLogR(DBG_HERE,"[%X] Failed to execute socket() [%s]\n", ThreadId, strerror(errno));
		return -1;
	}

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;
	if(setsockopt(nConnectFd, SOL_SOCKET, SO_RCVTIMEO, &struTimeval, sizeof(struTimeval)) < 0)
	{
		PrintDebugLogR(DBG_HERE,"[%X] Failed to execute setsockopt() [%s]\n", ThreadId, strerror(errno));
		close(nConnectFd);
		return EXCEPTION;
	}

	if(connect(nConnectFd,(struct sockaddr *)&struInAddr,sizeof(struct sockaddr))==-1)
	{
		PrintDebugLogR(DBG_HERE,"[%X] Failed to execute connect() [%s]\n", ThreadId, strerror(errno));
		close(nConnectFd);
		return EXCEPTION;
	}

	return nConnectFd;
}

/**
 * SendSocketNoSync �Ŀ�����汾
 * ��ͬ���ַ���Socket���ͱ���
 * Input:
 *	nConnectFd 	���ӵ��ļ�������
 *	pszSendBuffer	���ͻ�����
 *	nSendBufferLenth	Ҫ���͵ı��ĳ���
 *	nTimeout		��ʱʱ��(��),���Ϊ0�����ȱʡ�ĳ�ʱʱ��60
 *
 * Returns �ɹ�����NORMAL ʧ�ܷ���EXCEPTION
 */
RESULT SendSocketNoSyncR(INT nConnectFd,PSTR pszSendBuffer,UINT nSendBufferLenth, UINT nTimeout)
{
	INT nWritenLenth;
	PSTR pszSendBufferTemp;
	struct timeval struTimeval;
    pthread_t ThreadId = pthread_self();

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;
	if(setsockopt(nConnectFd, SOL_SOCKET, SO_SNDTIMEO,
		&struTimeval, sizeof(struTimeval)) < 0){
		PrintDebugLogR(DBG_HERE,"[%X] Failed to set timeout for socket [%s]\n", ThreadId, strerror(errno));
		return EXCEPTION;
	}
	pszSendBufferTemp=pszSendBuffer;
	while (nSendBufferLenth > 0)
	{
		nWritenLenth=send(nConnectFd,pszSendBufferTemp,nSendBufferLenth, 0);
		if (nWritenLenth < 0)
		{
			PrintDebugLogR(DBG_HERE, "[%X] SendSocketNoSyncR failed to send msg[%s]\n",
				ThreadId, strerror(errno));
			return EXCEPTION;
		}
		nSendBufferLenth-=nWritenLenth;
		pszSendBufferTemp+=nSendBufferLenth;
	}

	return NORMAL;
}

/**
 * RecvSocketNoSync �Ŀ�����汾
 *��ͬ���ַ���Socket��������
 *Input:
 *	nConnectFd	���ӵ��ļ�������
 *	pszRecvBuffer	���ջ�����
 *	nRecvBufferLen	Ҫ���յĳ���
 *	nTimeout		��ʱʱ��(��),���Ϊ0�����ȱʡ�ĳ�ʱʱ��60
 *
 * �ɹ�����NORMAL ʧ�ܷ���EXCEPTION
 */
RESULT RecvSocketNoSyncR(INT nConnectFd,PSTR pszRecvBuffer,UINT nRecvBufferLen,UINT nTimeout)
{
	INT nReadLenth = -1;
	PSTR pszRecvBufferTemp;
	struct timeval struTimeval;

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;
	if(setsockopt(nConnectFd, SOL_SOCKET, SO_RCVTIMEO,
		&struTimeval, sizeof(struTimeval)) < 0){
		PrintDebugLogR(DBG_HERE,"Failed to set timeout for socket [%s]\n", strerror(errno));
		return EXCEPTION;
	}

	pszRecvBufferTemp = pszRecvBuffer;
    int nRetry = 3;
	while(nRecvBufferLen>0)
	{
		nReadLenth = recv(nConnectFd, pszRecvBufferTemp, nRecvBufferLen, 0);
		if (nReadLenth < 0)
		{
            if (errno == EAGAIN && nRetry--)
                continue;

			PrintDebugLogR(DBG_HERE,"Failed to recv msg from socket(%d)[%s]\n",
				nConnectFd, strerror(errno));
			return EXCEPTION;
		}
		else if(nReadLenth == 0)
		{
			return NORMAL;
		}
		nRecvBufferLen-=nReadLenth;
		pszRecvBufferTemp+=nReadLenth;
        break;
	}

	return nReadLenth;
}

/*
 * ͬGPRSSERV����ͨ�ţ���ͬGPRSSEND
 * ExchDataGprsSvr() �Ŀ������
 */
static int ExchDataGprsSvrR(int nLogId, PSTR pszRespBuffer)
{
	STR szBuffer[MAX_BUFFER_LEN];
	INT nConnectFd;
    pthread_t ThreadId = pthread_self();
    int nTimeOut = 30; /* unit: second */

	/*
     * timeout is 60 seconds, give gprsserv more time to accept
	 */
	if((nConnectFd = CreateConnectSocketR("127.0.0.1", 8803, nTimeOut)) < 0)
	{
		PrintDebugLogR(DBG_HERE,
			"[%X] Failed to connect to '127.0.0.1:8803' GPRSSERV\n", ThreadId);
		return -1;
	}
	memset(szBuffer, 0, sizeof(szBuffer));
	sprintf(szBuffer, "%d,%s", nLogId, pszRespBuffer);
	if(SendSocketNoSyncR(nConnectFd, szBuffer, strlen(szBuffer), nTimeOut) < 0)
	{
		PrintErrorLogR(DBG_HERE, "[%X] Failed to send msg to GPRSSERV: %s\n", ThreadId, strerror(errno));
		close(nConnectFd);
		return -1;
	}
    //PrintDebugLogR(DBG_HERE, "[%X] Send msg to GPRSSERV [%s]\n", ThreadId, pszRespBuffer);

	memset(szBuffer, 0, sizeof(szBuffer));
	/*
     * 1. just check if gprsserv has received packet, so it's not neccesary to recv all datas that gprsserv respond.
     * 2. timeout is 600 seconds, give gprsserv more time to receive the msg.
	 */
	if(RecvSocketNoSyncR(nConnectFd, szBuffer, 4, nTimeOut) <= 0)
	{
		PrintErrorLogR(DBG_HERE, "[%X] Failed to receive msg from GPRSSERV: %s\n", ThreadId, strerror(errno));
		close(nConnectFd);
		return -1;
	}
    //PrintDebugLogR(DBG_HERE, "[%X] Receive msg from GPRSSERV [%s]\n", ThreadId, szBuffer);
	
	close(nConnectFd);

	return 0;
}

#if 0
static void *DeliverThreadGprsserv(void *arg)
{
	TASKPARASTRU *pstruTaskPara = NULL, *pstruTaskParaSafe = NULL;
	PSTR pPack = NULL;
    int nRet = -1;
    int i;
 
    while (1)
    {
        pthread_mutex_lock(&g_packets_lock);
        if (list_empty(&g_packets_gprsserv))
        {
            pthread_mutex_unlock(&g_packets_lock);
            sleep(1);
            continue;
        }

        list_for_each_entry_safe(pstruTaskPara, pstruTaskParaSafe, &g_packets_gprsserv, list)
        {
            list_del(&pstruTaskPara->list);
            break;
        }
        pthread_mutex_unlock(&g_packets_lock);

        for (i = 0; i < nCfgResendTimes; i++) {
            pPack = (PSTR)pstruTaskPara->struPack.pPack;
            nRet = ExchDataGprsSvrR(pstruTaskPara->nDcsId, pPack);
            if (nRet == 0)
                break;
        }

        FreeTaskPara(pstruTaskPara);
    }

    return NULL;
}
#endif

/**
 * ����ne_gprsqueue��״̬
 * @para[in] pszIds: ���qs_id���ö��ŷָ�
 * @para[in] pszMsgStat: qs_id ��Ӧ�ļ�¼��Ҫ���µ�״̬
 */
static RESULT UpdateGprsQueueMsgstat(int nDcsId, PSTR pszMsgStat)
{
	char szSql[MAX_BUFFER_LEN];
	INT nDelay = 0;
	INT nRetry = 0;
    pthread_t ThreadId = pthread_self();


	memset(szSql, 0, sizeof(szSql));
	snprintf(szSql, sizeof(szSql),
		"update ne_gprsqueue set "
		"qs_msgstat='%s', qs_lasttime='%s' %s "
		"where qs_id = %d",
		pszMsgStat, MakeSTimeFromITime((INT)time(NULL)+ nDelay), nRetry?",qs_retrytimes=qs_retrytimes+1":"", nDcsId);

	PrintDebugLogR(DBG_HERE, "[%X] ��ʼִ��SQL[%s]\n", ThreadId, szSql);
	if(ExecuteSQL(szSql) != NORMAL)
	{
		PrintDebugLogR(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL; 
}

/**
 * ClientSendRecvUdp() �Ŀ������
 * ����UDP���ĵ�������
 *	
 * pszIpAddr	��������IP��ַ(���Ϊ�����͵�127.0.0.1)
 * nPort		�����������Ķ˿ں�
 * pszBuffer	���ͻ�����
 * nSendBufferLen	Ҫ���͵ı��ĳ���
 *
 * Returns	�ɹ������յ����ֽ�����ʧ�ܷ���EXCEPTION
 */
RESULT ClientSendRecvUdpR(PCSTR pszIpAddr,UINT nPort,PSTR pszBuffer,INT nSendBufferLen, int nLogId)
{
	INT nFd;
	struct sockaddr_in struServerAddr;
	struct sockaddr_in struSendAddr;
	struct sockaddr_in struRecvAddr;
	struct hostent *pstruHost;
	INT nLen, nAddrLen;
    char szRecvBuffer[MAX_BUFFER_LEN];

	if(pszIpAddr==NULL)
		pszIpAddr=LOCAL_HOST_IP_ADDR;

	memset(&struServerAddr,0,sizeof(struServerAddr));
	if((inet_aton(pszIpAddr,(struct in_addr *)&struServerAddr.sin_addr))==0)
	{
		pstruHost=gethostbyname(pszIpAddr);
		if(pstruHost==NULL)
		{
			PrintDebugLogR(DBG_HERE,"Failed to execute gethostbyname()[%s]\n",hstrerror(h_errno));	
			return EXCEPTION;
		}
		struServerAddr.sin_addr=*(struct in_addr *)pstruHost->h_addr_list[0];
	}
	struServerAddr.sin_port=htons(nPort);
	struServerAddr.sin_family=AF_INET;

	nFd=socket(AF_INET,SOCK_DGRAM,0);
	if(nFd==-1)
	{
		PrintDebugLogR(DBG_HERE,"Failed to execute socket() [%s]\n", strerror(errno));
		return EXCEPTION;
	}

	/* ���ָ���˱��� IP ��ַ���Ͱ� */
	if (*szLocalIpAddr)
	{
		memset(&struSendAddr, 0, sizeof (struct sockaddr_in));
		struSendAddr.sin_family = AF_INET;
		if((inet_aton(szLocalIpAddr,(struct in_addr *)&struSendAddr.sin_addr))==0)
			PrintDebugLogR(DBG_HERE,"Failed to inet_aton(%s): %s\n",
				szLocalIpAddr, hstrerror(h_errno));
		else if(bind(nFd, (struct sockaddr *)&struSendAddr, sizeof (struct sockaddr)) < 0)
			PrintDebugLogR(DBG_HERE,
				"Failed to bind IP(%s), send msg with any valid IP address\n", szLocalIpAddr);
	}

	/* ת��Զ�˵� IP ��ַ */
	if((inet_aton(pszIpAddr,(struct in_addr *)&struServerAddr.sin_addr))==0)
	{
        PrintDebugLogR(DBG_HERE,"Failed to inet_aton(%s): %s\n",
                pszIpAddr, hstrerror(h_errno));
		close(nFd);
		return EXCEPTION;
	}

	//���ͱ���
	nLen=SendUdpR(nFd, pszBuffer, nSendBufferLen,
		(struct sockaddr*)&struServerAddr, sizeof(struServerAddr),
		SOCK_TIMEOUT);
	if (nLen < 0)
	{
		close(nFd);
		return EXCEPTION;
	}

	//���ձ���
	memset(szRecvBuffer, 0, sizeof (szRecvBuffer));
	nAddrLen = sizeof(struRecvAddr);
	nLen=RecvUdpR(nFd, szRecvBuffer, sizeof (szRecvBuffer)-1,
		(struct sockaddr*)&struRecvAddr, &nAddrLen, SOCK_TIMEOUT);

	close(nFd);

    if (nLen > 0) {
        memset(pszBuffer, 0, MAX_BUFFER_LEN);
        memcpy(pszBuffer, szRecvBuffer, nLen);
        return nLen;
    } else{
        PrintDebugLogR(DBG_HERE, "[%X] [%d] Failed to recv from device\n", pthread_self(), nLogId);
		return EXCEPTION;
    }
}

static void _setTaskState(int nDcsId, int nState)
{
    TASKSTATESTRU *pstruTaskNode;

    pthread_mutex_lock(&gTaskStateList->lock);
    list_for_each_entry(pstruTaskNode, &gTaskStateList->tasks, list) {
        if (pstruTaskNode->nDcsId == nDcsId)
        {
            pstruTaskNode->nState = nState;
            break;
        }
    }
    pthread_mutex_unlock(&gTaskStateList->lock);
}

static int _UnEsc_c(BYTEARRAY *Pack)
{
    int i, j;
    char m_Pdu[MAX_BUFFER_LEN] = {0};
    int m_Len;

    if (Pack == NULL || Pack->pPack == NULL \
            || Pack->Len < 10 || Pack->Len > sizeof (m_Pdu))
        return -1;

    for(i=1, j=0; i<Pack->Len-1; i++, j++)
    {
        if(Pack->pPack[i]==0x5E && Pack->pPack[i+1]==0x5D)
        {
            m_Pdu[j] = Pack->pPack[i];
            i++;
        }
        else if(Pack->pPack[i]==0x5E && Pack->pPack[i+1]==0x7D)
        {
            m_Pdu[j] = 0x7E;
            i++;
        }
        else
        {
            m_Pdu[j] = Pack->pPack[i];
        }

    }
    m_Len = j;
    memcpy(Pack->pPack, m_Pdu, m_Len);

    return 0;
}

static int _is_valid_resp_pkg(char *pkg, int len)
{
    BYTEARRAY Pack;
    unsigned char pkg_tmp[MAX_BUFFER_LEN];

    if (len < 11 || len > sizeof (pkg_tmp))
        return 0;

    Pack.Len = len;
    Pack.pPack = pkg_tmp;
    memcpy(pkg_tmp, pkg, len);
    if (_UnEsc_c(&Pack) < 0) {
        PrintDebugLogR(DBG_HERE, "Failed to UnEsc_c()\n");
        return 0;
    }

#if 0
    int i, length;
    char pkg_tmp2[128] = {0};
    for (i = 0; i < len; i++) {
        length = strlen(pkg_tmp2);
        snprintf(pkg_tmp2+length, sizeof (pkg_tmp2)-length, "%.2X", pkg_tmp[i]);
    }
    PrintDebugLogR(DBG_HERE, "Check response: %s\n", pkg_tmp2);
#endif

    /* NP flag is set to 0x01, means NE is busy */
    if (pkg_tmp[9] == 0x01) {
        PrintDebugLogR(DBG_HERE, "[%X] Invalid response, NP flag: %x\n", pthread_self(), pkg_tmp[9]);
        return 0;
    }

    return 1;
}

void ProcessGprsOneTask(TASKPARASTRU *pstruTaskPara)
{
	PSTR pPack;
	int Len, i;
	int nRecvLen;
    pthread_t ThreadId = pthread_self();
    char pdu[MAX_BUFFER_LEN] = {0};
    char pdu_hex[MAX_BUFFER_LEN] = {0};
    unsigned char tmp_hex[MAX_BUFFER_LEN] = {0};
    int nIsSuccess = 0;
    int nRet = -1;
    int timeout_cnt = 0;
    int invalid_resp_cnt = 0;
    BYTEARRAY Pack;

    if (pstruTaskPara == NULL)
        return;

	pPack = (PSTR)pstruTaskPara->struPack.pPack;
	Len = pstruTaskPara->struPack.Len;

    snprintf(pdu, sizeof (pdu) - 1, pPack);
    /* ascii to hex */
    if(!AscUnEsc(&pstruTaskPara->struPack))
    {
        PrintDebugLogR(DBG_HERE, "[%X] [%d] ת�����[%s]\n", ThreadId, pstruTaskPara->nDcsId, pPack);
        return;
    }
    /* ��ԭת���struPack.Len ֵ���С */
    Len = pstruTaskPara->struPack.Len;
    memcpy(pdu_hex, pPack, Len);

    /* communicate with NE */
    nIsSuccess = 0;
    for (i = 0; i < pstruTaskPara->nResendTimes; i++) {
        memcpy(tmp_hex, pPack, Len);
        Pack.pPack = tmp_hex;
        Pack.Len = Len;
        AscEsc(&Pack);
        //PrintDebugLogR(DBG_HERE, "[%X] [%d] Times%d����[%s][%d]\n",
        //        ThreadId, pstruTaskPara->nDcsId, i,
        //       pstruTaskPara->szDeviceIp, pstruTaskPara->nPort);

		nRecvLen = ClientSendRecvUdpR(pstruTaskPara->szDeviceIp,
                pstruTaskPara->nPort, pPack, Len, pstruTaskPara->nDcsId);
		if (nRecvLen <= 0) {
            timeout_cnt++;
            if (timeout_cnt < 2 && timeout_cnt + invalid_resp_cnt < 3){
            	//sleep(1);
                continue;
            }
            else
                break;
        }


        memcpy(tmp_hex, pPack, nRecvLen);
        Pack.pPack = tmp_hex;
        Pack.Len = nRecvLen;
        AscEsc(&Pack);
        PrintDebugLogR(DBG_HERE, "[%X] [%d] Ӧ��[%s]\n", ThreadId, pstruTaskPara->nDcsId, Pack.pPack);

        if (_is_valid_resp_pkg(pPack, nRecvLen) == 0) {
            invalid_resp_cnt++;
            if (timeout_cnt+invalid_resp_cnt < 3) {
                sleep(10);
                memcpy(pPack, pdu_hex, Len);
                continue;
            } else
                break;
        }

        /* hex to ascii */
        pstruTaskPara->struPack.Len = nRecvLen;
        if(!AscEsc(&pstruTaskPara->struPack))
        {
            PrintDebugLogR(DBG_HERE,"[%X] [%d] ת�����\n", ThreadId, pstruTaskPara->nDcsId);
            break;
        }

        nIsSuccess = 1;
        break;
    }

    if (nIsSuccess == 0) { /* failed to communicate with NE */
        _setTaskState(pstruTaskPara->nDcsId, -1);
        return;
    }

    nIsSuccess = 0;
    //for (i = 0; i < nCfgResendTimes; i++) {
    	//PrintDebugLogR(DBG_HERE,"[%X] [%d] Send Message to GprsServ\n", ThreadId, pstruTaskPara->nDcsId);
        pPack = (PSTR)pstruTaskPara->struPack.pPack;
        nRet = ExchDataGprsSvrR(pstruTaskPara->nDcsId, pPack);
        //nRet = PushEleResp(pstruTaskPara->nDcsId, pPack);
        if (nRet == 0) {
            nIsSuccess = 1;
            //break;
        }
    //}

    if (nIsSuccess == 0) {
        _setTaskState(pstruTaskPara->nDcsId, -1);
        return;
    }

    /* work complete */
    _setTaskState(pstruTaskPara->nDcsId, 1);
}

static void *DeliverThreadNe(void *arg)
{
	TASKPARASTRU *pstruTaskPara, *pstruTaskParaSafe;

    while (1) {
        pthread_mutex_lock(&g_packets_lock);
        if (list_empty(&g_packets_ne)) {
            pthread_mutex_unlock(&g_packets_lock);
            sleep(1);
            continue;
        }
        list_for_each_entry_safe(pstruTaskPara, pstruTaskParaSafe, &g_packets_ne, list) {
            list_del(&pstruTaskPara->list);
            break;
        }
        pthread_mutex_unlock(&g_packets_lock);

        ProcessGprsOneTask(pstruTaskPara);
        FreeTaskPara(pstruTaskPara);
    }

    return NULL;
}

static TASKSTATESTRU* _DupTaskState(TASKSTATESTRU *src)
{
    TASKSTATESTRU *new;

    if (src == NULL)
        return NULL;

    new = (TASKSTATESTRU*)calloc(1, sizeof (TASKSTATESTRU));
    if (new == NULL) {
        PrintDebugLogR(DBG_HERE, "Failed to allocate memory\n");
        return NULL;
    }
    new->nDcsId = src->nDcsId;
    new->nState = src->nState;
    return new;
}

/*
 * ����gprs��Ϣ����
 */
RESULT ProcessPoolTaskQueue()
{
	STR szDataBuffer[MAX_BUFFER_LEN];
	STR szJsonBuffer[MAX_BUFFER_LEN];
	INT nDcsId, nPort;
    int nMaxDcsId = 0; /* restore the max nDcsId at one round */
	INT nDataLen=0;
	UINT nSleepTime=0;
	INT nRet, j;
	STR szDeviceIp[20], szKeyField[100], szEventTime[30];
	TASKPARASTRU *pstruTaskPara;
    TASKSTATESTRU *pstruTaskState, *pstruTaskStateSafe, *pstruTaskStateNew;
    struct list_head to_update;
    int nTaskLogId;
    int nFindIt = 0;
    XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
    cJSON* cjson_root = NULL;
    cJSON* cjson_item = NULL;
	redisReply *reply, *reply2;
	redisReply *HmReply;

	
    INIT_LIST_HEAD(&to_update);
	while(TRUE)
	{
        /* ping sql every 60 second 
        if (SQLPingInterval(60) <0)
        {
        	CloseDatabase();
	    	sleep(10);
	    	if(OpenDatabase(szServiceName, szDbName, szUser, szPwd) != NORMAL)
		    {
		        PrintErrorLogR(DBG_HERE, "grrusend�����ݿⷢ������ [%s]\n",
		                GetSQLErrorMessage());
		        return EXCEPTION;
		    }
        }*/
        
        if (RedisPingInterval(60)!=NORMAL)
        {
        	FreeRedisConn();
	    	sleep(10);
	    	if (ConnectRedis() !=NORMAL)
		    {
		        PrintErrorLogR(DBG_HERE, "Connect Redis Error Ping After\n");
		        return EXCEPTION;
		    }
        }

        /* clear remain tasks */
		if(nSleepTime ==0 || nSleepTime ==500)
		{
			sleep(1);
        }
        nSleepTime=0;
        

		/* step 1. walk the list of task state, update database */
        int nSucceed=0, nFailed=0, nInProcessing=0, nUpdated=0, nTotal=0, nCount=0;
        pthread_mutex_lock(&gTaskStateList->lock);
        list_for_each_entry(pstruTaskState, &gTaskStateList->tasks, list) {
            if (pstruTaskState->nState == 1) {
                pstruTaskStateNew = _DupTaskState(pstruTaskState);
                if (pstruTaskStateNew == NULL) {
                    continue;
                }
                list_add(&pstruTaskStateNew->list, &to_update);
                nSucceed++;
                pstruTaskState->nState = 2;
            } else if (pstruTaskState->nState == -1) {
                pstruTaskStateNew = _DupTaskState(pstruTaskState);
                if (pstruTaskStateNew == NULL) {
                    continue;
                }
                list_add(&pstruTaskStateNew->list, &to_update);
                nFailed++;
                pstruTaskState->nState = 2;
            } else if (pstruTaskState->nState == 0) {
                nInProcessing++;
            } else if (pstruTaskState->nState == 2) {
                nUpdated++;  //����ִ��
            }
            nTotal++;
        }
        nCount = gTaskStateList->nCount;
        pthread_mutex_unlock(&gTaskStateList->lock);
		//ֻ�������б�״̬�б���AddTaskStateɾ��
        list_for_each_entry_safe(pstruTaskState, pstruTaskStateSafe, &to_update, list) {
            list_del(&pstruTaskState->list);
            if (pstruTaskState->nState == 1)
            {
                //UpdateGprsQueueMsgstat(pstruTaskState->nDcsId, OMC_SENT_MSGSTAT);
            }
            else
            {
                //UpdateGprsQueueMsgstat(pstruTaskState->nDcsId, OMC_FAIL_MSGSTAT);
                memset(pstruXml, 0, sizeof(XMLSTRU));
				CreateXml(pstruXml, FALSE, OMC_ROOT_PATH, NULL);
                if (GetRedisPackageInfo(pstruTaskState->nDcsId, pstruXml)==NORMAL){
                	InsertEleQryLog(pstruXml);
                }
                DeleteXml(pstruXml);
            }
            free(pstruTaskState);
        }
        
        
        //if (nSucceed>0 || nInProcessing>0)
		//PrintDebugLogR(DBG_HERE, "[PoolTask]success: %d, failed: %d, inprocessing: %d, updated: %d. [total: %d VS %d]\n",
        //            nSucceed, nFailed, nInProcessing, nUpdated, nTotal, gTaskStateList->nCount);
		
		/* step 2. fetch tasks from redis */
		HmReply = redisCommand(redisconn,"keys Queue*");

	    //PrintDebugLogR(DBG_HERE,"HGETALL PoolTaskLog: %d, %d\n", HmReply->type, HmReply->elements);
	    if (HmReply == NULL || redisconn->err) {   //10.25
			PrintErrorLog(DBG_HERE, "Redis keys Queue* error: %s\n", redisconn->errstr);
			sleep(10);
			continue;
		}
		
	    if (HmReply->type == REDIS_REPLY_ARRAY && HmReply->elements>0) {
	    	
		    for (j = 0; j < HmReply->elements; j++) {

		        strcpy(szKeyField, HmReply->element[j]->str);
        
	        	//�����ѭ��������, ���ȼ�С���ȷ���
		        if (nInProcessing +j >= gTaskStateList->nMaxCount)
		    	{
		    		sleep(1);
		    		break;
		    	}
		        reply = redisCommand(redisconn,"RPOP %s", szKeyField);
				if (reply == NULL || redisconn->err) {   //10.25
					PrintErrorLog(DBG_HERE, "Redis BRPOP TaskLog error: %s\n", redisconn->errstr);
					break;
				}
			    //PrintDebugLogR(DBG_HERE, "PoolTask %s [%d]\n",  szTaskLog, reply->type);
			    if (!(reply->type == REDIS_REPLY_STRING))
				{
				 	freeReplyObject(reply);
				 	break;
				}
			 
			 	//��ʼ������
				nSleepTime++;
				if (reply->type == REDIS_REPLY_STRING){
				 	 strcpy(szJsonBuffer, reply->str);
				     PrintDebugLogR(DBG_HERE, "%s [%s]\n",  szKeyField, reply->str);
				     
				     cjson_root = cJSON_Parse(szJsonBuffer);
					 if(cjson_root == NULL)
					 {
					     PrintErrorLogR(DBG_HERE, "cJSON_Parse fail %s\n", szJsonBuffer);
					     continue;
					 }
					 cjson_item = cJSON_GetObjectItem(cjson_root, "qs_id");
					 nDcsId = cjson_item->valueint;
					 bufclr(szDataBuffer);
					 cjson_item = cJSON_GetObjectItem(cjson_root, "qs_content");
					 strcpy(szDataBuffer, cjson_item->valuestring);
					 nDataLen = strlen(szDataBuffer);
					 cjson_item = cJSON_GetObjectItem(cjson_root, "qs_tasklogid");
				     nTaskLogId = cjson_item->valueint;
				     cjson_item = cJSON_GetObjectItem(cjson_root, "qs_port");
					 nPort = cjson_item->valueint;
					 cjson_item = cJSON_GetObjectItem(cjson_root, "qs_ip");
					 strcpy(szDeviceIp, cjson_item->valuestring);
					 cjson_item = cJSON_GetObjectItem(cjson_root, "qs_eventtime");
					 strcpy(szEventTime, cjson_item->valuestring);
			   		 cJSON_Delete(cjson_root);
			   		 
			   		 if ((int)time(NULL) - (int)MakeITimeFromLastTime(szEventTime)>43200)
			   		 	continue;
			   		    
					 /* ignore this task if already exists */
					 nFindIt = 0;
				     pthread_mutex_lock(&gTaskStateList->lock);
				     list_for_each_entry(pstruTaskState, &gTaskStateList->tasks, list) {
				         if (pstruTaskState->nDcsId == nDcsId) {
				             nFindIt = 1;
				             break;
				         }
				     }
				     pthread_mutex_unlock(&gTaskStateList->lock);
				     if (nFindIt == 1)
				     {
				     	PrintErrorLogR(DBG_HERE,"Failed to list_for_each_entry %d\n", nDcsId);
				         continue;
					 }
				     /* this is a valid task */
					 pstruTaskPara = CreateTaskPara();
					 if (pstruTaskPara == NULL) {
					 	sleep(2);
					 	break;
					 }
			         
					 strncpy((STR*)pstruTaskPara->struPack.pPack, szDataBuffer, nDataLen);
					 pstruTaskPara->struPack.Len = nDataLen;
					 strcpy(pstruTaskPara->szDeviceIp, szDeviceIp);
					 pstruTaskPara->nPort = nPort;
					 pstruTaskPara->nDcsId = nDcsId;
					 pstruTaskPara->nTaskLogId = nTaskLogId;
				     if (nTaskLogId == 0) /* query task, just send 1 time */
				         pstruTaskPara->nResendTimes = 1;
				     else /* polling task, has 3 opportunities */
				         pstruTaskPara->nResendTimes = 1;
				     
				     nRet = AddTaskState(pstruTaskPara->nDcsId);
				     if (nRet < 0) { /* maybe gTaskStateList is full */
				         FreeTaskPara(pstruTaskPara);
				         reply2 = redisCommand(redisconn,"RPUSH %s %s", szKeyField, szJsonBuffer);
				         PrintDebugLogR(DBG_HERE, "RPUSH: %s, %s\n", szKeyField, szJsonBuffer);
				         freeReplyObject(reply2);
				         sleep(2);
				         break;     
				     } else {
				         pthread_mutex_lock(&g_packets_lock);
				         list_add_tail(&pstruTaskPara->list, &g_packets_ne);
				         pthread_mutex_unlock(&g_packets_lock);
				         if (pstruTaskPara->nDcsId > nMaxDcsId)
				             nMaxDcsId = pstruTaskPara->nDcsId;
				     }
				}
				freeReplyObject(reply);

			}
		    
		    freeReplyObject(HmReply);
		}
		else{
			freeReplyObject(HmReply);
			sleep(2);
			continue;  //break; //������
		}

	}
	return NORMAL;
}

/*
 * ����query set��Ϣ����
 */
RESULT ProcessRealTimeQueue()
{
	STR szDataBuffer[MAX_BUFFER_LEN];
	STR szJsonBuffer[MAX_BUFFER_LEN];
	INT nDcsId, nPort;
    int nMaxDcsId = 0; /* restore the max nDcsId at one round */
	INT nDataLen=0;
	UINT nSleepTime=0;
	INT nRet, i;
	STR szDeviceIp[20];
	TASKPARASTRU *pstruTaskPara;
    TASKSTATESTRU *pstruTaskState, *pstruTaskStateSafe, *pstruTaskStateNew;
    struct list_head to_update;
    int nTaskLogId;
    int nFindIt = 0;
    XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
    cJSON* cjson_root = NULL;
    cJSON* cjson_item = NULL;
	redisReply *reply, *reply2;

	
    INIT_LIST_HEAD(&to_update);
	while(TRUE)
	{
        /* ping sql every 60 second 
        if (SQLPingInterval(60) <0)
        {
        	CloseDatabase();
	    	sleep(10);
	    	if(OpenDatabase(szServiceName, szDbName, szUser, szPwd) != NORMAL)
		    {
		        PrintErrorLogR(DBG_HERE, "grrusend�����ݿⷢ������ [%s]\n",
		                GetSQLErrorMessage());
		        return EXCEPTION;
		    }
        }*/
		if (RedisPingInterval(60)!=NORMAL)
        {
        	FreeRedisConn();
	    	sleep(10);
	    	if (ConnectRedis() !=NORMAL)
		    {
		        PrintErrorLogR(DBG_HERE, "Connect Redis Error Ping After\n");
		        return EXCEPTION;
		    }
        }
        /* clear remain tasks */
		if(nSleepTime ==0 || nSleepTime ==100)
		{
			sleep(1);
        }
        nSleepTime=0;

		/* step 1. walk the list of task state, update database */
        int nSucceed=0, nFailed=0, nInProcessing=0, nUpdated=0, nTotal=0, nCount=0;
        pthread_mutex_lock(&gTaskStateList->lock);
        list_for_each_entry(pstruTaskState, &gTaskStateList->tasks, list) {
            if (pstruTaskState->nState == 1) {
                pstruTaskStateNew = _DupTaskState(pstruTaskState);
                if (pstruTaskStateNew == NULL) {
                    continue;
                }
                list_add(&pstruTaskStateNew->list, &to_update);
                nSucceed++;
                pstruTaskState->nState = 2;
            } else if (pstruTaskState->nState == -1) {
                pstruTaskStateNew = _DupTaskState(pstruTaskState);
                if (pstruTaskStateNew == NULL) {
                    continue;
                }
                list_add(&pstruTaskStateNew->list, &to_update);
                nFailed++;
                pstruTaskState->nState = 2;
            } else if (pstruTaskState->nState == 0) {
                nInProcessing++;
            } else if (pstruTaskState->nState == 2) {
                nUpdated++;
            }
            nTotal++;
        }
        nCount = gTaskStateList->nCount;
        pthread_mutex_unlock(&gTaskStateList->lock);
		//ֻ�������б�״̬�б���AddTaskStateɾ��
        list_for_each_entry_safe(pstruTaskState, pstruTaskStateSafe, &to_update, list) {
            list_del(&pstruTaskState->list);
            if (pstruTaskState->nState == 1)
            {
                //UpdateGprsQueueMsgstat(pstruTaskState->nDcsId, OMC_SENT_MSGSTAT);
            }
            else
            {
                //UpdateGprsQueueMsgstat(pstruTaskState->nDcsId, OMC_FAIL_MSGSTAT);
                memset(pstruXml, 0, sizeof(XMLSTRU));
				CreateXml(pstruXml, FALSE, OMC_ROOT_PATH, NULL);
                if (GetRedisPackageInfo(pstruTaskState->nDcsId, pstruXml)==NORMAL){
                	InsertEleQryLog(pstruXml);
                	//if (atoi(DemandStrInXmlExt(pstruXml, "<omc>/<������־��>"))==0)
                		//PublishToMQ(pstruXml, 1);
                }
                DeleteXml(pstruXml);
            }
            free(pstruTaskState);
        }
        //if (nSucceed>0 || nInProcessing>0)
		//PrintDebugLogR(DBG_HERE, "[RealTimeQueue]success: %d, failed: %d, inprocessing: %d, updated: %d. [total: %d VS %d]\n",
        //             nSucceed, nFailed, nInProcessing, nUpdated, nTotal, gTaskStateList->nCount);
		/* step 2. fetch tasks from redis */
		for(i=0; i< 100; i++)
	    {
	    	if (nInProcessing +i >= gTaskStateList->nMaxCount)
	    	{
	    		sleep(2);
	    		break;
	    	}

		    reply = redisCommand(redisconn,"BRPOP RealTimeQueue 2");
		    //PrintDebugLogR(DBG_HERE, "RealTimeQueue [%d]\n",  reply->type);
		    if (reply == NULL || redisconn->err) {   //10.25
				PrintErrorLog(DBG_HERE, "Redis BRPOP RealTimeQueue error: %s\n", redisconn->errstr);
				sleep(10);
				break;
			}
			
		    if(reply->type == REDIS_REPLY_NIL)
		    {
		    	freeReplyObject(reply);
		    	break;
		    }
		    nSleepTime++;
		    if (reply->type == REDIS_REPLY_ARRAY && reply->elements==2) {
		    	strcpy(szJsonBuffer, reply->element[1]->str);
		        PrintDebugLogR(DBG_HERE, "RealTimeQueue [%s]\n",  reply->element[1]->str);
		        
		        
		        cjson_root = cJSON_Parse(szJsonBuffer);
			    if(cjson_root == NULL)
			    {
			        PrintErrorLogR(DBG_HERE, "cJSON_Parse fail %s\n", szJsonBuffer);
			        continue;
			    }
			    cjson_item = cJSON_GetObjectItem(cjson_root, "qs_id");
			    nDcsId = cjson_item->valueint;
			    bufclr(szDataBuffer);
				cjson_item = cJSON_GetObjectItem(cjson_root, "qs_content");
				strncpy(szDataBuffer, cjson_item->valuestring, sizeof (szDataBuffer));
				nDataLen = strlen(szDataBuffer);
				cjson_item = cJSON_GetObjectItem(cjson_root, "qs_tasklogid");
	            nTaskLogId = cjson_item->valueint;
	            cjson_item = cJSON_GetObjectItem(cjson_root, "qs_port");
				nPort = cjson_item->valueint;
				cjson_item = cJSON_GetObjectItem(cjson_root, "qs_ip");
				strcpy(szDeviceIp, cjson_item->valuestring);
   				cJSON_Delete(cjson_root);
   				 
				/* ignore this task if already exists */
				nFindIt = 0;
	            pthread_mutex_lock(&gTaskStateList->lock);
	            list_for_each_entry(pstruTaskState, &gTaskStateList->tasks, list) {
	                if (pstruTaskState->nDcsId == nDcsId) {
	                    nFindIt = 1;
	                    break;
	                }
	            }
	            pthread_mutex_unlock(&gTaskStateList->lock);
	            if (nFindIt == 1)
	            {
	            	PrintErrorLogR(DBG_HERE,"Failed to list_for_each_entry %d\n", nDcsId);
	                continue;
				}

	            /* this is a valid task */
				pstruTaskPara = CreateTaskPara();
				if (pstruTaskPara == NULL) {
					sleep(2);
					break;
				}
				
    			
				strncpy((STR*)pstruTaskPara->struPack.pPack, szDataBuffer, nDataLen);
				pstruTaskPara->struPack.Len = nDataLen;
				strcpy(pstruTaskPara->szDeviceIp, szDeviceIp);
				pstruTaskPara->nPort = nPort;
				pstruTaskPara->nDcsId = nDcsId;
				pstruTaskPara->nTaskLogId = nTaskLogId;
	            if (nTaskLogId == 0) /* query task, just send 1 time */
	                pstruTaskPara->nResendTimes = 1;
	            else /* polling task, has 3 opportunities */
	                pstruTaskPara->nResendTimes = 1;
	            
	            nRet = AddTaskState(pstruTaskPara->nDcsId);
	            if (nRet < 0) { /* maybe gTaskStateList is full */
	                FreeTaskPara(pstruTaskPara);
	                reply2 = redisCommand(redisconn,"RPUSH RealTimeQueue %s", szJsonBuffer);
	                PrintDebugLogR(DBG_HERE, "RPUSH: %d, %s\n", reply2->integer, szJsonBuffer);
	                freeReplyObject(reply2);
	                sleep(2);
	                break;     //2021.8.18 add
	            } else {
	                pthread_mutex_lock(&g_packets_lock);
	                list_add_tail(&pstruTaskPara->list, &g_packets_ne);
	                pthread_mutex_unlock(&g_packets_lock);
	                if (pstruTaskPara->nDcsId > nMaxDcsId)
	                    nMaxDcsId = pstruTaskPara->nDcsId;
	            }
		    }
		    freeReplyObject(reply);
		}

	}
	return NORMAL;
}


/**
 * ���Ժ�̨�����Ƿ���������
 * @param[in] nCmppPid: ��̨���̵� pid ��
 * @return: ���� - NORMAL; ������ - EXCEPTION
 */
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
		if(strncmp(szFileName, "grrusend", 8)==0)
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
	sprintf(szCfgSeg, "GRRUSENDPROC%d", nPid);
	sprintf(szTemp, "%07d\n", (int)getpid()); //10.25
	ModifyCfgItem("grrusend.cfg", szCfgSeg, "GRRUSENDPID",szTemp);

	if (GetCfgItemR("grrusend.cfg","THREADPOOL","MaxThrNum",szTemp) == NORMAL)
		gMaxThrNum = atoi(szTemp);
	if (GetCfgItemR("grrusend.cfg","THREADPOOL","ResendTimeToGprsserv",szTemp) == NORMAL)
		nCfgResendTimes = atoi(szTemp);
	
	if (InitRedisMQ_cfg() != NORMAL)
    {
    	PrintErrorLogR(DBG_HERE,"Failed to InitRedisMQ_cfg()\n");
        return EXCEPTION;
    }
    gTaskStateList = (struct task_state_list*)calloc(1, sizeof (struct task_state_list));
    if (gTaskStateList == NULL) {
		PrintDebugLogR(DBG_HERE,"Failed to allocate memory for gTaskStateList\n");
		return EXCEPTION;
    }
    gTaskStateList->nMaxCount = 500;
    INIT_LIST_HEAD(&gTaskStateList->tasks);
    pthread_mutex_init(&gTaskStateList->lock, NULL);

    INIT_LIST_HEAD(&g_packets_gprsserv);
    INIT_LIST_HEAD(&g_packets_ne);
    pthread_mutex_init(&g_packets_lock, NULL);
    pthread_mutex_init(&g_EleResp_lock, NULL);

    pthread_t threadid;
    int i;
#if 0
    for (i = 0; i < 30; i++)
        pthread_create(&threadid, NULL, DeliverThreadGprsserv, NULL);
#endif

    for (i = 0; i < gMaxThrNum; i++)
        pthread_create(&threadid, NULL, DeliverThreadNe, NULL);

	struSigAction.sa_handler=SigHandle;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags=0;
	if(sigaction(SIGTERM,&struSigAction,NULL)==-1)
	{
		PrintDebugLogR(DBG_HERE,"��װ��������������[%s]\n", strerror(errno));
		return EXCEPTION;
	}

	struSigAction.sa_handler=SIG_IGN;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags=0;
	if(sigaction(SIGPIPE,&struSigAction,NULL)==-1)
	{
		PrintDebugLogR(DBG_HERE,"��װ��������������[%s]\n", strerror(errno));
		return EXCEPTION;
	}
	
	if(OpenDatabase(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		PrintDebugLogR(DBG_HERE,"�����ݿ����[%s]\n",GetSQLErrorMessage());
		return EXCEPTION;
	}
	//����redis
	if (ConnectRedis() !=NORMAL)
	{
		PrintErrorLogR(DBG_HERE, "Connect Redis Error Ping After\n");
		return EXCEPTION;
	}
	
	if (nPid == 1)
	   ProcessPoolTaskQueue();
	//else if (nPid == 2)
	//   ProcessRealTimeQueue();

	CloseDatabase();
	FreeRedisConn();
	return NORMAL;
}


RESULT ParentForkWork(int nProcCount)
{
	STR szTemp[MAX_STR_LEN];
	STR szCfgSeg[100];
	INT nPid;
	INT nListenSock=-1, nFd;
	fd_set SockSet;
	INT nMaxSock;
	socklen_t nLen;
	struct timeval struTimeout;
	struct sockaddr_in struAddr;
	
	while(BOOLTRUE)
	{
		if (strcmp(szHostState, "master") == 0 && nListenSock < 0)
		{
			nListenSock=CreateListenSocket(NULL,nMasterPort);
			if(nListenSock<0)
			{
				PrintDebugLogR(DBG_HERE,"��������ӿڴ���\n");
			}
		}
		if (strcmp(szHostState, "master") == 0 && nListenSock > 0)
		{
			FD_ZERO(&SockSet);
			FD_SET(nListenSock,&SockSet);
			nMaxSock=nListenSock;
			struTimeout.tv_sec = 10;
			struTimeout.tv_usec = 0;
	
				
			switch(select(nMaxSock + 1, &SockSet, NULL, NULL, &struTimeout))
			{
				case -1:
					PrintDebugLogR(DBG_HERE, "select�������ô���[%s]\n", \
						strerror(errno));
					break;
				case 0:
					break;
				default:
					if(FD_ISSET(nListenSock,&SockSet))
					{
						nLen = sizeof(struAddr);
						if((nFd = accept(nListenSock,(struct sockaddr *)&struAddr, &nLen)) < 0)
								break;
						sleep(1);
						close(nFd);
					}
					break;
			}
		
		}
		if (strcmp(szHostState, "backup") == 0) 
			sleep(10);
		//����ӽ���
		for(nPid=1; nPid< nProcCount+1; nPid++)
		{   
			sprintf(szCfgSeg, "GRRUSENDPROC%d", nPid);

			if (GetCfgItemR("grrusend.cfg", szCfgSeg, "GRRUSENDPID",szTemp) != NORMAL)
				return EXCEPTION;
			if (TestTimeServPidStat(atoi(szTemp)) == EXCEPTION)
			{
				switch (fork())
				{
					case -1:
						PrintDebugLogR(DBG_HERE, "�����ӽ���ʧ��\n");
						break;
					case 0:
					{
						printf("�����ӽ���\n");
						TimeServChildProcessWork(nPid);
						exit(0);
					}
					default:
						break;
				}
			}
			sleep(1);
		} 
	}
}

/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	INT nProcCount, i;

	if(argc!=2)
	{
		Usage(argv[0]);
		return EXCEPTION;
	}
	if(strcmp(argv[1],"stop")==0)
	{
		sprintf(szTemp, "clearprg %s", argv[0]);
		system(szTemp);
		return NORMAL;
	}
	if(strcmp(argv[1],"start")!=0)
	{
		Usage(argv[0]);
		return NORMAL;
	}

	if(TestPrgStat(argv[0])==NORMAL)
	{
        fprintf(stderr, "%s is running\n", argv[0]);
		return EXCEPTION;
	}

	if(DaemonStart()!=NORMAL)
	{
		PrintDebugLogR(DBG_HERE,"Failed to execute DaemonStart()\n");
		return EXCEPTION;
	}
	if(CreateIdFile(argv[0])!=NORMAL)
	{
        PrintErrorLogR(DBG_HERE,"Failed to execute CreateIdFile()\n");
		return EXCEPTION;
	}
	
	if(GetDatabaseArg(szServiceName,szDbName,szUser,szPwd)!=NORMAL)
	{
		strcpy(szServiceName,"omc");
		strcpy(szDbName,"omc");
		strcpy(szUser,"omc");
		strcpy(szPwd,"omc");
	}
	
	if (GetCfgItemR("grrusend.cfg","GRRUSEND","MaxProcesses",szTemp) != NORMAL)
		return EXCEPTION;
	nProcCount = atoi(szTemp);
	
	if (GetCfgItemR("grrusend.cfg","GRRUSEND","ApplServPort",szTemp) != NORMAL)
		return EXCEPTION;
	nGprsServPort = atoi(szTemp);
	
	if (GetCfgItemR("grrusend.cfg","GRRUSEND","ApplServAddr",szGprsServIp) != NORMAL)
		return EXCEPTION;
		
	if (GetCfgItemR("grrusend.cfg","CLUSTER","HostState",szHostState) != NORMAL)
	{
		strcpy(szHostState, "backup");
	}
	
	if (GetCfgItemR("grrusend.cfg","CLUSTER","MasterIp", szMasterIp) != NORMAL)
	{
		strcpy(szMasterIp, "127.0.0.1");;
	}
	 
	if (GetCfgItemR("grrusend.cfg","CLUSTER","MasterPort",szTemp) != NORMAL)
		nMasterPort = 8841;
	else
		nMasterPort=atoi(szTemp);

	/* ��ȡ�󶨵�IP������������ж��IP���û���ָ��һ�����ڷ��� */
	if (GetCfgItemR("grrusend.cfg","GRRUSEND","GrruSendAddr",szLocalIpAddr) != NORMAL)
		szLocalIpAddr[0] = 0;

	for(i=1; i< nProcCount+1; i++)
	{
		 switch (fork())
		{
			case -1:
				PrintDebugLogR(DBG_HERE, "Failed to fork()\n");
				break;
			case 0:
			{
				TimeServChildProcessWork(i);
				exit(0);
			}
			default:
				break;
		}
	}
   
	ParentForkWork(nProcCount);
	
	return NORMAL;
}
