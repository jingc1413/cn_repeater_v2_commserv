/*
 * ����: GRRU�ɼ�������
 *
 * �޸ļ�¼:
 * ��־�� -		2010-06-12 ����
 */
#include <ebdgdl.h>
#include <ebdgml.h>
#include <omcpublic.h>
#include <mobile2g.h>
#include <hiredis/hiredis.h>
#include "cJSON.h"

#define VERSION "2021.12"

static STR szClientIp[30];
static int nClientPort;
static int nGprsPort;
static int nGprsPort_heartbeat = 8804;
static int nApplPort;
static unsigned int nRepeaterId;
static int nDeviceId;
static int nWuXian=0;
static int nNewSock=0;
static STR szLocalIpAddr[32] = {0};

STR szService[MAX_STR_LEN];
STR szDbName[MAX_STR_LEN];
STR szUser[MAX_STR_LEN];
STR szPwd[MAX_STR_LEN];


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
 * ���̳��ӽ��̹��캯�� 
 */
static RESULT StartPoolChildPro(PVOID pArgment)
{
	
	if(GetDatabaseArg(szService, szDbName, szUser, szPwd) != NORMAL)
	{
		strcpy(szService, "omc");
		strcpy(szDbName, "omc");
		strcpy(szUser, "omc");
		strcpy(szPwd, "omc");
	}
	if (nWuXian==1 || getenv("WUXIAN")!=NULL)
	{
		if (ConnectRedis() !=NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "Connect Redis Error \n");
	        return EXCEPTION;
	    }
	}
	else{

		if(OpenDatabase(szService, szDbName, szUser, szPwd) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "���̳��ӽ��̴����ݿⷢ������ [%s]\n",
				GetSQLErrorMessage());
			return EXCEPTION;
		}
	}
	
	//PrintDebugLog(DBG_HERE, "���̷��ؽ���\n");
	return NORMAL;
}

/*
 * ͨѶ���̳��ӽ�����������
 */
static RESULT EndPoolChildPro(PVOID pArgment)
{
	if (nWuXian==1 || getenv("WUXIAN")!=NULL)
		FreeRedisConn();
    else
    	CloseDatabase();
    return NORMAL;
}



/*
 * ʹ��˵��
 */
static VOID Usage(PSTR pszProg)
{
    fprintf(stderr, "network management system(grrurecv) V%s\n", VERSION);
    fprintf(stderr, "%s start (startup program)\n"
            "%s stop  (close program) \n" , pszProg, pszProg);
}

int strHexToInt(char* strSource) 
{ 
    int nTemp=0; 
    int i,j,len,flen; 

    len = strlen(strSource); 
    flen = --len; 
    for(i = 0; i <= len; i++) 
    { 
        if(strSource[i] > 'g' || strSource[i] < '0' || ( strSource[i] > '9' && strSource[i] < 'A' ) ) 
        { 
            PrintErrorLog(DBG_HERE,"��������ȷ��16�����ַ���!������� [%s]\n", strSource);
            return -1; 
        } 
        else 
        { 
            int nDecNum; 
            switch(strSource[i]) 
            { 
                case 'a': 
                case 'A': nDecNum = 10; break; 
                case 'b': 
                case 'B': nDecNum = 11; break; 
                case 'c': 
                case 'C': nDecNum = 12; break; 
                case 'd': 
                case 'D': nDecNum = 13; break; 
                case 'e': 
                case 'E': nDecNum = 14; break; 
                case 'f': 
                case 'F': nDecNum = 15; break; 
                case '0': 
                case '1': 
                case '2': 
                case '3': 
                case '4': 
                case '5': 
                case '6': 
                case '7': 
                case '8': 
                case '9': nDecNum = strSource[i] - '0'; break; 
                default: return 0; 
            } 
        
            for(j = flen; j > 0; j-- ) 
            { 
                nDecNum *= 16; 
            } 
            flen--; 
            nTemp += nDecNum; 
        
        } 
    } 
    return nTemp;
}

//����ASCII���ִ���
//Pack ������������
//m_Pdu Ϊ�����Ľ��
BOOL AscUnEsc(BYTEARRAY *Pack)
{
	int Hivalue,Lovalue,temp;
	int i, j;
	char m_Pdu[4096];
	
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
	
	//����ݽ���������
    memset(Pack->pPack, 0, j+1);
	Pack->Len = j;
	memcpy(Pack->pPack, m_Pdu, j);
	    
    
	return TRUE;
}

//ASCII���ִ���(ASCIIר�̳�HEX)
BOOL AscEsc(BYTEARRAY *Pack)
{
 	//�Գ���ʼ��־�ͽ�����־�����������ת��
	int len = (Pack->Len-2)*2+2;
	BYTE pdu[len+1];         //ת��
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
	//PrintDebugLog(DBG_HERE, "����2GЭ�鱨��[%s]\n", Pack->pPack);
	return TRUE;
}




/*
 * ͬGPRSSERV����ͨ�ţ���ͬGPRSSEND
 */
RESULT ExchDataGprsSvr(PSTR pszRespBuffer, unsigned int nRepeaterId, int nDeviceId, int is_heartbeat)
{
    int fd;
	STR szBuffer[MAX_BUFFER_LEN];
	
    if (pszRespBuffer == NULL)
        return -1;

    if (is_heartbeat == 1)
    {
    	/*
        struct sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof (struct sockaddr_in));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        peer_addr.sin_port = htons(nGprsPort_heartbeat);

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
        {
            PrintErrorLog(DBG_HERE, "Failed to create socket\n");
            return -1;
        }

        len = sendto(fd, pszRespBuffer, strlen(pszRespBuffer), 0,
                (struct sockaddr*)&peer_addr, sizeof (peer_addr));
        if (len <= 0)
        {
            PrintErrorLog(DBG_HERE, "Failed to send msg to [127.0.0.1][%d]\n", nGprsPort_heartbeat);
            close(fd);
            return -1;
        }
        close(fd);
        */
        sprintf(szBuffer, "%u_%d", nRepeaterId, nDeviceId);
        PushHeartBeat(szBuffer);
    }
    else
    {
        if((fd = CreateConnectSocket("127.0.0.1", nGprsPort, 60)) < 0)
        {
            PrintErrorLog(DBG_HERE, \
                    "Failed to connect to [127.0.0.1][%d]\n", nGprsPort);
            return -1;
        }
		memset(szBuffer, 0, sizeof(szBuffer));
		sprintf(szBuffer, "%s", pszRespBuffer);
        PrintTransLog(DBG_HERE, "Send to gprsserv [%u]: %s\n", nRepeaterId, pszRespBuffer);
        if(SendSocketNoSync(fd, szBuffer, strlen(szBuffer), 60) < 0)
        {
            PrintErrorLog(DBG_HERE, "Failed to send msg to [127.0.0.1][%d]%s\n", nGprsPort, pszRespBuffer);
            close(fd);
            return -1;
        }
		
		if(RecvSocketNoSync(fd, pszRespBuffer, 4, 20) < 0)//10.28  5���Ϊ20��
		{
			PrintErrorLog(DBG_HERE, "Failed to recv msg\n");
			close(fd);
			return EXCEPTION;
		}
        close(fd);
    }

    return 0;
}


RESULT ExchDataApplSvr(PSTR pszRespBuffer)
{
	STR szBuffer[100];
	INT nConnectFd;

	/*
	 *	��������
	 */
	if((nConnectFd = CreateConnectSocket("127.0.0.1", nApplPort, 60)) < 0)
	{
		PrintErrorLog(DBG_HERE, \
			"ͬӦ�÷������[127.0.0.1][%d]�������Ӵ���,��ȷ��gprsserv�Ѿ�����\n", nGprsPort);
		return EXCEPTION;
	}
	PrintDebugLog(DBG_HERE,"����applserv[%s]\n", pszRespBuffer);		
	/*
	 *	�������ݵ�APPLSERV
	 */
	if(SendSocketNoSync(nConnectFd, pszRespBuffer, strlen(pszRespBuffer), 60) < 0)
	{
		PrintErrorLog(DBG_HERE, "�������ݵ�APPLSERV�������\n");
		close(nConnectFd);
		return EXCEPTION;
	}
	
	/*
	 *	����APPLSERV�����Ӧ��
	 */
	memset(szBuffer, 0, sizeof(szBuffer));
	if(RecvSocketNoSync(nConnectFd, szBuffer, sizeof(szBuffer), 60) < 0)
	{
		PrintErrorLog(DBG_HERE, "��������APPLSERV�����Ӧ���Ĵ���\n");
		close(nConnectFd);
		return EXCEPTION;
	}
	
	close(nConnectFd);

	return NORMAL;
}

INT SendUdpR(INT nConnectFd,PSTR pszSendBuffer,UINT nSendBufferLenth, struct sockaddr *pClientAddr, INT nCliLen, UINT nTimeout)
{
	INT nWritenLenth;
	PSTR pszSendBufferTemp;
	struct timeval struTimeval;

	struTimeval.tv_sec = nTimeout>0?nTimeout:60; /* ���� socket ��ʱʱ�� */
	struTimeval.tv_usec = 0;

	if(setsockopt(nConnectFd, SOL_SOCKET, SO_SNDTIMEO, &struTimeval, sizeof(struTimeval)) < 0){
		PrintDebugLog(DBG_HERE,"���÷��ͳ�ʱʱ�����[%s]\n",strerror(errno));
		return EXCEPTION;
	}
	pszSendBufferTemp=pszSendBuffer;

    nWritenLenth=sendto(nConnectFd,pszSendBufferTemp,nSendBufferLenth, 0, pClientAddr, nCliLen);
    if(nWritenLenth < 0)
    {
        PrintDebugLog(DBG_HERE,"�������ݵ� UDPSERV ����[%s]\n", strerror(errno));
        return EXCEPTION;
    }

	return NORMAL;
}

RESULT ClientSendUdpR(PCSTR pszIpAddr,UINT nPort,PSTR pszBuffer,INT nSendBufferLen)
{
	INT nFd;
	struct sockaddr_in struServerAddr;
	struct sockaddr_in struSendAddr;
	struct hostent *pstruHost;
	INT nLen;
 

	if(pszIpAddr==NULL)
		pszIpAddr=LOCAL_HOST_IP_ADDR;

	memset(&struServerAddr,0,sizeof(struServerAddr));
	if((inet_aton(pszIpAddr,(struct in_addr *)&struServerAddr.sin_addr))==0)
	{
		pstruHost=gethostbyname(pszIpAddr);
		if(pstruHost==NULL)
		{
			PrintDebugLog(DBG_HERE,"Failed to execute gethostbyname()[%s]\n",hstrerror(h_errno));	
			return EXCEPTION;
		}
		struServerAddr.sin_addr=*(struct in_addr *)pstruHost->h_addr_list[0];
	}
	struServerAddr.sin_port=htons(nPort);
	struServerAddr.sin_family=AF_INET;

	nFd=socket(AF_INET,SOCK_DGRAM,0);
	if(nFd==-1)
	{
		PrintDebugLog(DBG_HERE,"Failed to execute socket() [%s]\n", strerror(errno));
		return EXCEPTION;
	}

	/* ���ָ���˱��� IP ��ַ���Ͱ� */
	if (*szLocalIpAddr)
	{
		memset(&struSendAddr, 0, sizeof (struct sockaddr_in));
		struSendAddr.sin_family = AF_INET;
		if((inet_aton(szLocalIpAddr,(struct in_addr *)&struSendAddr.sin_addr))==0)
			PrintDebugLog(DBG_HERE,"Failed to inet_aton(%s): %s\n",
				szLocalIpAddr, hstrerror(h_errno));
		else if(bind(nFd, (struct sockaddr *)&struSendAddr, sizeof (struct sockaddr)) < 0)
			PrintDebugLog(DBG_HERE,
				"Failed to bind IP(%s), send msg with any valid IP address\n", szLocalIpAddr);
	}

	/* ת��Զ�˵� IP ��ַ */
	if((inet_aton(pszIpAddr,(struct in_addr *)&struServerAddr.sin_addr))==0)
	{
        PrintDebugLog(DBG_HERE,"Failed to inet_aton(%s): %s\n",
                pszIpAddr, hstrerror(h_errno));
		close(nFd);
		return EXCEPTION;
	}

	//���ͱ���
	nLen=SendUdpR(nFd, pszBuffer, nSendBufferLen,
		(struct sockaddr*)&struServerAddr, sizeof(struServerAddr), 10);

	close(nFd);

    
    return NORMAL;
}

/* update ne_element */
RESULT UpdateNeElement(UINT nRepeaterId, INT nDeviceId)
{
	char szSql[MAX_BUFFER_LEN];

	if (strcmp(szClientIp, "0.0.0.0") == 0)
		return EXCEPTION;

	memset(szSql, 0, sizeof(szSql));
	snprintf(szSql, sizeof(szSql), "update ne_element set ne_deviceip='%s', ne_deviceport=%d where ne_repeaterid = %u ",
			szClientIp, nClientPort, nRepeaterId);
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

//���浽ne_deviceip����
RESULT SaveDeviceIp(UINT nRepeaterId, INT nDeviceId)
{
    char szSql[MAX_BUFFER_LEN];
	CURSORSTRU struCursor;
	STR szDeviceIp[20];
	int nDevicePort;
	 
	if (strcmp(szClientIp, "0.0.0.0") == 0)
		return EXCEPTION;

    memset(szSql, 0, sizeof(szSql));
    sprintf(szSql, "select qs_deviceip, qs_port from ne_deviceip where qs_RepeaterId = %u and qs_DeviceId = %d",
	        nRepeaterId, nDeviceId);
	PrintDebugLog(DBG_HERE, "Execute SQL[%s]\n", szSql);
	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "Execute SQL[%s]����, ��ϢΪ[%s]\n", szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}

	if(FetchCursor(&struCursor) != NORMAL)
	{
	    FreeCursor(&struCursor);
		snprintf(szSql, sizeof(szSql), "insert into ne_deviceip (qs_repeaterid,qs_deviceid, qs_deviceip, qs_port, qs_eventtime) values (%u, %d, '%s', %d, sysdate)",
    	     nRepeaterId, nDeviceId, szClientIp, nClientPort);
    	PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(ExecuteSQL(szSql) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
    	    return EXCEPTION;
		}
		CommitTransaction();
	}
	else
	{
		strcpy(szDeviceIp, GetTableFieldValue(&struCursor, "qs_deviceip"));
		nDevicePort=atoi(GetTableFieldValue(&struCursor, "qs_port"));
		FreeCursor(&struCursor);
		
		if (strcmp(szDeviceIp, szClientIp)!=0 || (nClientPort!=nDevicePort))
		{
		    snprintf(szSql, sizeof(szSql), "update ne_deviceip set qs_deviceip = '%s', qs_port = %d, qs_eventtime = sysdate where qs_repeaterid = %u and qs_deviceid = %d",
		         szClientIp, nClientPort, nRepeaterId, nDeviceId);
		    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
			if(ExecuteSQL(szSql) != NORMAL)
			{
				PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
					szSql, GetSQLErrorMessage());
		        return EXCEPTION;
			}
			CommitTransaction();
		}
		
	}
	return NORMAL; 
}


RESULT HmSetDeviceIp(UINT nRepeaterId, INT nDeviceId)
{
	redisReply *reply;

	 
	if (strcmp(szClientIp, "0.0.0.0") == 0)
		return EXCEPTION;
	
	reply = redisCommand(redisconn,"HSET ne_deviceip %u_%d {\"qs_deviceip\":\"%s\",\"qs_port\":%d,\"qs_eventtime\":\"%s\"}", nRepeaterId, nDeviceId,
  			szClientIp, nClientPort, GetSysDateTime());
  			
	PrintDebugLog(DBG_HERE, "HSET ne_deviceip: %u_%d {\"qs_deviceip\":\"%s\",\"qs_port\":%d,\"qs_eventtime\":\"%s\"}\n", nRepeaterId, nDeviceId,
  			szClientIp, nClientPort, GetSysDateTime());
	freeReplyObject(reply);
		
	return NORMAL; 
}

RESULT PushHeartBeat(PSTR pszHeartBeat)
{
	redisReply *reply;
	
	reply = redisCommand(redisconn,"LPUSH HeartBeatQueue %s", pszHeartBeat);
	if (reply == NULL || redisconn->err) {   //10.25
		PrintErrorLog(DBG_HERE, "Redis LPUSH ElementSQLQueue error: %s\n", redisconn->errstr);
		return -1;
	}
	PrintDebugLog(DBG_HERE, "LPUSH HeartBeatQueue: %d, %d, %s\n", reply->type, reply->integer, pszHeartBeat);
	if(!(reply->type == REDIS_REPLY_INTEGER && reply->integer > 0))
	{
		if (reply->type == REDIS_REPLY_ERROR)
    		PrintErrorLog(DBG_HERE,"LPUSH ElementSQLQueue Error, type=%d,%s[%s]\n", reply->type, reply->str, pszHeartBeat);
    	else
    		PrintErrorLog(DBG_HERE,"LPUSH ElementSQLQueue Error, type=%d, %s\n", reply->integer, pszHeartBeat);
    }
    freeReplyObject(reply);
    
	return NORMAL;
}

RESULT UpdateGprsQueue(int nId, PSTR pszMsgStat)
{
    char szSql[MAX_BUFFER_LEN];

    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "update ne_gprsqueue set qs_msgstat= '%s', qs_lasttime='%s', qs_retrytimes=qs_retrytimes+1 where qs_id = %d", 
    	pszMsgStat, MakeSTimeFromITime((INT)time(NULL)+ 10), nId);
     
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

RESULT GetNeInfo(UINT nRepeaterId, int nDeviceId)
{
    CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	TBINDVARSTRU struBindVar;
	STR szDeviceIp[20];
	int nDevicePort;
	
	memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_INT;
	struBindVar.struBind[0].VarValue.nValueInt = nRepeaterId;
	struBindVar.nVarCount++;
	
	struBindVar.struBind[1].nVarType = SQLT_INT;
	struBindVar.struBind[1].VarValue.nValueInt = nDeviceId;
	struBindVar.nVarCount++;
	
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select ne_NeId, ne_deviceip,ne_deviceport from ne_Element where ne_RepeaterId = :v_0 and ne_DeviceId = :v_1 ");
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%u][%d]\n", szSql, nRepeaterId,  nDeviceId);
	if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
	    FreeCursor(&struCursor);
    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s][%u][%d]û���ҵ���¼\n", szSql, nRepeaterId,  nDeviceId);
	    return EXCEPTION;
	}
	strcpy(szDeviceIp, GetTableFieldValue(&struCursor, "ne_deviceip"));
	nDevicePort=atoi(GetTableFieldValue(&struCursor, "ne_deviceport"));

	FreeCursor(&struCursor);

	if (strcmp(szDeviceIp, szClientIp)!=0 || (nClientPort!=nDevicePort))
	{
		//UpdateNeElement(nRepeaterId, nDeviceId);
		strcpy(szClientIp, szDeviceIp);
		nClientPort=nDevicePort;
	}

	return NORMAL;
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

RESULT ProcessRedisQueue(int nSock, UINT nRepeaterId, int nDeviceId, struct sockaddr *pstruClientAddr, INT nAddrLen)
{
	STR szDataBuffer[MAX_BUFFER_LEN];
	char szMessage[MAX_BUFFER_LEN];
	int nDcsId, nTaskLogId;
	int nDataLen=0;
	STR szDeviceIp[20],szEventTime[30];
	int nPort;
	BYTEARRAY struPack;
	cJSON* cjson_root = NULL;
    cJSON* cjson_item = NULL;
	redisReply *reply;
	static time_t lasttime = 0;
    time_t nowtime;
	
	//while(TRUE)
	{
		reply = redisCommand(redisconn,"RPOP Queue%u", nRepeaterId);
	    if (reply == NULL || redisconn->err) {   //10.25
			PrintErrorLog(DBG_HERE, "Redis RPOP GprsQueue error: %s\n", redisconn->errstr);
			return -1;//break;
		}
	    if (!(reply->type == REDIS_REPLY_STRING)){
		    if(reply->type == REDIS_REPLY_NIL){
		    	nowtime = time(NULL);
				if (nowtime - lasttime >= 60){
					PrintDebugLog(DBG_HERE, "Redis RPOP Queue%u_%d is Null, No Task\n", nRepeaterId, nDeviceId);
					lasttime = nowtime;
				}
				freeReplyObject(reply);

		    }else{
		    	PrintErrorLog(DBG_HERE, "Redis RPOP Queue%u_%d Error Type: %d, %s\n", nRepeaterId, nDeviceId, reply->type, reply->str);
		    	freeReplyObject(reply);
		    	FreeRedisConn();
			    sleep(1);
		    	if (ConnectRedis() !=NORMAL)
			    {
			        PrintErrorLog(DBG_HERE, "Connect Redis Error RPOP EleParamQueue After\n");
			        return EXCEPTION;
			    }
		    }
		    
		    return -1;//break;
		}
	    
		if (reply->type == REDIS_REPLY_STRING){
			strcpy(szMessage, reply->str);
		}
		freeReplyObject(reply);
		
		cjson_root = cJSON_Parse(szMessage);
	    if(cjson_root == NULL)
	    {
	        PrintErrorLog(DBG_HERE, "parse ne_gprsqueue fail.\n");
	        return -1;
	    }
	    
	    {
  
		    cjson_item = cJSON_GetObjectItem(cjson_root, "qs_id");
		    nDcsId = cjson_item->valueint;
		    bufclr(szDataBuffer);
			cjson_item = cJSON_GetObjectItem(cjson_root, "qs_content");
			memset(szDataBuffer, 0, sizeof(szDataBuffer));
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
   						
			
			PrintDebugLog(DBG_HERE, "send msg to device[%s][Queue%u_%d][%d][%s]\n", szDeviceIp, nRepeaterId, nDeviceId, nDcsId, szDataBuffer);
			
			PrintTransLog(DBG_HERE, "send msg to device[Queue%u_%d][%d][%s]\n",  nRepeaterId, nDeviceId, nDcsId, szDataBuffer);
			
			if ((int)time(NULL) - (int)MakeITimeFromLastTime(szEventTime)>43200) 
				return -1;//continue;
			
			struPack.pPack = szDataBuffer;
			struPack.Len = nDataLen;
			//�Ȼ�ԭת�屨��,��ȥ��STX��ETX
    		if(!AscUnEsc(&struPack))
			{
	    		PrintErrorLog(DBG_HERE,"ת�����[%s]\n",struPack.pPack);
				return -1;
			}
			if(SendUdp(nSock, szDataBuffer, struPack.Len, 10, pstruClientAddr, nAddrLen) < 0)
		    {
		    	PrintErrorLog(DBG_HERE, "���Ͳ�ѯ���������Ĵ���[%u][%d]\n", nRepeaterId, nDeviceId);
		    	return EXCEPTION;
		    }
		    /*
		    memset(szDataBuffer, 0, sizeof(szDataBuffer));
		    nRecvLen = RecvUdp(nSock, szDataBuffer, sizeof(szDataBuffer)-1, 10, pstruClientAddr, &nAddrLen);
		    if (nRecvLen < 0)
		    {
		    	PrintErrorLog(DBG_HERE, "recv resp fail[%d][%d]\n", nRepeaterId, nDeviceId);
		    	continue;
		    }
					    
		    struPack.pPack = szDataBuffer;
			struPack.Len = nRecvLen;
			//�Ƚ���ת�崦��ȥ��STX��ETX
			if(!AscEsc(&struPack))
			{
			    PrintErrorLog(DBG_HERE,"ת�����[%s]\n",struPack.pPack);
				return EXCEPTION;
			}
		    PrintDebugLog(DBG_HERE, "recv from device[%d][Queue%d_%d][%s]\n", nDcsId, nRepeaterId, nDeviceId, szDataBuffer);
		    
		    if (ExchDataGprsSvr(szDataBuffer,nRepeaterId, 0) != NORMAL)
		    {
		    	PrintErrorLog(DBG_HERE, "send  to gprsserv fail[%d][%d]\n", nRepeaterId, nDeviceId);
		    	return EXCEPTION;
		    }
			*/

	    }

	}
	return NORMAL;
}

RESULT ProcessGprsQueue(int nSock, UINT nRepeaterId, int nDeviceId, struct sockaddr *pstruClientAddr, INT nAddrLen)
{
    STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;
	STR szDataBuffer[MAX_BUFFER_LEN];
	int nDcsId;
	int nDataLen=0;
	STR szDeviceIp[20];
	int nPort, nRecvLen;
	BYTEARRAY struPack;
	
	//while(TRUE)
	{
		memset(szSql, 0, sizeof(szSql));
		if (strcmp(getenv("DATABASE"), "mysql") == 0)
			snprintf(szSql,sizeof(szSql),"select qs_id,  qs_ip, qs_port, qs_telephonenum, qs_content, qs_retrytimes  from ne_gprsqueue where "
		       "qs_msgstat='%s' and qs_repeaterid= %u and qs_lasttime<'%s%s' limit %d",
				OMC_NOTSND_MSGSTAT, nRepeaterId,  GetSystemDate(), GetSystemTime(), 50);
		else
			snprintf(szSql,sizeof(szSql),"select qs_id,  qs_ip, qs_port, qs_telephonenum, qs_content, qs_retrytimes  from ne_gprsqueue where "
		       "qs_msgstat='%s' and qs_repeaterid= %u and qs_lasttime<'%s%s' and rownum <=%d",
				OMC_NOTSND_MSGSTAT, nRepeaterId,  GetSystemDate(),GetSystemTime(), 50);
		//PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s]\n", szSql);
		if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	CloseDatabase();
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {

	    	nDcsId = atoi(GetTableFieldValue(&struCursor, "qs_id"));
	    	nPort = atoi(GetTableFieldValue(&struCursor, "qs_port"));
	    	strcpy(szDeviceIp, GetTableFieldValue(&struCursor, "qs_ip"));
	    	bufclr(szDataBuffer);
	    	strcpy(szDataBuffer, GetTableFieldValue(&struCursor, "qs_content"));
	    	nDataLen = strlen(szDataBuffer);
	    	
	    	if (nDataLen < 16) continue;
	    	if (atoi(GetTableFieldValue(&struCursor, "qs_retrytimes")) >= 2)
	    	{
	    		UpdateGprsQueue(nDcsId, OMC_FAIL_MSGSTAT);
	    		continue;
	    	}
		    		
			
			PrintDebugLog(DBG_HERE, "send msg[%s][%d][%s]\n", szDeviceIp, nPort, szDataBuffer);
			
			struPack.pPack = szDataBuffer;
			struPack.Len = nDataLen;
			//�Ȼ�ԭת�屨��,��ȥ��STX��ETX
    		if(!AscUnEsc(&struPack))
			{
	    		PrintErrorLog(DBG_HERE,"ת�����[%s]\n",struPack.pPack);
				return -1;
			}
			if(SendUdp(nSock, szDataBuffer, struPack.Len, 10, pstruClientAddr, nAddrLen) < 0)
		    {
		    	PrintErrorLog(DBG_HERE, "���Ͳ�ѯ���������Ĵ���[%u][%d]\n", nRepeaterId, nDeviceId);
		    	return EXCEPTION;
		    }
		    memset(szDataBuffer, 0, sizeof(szDataBuffer));
		    nRecvLen = RecvUdp(nSock, szDataBuffer, sizeof(szDataBuffer)-1, 10, pstruClientAddr, &nAddrLen);
		    if (nRecvLen < 0)
		    {
		    	UpdateGprsQueue(nDcsId, OMC_FAIL_MSGSTAT);
		    	PrintErrorLog(DBG_HERE, "���ղ�ѯ����Ӧ����ʧ��[%u][%d]\n", nRepeaterId, nDeviceId);
		    	return EXCEPTION;
		    }
					    
		    struPack.pPack = szDataBuffer;
			struPack.Len = nRecvLen;
			//�Ƚ���ת�崦��ȥ��STX��ETX
			if(!AscEsc(&struPack))
			{
				UpdateGprsQueue(nDcsId, OMC_FAIL_MSGSTAT);
			    PrintErrorLog(DBG_HERE,"ת�����[%s]\n",struPack.pPack);
				continue;
			}
		    PrintDebugLog(DBG_HERE, "recv device[%s]\n", szDataBuffer);
		    
		    if (ExchDataGprsSvr(szDataBuffer, nRepeaterId, nDeviceId, 0) != NORMAL)
		    {
		    	UpdateGprsQueue(nDcsId, OMC_FAIL_MSGSTAT);
		    	continue;
		    }
			
			UpdateGprsQueue(nDcsId, OMC_SENT_MSGSTAT);
	    }
	    FreeCursor(&struCursor);

	}
	return NORMAL;
}

char* charToHex(char* str, int len)
{
    //int len = strlen(str);
    char* hex = malloc(2 * len + 1); // Allocate memory for the hexadecimal string
    for (int i = 0; i < len; i++) {
        sprintf(hex + 2 * i, "%02x", str[i]); // Convert each character to a hexadecimal value
    }
    hex[2 * len] = '\0'; // Add the null terminator
    return hex;
}

void charArrayToHex(char* input, char* output, int len) {
    static const char hexdigits[] = "0123456789abcdef";
    
    for (int i = 0; i < len; ++i) {
        unsigned char c = input[i];
        output[i*2]   = hexdigits[c >> 4];  // High nibble
        output[i*2+1] = hexdigits[c & 0xf];  // Low nibble
    }
    output[len * 2] = '\0';  // Null terminate the hex string
}

RESULT ProcessGrruData(INT nSock, PSTR pszCaReqBuffer, INT nLen, struct sockaddr *pstruClientAddr, INT nAddrLen)
{
    STR szReqBuffer[MAX_BUFFER_LEN];
    STR szResqBuffer[MAX_BUFFER_LEN];
    INT nReqLen, i,  nRespLen;
    DECODE_OUT	Decodeout;
    OBJECTSTRU struObject[1000];
    BYTEARRAY struPack;
    int is_heartbeat = 0;
    int nStart=-1;
    int nEnd=-1;
    int index = 0;

	char pHex[2048];
 	charArrayToHex(pszCaReqBuffer, pHex, nLen);
	PrintDebugLog(DBG_HERE, "recv data, %s\n", pHex);

    for(index=0; index<nLen; index++){
        if(pszCaReqBuffer[index] == 0x7E){
            nStart = index;
            break;
        }
    }

	
    if (nStart<0)
    {
		PrintDebugLog(DBG_HERE, "find start flag, %s\n", pHex);
        PrintErrorLog(DBG_HERE,"Failed to find start flag '0x7E', %s\n", pHex);
        return EXCEPTION;
    }
    for(index=nStart+1; index<nLen;index++){
        if(pszCaReqBuffer[index] == 0x7E){
            nEnd = index;
            break;
        }
    }

    if (nEnd<0)
    {
		PrintDebugLog(DBG_HERE, "find end flag, %s\n", pHex);
        PrintErrorLog(DBG_HERE,"Failed to find end flag '0x7E'\n");
        return EXCEPTION;
    }
    nReqLen = nEnd - nStart +1;
    bufclr(szReqBuffer);
    memcpy(szReqBuffer, pszCaReqBuffer+nStart, nReqLen);

    struPack.pPack = (BYTE*)szReqBuffer;
    struPack.Len = nReqLen;

    if (Decode_2G(M2G_TCPIP, &struPack, &Decodeout, struObject) != NORMAL)
    {
        PrintErrorLog(DBG_HERE,"Failed to decode msg\n");
        return EXCEPTION; 
    }
	if (Decodeout.MAPLayer.ObjCount==0) return EXCEPTION;
		
    PrintDebugLog(DBG_HERE, "����2GЭ��ɹ�,�����ʶ[%d],�������[%d],վ����[%u],�豸���[%d],�����ʶ[%d],������[%d]\n",
            Decodeout.MAPLayer.CommandFalg, Decodeout.APLayer.ErrorCode, 
            Decodeout.NPLayer.structRepeater.RepeaterId, Decodeout.NPLayer.structRepeater.DeviceId,
            Decodeout.NPLayer.NetFlag, Decodeout.MAPLayer.ObjCount);
    
    nRepeaterId=Decodeout.NPLayer.structRepeater.RepeaterId;
    nDeviceId=Decodeout.NPLayer.structRepeater.DeviceId;  

    INT nCommUpType=0;
    int nUpQB=Decodeout.NPLayer.NetFlag;
    int nObjCount=0;

    for(i=0; i< Decodeout.MAPLayer.ObjCount; i++)
    {
        if (struObject[i].MapID == 0x0141)
        {
            nCommUpType = struObject[i].OC[0];
            if (nCommUpType == 0x07)
                is_heartbeat = 1;
            break;
        }
    }

    /* CommandFalg==1 means this msg is alarm or heartbeat from NE,
     * and if is heartbeat, do not response and do not update into database */
    if(Decodeout.MAPLayer.CommandFalg == 1 )
    {
    	if (getenv("OEMRESP")!=NULL)
			nObjCount = Decodeout.MAPLayer.ObjCount;
		
        memset(szResqBuffer, 0, sizeof(szResqBuffer));
        struPack.pPack = (BYTE*)szResqBuffer;
        struPack.Len = 0;
        if (Decodeout.NPLayer.APID == 0x03) /* MAP:C */
        {
            if (Encode_Das(M2G_TCPIP, RELAPSECOMMAND, nUpQB, &Decodeout.NPLayer.structRepeater, struObject, nObjCount, &struPack) != NORMAL)
            {
                PrintErrorLog(DBG_HERE, "Failed to execute Encode_Das()\n");
                return EXCEPTION; 
            }
        }
        else
        {
            if (Encode_2G(M2G_TCPIP, RELAPSECOMMAND, nUpQB, &Decodeout.NPLayer.structRepeater, struObject, nObjCount, &struPack) != NORMAL)
            {
                PrintErrorLog(DBG_HERE, "Failed to execute Encode_2G()\n");
                return EXCEPTION; 
            }
        }

        /* response to NE  �������ӻ�Ӧ���� 2020.10.23 fuzhigang*/
        nRespLen = struPack.Len;


    	if(SendUdp(nSock, szResqBuffer, nRespLen, 10, pstruClientAddr, nAddrLen) < 0)
        {
            PrintErrorLog(DBG_HERE, "Failed to response to device %u_%d\n", nRepeaterId, nDeviceId);
        }
        PrintDebugLog(DBG_HERE, "Send Response to device %u_%d\n", nRepeaterId, nDeviceId);
        //PrintHexDebugLog("Send Response",  szResqBuffer, nRespLen);
        
		if (nWuXian==1 || getenv("WUXIAN")!=NULL)
			HmSetDeviceIp(nRepeaterId, nDeviceId);
		else{
			// 2023-9-5 fix by jingc - update ne_deviceip table
			SaveDeviceIp(nRepeaterId, nDeviceId);
		}
    }

    bufclr(szReqBuffer);
    memcpy(szReqBuffer, pszCaReqBuffer+nStart, nReqLen);
    struPack.pPack = (BYTE*)szReqBuffer;
    struPack.Len = nReqLen;
    if(!AscEsc(&struPack))
    {
        PrintErrorLog(DBG_HERE,"ת�����[%s]\n",struPack.pPack);
        return -1;
    }
    //�ж��Ƿ�����������ת����hbserv������gprsserv
    ExchDataGprsSvr(szReqBuffer, nRepeaterId, nDeviceId, is_heartbeat);
    
    if(nWuXian==1 || getenv("WUXIAN")!=NULL)
    {
		sleep(2);
		ProcessRedisQueue(nSock, nRepeaterId, nDeviceId, pstruClientAddr, nAddrLen);
	}


    return NORMAL;
}


/* 
 * GRRU���������� 
 */
RESULT GrruReqWork(INT nSock, PVOID pArg)
{
	STR szCaReqBuffer[MAX_BUFFER_LEN];		/* ��������ͨѶ���� */
	INT nRecvLen;
	struct sockaddr_in struClientAddr;
	INT nAddrLen;

    if (RedisPingInterval(60)!=NORMAL)
    {
    	FreeRedisConn();
    	sleep(10);
    	if (ConnectRedis() !=NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "Connect Redis Error Ping After\n");
	        return EXCEPTION;
	    }
    }

	{
	    /* 
	     * �����豸������ 
	     */
	    memset(szCaReqBuffer, 0, sizeof(szCaReqBuffer));
        nAddrLen = sizeof (struct sockaddr);
	    nRecvLen = RecvUdp(nSock, szCaReqBuffer, sizeof(szCaReqBuffer)-1, 10, (struct sockaddr *)&struClientAddr, &nAddrLen);
	    if (nRecvLen < 0)
	    {
	    	PrintErrorLog(DBG_HERE, "�����豸������ʧ��\n");
	    	return EXCEPTION;
	    }
	    else if (nRecvLen <= 1)
	    {
	        return NORMAL;
	    }
        
    	memset(szClientIp, 0, sizeof(szClientIp));
    	inet_ntop(AF_INET, &struClientAddr.sin_addr, szClientIp, sizeof(szClientIp));
    	nClientPort = ntohs(struClientAddr.sin_port);
		PrintDebugLog(DBG_HERE, "recv device [%s][%d] connected\n", szClientIp,  ntohs(struClientAddr.sin_port));    
	    PrintHexDebugLog("recv req",  szCaReqBuffer, nRecvLen);
	    
	   	
	    /* 
	     * �ظ��豸Ӧ�𣬷����ϱ����ĵ�GPRSSERV���� 
	     */
	    if (ProcessGrruData(nSock, szCaReqBuffer, nRecvLen, (struct sockaddr *)&struClientAddr, nAddrLen)!=NORMAL)
	    {
	    	//PrintErrorLog(DBG_HERE, "������ʧ��\n");
	    	return EXCEPTION;
	    }
	    	   
	    
	}
	
	return NORMAL;	

}

/*	
 * ������
 */
RESULT main(INT argc, PSTR *argv)
{
	POOLSVRSTRU struPoolSvr;
	STR szService[MAX_STR_LEN];
	STR szDbName[MAX_STR_LEN];
	STR szUser[MAX_STR_LEN];
	STR szPwd[MAX_STR_LEN];
	STR szTemp[MAX_STR_LEN];
	
	if(argc != 2)
	{	
		Usage(argv[0]);
		return NORMAL;
	}
	
	if(strcmp(argv[1], "stop") == 0)
	{
		StopPrg(argv[0]);
		sleep(1);
		sprintf(szTemp, "clearprg %s", argv[0]);
		system(szTemp);
		return NORMAL;
	}

    if (strcmp(argv[1], "start") != 0)
    {
        Usage(argv[0]);
        return NORMAL;
    }

	if(TestPrgStat(argv[0]) == NORMAL)
	{
        fprintf(stderr, "%s is running\n", argv[0]);
		return EXCEPTION;
	}
	
	if(DaemonStart() == EXCEPTION)
	{
        PrintErrorLog(DBG_HERE,"Failed to execute DaemonStart()\n");
		CloseDatabase();		
		return EXCEPTION;
	}
	
	if(CreateIdFile(argv[0]) == EXCEPTION)
	{
        PrintErrorLog(DBG_HERE,"Failed to execute CreateIdFile()\n");
		CloseDatabase();
		return EXCEPTION;
    }
    
    if(GetDatabaseArg(szService, szDbName, szUser, szPwd) != NORMAL)
	{
		strcpy(szService, "omc");
		strcpy(szDbName, "omc");
		strcpy(szUser, "omc");
		strcpy(szPwd, "omc");
	}
	/*	
	if(OpenDatabase(szService, szDbName, szUser, szPwd) != NORMAL)
	{
        PrintErrorLog(DBG_HERE, "Failed to open database[%s]!\n", \
			GetSQLErrorMessage());
		return EXCEPTION;
	}
	
    CloseDatabase();
	*/
    if (GetCfgItem("gprsserv.cfg","GPRSSERV","ListenPort",szTemp) != NORMAL)
        return EXCEPTION;
    nGprsPort=atoi(szTemp);
    
    if (GetCfgItem("gprsserv.cfg","GPRSSERV","HeartBeatListenPort",szTemp) == NORMAL)
        nGprsPort_heartbeat=atoi(szTemp);
    
    if (GetCfgItem("applserv.cfg","APPLSERV","ListenPort2",szTemp) != NORMAL)
        return EXCEPTION;
    nApplPort=atoi(szTemp);
    
    if (GetCfgItem("grrurecv.cfg","GRRURECV","GrruSendAddr",szLocalIpAddr) != NORMAL)
		szLocalIpAddr[0] = 0;
	
    //if (GetCfgItem("grrurecv.cfg","GRRURECV","WuXian",szTemp) == NORMAL)
    //	nWuXian=atoi(szTemp);
    if (getenv("WUXIAN")!=NULL)
        nWuXian=1;
    
    if (GetCfgItem("grrurecv.cfg","GRRURECV","NewSock",szTemp) == NORMAL)
        nNewSock=atoi(szTemp);
        
    
    if (nWuXian==1 || getenv("WUXIAN")!=NULL)
	{
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
	    FreeRedisConn();
	}
    
	STR szArgName[100];
	INT i;
	memset(&struPoolSvr,0,sizeof(struPoolSvr));
	for(i=0;i<10;i++)
	{
		sprintf(szArgName,"ListenPort%d",i+1);
		if (GetCfgItem("grrurecv.cfg","GRRURECV", szArgName,szTemp) != NORMAL)
		{
			if(i==0)
			{
                PrintErrorLog(DBG_HERE,"Failed to get listen port from applserv.cfg\n");
				return EXCEPTION;
			}
			break;
		}
		sprintf(szArgName,"ListenIp%d",i+1);
		if (GetCfgItem("grrurecv.cfg","GRRURECV", szArgName,szTemp) == NORMAL)
			strcpy(struPoolSvr.szListenIpAddr, szTemp);
		
		struPoolSvr.nListenPort[i]=atoi(szTemp);
		//nClientPort = atoi(szTemp);
	}
	if(i<MAX_LISTEN_PORT)
		struPoolSvr.nListenPort[i]=-1;
	
	
	SetUdpPoolMinNum(2);
	if (GetCfgItem("grrurecv.cfg","GRRURECV","MaxProcesses",szTemp) != NORMAL)
        return EXCEPTION;
	SetUdpPoolMaxNum(atoi(szTemp));

    struPoolSvr.funcPoolStart = StartPoolChildPro;
    struPoolSvr.pPoolStartArg = NULL;
    struPoolSvr.funcPoolWork = GrruReqWork;
    struPoolSvr.pPoolWorkArg = NULL;
    struPoolSvr.funcPoolEnd = EndPoolChildPro;
    struPoolSvr.pPoolEndArg = NULL;
    struPoolSvr.funcPoolFinish = NULL;
    struPoolSvr.pPoolFinishArg = NULL;
	
	StartUdpPool(&struPoolSvr);	

	return NORMAL;
}
