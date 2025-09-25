/*
 * ����: ����ϵͳӦ�÷�����
 *
 * �޸ļ�¼:
 * ��־�� -		2008-9-8 ����
 */
 
#include <ebdgdl.h>
#include <ebdgml.h>
#include <omcpublic.h>
#include <applserver.h>
#include <mobile2g.h>

STR szService[MAX_STR_LEN];
STR szDbName[MAX_STR_LEN];
STR szUser[MAX_STR_LEN];
STR szPwd[MAX_STR_LEN];

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

	if(OpenDatabase(szService, szDbName, szUser, szPwd) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "���̳��ӽ��̴����ݿⷢ������ [%s]\n",
			GetSQLErrorMessage());
		return EXCEPTION;
	}
	
	if (getenv("WUXIAN")!=NULL)
	{
		if (ConnectRedis() !=NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "Connect Redis Error \n");
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
	
	CloseDatabase();
	if (getenv("WUXIAN")!=NULL)
		FreeRedisConn();
	//PrintDebugLog(DBG_HERE, "�ر����ݿ�ɹ�\n");
	return NORMAL;
}



/*
 * ʹ��˵��
 */
static VOID Usage(PSTR pszProg)
{
	fprintf(stderr, "����ϵͳ(Ӧ�÷������)\n");
	fprintf(stderr, "%s start ��������\n"
					"%s stop ֹͣ����\n"
					"%s -h ��ʾ����Ϣ\n", pszProg, pszProg, pszProg);
}

DIVCFGSTRU struDivCfg[]=
{
    {"���״���",	4},
	{"��Ԫ���",	10},
	{"վ����",	10},
	{"�豸���",	10},
	{"��ض���",	1000},
	{"��ض�������",1800},
	{"Э������",	10},
	{"վ��绰",	28},
	{"վ��IP",	    28},
	{"��ˮ��",	    28},
	{"�����",	    8},
	{"�豸����",	10},
	{"������ʶ",	50},
	{"�豸�ͺ�",	50},
	{"�˿ں�",	    8},
	{"�������",	30},
	{"����",	    2},
	{"ʱ��",	    20},
	{"վ��ȼ�",	2},
	{"ͨ�ŷ�ʽ",	10},
	{"��¼��ˮ��",	10},
	{"������ˮ��",	50},
	{NULL,		    0}
};

/*
 * ���䳤���Ľ�ѹ
 */
static RESULT UnpackDivStr( char *szBuffer, PXMLSTRU pstruXml, PDIVCFGSTRU pstruDivCfg)
{
	INT	i;
	INT nFieldNum;
	STR *pszBuffer;
	STR *pszField[100];
	STR szPath[MAX_PATH_LEN];
	
	pszBuffer = szBuffer;

	nFieldNum = SeperateString( pszBuffer, '|', pszField, 100 );
	
	/*	ѭ�����������XML	*/
	//for ( i = 0; i < nFieldNum; i ++ )
	for(i=0; (i<nFieldNum) && (pstruDivCfg[i].pszName!=NULL);i++)
	{
	    sprintf(szPath,"%s/<%s>", OMC_ROOT_PATH, pstruDivCfg[i].pszName);
		TrimAllSpace(pszField[i]);
		//PrintDebugLog( DBG_HERE, "%s[%s]\n", szPath, pszField[i] );
		InsertInXmlExt( pstruXml, szPath, pszField[i], MODE_AUTOGROW|MODE_UNIQUENAME );
		if (i> nFieldNum) break;
	}
	return NORMAL;
}


/*
 * �����ѯ���ý���
 */
RESULT ProcessQuerySetTrans(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
	REPEATER_INFO struRepeater;
	COMMANDHEAD struHead;
	STR szBuffer[MAX_BUFFER_LEN];
	int nNeId, nSpecialType;
	STR szType[2+1];   			/*��������  */
	STR szTemp[10];
	int nObjCount, i;
	STR szQryEleParam[MAX_BUFFER_LEN];
	STR szSetEleParam[MAX_BUFFER_LEN];
	STR szSetEleValue[MAX_BUFFER_LEN];
	PSTR pszSepParamStr[MAX_OBJECT_NUM];  /* �ָ��ַ�����*/
	PSTR pszSepValueStr[MAX_OBJECT_NUM];  /* �ָ��ַ�����*/
	
	/*
 	 * �������ģ�����������ת��Ϊxml
     */
    memset(pstruXml,0,sizeof(XMLSTRU));
	CreateXml(pstruXml,FALSE,OMC_ROOT_PATH,NULL);

	if(UnpackDivStr(pszCaReqBuffer, pstruXml, struDivCfg)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����BS���ͱ���ʧ��\n");
		return EXCEPTION;
	}
    memset(szBuffer, 0, sizeof(szBuffer));
	ExportXml(pstruXml, szBuffer, sizeof(szBuffer));
	PrintDebugLog(DBG_HERE,"�䳤ת������[%s][%d]\n",szBuffer, strlen(szBuffer));
    
    memset(&struHead, 0, sizeof(COMMANDHEAD));
    memset(&struRepeater, 0, sizeof(REPEATER_INFO));
	nNeId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��Ԫ���>"));  
	struRepeater.nCommType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));  
	struRepeater.nRepeaterId = atol(DemandStrInXmlExt(pstruXml,"<omc>/<վ����>")); 
	struRepeater.nDeviceId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸���>")); 
	strcpy(struRepeater.szTelephoneNum, DemandStrInXmlExt(pstruXml,"<omc>/<վ��绰>"));
	
	strcpy(struRepeater.szIP, DemandStrInXmlExt(pstruXml,"<omc>/<վ��IP>"));
	if (strlen(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")) == 0)
		struRepeater.nPort = 0;
	else
		struRepeater.nPort = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")); 
		
	struRepeater.nProtocolDeviceType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸����>"));
	strcpy(struRepeater.szSpecialCode, DemandStrInXmlExt(pstruXml,"<omc>/<������ʶ>"));
	strcpy(struRepeater.szReserve, DemandStrInXmlExt(pstruXml,"<omc>/<�豸�ͺ�>"));
	strcpy(struRepeater.szNetCenter, DemandStrInXmlExt(pstruXml,"<omc>/<�������>"));
	
	if (getAgentState(struRepeater.szNetCenter) == BOOLTRUE)
		InsertInXmlExt(pstruXml,"<omc>/<����״̬>", "0", MODE_AUTOGROW|MODE_UNIQUENAME);
	else
		InsertInXmlExt(pstruXml,"<omc>/<����״̬>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
	//�ж��Ƿ�2G�� ��2G�� SnmpЭ��
	struHead.nProtocolType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<Э������>"));
	struHead.nCommandCode = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�����>"));
	// ������������  2009.1.7
    //  178����������ѯ 179������������ 180���̲�����ѯ 181���̲������� 176���빤��ģʽ����
	nSpecialType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��ˮ��>"));
	if (nSpecialType > 170 && nSpecialType < 1000)
	{
	    struHead.nCommandCode = nSpecialType;

		sprintf(szTemp, "%d", nSpecialType);
		InsertInXmlExt(pstruXml,"<omc>/<�����>", szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
	    nSpecialType = 0;
	}
	strcpy(szType, DemandStrInXmlExt(pstruXml,"<omc>/<����>"));
	
	strcpy(szQryEleParam, DemandStrInXmlExt(pstruXml,"<omc>/<��ض���>"));
	if (strlen(szQryEleParam) == 0)
	{
		PrintDebugLog(DBG_HERE,"��ѯ[%u][%s]��ض���Ϊ��\n", struRepeater.nRepeaterId, struRepeater.szTelephoneNum);
		DeleteXml(pstruXml);
		return EXCEPTION;
	}
	//����
	if (strcmp(szQryEleParam, "0606") == 0 || strcmp(szQryEleParam, "00000606") == 0)
	{	
		strcpy(szType, "61");
	}
	//ʱ϶
	strcpy(szQryEleParam, DemandStrInXmlExt(pstruXml,"<omc>/<��ض���>"));
	if (strcmp(szQryEleParam, "0875") == 0 || nSpecialType == 71)
	{	
		strcpy(szType, "71");
	}
	
    //��������վ��ȼ�Ϊ��:OMC_QUICK_MSGLEVEL
	InsertInXmlExt(pstruXml,"<omc>/<վ��ȼ�>", OMC_QUICK_MSGLEVEL, MODE_AUTOGROW|MODE_UNIQUENAME);
		
	if ((strcmp(szType,"11") == 0) || (strcmp(szType ,"21") == 0))//��ѯ 11 2g 21 ��2g
	{
	    PrintDebugLog(DBG_HERE,"�����ѯ����\n");
		if (struRepeater.nCommType == 7)//ͨ�ŷ�ʽ��snmpЭ��
		{
			struHead.nProtocolType = PROTOCOL_SNMP;
			QryElementParam(M2G_SNMP_TYPE, &struHead, &struRepeater, pstruXml);
        	//�������������
        	SaveToSnmpQueue(pstruXml);
        	SaveEleQryLog(pstruXml);
		}
        //else if (strcmp(szType, "21") ==0) //ͨ�ŷ�ʽ��UDP, GPRS
        else if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6) //ͨ�ŷ�ʽ��UDP, GPRS
        {
        	ResolveQryParamArrayGprs(szQryEleParam);
        	PrintDebugLog(DBG_HERE, "��ض���[%s]\n", szQryEleParam);
	        nObjCount = SeperateString(szQryEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
	
	        for(i=0; i< nObjCount; i++)
		    {
		    	InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
	        	//����GPRS��ʽ
	        	QryElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
	        	//�������������
	        	SaveToGprsQueue(pstruXml);
	        	SaveEleQryLog(pstruXml);
        	}
        	
        }
        else //ͨ�ŷ�ʽ������
        {
        	if(struHead.nProtocolType == PROTOCOL_2G ||
        	   struHead.nProtocolType == PROTOCOL_DAS ||
        	   struHead.nProtocolType == PROTOCOL_JINXIN_DAS)
        	{	
	        	strcpy(szQryEleParam, DemandStrInXmlExt(pstruXml,"<omc>/<��ض���>"));
	        	ResolveQryParamArray(szQryEleParam);
	 	    	PrintDebugLog(DBG_HERE, "��ض���[%s]\n", szQryEleParam);
	        	nObjCount = SeperateString(szQryEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
	
	        	for(i=0; i< nObjCount; i++)
		    	{
		    	    //��ض����ÿո�ָ�
	 	    	    InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime((INT)time(NULL)+i*10), MODE_AUTOGROW|MODE_UNIQUENAME);
		    	    InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
					QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
					SaveToMsgQueue(pstruXml);
					SaveEleQryLog(pstruXml);
		    	}
		    }
		    else if (struHead.nProtocolType == PROTOCOL_GSM || 
		    		struHead.nProtocolType == PROTOCOL_CDMA ||
		    		struHead.nProtocolType == PROTOCOL_HEIBEI ||
		    		struHead.nProtocolType == PROTOCOL_HEIBEI2 ||
		    		struHead.nProtocolType == PROTOCOL_XC_CP ||
		    		struHead.nProtocolType == PROTOCOL_SUNWAVE ||
		    		struHead.nProtocolType == PROTOCOL_WLK)
		    {
	 	    	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime((INT)time(NULL)), MODE_AUTOGROW|MODE_UNIQUENAME);
			    QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
				SaveToMsgQueue(pstruXml);
				SaveEleQryLog(pstruXml);
		    }
		    	
		}

	}
	else if ((strcmp(szType,"12") == 0) || (strcmp(szType ,"22") == 0)) //12 2g����  22 ��2g����
	{
		if (struRepeater.nCommType == 7)//snmpЭ��
		{
			struHead.nProtocolType = PROTOCOL_SNMP;
			SetElementParam(M2G_SNMP_TYPE, &struHead, &struRepeater, pstruXml);
        	//�������������
        	SaveToSnmpQueue(pstruXml);
        	SaveEleSetLog(pstruXml);
		}
        //else if (strcmp(szType, "22") ==0)
        else if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6) //ͨ�ŷ�ʽ��UDP, GPRS
        {
        	SetElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
        	//�������������
        	SaveToGprsQueue(pstruXml);
        	SaveEleSetLog(pstruXml);
        }
        else
        {
        	if(struHead.nProtocolType == PROTOCOL_2G ||
        	   struHead.nProtocolType == PROTOCOL_DAS ||
        	   struHead.nProtocolType == PROTOCOL_JINXIN_DAS)
        	{	
	        	strcpy(szSetEleParam, DemandStrInXmlExt(pstruXml,"<omc>/<��ض���>"));
				strcpy(szSetEleValue, DemandStrInXmlExt(pstruXml,"<omc>/<��ض�������>"));
			
			
				ResolveSetParamArray(szSetEleParam, szSetEleValue);
	        	nObjCount = SeperateString(szSetEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
	        	nObjCount = SeperateString(szSetEleValue, '|', pszSepValueStr, MAX_SEPERATE_NUM);
	        	for(i=0; i< nObjCount; i++)
		    	{
		    	    //��ض����ÿո�ָ�
	 	    	    InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime((INT)time(NULL)+i*10), MODE_AUTOGROW|MODE_UNIQUENAME);
		    	    InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
		    	    InsertInXmlExt(pstruXml,"<omc>/<��ض�������>",  pszSepValueStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
		    	             
					SetElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
					SaveToMsgQueue(pstruXml);
					SaveEleSetLog(pstruXml);
				}
			}
		    else if (struHead.nProtocolType == PROTOCOL_GSM || 
		    		struHead.nProtocolType == PROTOCOL_CDMA ||
		    		struHead.nProtocolType == PROTOCOL_HEIBEI ||
		    		struHead.nProtocolType == PROTOCOL_HEIBEI2 ||
		    		struHead.nProtocolType == PROTOCOL_XC_CP ||
		    		struHead.nProtocolType == PROTOCOL_SUNWAVE ||
		    		struHead.nProtocolType == PROTOCOL_WLK)
			{
				InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime((INT)time(NULL)), MODE_AUTOGROW|MODE_UNIQUENAME);
				SetElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
				SaveToMsgQueue(pstruXml);
				SaveEleSetLog(pstruXml);
			}	
		}

	}
	else if (strcmp(szType,"31") == 0)//31 ����б�
	{ 
		strcpy(szTemp, DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));
        if (strcmp(szTemp, "6") == 0 || strcmp(szTemp, "5") == 0)
        {
			QueryMapList(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
			SaveToGprsQueue(pstruXml);
			SaveEleQryLog(pstruXml);
		}
        else
        {
			QueryMapList(M2G_SMS, &struHead, &struRepeater, pstruXml);
			SaveToMsgQueue(pstruXml);
			SaveEleQryLog(pstruXml);
		}
		
	}
	else if (strcmp(szType,"41") == 0)//41 pesq���в���
	{
		InsertMosTask(pstruXml);
		DeleteXml(pstruXml);
		return NORMAL;
	}
	else if (strcmp(szType,"51") == 0)//51 das
	{
		if (struRepeater.nCommType == 7)//ͨ�ŷ�ʽ��snmpЭ��
		{
			struHead.nProtocolType = PROTOCOL_SNMP;
			QueryDasList(M2G_SNMP_TYPE, &struHead, &struRepeater, pstruXml);
        	//�������������
        	SaveToSnmpQueue(pstruXml);
        	SaveEleQryLog(pstruXml);
		}
		else
		{
			//PrintDebugLog(DBG_HERE,"here\n");
			QueryDasList(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
			SaveToGprsQueue(pstruXml);
			SaveEleQryLog(pstruXml);
		}
	}
	else if (strcmp(szType,"52") == 0)//52 rfid
	{
		QueryRfidList(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
		SaveToGprsQueue(pstruXml);
		SaveEleQryLog(pstruXml);
	}
	else if (strcmp(szType,"61") == 0)//61 ����
	{ 
		strcpy(szTemp, DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));
        if (strcmp(szTemp, "6") == 0)
        {
        	QryElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
        	SaveToGprsQueue(pstruXml);
        	SaveEleQryLog(pstruXml);
		}
        else
        {
			QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
			SaveToMsgQueue(pstruXml);
			SaveEleQryLog(pstruXml);
		}
		
		InitBatPick(pstruXml);
		
		SaveBatPickLog(pstruXml);
		
	}
	else if (strcmp(szType,"71") == 0)//71 ʱ϶
	{ 
		strcpy(szTemp, DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));
        if (strcmp(szTemp, "6") == 0)
        {
        	QryElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
        	SaveToGprsQueue(pstruXml);
        	SaveEleQryLog(pstruXml);
		}
        else
        {
			QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
			SaveToMsgQueue(pstruXml);
			SaveEleQryLog(pstruXml);
		}
		
		CheckShiXi(pstruXml);
		
		SaveShiXiLog(pstruXml);
		
	}
	
	// 2009.1.7 ���������,�ֶ�Ϊ��ˮ�Ŷ���
	if (nSpecialType > 0)
	{
	    //   1-Զ������, 2-��ѯcqt����������3-����pesq���в��⣬4-����pesq���в��⣬
		//   5-gprs�����������6-��ѯgprs���Խ������
		if ((strcmp(szType,"12") == 0) || (strcmp(szType ,"22") == 0))
		    InsertInXmlExt(pstruXml,"<omc>/<��������>", "2", MODE_AUTOGROW|MODE_UNIQUENAME);
		else if (nSpecialType == 4)
		    InsertInXmlExt(pstruXml,"<omc>/<��������>", "3", MODE_AUTOGROW|MODE_UNIQUENAME);
		else
		    InsertInXmlExt(pstruXml,"<omc>/<��������>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
		
		sprintf(szTemp, "%d", nSpecialType);
		InsertInXmlExt(pstruXml,"<omc>/<�����>", szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
				
	    //InsertSpecialCommandLog(pstruXml);
	    //����Զ������
	    if (nSpecialType == 1)
	        RemortUpdateDbOperate1(pstruXml);
	}
	
	DeleteXml(pstruXml);
	return NORMAL;
    
}

/*
 * �����ϱ�����
 */
RESULT ProcessSmsDeliverTrans(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
	STR szMobile[25];				/* �ֻ��� */
//	STR szMsgCont[150];				/*  �ϱ���Ϣ����*/
	STR szMsgCont[MAX_CONTENT_LEN];	/*  �ϱ���Ϣ����*/
	STR szSPNumber[22];				/*  �ط����*/

	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
    memset(szMsgCont, 0, sizeof(szMsgCont));
	strcpy(szMobile, DemandStrInXmlExt(pstruXml,OMC_MOBILE_PATH));
	strcpy(szMsgCont, DemandStrInXmlExt(pstruXml,OMC_MSGCONT_PATH));
	strcpy(szSPNumber, DemandStrInXmlExt(pstruXml,OMC_SPNUMBER_PATH));
	DeleteXml(pstruXml);
	
	if (strcmp(szMsgCont, "smssend") != 0)
	DecodeAndProcessSms(szMsgCont, szMobile, szSPNumber);
	/*
	int nCurProtocol=GetProtocolFrom(szMobile);
    if(nCurProtocol==PROTOCOL_2G)
	{
		DecodeAndProcessSms(szMsgCont, szMobile, szSPNumber);
	}
	*/
	return NORMAL;
	
}


/*
 * ����GPRS��ѯ���ý���
 */
RESULT ProcessGprsQrySetTrans(INT nSock, PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
	CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	STR	szCaRespBuffer[MAX_BUFFER_LEN];		/* ����Ӧ��ͨѶ���� */
	STR szTemp[10], szBuffer[MAX_BUFFER_LEN];
	int nDetail=0, nTimes;

	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
    UINT nRepeaterId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<repeaterid>"));
	int nDeviceId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<deviceid>"));
	
	//������Ԫgprs����״̬
	sprintf(szSql,"update ne_element set ne_state = '1' where  ne_repeaterid = %u and ne_deviceid = %d ", 
	    nRepeaterId, nDeviceId);
	PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	if(ExecuteSQL(szSql)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	CommitTransaction();
	
	for(nTimes=0; nTimes< 22; nTimes++)
	{
		//�ȴ���Ϣ
		sleep(5);
		memset(szSql, 0, sizeof(szSql));
		sprintf(szSql, "select qs_netflag, qs_telephonenum, qs_content from NE_GPRSQUEUE where QS_REPEATERID = %u and QS_DEVICEID = %d and QS_MSGSTAT = '0' order by QS_ID",
		        nRepeaterId, nDeviceId);
		//PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
		if(SelectTableRecord(szSql, &struCursor) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", szSql, GetSQLErrorMessage());
			return EXCEPTION;
		}
		while(FetchCursor(&struCursor) == NORMAL)
		{
			nDetail++;
			bufclr(szBuffer);
			InsertInXmlRel(pstruXml, "<omc>/<��ϸ>", nDetail,	"<��ˮ��>", 
					GetTableFieldValue(&struCursor, "qs_netflag"), MODE_BRANCHREL);
			strcpy(szBuffer, GetTableFieldValue(&struCursor, "qs_content"));
			InsertInXmlRel(pstruXml, "<omc>/<��ϸ>", nDetail,	"<����>", szBuffer, MODE_BRANCHREL);
		}
		FreeCursor(&struCursor);
		if (nDetail > 0)//����Ϣ
		{
			sprintf(szSql, "update NE_GPRSQUEUE set qs_msgstat = '1' where  qs_repeaterid = %u and qs_deviceid = %d ", 
			    nRepeaterId, nDeviceId);
			PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
			if(ExecuteSQL(szSql)!=NORMAL)
			{
				PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
				return EXCEPTION;
			}
			CommitTransaction();
			
			sprintf(szTemp, "%d", nDetail);
			InsertInXmlExt(pstruXml, "<omc>/<��ϸ��>", szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
			/* 
			 * ����gprsrecvӦ���� 
			 */
			memset(szCaRespBuffer, 0, sizeof(szCaRespBuffer));
			ExportXml(pstruXml, szCaRespBuffer, sizeof(szCaRespBuffer));
			
			if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
			{
				PrintErrorLog(DBG_HERE, "����GPRS��ѯ���ñ��Ĵ���!\n");
			}
			PrintDebugLog(DBG_HERE, "����GPRS��ѯ���ñ���[%s]\n", szCaRespBuffer);
			
		    break;
		}
		else
			continue;
	}
	
	DeleteXml(pstruXml);
	
	//�����ѻ�״̬	
	return NORMAL;
	
}


/*
 * ����GPRS�ѻ�����
 */
RESULT ProcessGprsOffLine(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
	STR szSql[MAX_SQL_LEN];

	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
    UINT nRepeaterId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<repeaterid>"));
	int nDeviceId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<deviceid>"));
	
	//������Ԫgprs�ѻ�״̬
	sprintf(szSql,"update ne_element set ne_state = '0' where  ne_repeaterid = %u and ne_deviceid = %d ", 
	    nRepeaterId, nDeviceId);
	PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	if(ExecuteSQL(szSql)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	CommitTransaction();

	DeleteXml(pstruXml);
	
	//�����ѻ�״̬	
	return NORMAL;
	
}

/*
 * ����״̬����
 */
RESULT ProcessSmsStatusReport(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
	char szSql[MAX_BUFFER_LEN];
	STR szMobile[25];			/* �ֻ��� */
	STR szMsgStat[8], szStat[100];			/*  ����״̬*/
	int nNeId, nMsgSerial, nStat=-1;			/*  ��Ԫ��*/
	STR szType[3];
	TBINDVARSTRU struBindVar;

	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
    strcpy(szType, DemandStrInXmlExt(pstruXml, "<omc>/<����>"));
	strcpy(szMobile, DemandStrInXmlExt(pstruXml,OMC_MOBILE_PATH));
	strcpy(szMsgStat, DemandStrInXmlExt(pstruXml, "<omc>/<״̬>"));
	nNeId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��Ԫ>"));
	nMsgSerial = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��־��>"));
	DeleteXml(pstruXml);
	
	if (strcmp(szMsgStat, "DELIVRD") == 0)
	{
	    nStat = 1;
	    strcpy(szStat, "�����ѵ���");
	}
	else
	{
		/*
		if (strncmp(szMsgStat, "EXPIRED", 7) == 0)
            strcpy(szStat, "�û��ػ����߲��ڷ�����");
        else if (strncmp(szMsgStat, "DELETED", 7) == 0)
            strcpy(szStat, "����ɾ����Ϣ");
        else if (strncmp(szMsgStat, "UNDELIV", 7) == 0)
            strcpy(szStat, "�û�ͣ����״̬");
        else if (strncmp(szMsgStat, "ACCEPTD", 7) == 0)
            strcpy(szStat, "��Ϣ���Ͽ�");
        else if (strncmp(szMsgStat, "UNKNOWN", 7) == 0)
            strcpy(szStat, "δ֪״̬");
        else if (strncmp(szMsgStat, "REJECTD", 7) == 0)
            strcpy(szStat, "ĳЩԭ�򱻾ܾ�");
        else if (strncmp(szMsgStat, "MK", 2) == 0)
            strcpy(szStat, "�û�ͣ����״̬");
        else if (strncmp(szMsgStat, "MI", 2) == 0)
            strcpy(szStat, "�û��ػ����߲��ڷ�����");
        else
        */
	    	strcpy(szStat, "�����޷�����");
	 }
	memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_INT;
	struBindVar.struBind[0].VarValue.nValueInt = nStat;
	struBindVar.nVarCount++;
	
	struBindVar.struBind[1].nVarType = SQLT_INT;
	struBindVar.struBind[1].VarValue.nValueInt = nMsgSerial;
	struBindVar.nVarCount++;
	
	if (strcmp(szType, "12") == 0 || strcmp(szType, "22") == 0)
	    sprintf(szSql, "update man_elesetlog set  set_SmsStatus= :v_0,set_SmsTime = sysdate where set_elesetlogid = :v_1");
    else 
        sprintf(szSql, "update man_eleqrylog set  qry_SmsStatus= :v_0,qry_SmsTime = sysdate where qry_eleqrylogid = :v_1");
    PrintDebugLog(DBG_HERE,"ִ��SQL���[%s][%d][%d]\n",szSql, nStat,  nMsgSerial);
    if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	{
	    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
	    return EXCEPTION;
	}
	
	memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_STR; 
	strcpy(struBindVar.struBind[0].VarValue.szValueChar, szStat);
	struBindVar.nVarCount++;
	
	struBindVar.struBind[1].nVarType = SQLT_INT;
	struBindVar.struBind[1].VarValue.nValueInt = nNeId;
	struBindVar.nVarCount++;
	
	sprintf(szSql, "update man_smsstatus set smsstatus= :v_0, eventtime= sysdate where neid= :v_1");
	if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	{
	    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
	    return EXCEPTION;
	}
	PrintDebugLog(DBG_HERE,"ִ��SQL���[%s][%s][%d]\n",szSql, szStat,  nNeId);
	if(GetAffectedRows()<1)
	{
		sprintf(szSql,"insert into man_smsstatus (neid, smsstatus, eventtime) values(%d, '%s', sysdate)",
		        nNeId, szStat);
		PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
		if(ExecuteSQL(szSql)!=NORMAL)
		{
			PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��,������Ϣ[%s]\n",szSql,GetSQLErrorMessage());
			return EXCEPTION;
		}
	}
	CommitTransaction();
	return NORMAL;
}


/* 
 * ��ѵ���� 
 */
RESULT ProcessTurnTask(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
    CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	REPEATER_INFO struRepeater;
	COMMANDHEAD struHead;
	int nTaskId, nTaskLogId;	/* ����� */
	int nProStyle;          /* ��������*/
	int nProtocolTypeId, nProtocolDeviceTypeId, nDeviceTypeId, nPollDeviceTypeId;
	int nDeviceStatusId, nNeId;
	int nFailEleCount=0, nObjCount, i=0;
	int nEleCount=0;
	int nTxPackCount=0;
	STR szQryEleParam[MAX_BUFFER_LEN];
	STR szSetEleParam[MAX_BUFFER_LEN];
	STR szSetEleValue[MAX_BUFFER_LEN];
	STR szServerTelNum[20], szTemp[200];
	//STR szCmdObjectList[MAX_BUFFER_LEN];
	PSTR pszSepParamStr[MAX_OBJECT_NUM];  /* �ָ��ַ�����*/
	PSTR pszSepValueStr[MAX_OBJECT_NUM];  /* �ָ��ַ�����*/
	//STR szBuffer[MAX_BUFFER_LEN];
	STR szStyle[10];
	STR szFilterNeId[MAX_BUFFER_LEN];
	int nFilterLen=0;
	int nNowTime, nTimes=0;
	STR szUploadTime[100];
	STR szTaskQryParm[1000], szBase[2000], szRadio[2000], szAlarm[2000], szAlarmen[2000];
	STR szRealTime[2000], szRadioSC[2000], szObjList[2000], szSepStr[2000];
	STR szBasePoll[2000], szRadioPoll[2000], szAlarmPoll[2000], szAlarmenPoll[2000];
	STR szRealTimePoll[2000], szRadioSCPoll[2000];
	PSTR pszTempStr[ MAX_OBJECT_NUM];
	INT nSeperateNum;

 	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
	nTaskId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<taskid>"));
	
	strcpy(szStyle, DemandStrInXmlExt(pstruXml,"<omc>/<style>"));

    //��¼������־

    InsertTaskLog(nTaskId, &nTaskLogId, szStyle);
    sprintf(szTemp, "%d", nTaskId);
    InsertInXmlExt(pstruXml,"<omc>/<�����>",  szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
    sprintf(szTemp, "%d", nTaskLogId);
    InsertInXmlExt(pstruXml,"<omc>/<������־��>",  szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
    
    if (getAgentState(szServerTelNum) == BOOLTRUE)
		InsertInXmlExt(pstruXml,"<omc>/<����״̬>", "0", MODE_AUTOGROW|MODE_UNIQUENAME);
	else
		InsertInXmlExt(pstruXml,"<omc>/<����״̬>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
	
    memset(szFilterNeId, 0, sizeof(szFilterNeId));
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), "select TSK_FILTER from man_task where tsk_taskid = %d", nTaskId);
    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
    if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
    if(FetchCursor(&struCursor) == NORMAL)
	{
		sprintf(szFilterNeId, "%s", (GetTableFieldValue(&struCursor, "TSK_FILTER")));
		TrimRightChar(szFilterNeId, ',');
		nFilterLen = strlen(szFilterNeId);
	}
	FreeCursor(&struCursor);
	
	memset(szSql, 0, sizeof(szSql));
	snprintf(szSql, sizeof(szSql), "select * from man_TaskDetail where tkd_TaskId = %d", nTaskId);
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
    if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]������\n", szSql);
		FreeCursor(&struCursor);
		return EXCEPTION;
	}
	else
	{
		nProStyle = atoi(GetTableFieldValue(&struCursor, "TKD_STYLE"));
	    if (nProStyle == 0)  //���ٲ�ѯ,�����ѯ��������ѯ 0Ϊ��ѯ:COMMAND_QUERY
	    {
             strcpy(szTaskQryParm, (GetTableFieldValue(&struCursor, "tkd_qryparam")));
		     strcpy(szBase, (GetTableFieldValue(&struCursor, "tkd_base")));
			 strcpy(szRadio, (GetTableFieldValue(&struCursor, "tkd_radio")));
			 strcpy(szAlarmen, (GetTableFieldValue(&struCursor, "tkd_alarmen")));
			 strcpy(szAlarm, (GetTableFieldValue(&struCursor, "tkd_alarm")));
			 strcpy(szRadioSC, (GetTableFieldValue(&struCursor, "tkd_radiosc")));
			 strcpy(szRealTime, (GetTableFieldValue(&struCursor, "tkd_realtime")));
        } 
        else //���� 1Ϊ��ѯ:COMMAND_SET
        {
            memset(szSetEleParam, 0, sizeof(szSetEleParam));
          	strcpy(szSetEleParam, (GetTableFieldValue(&struCursor, "tkd_setparam")));
            
            memset(szSetEleValue, 0, sizeof(szSetEleValue));
            strcpy(szSetEleValue, (GetTableFieldValue(&struCursor, "tkd_setvalue")));
        }
		FreeCursor(&struCursor);
	}
	
	//ȡ������ϸ
	memset(szSql, 0, sizeof(szSql));

	//2009.12.9 add ���˻���
	if(nFilterLen > 1)
		snprintf(szSql, sizeof(szSql), "select a.*, b.ne_neid,b.ne_protocoldevicetypeid,b.ne_devicetypeid,b.ne_protocoltypeid,b.ne_devicestatusid, b.ne_commtypeid, b.ne_netelnum, b.ne_telnum1, b.ne_telnum2,b.ne_telnum3,b.ne_telnum4,b.ne_telnum5,"
	  		" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport,  b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid where  (b.ne_devicestatusid=0 or b.ne_devicestatusid=17) and b.ne_neid in (%s)",
	        	szFilterNeId);
	else
		snprintf(szSql, sizeof(szSql), "select a.*, b.ne_neid,b.ne_protocoldevicetypeid,b.ne_devicetypeid,b.ne_protocoltypeid,b.ne_devicestatusid, b.ne_commtypeid, b.ne_netelnum, b.ne_telnum1, b.ne_telnum2,b.ne_telnum3,b.ne_telnum4,b.ne_telnum5,"
	  		" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport,  b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid  where (b.ne_devicestatusid=0 or b.ne_devicestatusid=17) and b.ne_devicetypeid <> 146");
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	nNowTime = (int)time(NULL);	
    while(FetchCursor(&struCursor) == NORMAL)
	{
		nDeviceTypeId = atoi(GetTableFieldValue(&struCursor, "ne_devicetypeid"));
	    nProtocolTypeId= atoi(GetTableFieldValue(&struCursor, "ne_protocoltypeid"));
        nProtocolDeviceTypeId= atoi(GetTableFieldValue(&struCursor, "ne_protocoldevicetypeid"));
        
        nDeviceStatusId= atoi(GetTableFieldValue(&struCursor, "ne_devicestatusid"));
        strcpy(szServerTelNum,  (GetTableFieldValue(&struCursor, "ne_servertelnum")));
        
        InsertInXmlExt(pstruXml,"<omc>/<��Ԫ���>", GetTableFieldValue(&struCursor, "ne_neid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸����>", GetTableFieldValue(&struCursor, "ne_protocoldevicetypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<Э������>", GetTableFieldValue(&struCursor, "ne_protocoltypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ����>", GetTableFieldValue(&struCursor, "ne_repeaterid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸���>", GetTableFieldValue(&struCursor, "ne_deviceid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ��绰>", GetTableFieldValue(&struCursor, "ne_netelnum"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ��IP>", GetTableFieldValue(&struCursor, "ne_deviceip"), MODE_AUTOGROW|MODE_UNIQUENAME);
        
        //InsertInXmlExt(pstruXml,"<omc>/<������ʶ>", GetTableFieldValue(&struCursor, "ne_otherdeviceid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸�ͺ�>", GetTableFieldValue(&struCursor, "ne_devicemodelid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�˿ں�>", GetTableFieldValue(&struCursor, "ne_deviceport"), MODE_AUTOGROW|MODE_UNIQUENAME);

        InsertInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>", GetTableFieldValue(&struCursor, "ne_commtypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������>",  szServerTelNum, MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������1>", (GetTableFieldValue(&struCursor, "ne_telnum1")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������2>", (GetTableFieldValue(&struCursor, "ne_telnum2")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������3>", (GetTableFieldValue(&struCursor, "ne_telnum3")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������4>", (GetTableFieldValue(&struCursor, "ne_telnum4")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������5>", (GetTableFieldValue(&struCursor, "ne_telnum5")), MODE_AUTOGROW|MODE_UNIQUENAME);
		
		strcpy(szObjList, (GetTableFieldValue(&struCursor, "ne_objlist")));
		
		memset(szBasePoll, 0, sizeof(szBasePoll));
		memset(szRadioPoll, 0, sizeof(szRadioPoll));
		memset(szAlarmenPoll, 0, sizeof(szAlarmenPoll));
		memset(szAlarmPoll, 0, sizeof(szAlarmPoll));
		memset(szRadioSCPoll, 0, sizeof(szRadioSCPoll));
		memset(szRealTimePoll, 0, sizeof(szRealTimePoll));
		nPollDeviceTypeId = atoi(GetTableFieldValue(&struCursor, "devicetypeid"));

		if (nPollDeviceTypeId > 0)
		{
			strcpy(szBasePoll, GetTableFieldValue(&struCursor, "base_id"));
			strcpy(szRadioPoll, GetTableFieldValue(&struCursor, "radio_id"));
			strcpy(szAlarmenPoll, GetTableFieldValue(&struCursor, "alarmen_id"));
			strcpy(szAlarmPoll, GetTableFieldValue(&struCursor, "alarm_id"));
			strcpy(szRadioSCPoll, GetTableFieldValue(&struCursor, "radiosc_id"));
			strcpy(szRealTimePoll, GetTableFieldValue(&struCursor, "realtime_id"));
		}
		
        //Ϊ��ѯ:COMMAND_QUERY
        if (nProStyle == 0)
        {
            InsertInXmlExt(pstruXml,"<omc>/<�����>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
            InsertInXmlExt(pstruXml,"<omc>/<����>", "11", MODE_AUTOGROW|MODE_UNIQUENAME);
        }
        else
        {
        	if(strcmp(szStyle, "201") == 0 || strcmp(szStyle, "202") == 0 || 
        	   strcmp(szStyle, "211") == 0 || strcmp(szStyle, "212") == 0)
				InsertInXmlExt(pstruXml,"<omc>/<�����>", "181", MODE_AUTOGROW|MODE_UNIQUENAME);
			else
            	InsertInXmlExt(pstruXml,"<omc>/<�����>", "2", MODE_AUTOGROW|MODE_UNIQUENAME);
            InsertInXmlExt(pstruXml,"<omc>/<����>", "22", MODE_AUTOGROW|MODE_UNIQUENAME);
        }
        
        //վ��ȼ�Ϊ��ͨ:OMC_NORMAL_MSGLEVEL
        InsertInXmlExt(pstruXml,"<omc>/<վ��ȼ�>", OMC_NORMAL_MSGLEVEL, MODE_AUTOGROW|MODE_UNIQUENAME);
        
                
        memset(&struHead, 0, sizeof(COMMANDHEAD));
        memset(&struRepeater, 0, sizeof(REPEATER_INFO));
        //��ֵ
        nNeId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��Ԫ���>"));  
	    struRepeater.nCommType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));  
	    struRepeater.nRepeaterId = atol(DemandStrInXmlExt(pstruXml,"<omc>/<վ����>")); 
	    struRepeater.nDeviceId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸���>")); 
	    strcpy(struRepeater.szTelephoneNum, DemandStrInXmlExt(pstruXml,"<omc>/<վ��绰>"));
	    strcpy(struRepeater.szIP, DemandStrInXmlExt(pstruXml,"<omc>/<վ��IP>"));
	    
	    if (strlen(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")) == 0)
	    	struRepeater.nPort = 0;
	    else
	    	struRepeater.nPort = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")); 
	    
	    struRepeater.nProtocolDeviceType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸����>"));
	    strcpy(struRepeater.szSpecialCode, DemandStrInXmlExt(pstruXml,"<omc>/<������ʶ>"));
	    strcpy(struRepeater.szReserve, DemandStrInXmlExt(pstruXml,"<omc>/<�豸�ͺ�>"));
	    strcpy(struRepeater.szNetCenter, DemandStrInXmlExt(pstruXml,"<omc>/<�������>"));
	    
	    struHead.nProtocolType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<Э������>"));
	    struHead.nCommandCode = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�����>"));
	    nEleCount ++; //��Ԫ��
	    
	    // ��ʱ���в�������100 ����103
	    if (strcmp(szStyle, "100") == 0)
	    {
	        InsertMosTask(pstruXml);
	        continue;
	    }
	    memset(szQryEleParam, 0, sizeof(szQryEleParam));
	    if (nProStyle == 0)  //���ٲ�ѯ,�����ѯ��������ѯ 0Ϊ��ѯ:COMMAND_QUERY
	    {
		     //����ѵ������Ч��2.0
		     if ( (strcmp(szStyle, "1") == 0 || strcmp(szStyle, "2") == 0) &&
				    nDeviceTypeId == 146)
			    continue;

             if (strlen(szServerTelNum) == 0 && struRepeater.nCommType != 6)
             {
                 nFailEleCount ++;
                 InsertFailNeid(nTaskLogId, nNeId, "�绰����У��ʧ��", "��Ԫ������ĺ���Ϊ��");
                 continue;
             }
                          
             if (struHead.nProtocolType == PROTOCOL_2G || 
             	 struHead.nProtocolType == PROTOCOL_DAS ||
             	 struHead.nProtocolType == PROTOCOL_JINXIN_DAS)
             {
             	 if (strcmp(szStyle, "1") == 0) //������ѵ
		     	 {
		     	 	 memset(szRadio, 0, sizeof(szRadio));
		     	 	 strcpy(szRadio, (GetTableFieldValue(&struCursor, "ne_ActiveRow")));
		     	 	 nSeperateNum = SeperateString(szRadio,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             for(i=0; i< nSeperateNum; i++)
		             {
		             	memset(szTemp, 0, sizeof(szTemp));
		             	strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
		             	sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		             }
		             PrintDebugLog(DBG_HERE, "neid[%d]EleParam[%s]\n", nNeId, szQryEleParam);
		     	 }
		     	 	
		     	 if (strcmp(szStyle, "213") == 0)//������ѯ
		     	 {
                     // man_taskpoll table filter logic
			 	     if (strstr(szTaskQryParm, "base") != NULL && strlen(szBase) >= 4)                    
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szBase);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
		             	 	if (nPollDeviceTypeId > 0 && strstr(szBasePoll, szTemp) == NULL){
								//PrintDebugLog(DBG_HERE, "base not found mapid, repeaterid[%08X], neid[%d], basepoll[%s], mapid[%s]\n", struRepeater.nRepeaterId, nNeId, szBasePoll, szTemp);
								continue;
							}
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "alarmen") != NULL && strlen(szAlarmen) >= 4)
			     	 {
		             	 memset(szSepStr, 0, sizeof(szSepStr));
		             	 strcpy(szSepStr, szAlarmen);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
				 	     for(i=0; i< nSeperateNum; i++)
				 	     {
							 memset(szTemp, 0, sizeof(szTemp));
					 	     strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
					 	     if (nPollDeviceTypeId > 0 && strstr(szAlarmenPoll, szTemp) == NULL){
									//PrintDebugLog(DBG_HERE, "alarmen not found mapid, repeaterid[%08X], neid[%d], alarmpoll[%s], mapid[%s]\n", struRepeater.nRepeaterId, nNeId, szAlarmenPoll, szTemp);
			             	 		continue;
						 	 }
					 	     //if (strstr(szObjList, szTemp) != NULL)
					 	     	sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
				 	     }
			     	 }
			     	 if (strstr(szTaskQryParm, "radio") != NULL && strlen(szRadio) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRadio);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRadioPoll, szTemp) == NULL){
		             	 		//PrintDebugLog(DBG_HERE, "radio not found mapid, repeaterid[%08X], neid[%d], radiopoll[%s], mapid[%s]\n", struRepeater.nRepeaterId, nNeId, szRadioPoll, szTemp);
								continue;
							}
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "radiosc") != NULL && strlen(szRadioSC) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRadioSC);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRadioSCPoll, szTemp) == NULL){
		             	 		//PrintDebugLog(DBG_HERE, "radiosc not found mapid, repeaterid[%08X], neid[%d], radioscpoll[%s], mapid[%s]\n", struRepeater.nRepeaterId, nNeId, szRadioSCPoll, szTemp);
								continue;
							}
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "realtime") != NULL && strlen(szRealTime) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRealTime);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRealTimePoll, szTemp) == NULL){
		             	 		//PrintDebugLog(DBG_HERE, "realtime not found mapid, repeaterid[%08X], neid[%d], realtimepoll[%s], mapid[%s]\n", struRepeater.nRepeaterId, nNeId, szRealTimePoll, szTemp);
								continue;
							}
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
		     	 }
		     	 else//���ٲ�ѯ
		     	 {
		     	 	 memset(szAlarm, 0, sizeof(szAlarm));
		     	 	 //strcpy(szAlarmen, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmEnabledObjList")));
			 	     strcpy(szAlarm, (GetTableFieldValue(&struCursor, "ne_AlarmObjList")));
			     	 //������ѵ�����������в���
			     	 PrintDebugLog(DBG_HERE, "neid[%d]alarm[%s]\n", nNeId, szAlarm);
	             }
	             
	             
             	 if (strstr(szTaskQryParm, "alarm") != NULL && strlen(szAlarm) >= 4)
	             {
	             	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 strcpy(szSepStr, szAlarm);
	             	 nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
	             	 for(i=0; i< nSeperateNum; i++)
	             	 {
						memset(szTemp, 0, sizeof(szTemp));
						if (strstr(pszTempStr[i], ":"))
	             	 		strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i])-2);
	             	 	else
	             	 		strncpy(szTemp, pszTempStr[i], strlen(pszTempStr[i]));
						if (nPollDeviceTypeId > 0 && strstr(szAlarmPoll, szTemp) == NULL)
		             	 		continue;
						//if (strstr(szObjList, szTemp) != NULL)
                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
	                 }
	             }
	                          
             	 
             	 PrintDebugLog(DBG_HERE, "repeaterid[%08X], neid[%d]taskQryParm[%s]EleParam[%s]\n", struRepeater.nRepeaterId, nNeId, szTaskQryParm, szQryEleParam);
		     	 	             
	             if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6)
	             	ResolveQryParamArrayGprs(szQryEleParam);
	             else	
	             	ResolveQryParamArray(szQryEleParam);
	             //Ϊ�ղ���ѵ     
	             if (strlen(szQryEleParam) == 0) {
					
					continue;
				 }
	             PrintDebugLog(DBG_HERE, "turn task[%s]\n", szQryEleParam);
	             
	             if (nTimes++>= 6)//У׼����ʱ��
	        	 {
	        	 	 nNowTime ++;
	        	 	 nTimes = 0;
	        	 	 if (nNowTime - 5000 >=(int)time(NULL))
	        	 	 	 nNowTime = (int)time(NULL);
	        	 }

	             nObjCount = SeperateString(szQryEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
	             for(i=0; i< nObjCount; i++)
		         {
		             //��ض����ÿո�ָ�
	 	             //strcpy(szQryEleParam, "0301 0304 0704"); // for test
	 	             if (strlen(szUploadTime) == 14)
	 	             	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", szUploadTime, MODE_AUTOGROW|MODE_UNIQUENAME);
	 	             else 
	 	             	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime(nNowTime+i*5000), MODE_AUTOGROW|MODE_UNIQUENAME);
	 	             
		             InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
	            	 
	            	 if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6)
	            	 {
						long long curr_time = (long long)get_timestamp();
	            	 	if (QryElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml) != NORMAL){
							PrintDebugLog(DBG_HERE, "query element param failed[%d]\n", struRepeater.nCommType);
							continue;
						}
	            	 	SaveToGprsQueue(pstruXml);
						long long end_time = (long long)get_timestamp();
						if ((end_time-curr_time)>10*1000){
							//over 10 second
							PrintDebugLog(DBG_HERE, "process SaveToGprsQueue cost time over[%lld - %lld - %lld]\n", curr_time, end_time, end_time-curr_time);
					 	}

	            	 }
	            	 else
	            	 {
						long long curr_time = (long long)get_timestamp();
			         	if(QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml) != NORMAL){
			        		PrintDebugLog(DBG_HERE, "query element param failed[%d]\n", struRepeater.nCommType);
							continue;
						}
			         	//��̬�����ط����
			         	//DistServerTelNum(pstruXml);
			         	SaveToMsgQueue_Tmp(pstruXml);
						long long end_time = (long long)get_timestamp();
						if ((end_time-curr_time)>10*1000){
							//over 10 second
							PrintDebugLog(DBG_HERE, "process SaveToMsgQueue_Tmp cost time over[%lld - %lld - %d]\n", curr_time, end_time, end_time-curr_time);
					 	}
			         }
					 long long curr_time = (long long)get_timestamp();
					 SaveEleQryLog(pstruXml);
					 long long end_time = (long long)get_timestamp();
					 if ((end_time-curr_time)>10*1000){
						//over 10 second
						PrintDebugLog(DBG_HERE, "process save eleqrylog cost time over[%lld - %lld - %d]\n", curr_time, end_time, end_time-curr_time);
					 }
					 PrintDebugLog(DBG_HERE, "process save eleqrylog cost time end[%lld]\n", end_time);
			         nTxPackCount++;
			         //if (nTaskId != 11932 && i>= 1) break;
			         	
		         }
		     }
		    else if (struHead.nProtocolType == PROTOCOL_GSM ||  //add by wwj at 2010.07.28
				struHead.nProtocolType == PROTOCOL_CDMA ||
				struHead.nProtocolType == PROTOCOL_HEIBEI ||
				struHead.nProtocolType == PROTOCOL_XC_CP ||
				struHead.nProtocolType == PROTOCOL_SUNWAVE ||
				struHead.nProtocolType == PROTOCOL_WLK)
			{
				//ѡȡ~2GЭ��COMMADOBJECTS
				CURSORSTRU struCursor_CG;
				char szSpoolQryParam[100];
				char szObjects[2000];
				int nCmdCode;
				
				sprintf(szSql, "select CDO_COMMANDCODE, CDO_SPOOLQUERYPARAM, CDO_OBJECTS  from ne_commandobjects where CDO_PROTOCOLTYPE = %d", struHead.nProtocolType);

				PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
				if(SelectTableRecord(szSql, &struCursor_CG) != NORMAL)
				{
					PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
								  szSql, GetSQLErrorMessage());
					FreeCursor(&struCursor_CG);			  
					return EXCEPTION;
				}
			    while(FetchCursor(&struCursor_CG) == NORMAL)
			    {		
			    	strcpy(szSpoolQryParam,  TrimAllSpace(GetTableFieldValue(&struCursor_CG, "CDO_SPOOLQUERYPARAM")));
			    	strcpy(szObjects,  TrimAllSpace(GetTableFieldValue(&struCursor_CG, "CDO_OBJECTS")));				
					nCmdCode = atoi(GetTableFieldValue(&struCursor_CG, "CDO_COMMANDCODE"));
					
					if ((strstr(szSpoolQryParam, "base") != NULL && strstr(szTaskQryParm, "base") != NULL) ||
						(strstr(szSpoolQryParam, "radio") != NULL && strstr(szTaskQryParm, "radio") != NULL) ||
						(strstr(szSpoolQryParam, "alarm") != NULL && strstr(szTaskQryParm, "alarm") != NULL) ||
						(strstr(szSpoolQryParam, "alarmen") != NULL && strstr(szTaskQryParm, "alarmen") != NULL))
					{	
						struHead.nCommandCode = nCmdCode;
						//���ͱ���
						nNowTime = (int)time(NULL);	
						InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime(nNowTime), MODE_AUTOGROW|MODE_UNIQUENAME);
				        InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  szObjects, MODE_AUTOGROW|MODE_UNIQUENAME);
				        PrintDebugLog(DBG_HERE, "mark1[%s]\n", szObjects);
						QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
					    SaveToMsgQueue_Tmp(pstruXml);
					    SaveEleQryLog(pstruXml);
					    nTxPackCount++;
					    if (nTaskId == 4 && i>= 1)	break;
			         	if (nTaskId != 4 && i>= 0) break;
					}
				}
				FreeCursor(&struCursor_CG);	   
				
			}else{
				PrintErrorLog(DBG_HERE, "not soupport protocol, %d\n", struHead.nProtocolType);
			}	
        } 
        else //���� 1Ϊ��ѯ:COMMAND_SET
        {
        	
             //�豸״̬������
            /*if (nDeviceStatusId > 10)
            {
                nFailEleCount ++;
                InsertFailNeid(nTaskLogId, nNeId,  "��ѯʧ��",  GetDeviceStatu(nDeviceStatusId));
                continue;
            }*/
            if (strlen(szServerTelNum) == 0 && struRepeater.nCommType != 6)
            {
                nFailEleCount ++;
                InsertFailNeid(nTaskLogId, nNeId, "�绰����У��ʧ��", "��Ԫ������ĺ���Ϊ��");
                continue;
            }
            /******************** 
            memset(szSetEleParam, 0, sizeof(szSetEleParam));
            //strcpy(szSetEleParam, TrimAllSpace(GetTableFieldValue(&struCursor, "tkd_base")));
           
            //if (strcmp(szStyle, "214") == 0)//�������� web�����Ҫ�޸� add by wwj at 2010.07.28
            	strcpy(szSetEleParam, TrimAllSpace(GetTableFieldValue(&struCursor, "tkd_setparam")));
            
            memset(szSetEleValue, 0, sizeof(szSetEleValue));
            strcpy(szSetEleValue, TrimAllSpace(GetTableFieldValue(&struCursor, "tkd_setvalue")));
           ******************/
 
            if(strcmp(szStyle, "201") == 0 && strcmp(szSetEleValue, "300000000") == 0 )
            {
             	time_t struTimeNow;
			  	struct tm struTmNow;
			  
			  	time(&struTimeNow);
			    memset(szSetEleValue,0,sizeof(szSetEleValue));
			  	struTmNow=*localtime(&struTimeNow);
			  	//3YYYYMMDD 7λ
			  	snprintf(szSetEleValue,sizeof(szSetEleValue),"3%04d%02d%02d",
			  	struTmNow.tm_year+1900,struTmNow.tm_mon+1,struTmNow.tm_mday-1);
            }
            
            //Զ������, ����������ˮ��Ϊ1
            if(strcmp(szStyle, "200") == 0)
            {
            	InsertInXmlExt(pstruXml,"<omc>/<��ˮ��>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
            }
                                       
            ResolveSetParamArray(szSetEleParam, szSetEleValue);
            PrintDebugLog(DBG_HERE, "��ѵ���ö���[%s][%s]\n", szSetEleParam, szSetEleValue);
            
            if (nTimes++>= 6)//У׼����ʱ��
	        {
	        	 nNowTime ++;
	        	 nTimes = 0;
	        	 if (nNowTime - 6000 >=(int)time(NULL))
	        	 	 nNowTime = (int)time(NULL);
	        }
	        	 
            nObjCount = SeperateString(szSetEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
            nObjCount = SeperateString(szSetEleValue, '|', pszSepValueStr, MAX_SEPERATE_NUM);
            for(i=0; i< nObjCount; i++)
	        {
	            //��ض����ÿո�ָ�
 	            
 	             if (strlen(szUploadTime) == 14)
 	            	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", szUploadTime, MODE_AUTOGROW|MODE_UNIQUENAME);
 	             else 
 	            	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime(nNowTime + i*6000), MODE_AUTOGROW|MODE_UNIQUENAME);
	             InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
	             InsertInXmlExt(pstruXml,"<omc>/<��ض�������>",  pszSepValueStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
            	 if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6)
	             {
	            	 SetElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
	            	 SaveToGprsQueue(pstruXml);
	             }
	             else
	             {
		         	SetElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
		         	//��̬�����ط����
		         	//DistServerTelNum(pstruXml);
		         	SaveToMsgQueue_Tmp(pstruXml);
		         }
		         SaveEleSetLog(pstruXml);
		         nTxPackCount++;
	        }
	        
	        //Զ������
            if(strcmp(szStyle, "200") == 0)
            {
            	 RemortUpdateDbOperate1(pstruXml);
            }

        }
        
        //������Ԫ�����ͱ�����
        if ((nEleCount >= 1000) || (nTxPackCount >= 1000))
        {
            UpdateTaskLogCount(nTaskLogId, nTaskId, nEleCount, nTxPackCount, nFailEleCount);
            nEleCount = 0;
            nTxPackCount = 0;
            nFailEleCount = 0;
            if (getTaskStopUsing(nTaskId) == BOOLTRUE)
                break;
        }
            
            

	}
    if ((nEleCount > 0) || (nTxPackCount > 0) || (nFailEleCount > 0))
	    UpdateTaskLogCount(nTaskLogId, nTaskId, nEleCount, nTxPackCount, nFailEleCount);
	FreeCursor(&struCursor);
    DeleteXml(pstruXml);

    return NORMAL;
	
}


/* 
 * ��ʱ��ѯ���� 
 */
RESULT ProcessTimeReTurnTask(PSTR pszCaReqBuffer)
{
	XMLSTRU struXml;
	PXMLSTRU pstruXml=&struXml;
    CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	REPEATER_INFO struRepeater;
	COMMANDHEAD struHead;
	int nTaskId, nTaskLogId;	/* ����� */
	int nProStyle=0;          /* ��������*/
	int nProtocolTypeId, nProtocolDeviceTypeId, nDeviceTypeId, nPollDeviceTypeId;
	int nDeviceStatusId, nNeId;
	int nFailEleCount=0, nObjCount, i=0;
	int nEleCount=0;
	int nTxPackCount=0;
	STR szQryEleParam[MAX_BUFFER_LEN];
	STR szServerTelNum[20], szTemp[200];
	//STR szCmdObjectList[MAX_BUFFER_LEN];
	PSTR pszSepParamStr[MAX_OBJECT_NUM];  /* �ָ��ַ�����*/
	STR szStyle[10];
	int nNowTime, nTimes=0;
	STR szUploadTime[100];
	STR szTaskQryParm[1000], szBase[2000], szRadio[2000], szAlarm[2000], szAlarmen[2000];
	STR szRealTime[2000], szRadioSC[2000], szObjList[2000], szSepStr[2000];
	STR szBasePoll[2000], szRadioPoll[2000], szAlarmPoll[2000], szAlarmenPoll[2000];
	STR szRealTimePoll[2000], szRadioSCPoll[2000];
	PSTR pszTempStr[ MAX_OBJECT_NUM];
	INT nSeperateNum;

 	memset(pstruXml,0,sizeof(XMLSTRU));
	if(ImportXml(pstruXml,FALSE,pszCaReqBuffer)!=NORMAL)
	{
		PrintErrorLog(DBG_HERE,"����[%s]�Ƿ�\n",pszCaReqBuffer);
		DeleteXml(pstruXml);
		return -1;
	}
	nTaskId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<taskid>"));
	
	strcpy(szStyle, DemandStrInXmlExt(pstruXml,"<omc>/<style>"));

    //��¼������־
    if (strcmp(szStyle, "215") == 0)//��ѯ
    InsertTaskLog(nTaskId, &nTaskLogId, szStyle);
    sprintf(szTemp, "%d", nTaskId);
    InsertInXmlExt(pstruXml,"<omc>/<�����>",  szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
    sprintf(szTemp, "%d", nTaskLogId);
    InsertInXmlExt(pstruXml,"<omc>/<������־��>",  szTemp, MODE_AUTOGROW|MODE_UNIQUENAME);
    
 	InsertInXmlExt(pstruXml,"<omc>/<����״̬>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
	//���ٸ�ѯ�������
    strcpy(szTaskQryParm, "alarm,alarmen");
	
	//ȡ������ϸ
	memset(szSql, 0, sizeof(szSql));

	if (strcmp(szStyle, "216") == 0)//�澯��ѯ
		snprintf(szSql, sizeof(szSql), "select a.*, b.ne_neid,b.ne_protocoldevicetypeid,b.ne_devicetypeid,b.ne_protocoltypeid,b.ne_devicestatusid, b.ne_commtypeid, b.ne_netelnum, b.ne_telnum1, b.ne_telnum2,b.ne_telnum3,b.ne_telnum4,b.ne_telnum5,"
	  		" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport, b.ne_otherdeviceid, b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid where  (b.ne_devicestatusid=0 or b.ne_devicestatusid=17) and b.ne_neid in (select alg_neid from alm_alarmlog where alg_alarmstatusid = 1 and alg_alarmid in (1,2,9,10,11,12,13,14,15,20,31,74,141,158,167,168,169,200,201,202,203,204,205,206,207,210,211,212,213,214,215,216,217))");
	  		//" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport, b.ne_otherdeviceid, b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid where  b.ne_neid in (select alg_neid from alm_alarmlog where alg_alarmstatusid = 1 and alg_alarmid in (1,2,9,10,11,12,13,14,15,20,31,74,141,158,167,168,169,200,201,202,203,204,205,206,207,210,211,212,213,214,215,216,217))");
	else if (strcmp(szStyle, "215") == 0)//��ѯ
		snprintf(szSql, sizeof(szSql), "select a.*, b.ne_neid,b.ne_protocoldevicetypeid,b.ne_devicetypeid,b.ne_protocoltypeid,b.ne_devicestatusid, b.ne_commtypeid, b.ne_netelnum, b.ne_telnum1, b.ne_telnum2,b.ne_telnum3,b.ne_telnum4,b.ne_telnum5,"
	  		" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport, b.ne_otherdeviceid, b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid  where (b.ne_devicestatusid=0 or b.ne_devicestatusid=17) "
	  		//" b.ne_servertelnum, b.ne_repeaterid, b.ne_deviceid, b.ne_deviceip, b.ne_deviceport, b.ne_otherdeviceid, b.ne_devicemodelid, ne_objlist, ne_ActiveCol,ne_ActiveRow, ne_AlarmObjList, ne_AlarmEnabledObjList  from ne_element b left join man_taskpoll a on b.ne_devicetypeid = a.devicetypeid  where "
	  		" and b.ne_lastupdatetime < to_date('%s000000', 'yyyymmddhh24miss')", GetSystemDate());
	  		//" b.ne_lastupdatetime < to_date('%s000000', 'yyyymmddhh24miss')", GetSystemDate());
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	nNowTime = (int)time(NULL);	
    while(FetchCursor(&struCursor) == NORMAL)
	{
		nDeviceTypeId = atoi(GetTableFieldValue(&struCursor, "ne_devicetypeid"));
	    nProtocolTypeId= atoi(GetTableFieldValue(&struCursor, "ne_protocoltypeid"));
        nProtocolDeviceTypeId= atoi(GetTableFieldValue(&struCursor, "ne_protocoldevicetypeid"));
        
        nDeviceStatusId= atoi(GetTableFieldValue(&struCursor, "ne_devicestatusid"));
        strcpy(szServerTelNum,  TrimAllSpace(GetTableFieldValue(&struCursor, "ne_servertelnum")));
        
        InsertInXmlExt(pstruXml,"<omc>/<��Ԫ���>", GetTableFieldValue(&struCursor, "ne_neid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸����>", GetTableFieldValue(&struCursor, "ne_protocoldevicetypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<Э������>", GetTableFieldValue(&struCursor, "ne_protocoltypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ����>", GetTableFieldValue(&struCursor, "ne_repeaterid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸���>", GetTableFieldValue(&struCursor, "ne_deviceid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ��绰>", GetTableFieldValue(&struCursor, "ne_netelnum"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<վ��IP>", GetTableFieldValue(&struCursor, "ne_deviceip"), MODE_AUTOGROW|MODE_UNIQUENAME);
        
        InsertInXmlExt(pstruXml,"<omc>/<������ʶ>", GetTableFieldValue(&struCursor, "ne_otherdeviceid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�豸�ͺ�>", GetTableFieldValue(&struCursor, "ne_devicemodelid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�˿ں�>", GetTableFieldValue(&struCursor, "ne_deviceport"), MODE_AUTOGROW|MODE_UNIQUENAME);

        InsertInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>", GetTableFieldValue(&struCursor, "ne_commtypeid"), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������>",  szServerTelNum, MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������1>", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_telnum1")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������2>", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_telnum2")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������3>", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_telnum3")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������4>", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_telnum4")), MODE_AUTOGROW|MODE_UNIQUENAME);
        InsertInXmlExt(pstruXml,"<omc>/<�������5>", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_telnum5")), MODE_AUTOGROW|MODE_UNIQUENAME);
		
		strcpy(szObjList, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_objlist")));
		
		memset(szBasePoll, 0, sizeof(szBasePoll));
		memset(szRadioPoll, 0, sizeof(szRadioPoll));
		memset(szAlarmenPoll, 0, sizeof(szAlarmenPoll));
		memset(szAlarmPoll, 0, sizeof(szAlarmPoll));
		memset(szRadioSCPoll, 0, sizeof(szRadioSCPoll));
		memset(szRealTimePoll, 0, sizeof(szRealTimePoll));
		nPollDeviceTypeId = atoi(GetTableFieldValue(&struCursor, "devicetypeid"));

		if (nPollDeviceTypeId > 0)
		{
			strcpy(szBasePoll, GetTableFieldValue(&struCursor, "base_id"));
			strcpy(szRadioPoll, GetTableFieldValue(&struCursor, "radio_id"));
			strcpy(szAlarmenPoll, GetTableFieldValue(&struCursor, "alarmen_id"));
			strcpy(szAlarmPoll, GetTableFieldValue(&struCursor, "alarm_id"));
			strcpy(szRadioSCPoll, GetTableFieldValue(&struCursor, "radiosc_id"));
			strcpy(szRealTimePoll, GetTableFieldValue(&struCursor, "realtime_id"));
		}
		
        //Ϊ��ѯ:COMMAND_QUERY
        if (nProStyle == 0)
        {
            InsertInXmlExt(pstruXml,"<omc>/<�����>", "1", MODE_AUTOGROW|MODE_UNIQUENAME);
            InsertInXmlExt(pstruXml,"<omc>/<����>", "11", MODE_AUTOGROW|MODE_UNIQUENAME);
        }

        
        //վ��ȼ�Ϊ��ͨ:OMC_NORMAL_MSGLEVEL
        InsertInXmlExt(pstruXml,"<omc>/<վ��ȼ�>", OMC_NORMAL_MSGLEVEL, MODE_AUTOGROW|MODE_UNIQUENAME);
        
                
        memset(&struHead, 0, sizeof(COMMANDHEAD));
        memset(&struRepeater, 0, sizeof(REPEATER_INFO));
        //��ֵ
        nNeId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<��Ԫ���>"));  
	    struRepeater.nCommType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<ͨ�ŷ�ʽ>"));  
	    struRepeater.nRepeaterId = atol(DemandStrInXmlExt(pstruXml,"<omc>/<վ����>")); 
	    struRepeater.nDeviceId = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸���>")); 
	    strcpy(struRepeater.szTelephoneNum, DemandStrInXmlExt(pstruXml,"<omc>/<վ��绰>"));
	    strcpy(struRepeater.szIP, DemandStrInXmlExt(pstruXml,"<omc>/<վ��IP>"));
	    
	    if (strlen(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")) == 0)
	    	struRepeater.nPort = 0;
	    else
	    	struRepeater.nPort = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�˿ں�>")); 
	    
	    struRepeater.nProtocolDeviceType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�豸����>"));
	    strcpy(struRepeater.szSpecialCode, DemandStrInXmlExt(pstruXml,"<omc>/<������ʶ>"));
	    strcpy(struRepeater.szReserve, DemandStrInXmlExt(pstruXml,"<omc>/<�豸�ͺ�>"));
	    strcpy(struRepeater.szNetCenter, DemandStrInXmlExt(pstruXml,"<omc>/<�������>"));
	    
	    struHead.nProtocolType = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<Э������>"));
	    struHead.nCommandCode = atoi(DemandStrInXmlExt(pstruXml,"<omc>/<�����>"));
	    nEleCount ++; //��Ԫ��
	    
	    memset(szQryEleParam, 0, sizeof(szQryEleParam));
	    if (nProStyle == 0)  //���ٲ�ѯ,�����ѯ��������ѯ 0Ϊ��ѯ:COMMAND_QUERY
	    {
		     //����ѵ������Ч��2.0
		     if ( (strcmp(szStyle, "1") == 0 || strcmp(szStyle, "2") == 0) &&
				    nDeviceTypeId == 146)
			    continue;

             if (strlen(szServerTelNum) == 0 && struRepeater.nCommType != 6)
             {
                 nFailEleCount ++;
                 InsertFailNeid(nTaskLogId, nNeId, "�绰����У��ʧ��", "��Ԫ������ĺ���Ϊ��");
                 continue;
             }
                          
             if (struHead.nProtocolType == PROTOCOL_2G)
             {	
		     	 if (strcmp(szStyle, "213") == 0)//������ѯ
		     	 {
		     	 	 	     	 	 
			 	     if (strstr(szTaskQryParm, "base") != NULL && strlen(szBase) >= 4)                    
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szBase);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], 4);
		             	 	if (nPollDeviceTypeId > 0 && strstr(szBasePoll, szTemp) == NULL)
		             	 		continue;
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "radio") != NULL && strlen(szRadio) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRadio);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], 4);
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRadioPoll, szTemp) == NULL)
		             	 		continue;
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "radiosc") != NULL && strlen(szRadioSC) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRadioSC);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], 4);
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRadioSCPoll, szTemp) == NULL)
		             	 		continue;
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
			     	 if (strstr(szTaskQryParm, "realtime") != NULL && strlen(szRealTime) >= 4)
			     	 {
	             	 	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 	 strcpy(szSepStr, szRealTime);
				 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
		             	 for(i=0; i< nSeperateNum; i++)
		             	 {
							memset(szTemp, 0, sizeof(szTemp));
		             	 	strncpy(szTemp, pszTempStr[i], 4);
		             	 	if (nPollDeviceTypeId > 0 && strstr(szRealTimePoll, szTemp) == NULL)
		             	 		continue;
		             	 	//if (strstr(szObjList, szTemp) != NULL)
		                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
		                 }
			     	 }
		     	 }
		     	 else//���ٲ�ѯ,�����ѯ
		     	 {
		     	 	 strcpy(szAlarmen, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmEnabledObjList")));
			 	     strcpy(szAlarm, TrimAllSpace(GetTableFieldValue(&struCursor, "ne_AlarmObjList")));
			     	 //������ѵ�����������в���
	             }
	             PrintDebugLog(DBG_HERE, "deviceid[%d]alarmenpoll[%s]alarmen[%s]\n", nPollDeviceTypeId, szAlarmenPoll, szAlarmen);
	             
             	 if (strstr(szTaskQryParm, "alarm") != NULL && strlen(szAlarm) >= 4)
	             {
	             	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 strcpy(szSepStr, szAlarm);
	             	 nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
	             	 for(i=0; i< nSeperateNum; i++)
	             	 {
						memset(szTemp, 0, sizeof(szTemp));
	             	 	strncpy(szTemp, pszTempStr[i], 4);
	             	 	if (nPollDeviceTypeId > 0 && strstr(szAlarmPoll, szTemp) == NULL)
		             	 		continue;
	             	 	//if (strstr(szObjList, szTemp) != NULL)
	                 		sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
	                 }
	             }
	            
		/****************** 
	             if (strstr(szTaskQryParm, "alarmen") != NULL && strlen(szAlarmen) >= 4)
		     	 {
	             	 memset(szSepStr, 0, sizeof(szSepStr));
	             	 strcpy(szSepStr, szAlarmen);
			 	     nSeperateNum = SeperateString(szSepStr,  ',', pszTempStr,  MAX_OBJECT_NUM);
			 	     for(i=0; i< nSeperateNum; i++)
			 	     {
						 memset(szTemp, 0, sizeof(szTemp));
				 	     strncpy(szTemp, pszTempStr[i], 4);
				 	     if (nPollDeviceTypeId > 0 && strstr(szAlarmenPoll, szTemp) == NULL)
		             	 		continue;
				 	     //if (strstr(szObjList, szTemp) != NULL)
				 	     	sprintf(szQryEleParam, "%s%s,", szQryEleParam, szTemp);
			 	     }
		     	 }
             	**********************/ 
             	 PrintDebugLog(DBG_HERE, "taskQryParm[%s]EleParam[%s]\n", szTaskQryParm, szQryEleParam);
		     	 	             
	             if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6)
	             	ResolveQryParamArrayGprs(szQryEleParam);
	             else	
	             	ResolveQryParamArray(szQryEleParam);
	             //Ϊ�ղ���ѵ     
	             if (strlen(szQryEleParam) == 0) continue;
	             PrintDebugLog(DBG_HERE, "��ѵ����[%s]\n", szQryEleParam);
	             
	             if (nTimes++>= 6)//У׼����ʱ��
	        	 {
	        	 	 nNowTime ++;
	        	 	 nTimes = 0;
	        	 	 if (nNowTime - 600 >=(int)time(NULL))
	        	 	 	 nNowTime = (int)time(NULL);
	        	 }

	             nObjCount = SeperateString(szQryEleParam, '|', pszSepParamStr, MAX_SEPERATE_NUM);
	             for(i=0; i< nObjCount; i++)
		         {
		             
		             //��ض����ÿո�ָ�
	 	             //strcpy(szQryEleParam, "0301 0304 0704"); // for test
	 	             if (strlen(szUploadTime) == 14)
	 	             	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", szUploadTime, MODE_AUTOGROW|MODE_UNIQUENAME);
	 	             else 
	 	             	InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime(nNowTime+i*600), MODE_AUTOGROW|MODE_UNIQUENAME);
	 	             
		             InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  pszSepParamStr[i], MODE_AUTOGROW|MODE_UNIQUENAME);
	            	 
	            	 if (struRepeater.nCommType == 5 || struRepeater.nCommType == 6)
	            	 {
	            	 	QryElementParam(M2G_TCPIP, &struHead, &struRepeater, pstruXml);
	            	 	SaveToGprsQueue(pstruXml);
	            	 }
	            	 else
	            	 {
			         	QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
			         	
			         	//��̬�����ط����
			         	//DistServerTelNum(pstruXml);
			         	SaveToMsgQueue_Tmp(pstruXml);
			         }
			         SaveEleQryLog(pstruXml);
			         nTxPackCount++;
			         //if (nTaskId != 11932 && i>= 1) break;
			         	
		         }
		     }
		    else if (struHead.nProtocolType == PROTOCOL_GSM ||  //add by wwj at 2010.07.28
				struHead.nProtocolType == PROTOCOL_CDMA ||
				struHead.nProtocolType == PROTOCOL_HEIBEI ||
				struHead.nProtocolType == PROTOCOL_XC_CP ||
				struHead.nProtocolType == PROTOCOL_SUNWAVE ||
				struHead.nProtocolType == PROTOCOL_WLK)
			{
				//ѡȡ~2GЭ��COMMADOBJECTS
				CURSORSTRU struCursor_CG;
				char szSpoolQryParam[100];
				char szObjects[2000];
				int nCmdCode;
				
				sprintf(szSql, "select CDO_COMMANDCODE, CDO_SPOOLQUERYPARAM, CDO_OBJECTS  from ne_commandobjects where CDO_PROTOCOLTYPE = %d", struHead.nProtocolType);

				PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
				if(SelectTableRecord(szSql, &struCursor_CG) != NORMAL)
				{
					PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
								  szSql, GetSQLErrorMessage());
					FreeCursor(&struCursor_CG);			  
					return EXCEPTION;
				}
			    while(FetchCursor(&struCursor_CG) == NORMAL)
			    {		
			    	strcpy(szSpoolQryParam,  TrimAllSpace(GetTableFieldValue(&struCursor_CG, "CDO_SPOOLQUERYPARAM")));
			    	strcpy(szObjects,  TrimAllSpace(GetTableFieldValue(&struCursor_CG, "CDO_OBJECTS")));				
					nCmdCode = atoi(GetTableFieldValue(&struCursor_CG, "CDO_COMMANDCODE"));
					
					if ((strstr(szSpoolQryParam, "base") != NULL && strstr(szTaskQryParm, "base") != NULL) ||
						(strstr(szSpoolQryParam, "radio") != NULL && strstr(szTaskQryParm, "radio") != NULL) ||
						(strstr(szSpoolQryParam, "alarm") != NULL && strstr(szTaskQryParm, "alarm") != NULL) ||
						(strstr(szSpoolQryParam, "alarmen") != NULL && strstr(szTaskQryParm, "alarmen") != NULL))
					{	
						struHead.nCommandCode = nCmdCode;
						//���ͱ���
						nNowTime = (int)time(NULL);	
						InsertInXmlExt(pstruXml,"<omc>/<��ʱ����ʱ��>", MakeSTimeFromITime(nNowTime), MODE_AUTOGROW|MODE_UNIQUENAME);
				        InsertInXmlExt(pstruXml,"<omc>/<��ض���>",  szObjects, MODE_AUTOGROW|MODE_UNIQUENAME);
				        PrintDebugLog(DBG_HERE, "mark1[%s]\n", szObjects);
						QryElementParam(M2G_SMS, &struHead, &struRepeater, pstruXml);
					    SaveToMsgQueue_Tmp(pstruXml);
					    SaveEleQryLog(pstruXml);
					    nTxPackCount++;
					    if (nTaskId == 4 && i>= 1)	break;
			         	if (nTaskId != 4 && i>= 0) break;
					}
				}
				FreeCursor(&struCursor_CG);	   
				
			}	
		     


        } 
               
        //������Ԫ�����ͱ�����
        if ((nEleCount >= 1000) || (nTxPackCount >= 1000))
        {
            UpdateTaskLogCount(nTaskLogId, nTaskId, nEleCount, nTxPackCount, nFailEleCount);
            nEleCount = 0;
            nTxPackCount = 0;
            nFailEleCount = 0;
            if (getTaskStopUsing(nTaskId) == BOOLTRUE)
                break;
        }
            
            

	}
    if ((nEleCount > 0) || (nTxPackCount > 0) || (nFailEleCount > 0))
	    UpdateTaskLogCount(nTaskLogId, nTaskId, nEleCount, nTxPackCount, nFailEleCount);
	FreeCursor(&struCursor);
    DeleteXml(pstruXml);

    return NORMAL;
	
}



/* 
 * Ӧ�÷��������������� 
 */
RESULT ApplReqWork(INT nSock, PVOID pArg)
{
	STR szCaReqBuffer[MAX_BUFFER_LEN];		/* ��������ͨѶ���� */
	STR	szCaRespBuffer[MAX_BUFFER_LEN];		/* ����Ӧ��ͨѶ���� */
	STR	szPackCd[4+1];						/* ���Ĵ��� */
	INT iRet;
		
	if (SQLPingInterval(60)<0)
    {
    	CloseDatabase();
    	sleep(1);
    	if(OpenDatabase(szService, szDbName, szUser, szPwd) != NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "gprsserv�����ݿⷢ������ [%s]\n",
	                GetSQLErrorMessage());
	        return EXCEPTION;
	    }
	    PrintDebugLog(DBG_HERE, "�������ݿ�ɹ�!\n");
    }
	//PrintDebugLog(DBG_HERE, "Ӧ�÷��������յ��µ���������\n");
	if (RedisPingInterval(60)<0)
    {
    	FreeRedisConn();
    	sleep(1); //10.25
    	if (ConnectRedis() !=NORMAL)
	    {
	        PrintErrorLog(DBG_HERE, "Connect Redis Error Ping After\n");
	        return EXCEPTION;
	    }
    }
	/* 
	 * �������������� 
	 */
	memset(szPackCd,0,sizeof(szPackCd));
	memset(szCaReqBuffer, 0, sizeof(szCaReqBuffer));
	iRet = RecvCaReqPacket(nSock, szCaReqBuffer, szPackCd);
	if (iRet < 0)
	{
		PrintErrorLog(DBG_HERE, "��������������ʧ��\n");
		return EXCEPTION;
	}
	
	PrintDebugLog(DBG_HERE, "[%s]��������������[%s]\n", szPackCd, szCaReqBuffer);	
	/*
	 * gprsrecv�����ѯ�����ã������ز�ѯ������GPRS����
	 */
	if (strcmp(szPackCd, GPRSREQTRANS) == 0)
	{
	    ProcessGprsQrySetTrans(nSock, szCaReqBuffer);
	}
	else
	{
	    //��ʼ��Ӧ����
	    memset(szCaRespBuffer, 0, sizeof(szCaRespBuffer));
	    strcpy(szCaRespBuffer, "0000");
	
	    /* 
	     * ��������Ӧ���� 
	     */
	    
        
        /* 
	     * ���������� 
	     */
	    if (strcmp(szPackCd, BS_QUERYSETTRANS) == 0)
	    {
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
	    	//����BSƽ̨���͵Ĳ�ѯ���ý���
	    	ProcessQuerySetTrans(szCaReqBuffer);
	    	
	    }
	    else if (strcmp(szPackCd, DELIVERTRANS) == 0)
	    {
	    	//�����ϱ�����
	    	ProcessSmsDeliverTrans(szCaReqBuffer);
	    	
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
	    	while(TRUE)
			{
		    	memset(szCaReqBuffer, 0, sizeof(szCaReqBuffer));
		    	iRet = RecvCa8801ReqPacket(nSock, szCaReqBuffer, szPackCd);
			    if (iRet <= 0)
			    {
			    	return EXCEPTION;
			    }
			    
				//PrintDebugLog(DBG_HERE, "[%s]��������������[%s]\n", szPackCd, szCaReqBuffer);
				
			    ProcessSmsDeliverTrans(szCaReqBuffer);
			    
			    if(SendCa8801RespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
			    {
			    	PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
			    	return EXCEPTION;
			    }
			}
	    }
	    else if (strcmp(szPackCd, SMSSTATSREPORT) == 0)
	    {
	    	//����״̬����
	    	ProcessSmsStatusReport(szCaReqBuffer);
	    	
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
	    }
	    else if (strcmp(szPackCd, BS_TRUNTASK) == 0)
	    {
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
	    	//������ѵ����
	    	ProcessTurnTask(szCaReqBuffer);
	    }
	    else if (strcmp(szPackCd, TIMERETRUNTASK) == 0)
	    {
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
	    	//����ʱ��ѯ����
	    	ProcessTimeReTurnTask(szCaReqBuffer);
	    }
	    else if (strcmp(szPackCd, GPRSOFFLINE) == 0)
	    {
	    	//�����ѻ�����
	    	ProcessGprsOffLine(szCaReqBuffer);
	    	
	    	if(SendCaRespPacket(nSock, szCaRespBuffer, strlen(szCaRespBuffer)) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "��������Ӧ���Ĵ���!\n");
	    		return EXCEPTION;
	    	}
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

	if(argc != 2)
	{	
		Usage(argv[0]);
		return NORMAL;
	}
	
	if(strcmp(argv[1], "stop") == 0)
	{
		StopPrg(argv[0]);
		return NORMAL;
	}
	else if(strcmp(argv[1], "-h") == 0)
	{
		Usage(argv[0]);
		return NORMAL;
	}
	//fprintf(stderr, "\t��ӭʹ������ϵͳ(Ӧ�÷�����)\n");
	fflush(stderr);

	if(TestPrgStat(argv[0]) == NORMAL)
	{
		fprintf(stderr, "%s�Ѿ������������ڷ���.��رջ������Ժ�������\n", \
			argv[0]);
		return EXCEPTION;
	}
	
	if(DaemonStart() == EXCEPTION)
	{
		PrintErrorLog(DBG_HERE,"���������� DaemonStart �����ػ�״̬����!\n");
		CloseDatabase();		
		return EXCEPTION;
	}
	
	if(CreateIdFile(argv[0]) == EXCEPTION)
	{
		PrintErrorLog(DBG_HERE,"���������� CreateIdFile ����!\n");
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
	//fprintf(stderr, "here1\n");
	if(OpenDatabase(szService, szDbName, szUser, szPwd) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "�����ݿ���� [%s]!\n", \
			GetSQLErrorMessage());
		return EXCEPTION;
	}
	//fprintf(stderr, "here2\n");
    InitMapObjectCache();
    //fprintf(stderr, "here3\n");
    InitMapObjectCache_CG();
    //fprintf(stderr, "here4\n");
    InitMapObjectCache_SNMP();
    //RecordAgentRecNum();
    //fprintf(stderr, "here5\n");
	CloseDatabase();
	if (getenv("WUXIAN")!=NULL)
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
		
	STR szArgName[100], szTemp[100];
	INT i;
	memset(&struPoolSvr,0,sizeof(struPoolSvr));
	for(i=0;i<10;i++)
	{
		sprintf(szArgName,"ListenPort%d",i+1);
		if (GetCfgItem("applserv.cfg","APPLSERV", szArgName,szTemp) != NORMAL)
		{
			if(i==0)
			{
				PrintErrorLog(DBG_HERE,"���ü����˿ڴ���\n");
				return EXCEPTION;
			}
			break;
		}
		struPoolSvr.nListenPort[i]=atoi(szTemp);
	}
	if(i<MAX_LISTEN_PORT)
		struPoolSvr.nListenPort[i]=-1;
	/*
	//����澯�ϱ��ȶ˿�
	struPoolSvr.nListenPort[0]=8801;
	//�����ѯ���õȶ˿�
	struPoolSvr.nListenPort[1]=8802;
	//gprs�ϱ��˿�
	struPoolSvr.nListenPort[2]=8803;
	struPoolSvr.nListenPort[3]=-1;
	*/
	if (GetCfgItem("applserv.cfg","APPLSERV","MinProcesses",szTemp) != NORMAL)
        return EXCEPTION;
	SetTcpPoolMinNum(atoi(szTemp));
	if (GetCfgItem("applserv.cfg","APPLSERV","MaxProcesses",szTemp) != NORMAL)
        return EXCEPTION;
	SetTcpPoolMaxNum(atoi(szTemp));
	
	fprintf(stderr,"���!\n������ʼ����\n");
	fflush(stderr);
	    	
    struPoolSvr.funcPoolStart = StartPoolChildPro;
    struPoolSvr.pPoolStartArg = NULL;
    struPoolSvr.funcPoolWork = ApplReqWork;
    struPoolSvr.pPoolWorkArg = NULL;
    struPoolSvr.funcPoolEnd = EndPoolChildPro;
    struPoolSvr.pPoolEndArg = NULL;
    struPoolSvr.funcPoolFinish = NULL;
    struPoolSvr.pPoolFinishArg = NULL;
	
	StartTcpPool(&struPoolSvr);

	return NORMAL;
}
