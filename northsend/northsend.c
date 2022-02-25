/***
 *����:�й���ͨ�����͸澯����
 *
 *������¼:
 *��־��2021-7-23����
 *
 *�޶���¼:
 *
 */

#include <ebdgdl.h>
#include "northsend.h"

#define MAX_WAIT_MSG_NUM	6


/*
 *ȫ�ֱ������� 
 */

static UINT nCommPort;									/*ͨ�ŷ����ַ*/
static STR szCommIpAddr[MAX_STR_LEN];					/*ͨ�ŷ���˿�*/

static INT nSmgpSock=-1;								/*�Ͷ������������׽���*/

static STR szUserName[50];
static STR szPassword[20];

static	STR szServiceName[MAX_STR_LEN];
static	STR szDbName[MAX_STR_LEN];
static	STR szUser[MAX_STR_LEN];
static	STR szPwd[MAX_STR_LEN];

static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(�й����Ŷ���Ϣ�ڵ����)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName, pszProgName, pszProgName);
}




RESULT RecvSmgpRespPack(INT nSmgpSock, UINT nExpectCmd, PSTR pszCommBuffer)
{
	INT nPacketLen;

	if(nSmgpSock < 0)
		return EXCEPTION;

	//while(BOOLTRUE)
	{
		memset(pszCommBuffer, 0, MAX_BUFFER_LEN);
		
		if(RecvSocketNoSync(nSmgpSock, pszCommBuffer, MAX_BUFFER_LEN, 
			DEF_COM_TIMEOUT) < 0)
		{
			PrintErrorLog(DBG_HERE, "�����й����Ŷ������ص�Ӧ����ʧ��\n");
			//close(nSmgpSock);
			//nSmgpSock = -1;
			return EXCEPTION;
		}

		PrintDebugLog(DBG_HERE, "Recv Message[%s]\n", pszCommBuffer);

		//if (strstr(pszCommBuffer, "") != NULL)
		//	ResponseActivetest(nSmgpSock, pszCommBuffer)
		
		
		/*
		 *���ȡ����Ϣ�󷵻�
		 */
		 //if(nExpectCmd == 0)
		 	return NORMAL;		
	}
}


/**
 *ǩ����¼������
 *
 *����ֵ:
 *			�ɹ�:NORMAL;
 *			ʧ��:EXCEPTION;
 */
RESULT ConnectLogin(PSTR pszIpAddr, UINT nPort)
{
	STR szCommBuffer[MAX_BUFFER_LEN];
	STR szTemp[MAX_STR_LEN];
	
		
	nSmgpSock = CreateConnectSocket(pszIpAddr, nPort, DEF_COM_TIMEOUT);
	if(nSmgpSock < 0)
	{
		PrintErrorLog(DBG_HERE, "��Ϣ�ڵ�����������ӷ���[%s][%d]ʧ��\n",
			pszIpAddr,nPort);
		sleep(10);
		return EXCEPTION;
	}

	
	

	memset(szCommBuffer, 0, sizeof(szCommBuffer));
	sprintf(szCommBuffer, "<Connect>\r\nUserName:%s\r\nPassword:%s\r\n</Connect>\r\n", szUserName, szPassword);

	PrintDebugLog(DBG_HERE, "send Connect Login\n[%s]\n",szCommBuffer);
	if(SendSocketNoSync(nSmgpSock, szCommBuffer, strlen(szCommBuffer), DEF_COM_TIMEOUT) == EXCEPTION)
	{
		PrintErrorLog(DBG_HERE, "����ǩ������ʧ��\n");
		close(nSmgpSock);
		nSmgpSock = -1;
		return EXCEPTION;
	}
	
	if(RecvSocketNoSync(nSmgpSock, szCommBuffer, strlen(szCommBuffer), DEF_COM_TIMEOUT) < 0)
	{
		PrintErrorLog(DBG_HERE, "����ǩ��Ӧ����ʧ��\n");
		close(nSmgpSock);
		nSmgpSock = -1;
		return EXCEPTION;
	}
	PrintDebugLog(DBG_HERE, "recv ConnectAck\n[%s]\n",szCommBuffer);
	//if (strstr(szCommBuffer, "<ConnectACK>")!=NULL && strstr(szCommBuffer, "<ConnectACK>")!=NULL)

	return nSmgpSock;
}

RESULT ActiveTestHeat(INT nSmgpSock)
{
	STR szCommBuffer[MAX_BUFFER_LEN];
	STR szTemp[MAX_STR_LEN];
	
	if(nSmgpSock < 0)
	    return EXCEPTION;
	
	memset(szCommBuffer, 0, sizeof(szCommBuffer));
	//<HeartBeat> ProvinceID</HeartBeat>
	sprintf(szCommBuffer, "<HeartBeat>110</HeartBeat>\r\n");

	PrintDebugLog(DBG_HERE, "send Heat[%s]\n", szCommBuffer);
	if(SendSocketNoSync(nSmgpSock, szCommBuffer, strlen(szCommBuffer), DEF_COM_TIMEOUT) == EXCEPTION)
	{
		PrintErrorLog(DBG_HERE, "������������ʧ��\n");
		close(nSmgpSock);
		nSmgpSock = -1;
		return EXCEPTION;
	}
	
	if(RecvSocketNoSync(nSmgpSock, szCommBuffer, strlen(szCommBuffer), DEF_COM_TIMEOUT) < 0)
	{
		PrintErrorLog(DBG_HERE, "��������Ӧ����ʧ��\n");
		close(nSmgpSock);
		nSmgpSock = -1;
		return EXCEPTION;
	}
	PrintDebugLog(DBG_HERE, "Recv Heat Resp[%s]\n", szCommBuffer);

	return NORMAL;
}

RESULT SaveTransLog(YIYANGSTRU *pstruYiYang)
{
    char szSql[MAX_BUFFER_LEN];
     
    memset(szSql, 0, sizeof(szSql));
    
    if (atoi(pstruYiYang->szAlarmStatusId)==1)
    	snprintf(szSql, sizeof(szSql), "INSERT INTO TF_ALARMTRANSFERLOG(NEID, SENDTIME, UPLOADRESULT, ALARMLOGID, ORIALARMID, "
            "ALARMTITLE, ALARMCREATETIME, NETYPE, NENAME, NEVENDOR, "
            "ALARMLEVEL, ALARMTYPE,  ALARMLOCATION, ALARMSTATUS,"
            "ALARMOBJID, EXTENDINFO, SUCCEED) VALUES( "
            "'%s', to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s', to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s', '%s', '%s', %d, "
            " '%s', '%s', %d)",
            pstruYiYang->szNeId, GetSysDateTime(), "�ɹ�", pstruYiYang->szAlarmLogId, pstruYiYang->szAlarmId,
            pstruYiYang->szAlarmTitle, pstruYiYang->szAlarmCreateTime, pstruYiYang->szNeType, pstruYiYang->szNeName, pstruYiYang->szNeVendor,
            pstruYiYang->szAlarmLevel, pstruYiYang->szAlarmType, pstruYiYang->szAlarmLocation, atoi(pstruYiYang->szAlarmStatusId),
            pstruYiYang->szAlarmObjId, pstruYiYang->szExtendInfo, 0
            );
    else       
    	snprintf(szSql, sizeof(szSql), "INSERT INTO TF_ALARMTRANSFERLOG(NEID, SENDTIME, UPLOADRESULT, ALARMLOGID, ORIALARMID, "
            "ALARMTITLE, ALARMCREATETIME, NETYPE, NENAME, NEVENDOR, "
            "ALARMLEVEL, ALARMTYPE,  ALARMLOCATION, ALARMSTATUS, STATUSTIME,"
            "ALARMOBJID, EXTENDINFO, SUCCEED) VALUES( "
            "'%s', to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s', to_date('%s', 'yyyy-mm-dd hh24:mi:ss'), '%s', '%s', '%s',"
            " '%s', '%s', '%s', %d, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'),"
            " '%s', '%s', %d)",
            pstruYiYang->szNeId, GetSysDateTime(), "�ɹ�", pstruYiYang->szAlarmLogId, pstruYiYang->szAlarmId,
            pstruYiYang->szAlarmTitle, pstruYiYang->szAlarmCreateTime, pstruYiYang->szNeType, pstruYiYang->szNeName, pstruYiYang->szNeVendor,
            pstruYiYang->szAlarmLevel, pstruYiYang->szAlarmType, pstruYiYang->szAlarmLocation, atoi(pstruYiYang->szAlarmStatusId),pstruYiYang->szAlarmClearTime,
            pstruYiYang->szAlarmObjId, pstruYiYang->szExtendInfo, 0
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


RESULT DeleteTrigger(PSTR pszAlarmLogId)
{
    char szSql[MAX_BUFFER_LEN];
 
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "delete from tf_ALarmLog_Trigger where alg_AlarmLogId=%s", pszAlarmLogId);
     
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


RESULT SendCurrMessage(INT nSmgpSock)
{
    STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;

	YIYANGSTRU struYiYangData;
	STR szSendBuffer[MAX_BUFFER_LEN];
	STR szAlarmLogId[100];
	
	
	if(nSmgpSock < 0)
	    return INVALID;
	
    //while(TRUE)
	{
	    /*
	 	 *	����澯����
	 	 */
	    sprintf(szSql,"SELECT alg_AlarmLogId, alg_alarmId, alg_NeId, alg_AlarmStatusId, to_char(alg_AlarmTime,'yyyy-mm-dd hh24:mi:ss') as alg_AlarmTime,to_char(alg_ClearTime,'yyyy-mm-dd hh24:mi:ss') as alg_ClearTime,"
	    						"b.alm_Name, b.alm_objid, b.alm_LevelId, b.alm_dealidea, b.alm_memo," 

								"n.ne_repeaterid, n.ne_deviceid, n.ne_Name, n.ne_installplace,"
								"p.ARE_PARENTID, p.are_Name as County, (SELECT are_Name FROM pre_Area WHERE pre_Area.are_areaid = p.ARE_PARENTID) as Region," 
								"ne_Company.co_Name AS alg_CompanyName," 
								"ne_Company.co_CompanyId AS alg_CompanyId, "
								"alm_AlarmLevel.alv_Name AS alg_LevelName," 
								"ne_DeviceType.dtp_Name AS alg_DeviceTypeName "
							"FROM tf_alarmlog_trigger t "
								"LEFT JOIN  alm_Alarm b ON b.alm_AlarmId = t.alg_AlarmId " 
								"LEFT JOIN  ne_Element n ON n.ne_NeId = t.alg_NeId "
								"LEFT JOIN  pre_Area p on n.ne_AreaId = p.are_AreaId "
								"LEFT JOIN  ne_Company ON ne_Company.co_CompanyId = n.ne_CompanyId  "
								"LEFT JOIN  alm_AlarmLevel ON alm_AlarmLevel.alv_AlarmLevelId = b.alm_LevelId "
								"LEFT JOIN  ne_DeviceType ON ne_DeviceType.dtp_DeviceTypeId = n.ne_DeviceTypeId "
							);
	    //PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	    if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    	CloseDatabase();
	    	sleep(60);
	    	return EXCEPTION;
	    }
	    while (FetchCursor(&struCursor) == NORMAL)
	    {
	        PrintDebugLog(DBG_HERE,"��ʼ����ʵʱ�澯================\n");
	        memset(&struYiYangData, 0, sizeof(YIYANGSTRU));
			strcpy(struYiYangData.szAlarmLogId, GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
			sprintf(struYiYangData.szAlarmId,  "%d", atoi(GetTableFieldValue(&struCursor, "alg_alarmId")));
			
			//strcpy(struYiYangData.szNeId ,GetTableFieldValue(&struCursor, "alg_NeId"));
			sprintf(struYiYangData.szNeId, "%08X%02X", atoi(GetTableFieldValue(&struCursor, "ne_repeaterid")), 
					atoi(GetTableFieldValue(&struCursor, "ne_deviceid")));
			 //��Ԫ����
            strcpy(struYiYangData.szNeName, GetTableFieldValue(&struCursor, "ne_Name"));
            
			strcpy(struYiYangData.szSystemVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName"));//������
			
			
            strcpy(struYiYangData.szAlarmCreateTime,  GetTableFieldValue(&struCursor, "alg_AlarmTime"));
            
            strcpy(struYiYangData.szNeType,  GetTableFieldValue(&struCursor, "alg_DeviceTypeName"));
            strcpy(struYiYangData.szAlarmStatusId,  GetTableFieldValue(&struCursor, "alg_AlarmStatusId"));
            
            if (atoi(GetTableFieldValue(&struCursor, "alg_AlarmStatusId"))==0)
            	strcpy(struYiYangData.szAlarmClearTime,  GetTableFieldValue(&struCursor, "alg_ClearTime"));
                       
            
            strcpy(struYiYangData.szNeVendor,  GetTableFieldValue(&struCursor, "alg_CompanyName")); //��Ԫ����
            strcpy(struYiYangData.szAlarmLevel,  GetTableFieldValue(&struCursor, "alm_LevelId")); //�澯����
            
            strcpy(struYiYangData.szAlarmType, "�豸�澯");  //�澯���ͣ��豸�澯
            
            strcpy(struYiYangData.szAlarmTitle,  GetTableFieldValue(&struCursor, "alm_Name"));
            strcpy(struYiYangData.szAlarmObjId, GetTableFieldValue(&struCursor, "alm_objid"));
            strcpy(struYiYangData.szProbableCauseTxt, GetTableFieldValue(&struCursor, "alm_dealidea"));
            
            strcpy(struYiYangData.szAlarmLocation, GetTableFieldValue(&struCursor, "ne_installplace")); //  �澯��λ��վ����,�豸��ţ��Զ��ŷָ���վ����Ϊ8λ16���������豸���Ϊ2λ16��������
                   
		      
		    strcpy(struYiYangData.szSystemName, "SUNWAVE OMC");
		    
		    strcpy(struYiYangData.szAlarmRegion, GetTableFieldValue(&struCursor, "Region"));
		    strcpy(struYiYangData.szAlarmCounty, GetTableFieldValue(&struCursor, "County"));
		   
 
 
			bufclr(szSendBuffer);
			snprintf(szSendBuffer, sizeof(szSendBuffer), 
				"<AlarmStart>\r\n"
				"ProvinceID:110\r\n"             //ʡ�ֱ���   110
				"IntVersion:V1.2.0\r\n"         //�ӿڰ汾��  V1.2.0
				"MsgSerial:%s\r\n"              //������Ϣ���
				"AlarmUniqueId:%s\r\n"          //�澯Ψһ��ʶ
                "ClearId:%s\r\n"                //�澯���ָ��  MsgSerial
                
                "StandardFlag:2\r\n"           //�澯��׼����ʶ  2
                "SubAlarmType: \r\n"
                "NeId:%s\r\n"                   //�豸��ʶ
                "NeName:%s\r\n"                 //�豸����
                "NeAlias: \r\n"
                "LocateNeName:%s\r\n"            //�澯��������
                "LocateNeType:Repeater\r\n"            //�澯��������
                
                "EquipmentClass:Repeater\r\n"
                "NeIp: \r\n"
                "SystemName:%s\r\n"             //ϵͳ��
                "Vendor:%s\r\n"                 //����
                "Version:1.0\r\n"
                "LocateNeStatus: \r\n"
                
                "ProjectNo: \r\n"
				"ProjectName: \r\n"
				"ProjectStartTime: \r\n"
				"ProjectEndTime: \r\n"
                "LocateInfo:%s\r\n"             //��λ��Ϣ
                "EventTime:%s\r\n"              //�澯����ʱ��
                "CancelTime:%s\r\n"             //�澯���ʱ��
                "DalTime:%s\r\n"                //�澯����ʱ��  sysdate
                
                "VendorAlarmType:%s\r\n"       //���Ҹ澯����
                "VendorSeverity:%s\r\n"       //���Ҹ澯����
                "VendorAlarmId:%s\r\n"       //���Ҹ澯��
                "AlarmSeverity:%s\r\n"         //���ܸ澯����
                "NmsAlarmId:%s\r\n"           //���ܸ澯ID
                "AlarmStatus:%s\r\n"         //�澯״̬   0����Ԫ�Զ���� 1����澯 2��ͬ�����
                "AckFlag:0\r\n"
				"AckTime: \r\n"
				"AckUser: \r\n"
                "AlarmTitle:%s\r\n"         //�澯����
                "StandardAlarmName:%s\r\n"         //�澯��׼��
                "ProbableCauseTxt:%s\r\n"   //�澯����ԭ��
                "AlarmText:%s\r\n"          //�澯����
                "CircuitNo: \r\n"
                "PortRate: \r\n"
                "Specialty:1\r\n"         	//רҵ   1  ������
                "BusinessSystem:OMC\r\n"     //��רҵ   OMC
                "AlarmLogicClass: \r\n"         //�澯�߼�����       Ϊ��
                "AlarmLogicSubClass: \r\n"         //�澯�߼�����    Ϊ��
                "EffectOnEquipment:4\r\n"         //���¼����豸��Ӱ��  4  
                "EffectOnBusiness:6\r\n"         //���¼���ҵ���Ӱ��  6
                "NmsAlarmType:1\r\n"
                "SendGroupFlag:0\r\n"
				"RelatedFlag:0\r\n"
				"AlarmProvince:����\r\n"
				"AlarmRegion:%s\r\n"
				"AlarmCounty:%s\r\n"
				"Site: \r\n"
				"AlarmActCount:0\r\n"
				"CorrelateAlarmFlag:1\r\n"
				"SheetSendStatus: \r\n"
				"SheetStatus: \r\n"
				"SheetNo:0\r\n"
				"AlarmMemo:0\r\n"
                "<AlarmEnd>\r\n",

                struYiYangData.szAlarmLogId,
                struYiYangData.szAlarmId, 
                struYiYangData.szAlarmLogId,
                
                struYiYangData.szNeId,
                struYiYangData.szNeName,
                struYiYangData.szNeName,      //struYiYangData.szAlarmTitle, //ԭ����Ӧ���Ǹ澯���ƣ��ָ�Ϊվ������
                //struYiYangData.szAlarmType, //�澯��������
                
                struYiYangData.szSystemName, //ϵͳ��
                struYiYangData.szNeVendor,   //����
                
                struYiYangData.szAlarmLocation,  //��λ��Ϣ
                struYiYangData.szAlarmCreateTime, //�澯����ʱ��
                struYiYangData.szAlarmClearTime,
                struYiYangData.szAlarmCreateTime, //�澯����ʱ��
                
                struYiYangData.szAlarmType,    //���Ҹ澯����
                struYiYangData.szAlarmLevel,   //���Ҹ澯����
                struYiYangData.szAlarmObjId,   //���Ҹ澯��
                
                struYiYangData.szAlarmLevel,   //���ܸ澯����
                struYiYangData.szAlarmObjId,   //���ܸ澯ID
                
                struYiYangData.szAlarmStatusId,
                struYiYangData.szAlarmTitle,
                struYiYangData.szAlarmTitle,
                struYiYangData.szProbableCauseTxt,
                struYiYangData.szAlarmTitle,
                
                struYiYangData.szAlarmRegion,
                struYiYangData.szAlarmCounty
				);
            
            PrintDebugLog(DBG_HERE,"send alarm message\n[%s]\n",szSendBuffer);  
            
            /*
			 *	�������ݵ���Ϣ���������
			 */
			if(SendSocketNoSync(nSmgpSock, szSendBuffer, strlen(szSendBuffer), 60) < 0)
			{
				FreeCursor(&struCursor);
				PrintErrorLog(DBG_HERE, "�������ݵ�����������\n");
				return INVALID;
			}
			
			
	        DeleteTrigger(struYiYangData.szAlarmLogId);
		    SaveTransLog(&struYiYangData);
		    


	    }
	    FreeCursor(&struCursor);
   
	    	    
	    //sleep(30);

	}
	return NORMAL;
}



/**
 *���������ֹ�ź�
 *
 *�޷���ֵ
 */
 static VOID SigHandle(INT nSigNo)
{
	if(nSigNo != SIGTERM)
		return;
	
	/*	
	 * ǩ�˴���
	 */
	//SignOutAdapter();
	CloseDatabase();
	close(nSmgpSock);
	nSmgpSock = -1;

	exit(0);
}





/**
 *������Ϣ�Ľ��ܺͷ��ͺ���
 *
 *�޸�˫���ӻ��ƣ�ȷ����select֮ǰ����������
 *
 *����ֵ:
 *			�ɹ�:NORMAL;
 *			ʧ��:EXCEPTION;
 */
static RESULT TransSmgpMsgTask(VOID)
{
	UINT nIdleTimes = 0;
	STR szCommBuffer[MAX_BUFFER_LEN];
	STR szTemp[MAX_BUFFER_LEN];
	
	INT nMaxFd;
	fd_set struSockSet;
	struct timeval struTimeOut;

	INT nTestTimes=0, nRet;

	while(BOOLTRUE)
	{

		if(nSmgpSock < 0)
		{
			if((nSmgpSock = ConnectLogin(szCommIpAddr, nCommPort)) < 0)
			{
				PrintErrorLog(DBG_HERE, "���ӵ�����ʧ��[%s][%d]\n", szCommIpAddr, nCommPort);
				sleep(10);
				continue;
			}
			PrintDebugLog(DBG_HERE, "ConnectSmgp success[%d]\n", nSmgpSock);
			
		}	

		FD_ZERO(&struSockSet);
		FD_SET(nSmgpSock, &struSockSet);

		struTimeOut.tv_sec = 10;
		struTimeOut.tv_usec = 0;
		nMaxFd =  nSmgpSock;
		switch(select(nMaxFd + 1, &struSockSet, NULL, NULL, &struTimeOut))
		{
			case -1:
				PrintErrorLog(DBG_HERE, "Select�������ô���\n");
				sleep(10);
				exit(0);

			case 0:
				nTestTimes++;
				if (nTestTimes >= 5)
				{
					if(ActiveTestHeat(nSmgpSock) != NORMAL)
					{
							PrintErrorLog(DBG_HERE, "ActivetestSmgp Error, ExitSmgp!!\n");
							close(nSmgpSock);
							nSmgpSock = -1;
							sleep(1);
					}
					nTestTimes=0;
				}
				break;

			default:
				nIdleTimes = 0;

				if(FD_ISSET(nSmgpSock, &struSockSet) && nSmgpSock>0)			/*���շ���˷��ر���*/
				{
					if(RecvSmgpRespPack(nSmgpSock, 0, szCommBuffer) <0)
					{
						PrintErrorLog(DBG_HERE, "�������ر��Ĵ���[%d]\n", nSmgpSock);
						sleep(1);
						continue;						
					}					
				}
				
				break;
								
		}
		
		//����ʵʱ�澯
		nRet=SendCurrMessage(nSmgpSock);
		if (nRet==INVALID)
		{
			close(nSmgpSock);
			nSmgpSock = -1;
			sleep(10);
		}
		else if (nRet==EXCEPTION)
		{
			sleep(10);
			exit(0);
		}	
		
	}
}


/**
 *����״̬���Ժ���
 *
 *����ֵ:
 *			�ɹ�:NORMAL;
 *			ʧ��:EXCEPTION;
 */
RESULT TestSmgpPidStat(int nSmgpPid)
{
	STR szShellCmd[MAX_STR_LEN];
	STR szFileName[100];
	INT nTempLen;
	FILE *pstruFile;

	memset(szShellCmd, 0, sizeof(szShellCmd));
	snprintf(szShellCmd, sizeof(szShellCmd), "ps -e | awk '$1 == %d {print $4}'", nSmgpPid);

	if((pstruFile = popen(szShellCmd, "r")) == NULL)
	{
		fprintf(stderr, "popen�д���\n");
		return EXCEPTION;
	}

	memset(szFileName, 0, sizeof(szFileName));
	while(fgets(szFileName, 10, pstruFile) != NULL)
	{
		nTempLen = strlen(szFileName);
		szFileName[nTempLen] = 0;

		if(strncmp(szFileName, "northsend", 9) == 0)
		{
			pclose(pstruFile);
			return NORMAL;
		}		
	}

	pclose(pstruFile);
	return EXCEPTION;
}


RESULT SmgpChildProcessWork(INT nPid)
{
	struct sigaction struSigAction;
	STR szTemp[MAX_STR_LEN];
	STR szCfgSeg[100];
	
	if(GetCfgItem("northsend.cfg", "NORTHPROC1", "CommServAddr", szCommIpAddr) != NORMAL)
		return EXCEPTION;

	if(GetCfgItem("northsend.cfg", "NORTHPROC1", "CommServPort", szTemp) != NORMAL)
		return EXCEPTION;
	nCommPort = atoi(szTemp);
	
	if(GetCfgItem("northsend.cfg", "NORTHPROC1", "LoginUserName", szUserName) != NORMAL)
		return EXCEPTION;
	if(GetCfgItem("northsend.cfg", "NORTHPROC1", "LoginPassword", szPassword) != NORMAL)
		return EXCEPTION;
	/*����SMGP���̺�*/
	sprintf(szTemp, "%06d", (int)getpid());
	ModifyCfgItem("northsend.cfg", "NORTHPROC1", "NORTHPID", szTemp);

	struSigAction.sa_handler = SigHandle;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags = 0;
	if(sigaction(SIGTERM, &struSigAction, NULL) == -1)
	{
		PrintErrorLog(DBG_HERE, "��װ��������������, �������[%s]������Ϣ[%s]\n",
			errno, strerror(errno));
		return EXCEPTION;
	}

	struSigAction.sa_handler = SIG_IGN;
	sigemptyset(&struSigAction.sa_mask);
	struSigAction.sa_flags = 0;
	if(sigaction(SIGPIPE, &struSigAction, NULL))
	{
		PrintErrorLog(DBG_HERE, "��װ��������������, �������[%s]������Ϣ[%s]\n",
			errno, strerror(errno));
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
	TransSmgpMsgTask();
	
	
	return NORMAL;
}


RESULT ParentForkWork(int nProcCount)
{
	STR szTemp[MAX_STR_LEN];
	STR szCfgSeg[100];
	INT nPid;

	while(BOOLTRUE)
	{
		sleep(1);
		/*����ӽ���*/
		for(nPid = 1; nPid < nProcCount + 1; nPid++)
		{
			//sprintf(szCfgSeg, "SMGPPROC%d", nPid);
			strcpy(szCfgSeg, "NORTHPROC1");
			/*SMGP���̺�*/
			if(GetCfgItem("northsend.cfg", szCfgSeg, "NORTHPID", szTemp) != NORMAL)
				return EXCEPTION;
			if(TestSmgpPidStat(atoi(szTemp)) == EXCEPTION)
			{
				switch(fork())
				{
					case -1:
						PrintErrorLog(DBG_HERE, "�����ӽ���ʧ��\n");
						break;

					case 0:
					{
						SmgpChildProcessWork(nPid);
						exit(0);
					}

					default:
						break;
				}
			}
			sleep(10);
		}
	}
}


/*
 * ������
 */
RESULT main(INT argc, PSTR argv[])
{
	STR szTemp[MAX_BUFFER_LEN];
	INT nProcCount;
	STR szFileName[200];

	//fprintf(stderr, "\t��ӭʹ���й���������ϵͳ(SMGP 3.1Э��ڵ����)");

	if(argc !=2)
	{
		Usage(argv[0]);
		return INVALID;
	}

	if(strcmp(argv[1], "stop") == 0)
	{
		sprintf(szTemp, "clearprg %s", argv[0]);
		system(szTemp);
		snprintf(szFileName, sizeof(szFileName), "%s/.%sid", getenv("HOME"), argv[0]);
		unlink(szFileName);
		return NORMAL;
	}

	if(strcmp(argv[1], "start") != 0)
	{
		Usage(argv[0]);
		return NORMAL;
	}

	if(TestPrgStat(argv[0]) == NORMAL)
	{
		fprintf(stderr, "�й�����SMGP 3.1 Э����Ϣ�ڵ���������Ѿ������������ڷ�����\n");
		return EXCEPTION;
	}

	if(DaemonStart() != 0)
	{
		PrintErrorLog(DBG_HERE, "�й�����SMGP 3.1Э����Ϣ�ڵ��������פʧ��\n");
		return EXCEPTION;
	}

	if(CreateIdFile(argv[0]) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "��������ID�ļ�����\n");
		return EXCEPTION;
	}

	

	//fprintf(stderr, "���!����ʼ����\n");

	ParentForkWork(1);

	return NORMAL;
}


