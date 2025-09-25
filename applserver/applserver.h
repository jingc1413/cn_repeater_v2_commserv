/*
 * ����: Ӧ�÷�����ͷ�ļ�
 *
 */

#ifndef __APPLSERVER_H__
#define __APPLSERVER_H__

#include <cgprotcl.h>
#include <mobile2g.h>

typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char	uchar;
typedef char	int8;				/* Signed integer >= 8	bits */
typedef short	int16;				/* Signed integer >= 16 bits */
typedef unsigned char	uint8;		/* Short for unsigned integer >= 8  bits */
typedef unsigned short	uint16;		/* Short for unsigned integer >= 16 bits */
typedef int		int32;
typedef unsigned int	uint32;		/* Short for unsigned integer >= 32 bits */
typedef unsigned long ulong;

STR	szGprsRespBuffer[100];		/* gprsӦ��ͨѶ���� */
				
#define MAX_MAPOBJECT_NUM			1200*10
#define MAX_MAPCGOBJECT_NUM			1200*7
#define MAX_MAPDASOBJECT_NUM		1200*10
#define MAX_MAPSNMPOBJECT_NUM		1200*4

/*
 * ���ƺ궨��
 */
#define MAX_PKGFIELD_NUM			200					/* ��������ֶ��� */
#define MAX_FIELDVALUE_LEN			256					/* �����ֶ�ֵ��󳤶� */

/*
 * ���ױ��Ĵ��붨��
 */
#define BS_QUERYSETTRANS		"6000"				    /* bs��ѯ���ý��� */
#define BS_SETTRANS		        "6001"					/* bs���ý��� */
#define BS_TRUNTASK		        "6002"					/* ��ѵ���� */
#define TIMERETRUNTASK          "6003"                                      /* ��ѵ���� */
#define DELIVERTRANS			"7000"					/* �����ϱ����� */
#define SMSSTATSREPORT			"7001"					/* ״̬���潻�� */
#define GPRSDELIVE			    "8000"					/* GPRS�ϱ����� */
#define GPRSREQTRANS			"8001"					/* GPRS��ѯ���������� */
#define GPRSRESQQRYSET			"8002"					/* GPRS��ѯ����Ӧ���� */
#define GPRSOFFLINE				"8003"					/* GPRS�ѻ����� */


//�����ض��󳤶�
#define MAX_OBJCONTEXT_LEN  4000
#define MAX_OBJECT_NUM		1200

/*
 * �澯���붨��
 */
#define ALM_DT_ID  		159     //��ͨ�澯id��
#define ALM_DH_ID		160     //�����澯id��
#define ALM_QHPF_ID		161     //cqt�л�Ƶ���澯id��
#define ALM_FTPCX_ID	165     //FTP����ʱС����ѡƵ���澯
#define ALM_DLSB_ID		166	    //GPRS��¼ʧ��
#define ALM_SJSB_ID		167	    //GPRS�����ϱ�ʧ��


//����ͨ�ŷ�ʽ

//Ӧ���־���붨��
#define M2G_SUCCESS			 0x00			//�ɹ�
#define M2G_FEW_SUCCESS		 0x01		//���������ִ��
#define M2G_COMMANDID_ERR		 0x02			//�����Ŵ�
#define M2G_LEN_ERR			 0x03			//��ض��󳤶ȴ�
#define M2G_CRC_ERR			 0x04			//CRCУ���
#define M2G_OTHER_ERR			 0xFE			//��������
#define M2G_COMMAND			 0xFF			//����
#define M2G_ENCODE_ERR		 0xCC			//�������
#define M2G_DECODE_ERR		 0xCD			//�������

#define M2G_MONITOROBJLEN_SIZE	 0x01			//��ض����ų���
#define M2G_MONITOROBJLEN_SIZE2	 0x02			//��ض����ų���
#define M2G_MONITOROBJID_SIZE		 0x02			//��ض����ų���

/////////////////////////////////////////////////////////////
///�����ʾ//////////////////////////////////////////////////

//�����ϱ����Ĺ���ʱXML BUG
#define MAX_CONTENT_LEN  255
//��ʱ�ϱ�������Ԫ�澯���󴮹�������neid BUG
#define MAX_ALARMOBJ_LEN  4000

#pragma pack(1)
//Э��ͷ
typedef struct _COMMANDHEAD
{
    int  nCommandLen;          //Э�鳤��
	int  nProtocolType;        //Э������ 2G 3G GSM CDMA �������� 
	int  nCommandCode;	       //�����������������
    char QA[28];			   //��ˮ��
	int  nObjectCount;         //������
	char ProtocolEdition[4];   //Э��汾�� 
	int  nRiority;             //�������ȼ� 0=������ͨ���� 1=������ѵ����	
}COMMANDHEAD;


typedef struct _REPEATER_INFO//��Ϣ�ṹ
{
    int nCommType;			 //ͨѶ��ʽ
    unsigned int nRepeaterId; //ֱ��վ���
	int nDeviceId;           //�豸���
    char szTelephoneNum[30]; //�绰����
	char szIP[30];			 //IP
	int  nPort;				 //�˿ں�
    char szNetCenter[30];    //�������

	int  nDeviceTypeId;         //�豸����
	int  nProtocolDeviceType; //Э���豸����
	int  nConnStatus;  //����״̬
	char szRouteAddr[20];
	char szAddrInfo[51];
	
	char szAlarmObjList[MAX_ALARMOBJ_LEN];   //�澯����
	char szAlarmEnableList[2048];   //�澯ʹ�ܶ���
    char szSpecialCode[51];
	char szReserve[51];       //����
}REPEATER_INFO;

typedef struct _MAPOBJECT
{
    int	 nMapLen;		//��ض��󳤶�
    char cErrorId;       //����id��
	char szMapId[8+1];		//��ض��� ��Ӧ���ID
	char szMapType[20];		//����
	char szMapData[MAX_OBJCONTEXT_LEN];	//��ض�������
}MAPOBJECT;

//���Ͱ��ṹ
typedef struct _SENDPACKAGE
{
	COMMANDHEAD struHead;
    REPEATER_INFO struRepeater;
    int nNeId;
    int nAreaId;
    int nTaskId;
    int nTaskLogId;
    int nMaintainLogId;
    int nMsgLogId;
    char szRecvMsg[200];
	MAPOBJECT struMapObjList[MAX_OBJECT_NUM];
}SENDPACKAGE;


typedef enum _PROTOCOLTYPE  //Э������
{
    PROTOCOL_2G= 1,			//2GЭ������
	PROTOCOL_3G,			//3GЭ������
	PROTOCOL_GSM,			//GSM Э������
	PROTOCOL_CDMA,			//CDMA Э������
	PROTOCOL_CONTROL,		//��������Э������   

	PROTOCOL_HEIBEI,
	PROTOCOL_XC_CP ,
	PROTOCOL_YINY_CP ,
	PROTOCOL_ZJYDGSM,
	PROTOCOL_XINLITONG,
	
	PROTOCOL_SYD ,          //��Ԫ��

	PROTOCOL_SUNWAVE,       //��ά
	PROTOCOL_WLK,           //������
	PROTOCOL_WUYOU,         //���� 14
	//add by qgl at 2006-10-26
	PROTOCOL_JINXIN_R9110AC_II2,	//15
	PROTOCOL_JINXIN_RA1000A_LDII2,	//16
	PROTOCOL_JINXIN_1000AW_LDII2,   //17
	PROTOCOL_JINXIN_1000A_LW,        //18
	PROTOCOL_JINXIN_R9122AC,		//19
	PROTOCOL_JINXIN_R9122AC_II2,	//20
	PROTOCOL_JINXIN_BPA9010,		//21
	PROTOCOL_JINXIN_R9110AC,		//22
	PROTOCOL_JINXIN_R9110AS,		//23
	PROTOCOL_JINXIN_S9180,			//24
	PROTOCOL_JINXIN_TPA9010,		//25
	PROTOCOL_JINXIN_TPA9020A,		//26	//�޸�TPA9020�ֳ�A\B��Э��
	PROTOCOL_JINXIN_TPA9020B,		//27   //modify by qgl at 2006-11-06
	PROTOCOL_JINXIN_RS2110BC1C2,//28 RS-2110B-C1C2,���ڿ��ֱ��վ modify by cqf at 2007-12-29 for ����
    PROTOCOL_JINXIN_M4000BC1C2,//29 M-4000B-C1C2,GSM���߷Ŵ��� modify by cqf at 2008-1-8 for ���� 

	PROTOCOL_AOWEI=40,				//��ά  add by qgl at 2006-09-13
	PROTOCOL_HEIBEI2=41,				//�ӱ�Э��֧��8λ���� add by qgl at 2006-09-18
	PROTOCOL_WUYOU1=42,				//����1.0Э��,֧��8λ���� add by qgl at 2006-10-17	
	PROTOCOL_XiNZHONG=43,            //����Э��,����άЭ����ͬ add by cqf at 2008-01-03
	PROTOCOL_FUYOU=44,                //���� modify by cqf at 2008-1-8 for ����
	PROTOCOL_SNMP=45,
	PROTOCOL_DAS=46,
	PROTOCOL_JINXIN_DAS=47 
}PROTOCOLTYPE; 

typedef enum _PROTOCOL_2G_COMMAND //2G�������
{	
    COMMAND_QUERY=1,              //��ѯ
	COMMAND_SET=2,                //����
	COMMAND_UP = 3,			      //�ϱ�
	COMMAND_QUERY_MAPLIST = 9,    //��ѯ������б�
	COMMAND_FCTPRM_QRY = 0x11,     //ԭ����������ѯ 178  ��Ϊ �л��������汾 0x11 2014/1/8 modify
	COMMAND_FACTORY_MODE = 176,   //���빤��ģʽ
	COMMAND_FCTPRM_SET = 179,     //������������   add  at 2008-12-15 for factory mode
    COMMAND_PRJPRM_QRY = 180,     //���̲�����ѯ   add  at 2008-12-15 for factory mode
	COMMAND_PRJPRM_SET = 181,      //���̲�������   add  at 2008-12-15 for factory mode

	COMMAND_QUERY_TEMP = 0xB2,     // ��ѯ�����ʶ add 20250904 ��ʱ���
	//COMMAND_SET_TEMP = 0xB3

}PROTOCOL_2G_COMMAND;

typedef enum _2G_UP_TYPE //2G�ϱ�����
{	
    DEVICE_ALARM=1,              //1:�豸�澯
	OPENSTATION=2,               //2����վ
	PERSONPATROL = 3,			 //3��Ѳ��
	DEVICEREPAIR = 4,           //4���豸�޸�
	DEVICECHANGED = 5,           //5���豸���ñ仯
	DEVICEMONITOR = 0x20,        //Ч�����
	DEVICESTARTUP = 0xCA         //202:Ч����������ϱ�
}UP_TYPE;


typedef enum _REMORT_UPDATE_STATUS //Զ������״̬
{
	UNSENDED_STATUS=0,  //����δ����
	SUCCESS_STATUS=1,   //�����ɹ�
	FAILURE_STATUS=2,   //����ʧ��
	SENDSET_STATUS=3,   //�������·�
    SENDED_STATUS = 5   //�����ѻظ��·��ɹ�
}REMORT_UPDATE_STATUS;


typedef enum _COMMTYPE //ͨѶ��ʽ
{
	M2G_RS232_TYPE=1,	//1
	M2G_DATA_TYPE,		//2
	M2G_SMS_TYPE,		//3
	M2G_TCPIP_TYPE,		//4
	M2G_GPRS_TYPE,		//5
	M2G_UDP_TYPE,		//6
	M2G_SNMP_TYPE		//7
}COMMTYPE;

#pragma pack()


//////////////////////////////////////////////

typedef struct _ALARMPARAMETER
{
    int nNeId;		
	unsigned int nRepeaterId;		
	short nDeviceId;
	char szNeTelNum[20];
	char szAlarmObjId[10];
	char szAlarmTime[20];
	BOOL bIsNewAlarm ;//trueΪ�����澯��false�澯ȡ��
	int nAlarmLogId;
	char szAlarmId[10];
	char szAlarmLevelId[10];
	char szAlarmName[50];  //�澯��
	char szAlarmLevelName[50];
	char szNeName[50];  //��Ԫ����
	char szAlarmObjList[4000];  //�澯�����б�
	char szAlarmInfo[1000];    //�澯��Ϣ

	int nAlarmAutoConfirmSuccess;  //�澯�Զ�ȷ��״̬ 1���ɹ�
	int nAlarmAutoClearSuccess ;  // �澯�Զ����״̬ 1���ɹ�
}ALARMPARAMETER;


typedef struct
{
	STR szMapId[9];
	STR szDataType[10];
	STR szObjType[20];
	STR szObjOid[50];
	int nDataLen;
}MAPOBJECTSTRU;

typedef struct
{
	STR szMapId[5];
	STR szDataType[10];
	int nDataLen;
	int nProtclType;
}MAPOBJECTSTRU_CG;

typedef struct
{
	STR szMapId[5];
	STR szDataType[10];
	STR szObjType[20];
	STR szObjOid[50];
	int nDataLen;
	int nDeviceType;
}MAPOBJECTSTRU_SNMP;

typedef struct
{
	STR szMapId[9];
	STR szDataType[10];
	STR szObjType[20];
	STR szObjOid[50];
	int nDataLen;
}MAPOBJECTSTRU_DAS;

typedef struct
{
	PSTR pszName;
	UINT nLen;
}DIVCFGSTRU;
typedef DIVCFGSTRU * PDIVCFGSTRU;

typedef struct
{
	PSTR pszName;
	PSTR pszType;
	UINT nLen;
}FIXLENCFGSTRU;
typedef FIXLENCFGSTRU * PFIXLENCFGSTRU;



/*
 * ��������
 */
//appl_process
int RecvCaReqPacket(int nSock, char *pszCaReqBuffer, char *TradeNo);
int RecvCa8801ReqPacket(int sockfd, char *psCaReqBuffer, char *psTradeNo);
int SendCaRespPacket(int nSock, char *pszCaRespBuffer, int nCommBufferLen);
int SendCa8801RespPacket(int nSock, char *pszCaRespBuffer, int nCommBufferLen);
RESULT DecodeAndProcessSms(PSTR pszUndecode,PSTR pszTelephone,PSTR pszNetCenterNum);
RESULT DecodeAndProcessGprs(PSTR pszUndecode, INT nLen);

//appl_qryandset
RESULT DecodeQryElementParam(SENDPACKAGE *pstruSendPackage);
RESULT DecodeQryOnTime(SENDPACKAGE *pstruSendPackage, BOOL bGprsTime, BOOL bBatchQuery);
RESULT DecodeSetElementParam(SENDPACKAGE *pstruSendPackage);
RESULT DecodeQueryMapList(SENDPACKAGE *pstruSendPackage);
RESULT QryElementParam(INT nCommType, COMMANDHEAD *pstruHead, REPEATER_INFO *pstruRepeater, PXMLSTRU pstruXml);
RESULT SetElementParam(INT nCommType, COMMANDHEAD *pstruHead, REPEATER_INFO *pstruRepeater, PXMLSTRU pstruXml);
RESULT QueryMapList(INT nCommType, COMMANDHEAD *pstruHead, REPEATER_INFO *pstruRepeater, PXMLSTRU pstruXml);
RESULT QueryAlarmObjectMapList(INT nCommType, COMMANDHEAD *pstruHead, REPEATER_INFO *pstruRepeater, PXMLSTRU pstruXml);
RESULT SaveSysErrorLog(INT nNeId, PSTR pszErrMapId,PSTR pszErrorId, PSTR pszRecvMsg);
RESULT UpdateEleFromCommByPacket(PSTR pszQryNumber, PSTR pszContent, PSTR pszAlarmName, PSTR pszAlarmValue);
RESULT UpdateEleSetLogFromComm(SENDPACKAGE *pstruSendPackage, PSTR pszQryNumber, PSTR pszProperty, PSTR pszContent);
RESULT UpdateEleFromCommBySetPacket(PSTR pszQryNumber, PSTR pszProperty);
RESULT UpdateEleQryLogFromComm(SENDPACKAGE *pstruSendPackage, PSTR pszProperty, PSTR pszContent);
RESULT UpdateEleQryLogOnTime(SENDPACKAGE *pstruSendPackage, PSTR pszProperty, PSTR pszContent);
RESULT UpdateEleObjList(SENDPACKAGE *pstruSendPackage,  PSTR pszObjectList, PSTR pszProvinceId, INT nUpdateWay);
RESULT QryEleFristTime(int nNeId, int nCommType);
RESULT SaveEleSetLog(PXMLSTRU pstruReqXml);
RESULT InsertFailNeid(int nTaskLogId, int nNeId, PSTR pszReason, PSTR pszEleState);
PSTR GetDeviceStatu(int nDeviceStatus);
RESULT UpdateTaskLogCount(int nTaskLogId, int nTaskId, int nEleCount, int nTxPackCount, int nFailEleCount);
BOOL getTaskStopUsing(int nTaskId);
BOOL ExistNeId(unsigned int nRepeaterId, int nDeviceId);
RESULT InsertSpecialCommandLog(PXMLSTRU pstruReqXml);
RESULT RemortUpdateDbOperate1(PXMLSTRU pstruReqXml);
RESULT RemortUpdateDbOperate2(PSTR pszQA);
RESULT RemortUpdateDbOperate3(PXMLSTRU pstruReqXml);
RESULT SetAllElementParam(int nNeId);
RESULT UpdateEleLastTime(int nNeId);

//appl_gprs
RESULT DecodeGprsPesq(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeGprsCqt(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);

RESULT DecodeFtpDownLoad(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeFtpUpLoad(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeGprsPing(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeGprsPdp(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeGprsAttach(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeMms(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeDayTestResult(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeWap(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT DecodeVp(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveGprsPesqLog(PXMLSTRU pstruReqXml);
RESULT SaveGprsCqtLog(PXMLSTRU pstruReqXml);
RESULT SaveFtpDownLoadLog(PXMLSTRU pstruReqXml);
RESULT SaveFtpUpLoadLog(PXMLSTRU pstruReqXml);
RESULT SaveGprsPingLog(PXMLSTRU pstruReqXml);
RESULT SaveGprsPdpLog(PXMLSTRU pstruReqXml);
RESULT SaveGprsAttachLog(PXMLSTRU pstruReqXml);
RESULT SaveGprsTestLog(PXMLSTRU pstruReqXml);
RESULT SaveMmsTestLog(PXMLSTRU pstruReqXml);
RESULT SaveDayTestResultLog(PXMLSTRU pstruReqXml);
RESULT SaveTimerUploadLog(INT nNeId, PXMLSTRU pstruReqXml);
RESULT SaveWapTestLog(PXMLSTRU pstruReqXml);
RESULT SaveVpTestLog(PXMLSTRU pstruReqXml);

RESULT ImSaveFtpDownLoadLog(PXMLSTRU pstruReqXml);
RESULT ImSaveFtpUpLoadLog(PXMLSTRU pstruReqXml);
RESULT ImSaveGprsAttachLog(PXMLSTRU pstruReqXml);
RESULT ImSaveGprsPdpLog(PXMLSTRU pstruReqXml);
RESULT ImSaveGprsPingLog(PXMLSTRU pstruReqXml);
RESULT ImSaveWapTestLog(PXMLSTRU pstruReqXml);

RESULT InitBatPick(PXMLSTRU pstruXml);// ����
RESULT SaveBatPickLog(PXMLSTRU pstruXml);// ����
int CheckBatPickTmp(PXMLSTRU pstruXml);// ����
RESULT SaveBatPickDat(PXMLSTRU pstruXml, char *, int);// ����
RESULT UpdateBatPickTmp(PXMLSTRU pstruXml);// ����
RESULT DeleteBatPickTmp(PXMLSTRU pstruXml);// ����

RESULT CheckShiXi(PXMLSTRU pstruXml);
RESULT SaveShiXiLog(PXMLSTRU pstruXml);
int CheckShiXiPack(PXMLSTRU pstruXml);
RESULT SaveShiXiDat(PXMLSTRU pstruXml, char *pszValue, int nLen);
RESULT DeleteShiXiTmp(PXMLSTRU pstruXml);

//appl_td
RESULT DecodeTdPesq(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdPesqLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdCqt(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdCqtLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdDayTestResult(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdDayTestResultLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdMms(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdMmsTestLog(PXMLSTRU pstruReqXml);

RESULT DecodeTdFtpDownLoad(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdFtpDownLoadLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdFtpUpLoad(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdFtpUpLoadLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdPing(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdPingTestLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdPdp(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdPdpTestLog(PXMLSTRU pstruReqXml);
RESULT DecodeTdAttach(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdAttachTestLog(PXMLSTRU pstruReqXml);

RESULT DecodeTdWap(PSTR pszMapData, INT nDataLen, PXMLSTRU pstruXml);
RESULT SaveTdWapTestLog(PXMLSTRU pstruReqXml);

RESULT SaveTdPsTestLog(PXMLSTRU pstruReqXml);
RESULT SaveTdTimerUploadLog(INT nNeId, PXMLSTRU pstruReqXml);


//appl_alarm
RESULT InitAlarmPara(int nNeId);
RESULT DealNewAlarm(PXMLSTRU pstruXml);
RESULT AlarmComeback(PXMLSTRU pstruXml);
RESULT AlarmFrequent(int nMaintainLogId, PSTR pszCondition);
RESULT TransferAlarm();
/*
 * appl_util
 */
PSTR TrimRightOneChar(PSTR pszInputString,CHAR cChar);
int ReplaceAlarmObjStr(char *sSrc, char *sMatchStr, char *sReplaceStr);
int ReplaceStr(char *sSrc, char *sMatchStr, char *sReplaceStr);
char *ReplaceCharByPos(char *str, char rchar, int pos);
time_t MakeITimeFromLastTime(PSTR pszLastTime);
int GetCurrent2GSquenue();
PSTR GetTypeNumber(PSTR pszType);
PSTR Get2GNumber(PSTR pszType, INT n2G_QB);
RESULT GetDbSerial(PUINT pnSerial,PCSTR pszItemName);
RESULT GetDbSequence(PUINT pnSerial,PCSTR pszItemName);
RESULT GetMySqlSequence(PUINT pnSerial,PCSTR pszTableName);
int GetProtocolFrom(PSTR pszMobile);
PSTR GetAlarmName(PSTR pszMapId);
RESULT GetSendPackageInfo(int QB, unsigned int RptId, SENDPACKAGE *pstruSendInfo);
RESULT SaveToMsgQueue(PXMLSTRU pstruReqXml);
RESULT SaveToMsgQueue_Tmp(PXMLSTRU pstruReqXml);
RESULT SaveToGprsQueue(PXMLSTRU pstruReqXml);
RESULT SaveEleQryLog(PXMLSTRU pstruReqXml);
RESULT SaveToMaintainLog(PSTR pszStyle, PSTR pszMemo, SENDPACKAGE *pstruSendInfo);
RESULT SaveToMaintainLog2(PSTR pszStyle, INT nNeId, COMMANDHEAD *pstruHead, REPEATER_INFO *pstruRepeater);
RESULT GetPackInfoFromMainLog(SENDPACKAGE *pstruSendInfo);
RESULT GetGprsPackageInfo(int QB, unsigned int RptId, SENDPACKAGE *pstruSendInfo);
RESULT GetSysParameter(PCSTR pszArgSql,PSTR pszArgValue);
PSTR GetAlarmObjList(int nNeId);
PSTR GetAlarmEnabledObjList(int nNeId);
RESULT GetAlarmObjList2(unsigned int nRepeaterId, int nDeviceId, PSTR pszNeTelNum, int *pNeId, PSTR pszNeName, PSTR pszAlarmObjList);
RESULT GetAlarmObjList3(unsigned int nRepeaterId, int nDeviceId, int *pNeId, PSTR pszNeName, PSTR pszAlarmObjList);
RESULT GetNeObjectList(INT nNeId, PSTR pszNeObjectList, PSTR pszNeDataList);
RESULT GetNeCmdObjects(int nProtocolTypeId,int nProtocolDeviceTypeId, PSTR pszCmdCode, PSTR pszCmdObject);
RESULT ResolveQryParamArray(PSTR pszQryEleParam);
RESULT ResolveQryParamArrayGprs(PSTR pszQryEleParam);
RESULT ResolveSetParamArray(PSTR pszSetEleParam, PSTR pszSetEleValue);
RESULT ResolveSetParamArrayGprs(PSTR pszSetEleParam, PSTR pszSetEleValue);
RESULT InsertTaskLog(int nTaskId, int *pnTaskLogId, PSTR pszStyle);
RESULT SetTaskUsing(int nTaskId);
INT GetNeId(unsigned int nRepeaterId, int nDeviceId, PSTR pszNeTelNum, BOOL *pIsNewNeId);
RESULT GetNeInfo(SENDPACKAGE *pstruNeInfo);
int strHexToInt(char* strSource);
RESULT InitMapObjectCache(VOID);
RESULT InitMapObjectCache_CG(VOID);
RESULT GetMapIdFromCache(PSTR pszMapId,PSTR pszDataType,PINT pDataLen);
RESULT GetMapIdFromCache2(PSTR pszMapId,PSTR pszDataType,PSTR pszObjType, PINT pDataLen);
RESULT GetMapIdFromCache_CG(int, PSTR pszMapId,PSTR pszDataType,PINT pDataLen);
RESULT DecodeMapDataFromType(PSTR pszDataType,INT nDataLen, PSTR pszOC, PSTR pszMapData);
RESULT DecodeMapDataFromMapId(PSTR pszMapId, PSTR pszOC, PSTR pszMapData, PSTR pszMapType);
RESULT EncodeMapDataFromMapId(PSTR pszMapId, PSTR pszMapData,  PBYTE pszOC);
RESULT SaveToMapList(PSTR pszQrySerial, PSTR pszMapId);
RESULT GetMapId0009List(PSTR pszQrySerial, PSTR pszMapId0009List);
RESULT DistServerTelNum(PXMLSTRU pstruReqXml);
RESULT RecordAgentRecNum();
BOOL getAgentState(PSTR pszAgentNo);
RESULT SaveToRecordDeliveCrc(unsigned int nRepeaterId, INT nDeviceId, INT nNetFlag, INT nType);
RESULT CqtMathchJob(int nNeId, PSTR pszTelNum, PSTR pszCallNum, PSTR pszBeCallNum);
RESULT CqtMathchJob2(int nNeId, PSTR pszTelNum, PSTR pszBeCallNum);
BOOL ExistAlarmLog(int nAlarmId, int nNeId, PINT pnAlarmCount);
RESULT InsertMosTask(PXMLSTRU pstruXml);
RESULT SaveToAlarmLog(int nAlarmId, int nNeId, int nAlarmCount);
RESULT GetDeviceIp(unsigned int nRepeaterId, int nDeviceId, PSTR pszDeviceIp, int *pPort);
BOOL AscEsc(BYTEARRAY *Pack);
BOOL AscUnEsc(BYTEARRAY *Pack);
RESULT ByteSplit(int, BYTEARRAY *Pack, char *pszObjList);
RESULT ByteCombine(int, BYTEARRAY *Pack, char *pszCmdObjList);
int EncodeCmdBodyFromCmdId(int, const char *pszObjList, const OMCOBJECT *pstruOmcObj, int nOmcObjNum,
		UBYTE *pubCmdBody, int nCmdBodyMaxLen, UBYTE *pubCmdBodyLen);
int DecodeCmdBodyFromCmdId(int, char *pszCmdObjList, CMDHEAD *pCmdHead, 
		UBYTE *pubCmdBody, OMCOBJECT *pstruOmcObj, int *pnOmcObjNum);
			
//appl_redis
RESULT InitRedisMQ_cfg();
RESULT ConnectRedis();
RESULT FreeRedisConn();
RESULT GetRedisPackageInfo(int nQB, SENDPACKAGE *pstruSendInfo, PXMLSTRU  pstruXml);
RESULT GetRedisDeviceIpInfo(unsigned int nRepeaterId, int nDeviceId,PSTR pszDeviceIp, int *pPort);
RESULT PublishToMQ(PXMLSTRU  pstruXml, int nReqFlag);
RESULT PublishToRedis(PXMLSTRU  pstruXml, int nReqFlag);
RESULT RedisHsetAndPush(PXMLSTRU  pstruXml);
RESULT HmsetEleParam(PSTR pszHmsetParam);
RESULT PushElementParamQueue(int nTaskLogId, PSTR pszEleParamSQL);
RESULT PushElementSQL(PSTR pszEleSQL);
RESULT PushEffectControl(int nTaskLogId, PSTR pszEffSQL);
int RedisPingInterval(int timeout);
RESULT GetEleRespQueue(PSTR pszRespQueue);
RESULT HmsetPoolTaskLog(int nTaskLogId, int nFailTimes, int nStyle);

#endif
