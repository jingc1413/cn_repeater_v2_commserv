/*
 * ����: Ӧ�÷���redis ��������
 *
 * �޸ļ�¼:
 * 2021-9-8 - ��־�� ����
 */
 
#include <ebdgdl.h>
#include <omcpublic.h>
#include "applserver.h"

#include <hiredis/hiredis.h>
#include "cJSON.h"

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
	if (redisconn==NULL)
		return -1;
	
    last = nowtime;
    reply = redisCommand(redisconn,"ping");
	//printf("ping:%d %s\n", reply->type, reply->str);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis ping error: %s\n", redisconn->errstr);
		return -1;
	}
	
	if (reply->type==REDIS_REPLY_STATUS && strcmp(reply->str, "PONG")==0)
	{
		PrintDebugLog(DBG_HERE, "Redis ping success: %d,%s\n", reply->type,reply->str);
		freeReplyObject(reply);
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
/*
 * save redis man_eleqrylog
 * push redis ne_gprsqueue
 *
 */
RESULT RedisHsetAndPush(PXMLSTRU  pstruXml)
{
	redisReply *reply;
  	cJSON* cjson_que = NULL;
  	cJSON* cjson_log = NULL;
  	char* pMessageBoby = NULL;
  	char* pEleQryLogBoby = NULL;
  
	cjson_log = cJSON_CreateObject();
	cjson_que = cJSON_CreateObject();
	//��ˮ��  mmddhhmm+5λ��ˮ��
    cJSON_AddStringToObject(cjson_log, "qry_eleqrylogid", DemandStrInXmlExt(pstruXml, "<omc>/<��ˮ��>"));
    cJSON_AddStringToObject(cjson_log, "qry_eleid", DemandStrInXmlExt(pstruXml, "<omc>/<��Ԫ���>"));
    cJSON_AddStringToObject(cjson_log, "qry_property",	DemandStrInXmlExt( pstruXml, "<omc>/<��ض���>"));
	cJSON_AddStringToObject(cjson_log, "qry_style",		DemandStrInXmlExt(pstruXml, "<omc>/<�����>"));
			
	cJSON_AddStringToObject(cjson_log, "qry_commtype",  DemandStrInXmlExt( pstruXml, "<omc>/<ͨ�ŷ�ʽ>"));
	cJSON_AddStringToObject(cjson_log, "qry_taskid", 	DemandStrInXmlExt( pstruXml, "<omc>/<�����>"));
	cJSON_AddStringToObject(cjson_log, "qry_begintime", GetSysDateTime());
	cJSON_AddStringToObject(cjson_log, "qry_user",	DemandStrInXmlExt( pstruXml, "<omc>/<�û�>"));
	
	cJSON_AddStringToObject(cjson_log, "qry_number", DemandStrInXmlExt(pstruXml, "<omc>/<��ˮ��>"));	//2021.12.23 add
    cJSON_AddStringToObject(cjson_log, "qry_tasklogid",  DemandStrInXmlExt( pstruXml, "<omc>/<������־��>"));
	cJSON_AddStringToObject(cjson_log, "qry_windowlogid",	DemandStrInXmlExt( pstruXml,"<omc>/<������ˮ��>"));
    cJSON_AddStringToObject(cjson_log, "qry_packstatus", 	"Sent");
    //2022.1.4
    cJSON_AddNumberToObject(cjson_log, "qs_netflag",  	atoi(DemandStrInXmlExt(pstruXml, "<omc>/<���к�>")));
	cJSON_AddNumberToObject(cjson_log, "ne_repeaterid",  atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>"))); 
	cJSON_AddNumberToObject(cjson_log, "ne_deviceid",  	 atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")));
	 
    //ne_gprsqueue����Ҫ��ѯ
	cJSON_AddStringToObject(cjson_log, "ne_protocoltypeid",  DemandStrInXmlExt( pstruXml, "<omc>/<Э������>"));
	cJSON_AddStringToObject(cjson_log, "ne_devicetypeid",   DemandStrInXmlExt( pstruXml, "<omc>/<�豸����>"));
	cJSON_AddStringToObject(cjson_log, "ne_alarmlist", 		DemandStrInXmlExt( pstruXml,"<omc>/<�澯�б�>"));
	cJSON_AddStringToObject(cjson_log, "ne_alarmenablelist", 	DemandStrInXmlExt( pstruXml,"<omc>/<�澯ʹ��>"));
	cJSON_AddStringToObject(cjson_log, "ne_deviceip", 	DemandStrInXmlExt( pstruXml,"<omc>/<վ��IP>"));
	cJSON_AddStringToObject(cjson_log, "ne_deviceport", DemandStrInXmlExt( pstruXml,"<omc>/<�˿ں�>"));
	
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//��������Ϣ���� cjson_que
	cJSON_AddNumberToObject(cjson_que, "qs_id",  atoi(DemandStrInXmlExt(pstruXml, "<omc>/<��־��>")));
	cJSON_AddNumberToObject(cjson_que, "qs_repeaterid",  atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>"))); 
	cJSON_AddNumberToObject(cjson_que, "qs_deviceid",  	 atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>"))); 
	cJSON_AddNumberToObject(cjson_que, "qs_tasklogid", atoi(DemandStrInXmlExt(pstruXml, "<omc>/<������־��>")));
	cJSON_AddStringToObject(cjson_que, "qs_eventtime", GetSysDateTime());
	cJSON_AddStringToObject(cjson_que, "qs_ip", DemandStrInXmlExt(pstruXml,"<omc>/<վ��IP>"));
	cJSON_AddNumberToObject(cjson_que, "qs_port", atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")));
	cJSON_AddStringToObject(cjson_que, "qs_content", DemandStrInXmlExt(pstruXml,"<omc>/<��Ϣ����>"));
	
  	pEleQryLogBoby = cJSON_PrintUnformatted(cjson_log);
  	cJSON_Delete(cjson_log);
  	
  	pMessageBoby = cJSON_PrintUnformatted(cjson_que);
  	cJSON_Delete(cjson_que);
  	
  	
  	reply = redisCommand(redisconn,"HSET man_eleqrylog %u_%d_%d %s", 
  			atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")), 
  			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")), 
  			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<���к�>")),
  			pEleQryLogBoby);
	PrintDebugLog(DBG_HERE, "HSET man_eleqrylog: %u_%d_%d, %s\n", 
			atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")), 
  			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")), 
  			atoi(DemandStrInXmlExt(pstruXml, "<omc>/<���к�>")), 
			pEleQryLogBoby);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HSET man_eleqrylog error: %s\n", redisconn->errstr);
		return -1;
	}
	freeReplyObject(reply);
	
    int commProtocolType = atoi(DemandStrInXmlExt(pstruXml, "<omc>/<ͨ�ŷ�ʽ>"));
	PrintDebugLog(DBG_HERE, "protocol type: %d\n", commProtocolType);
	//if (commProtocolType==6){
	//reply = redisCommand(redisconn,"LPUSH Queue%u %s", atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")), pMessageBoby);
	//}else{
		reply = redisCommand(redisconn,"LPUSH qryset_queue %s", pMessageBoby);	
		PrintDebugLog(DBG_HERE, "lpush qryset_queue %d %s\n", atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")),  pMessageBoby);
	//	PrintDebugLog(DBG_HERE, "LPUSH %u_%d %s\n", atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")),
	//		atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")),  pMessageBoby);
	//}
	//PrintDebugLog(DBG_HERE, "LPUSH %u_%d %s\n", atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")),
	//		atoi(DemandStrInXmlExt(pstruXml, "<omc>/<�豸���>")),  pMessageBoby);
	
    freeReplyObject(reply);
     
	//reply = redisCommand(redisconn,"EXPIRE Queue%u 43200", atol(DemandStrInXmlExt(pstruXml, "<omc>/<վ����>")));
    //freeReplyObject(reply);
    
    if(pEleQryLogBoby)
		free(pEleQryLogBoby);
	if(pMessageBoby)
		free(pMessageBoby);
    
	return NORMAL;
}

RESULT PushElementParamQueue(int nTaskLogId, PSTR pszEleParamSQL)
{
	redisReply *reply;
	
	reply = redisCommand(redisconn,"LPUSH EleParamQueue%d %s", nTaskLogId, pszEleParamSQL);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis LPUSH EleParamQueue error: %s\n", redisconn->errstr);
		return -1;
	}
    PrintDebugLog(DBG_HERE, "LPUSH EleParamQueue%d:  %s\n", nTaskLogId, pszEleParamSQL);
    freeReplyObject(reply);
    
	return NORMAL;
}

RESULT PushGprsQueue(UINT nRepeaterId, int nDeviceId, PSTR pszMsgCont)
{
	redisReply *reply;
	
	reply = redisCommand(redisconn,"LPUSH %u_%d %s", nRepeaterId,nDeviceId,  pszMsgCont);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis LPUSH EleParamQueue error: %s\n", redisconn->errstr);
		return -1;
	}
    PrintDebugLog(DBG_HERE, "LPUSH %u_%d \n[%s]\n", nRepeaterId, nDeviceId,  pszMsgCont);
    freeReplyObject(reply);
    
	return NORMAL;
}

RESULT PushElementSQL(PSTR pszEleSQL)
{
	redisReply *reply;
	
	reply = redisCommand(redisconn,"LPUSH ElementSQLQueue %s", pszEleSQL);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis LPUSH ElementSQLQueue error: %s\n", redisconn->errstr);
		return -1;
	}
    PrintDebugLog(DBG_HERE, "LPUSH ElementSQLQueue: %d, %s\n", reply->integer, pszEleSQL);
    freeReplyObject(reply);
    
	return NORMAL;
}

RESULT HmsetEleParam(PSTR pszHmsetParam)
{
	redisReply *reply;
	const char *pszField[200];
	int nFieldNum;
	char szBuffer[8192*2];
	
	memset(szBuffer, 0, sizeof(szBuffer));
	strcpy(szBuffer, pszHmsetParam);
	nFieldNum = SeperateString(szBuffer, '|', pszField, 200);
	reply = redisCommandArgv(redisconn, nFieldNum, pszField, NULL);
	//reply = redisCommand(redisconn,"HMSET ne_elementparam %s", pszHmsetParam);
	PrintDebugLog(DBG_HERE, "%s\n", pszHmsetParam);
	if (reply->type != REDIS_REPLY_STATUS && strcmp(reply->str, "OK")!=0)
		PrintErrorLog(DBG_HERE, "%s\n", pszHmsetParam);
	freeReplyObject(reply);
	return NORMAL;
}

RESULT HmsetPoolTaskLog(int nTaskLogId, int nFailTimes, int nStyle)
{
	redisReply *reply;
	int nSortId;
	
	int nEndTime=(int)time(NULL) + nFailTimes * 60;
	
	if (nStyle==214)
		nSortId=1;
	else if (nStyle==213)  //pm ������ѯ
		nSortId=2;
	else if (nStyle==200)  //batch upgrade  �����������൱����������
		nSortId=3;
	else if (nStyle==2) // fast polling  ������ѯ
		nSortId=4;
	else if (nStyle==1) //������ѵ
		nSortId=5;
	else 
		nSortId=6;	   
	//reply = redisCommand(redisconn,"HSET PoolTaskLog TaskLog%d %d", nTaskLogId, nEndTime);
	reply = redisCommand(redisconn,"ZADD PoolTaskLog %d TaskLog%d:%d", nSortId, nTaskLogId, nEndTime); 
	PrintDebugLog(DBG_HERE, "ZADD PoolTaskLog %d TaskLog%d:%d\n", nSortId, nTaskLogId, nEndTime);
	freeReplyObject(reply);
    
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
			
	cJSON_AddStringToObject(cjson_mq, "qrt_eventtime", GetSysDateTime());
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
  	PrintDebugLog(DBG_HERE, "Publish MQ[%s]\n", pMessageBoby);
 	
	reply = redisCommand(redisconn,"PUBLISH mq.man_eleqrylog %s", pMessageBoby);
	
    PrintDebugLog(DBG_HERE, "PUBLISH mq.man_eleqrylog: %d\n", reply->integer);
    freeReplyObject(reply);
 
  	
		
  	return NORMAL;
}

RESULT GetRedisPackageInfo(int nQB, SENDPACKAGE *pstruSendInfo, PXMLSTRU  pstruXml)
{

	char szMessage[8192];
	redisReply *reply;
	cJSON* cjson_root = NULL;
	cJSON* cjson_item = NULL;
	
	reply = redisCommand(redisconn,"HGET man_eleqrylog %u_%d_%d", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HGET man_eleqrylog error: %s\n", redisconn->errstr);
		return -1;
	}
	PrintDebugLog(DBG_HERE, "HGET man_eleqrylog: %u_%d_%d %d %s\n", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB, reply->type,reply->str);
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
        reply = redisCommand(redisconn,"HDEL man_eleqrylog %u_%d_%d", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB);
	    PrintDebugLog(DBG_HERE, "HDEL man_eleqrylog: %u_%d_%d", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB);
	    freeReplyObject(reply);
        return -1;
    }
    
    cjson_item = cJSON_GetObjectItem(cjson_root, "qry_eleqrylogid");
    pstruSendInfo->nMsgLogId = atoi(cjson_item->valuestring);
    
    cjson_item = cJSON_GetObjectItem(cjson_root, "qry_number");
	strcpy(pstruSendInfo->struHead.QA, cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<��ˮ��>",  pstruSendInfo->struHead.QA, MODE_AUTOGROW|MODE_UNIQUENAME);
	
    cjson_item = cJSON_GetObjectItem(cjson_root, "qry_eleid");
	pstruSendInfo->nNeId = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<��Ԫ���>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_property");
	InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_begintime");
	InsertInXmlExt(pstruXml,"<omc>/<��ʼʱ��>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	InsertInXmlExt(pstruXml,"<omc>/<����ʱ��>",  GetSysDateTime(), MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_style");
	pstruSendInfo->struHead.nCommandCode = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<�����>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_commtype");
	pstruSendInfo->struRepeater.nCommType = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
    cjson_item = cJSON_GetObjectItem(cjson_root, "ne_protocoltypeid");
	pstruSendInfo->struHead.nProtocolType = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<Э������>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);

	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_taskid");
	pstruSendInfo->nTaskId = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<�����>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_tasklogid");
	pstruSendInfo->nTaskLogId = atoi(cjson_item->valuestring);
	InsertInXmlExt(pstruXml,"<omc>/<������־��>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_windowlogid");
	InsertInXmlExt(pstruXml,"<omc>/<������ˮ��>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	strcpy(pstruSendInfo->struRepeater.szSpecialCode, cjson_item->valuestring);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qry_user");
	InsertInXmlExt(pstruXml,"<omc>/<�û�>",  cjson_item->valuestring, MODE_AUTOGROW|MODE_UNIQUENAME);
	

	cjson_item = cJSON_GetObjectItem(cjson_root, "ne_deviceip");
	strcpy(pstruSendInfo->struRepeater.szIP, cjson_item->valuestring);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "ne_deviceport");
	pstruSendInfo->struRepeater.nPort = atoi(cjson_item->valuestring); 
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "ne_alarmlist");  //�澯�б�
	strcpy(pstruSendInfo->struRepeater.szAlarmObjList, cjson_item->valuestring);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "ne_alarmenablelist"); //�澯ʹ��
	strcpy(pstruSendInfo->struRepeater.szAlarmEnableList, cjson_item->valuestring);
	
	
	cJSON_Delete(cjson_root);

	//�����б�ɾ��
	{
		reply = redisCommand(redisconn,"HDEL man_eleqrylog %u_%d_%d", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB);
	    PrintDebugLog(DBG_HERE, "HDEL man_eleqrylog: %u_%d_%d\n", pstruSendInfo->struRepeater.nRepeaterId,
			pstruSendInfo->struRepeater.nDeviceId, 	nQB);
	    freeReplyObject(reply);
	}
	return NORMAL;
}

RESULT GetRedisDeviceIpInfo(UINT nRepeaterId, int nDeviceId,PSTR pszDeviceIp, int *pPort)
{

	char szMessage[1000];
	redisReply *reply;
	cJSON* cjson_root = NULL;
	cJSON* cjson_item = NULL;
	
	reply = redisCommand(redisconn,"HGET ne_deviceip %u_%d", nRepeaterId, nDeviceId);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis HGET ne_deviceip error: %s\n", redisconn->errstr);
		return -1;
	}
	PrintDebugLog(DBG_HERE, "HGET ne_deviceip: %u_%d %d %s\n", nRepeaterId,
			nDeviceId, 	reply->type, reply->str);
	if(reply->type == REDIS_REPLY_NIL){
		freeReplyObject(reply);
        return -1;
	}
	strcpy(szMessage, reply->str);
	freeReplyObject(reply);
	
	cjson_root = cJSON_Parse(szMessage);
    if(cjson_root == NULL)
    {
        PrintErrorLog(DBG_HERE, "parse ne_deviceip fail.\n");
        return -1;
    }
    cjson_item = cJSON_GetObjectItem(cjson_root, "qs_deviceip");
	strcpy(pszDeviceIp, cjson_item->valuestring);
	//PrintDebugLog(DBG_HERE,"%s\n", pszDeviceIp);
	
	cjson_item = cJSON_GetObjectItem(cjson_root, "qs_port");
    *pPort = cjson_item->valueint;
	//PrintDebugLog(DBG_HERE,"%d\n", atoi(cjson_item->valuestring));
	cJSON_Delete(cjson_root);

	return NORMAL;
}

RESULT GetEleRespQueue(PSTR pszRespQueue)
{

 	redisReply *reply;
 
	//reply = redisCommand(redisconn,"RPOP mylist");
	reply = redisCommand(redisconn,"BRPOP ElementRespQueue 5");
    if(reply->type == REDIS_REPLY_NIL){
    	freeReplyObject(reply);
    	return -1;
    }

   
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements==2) {

	    strcpy(pszRespQueue, reply->element[1]->str);
        //PrintDebugLog(DBG_HERE, "RPOP [%s]\n",  reply->element[1]->str);
        freeReplyObject(reply);
	}
		   
	return NORMAL;
}


RESULT PushEffectControl(int nTaskLogId, PSTR pszEffSQL)
{
	redisReply *reply;
	
	reply = redisCommand(redisconn,"LPUSH EffectQueue%d %s", nTaskLogId, pszEffSQL);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis LPUSH EffectQueue error: %s\n", redisconn->errstr);
		return -1;
	}
    PrintDebugLog(DBG_HERE,"EffectQueue%d[%s]\n",nTaskLogId, pszEffSQL);
    freeReplyObject(reply);
    
	return NORMAL;
}