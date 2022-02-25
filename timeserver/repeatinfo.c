/***
 * ����: ����ϵͳ�澯��ʱ����������
 * ��ʱ����ÿ��ִ��һ��
 * �޸ļ�¼:
 * ��־�� 2008-11-8 ����
 */

#include <ebdgdl.h>
#include "omcpublic.h"

static	STR szServiceName[MAX_STR_LEN];
static	STR szDbName[MAX_STR_LEN];
static	STR szUser[MAX_STR_LEN];
static	STR szPwd[MAX_STR_LEN];


static VOID Usage(PSTR pszProgName)
{
	fprintf(stderr,"����ϵͳ(ÿ��վ�㵼������)\n"
		"%s start ��������\n"
		"%s stop  �رճ���\n"
		"%s -h    ��ʾ������Ϣ\n",
		pszProgName,pszProgName,pszProgName);
}

static int ProcessRepeatInfo()
{
	FILE *fp;
	char szFileName[FILENAME_MAX];
	char szBuffer[MAX_BUFFER_LEN];
	STR szSql[MAX_SQL_LEN];
	CURSORSTRU struCursor;

	memset(szFileName,0,sizeof(szFileName));
	snprintf(szFileName,FILENAME_MAX,"%s/data/repeater_%s.txt",getenv("HOME"), GetSystemDate());
	if((fp = fopen(szFileName,"w")) == NULL)
	{
		fprintf(stderr,"Open %s Error:%s\n\a",szFileName,strerror(errno));
		return(-1);
	}
	sprintf(szSql,"select *  from v_repeaterinfo ");
	//PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
	   	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
	    return EXCEPTION;
	}
	while (FetchCursor(&struCursor) == NORMAL)
	{
		memset(szBuffer, 0, sizeof(szBuffer));
		snprintf(szBuffer, sizeof(szBuffer), \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|" \
				"%s|%s|%s|%s|%s|%s|%s|%s|\n", \
				GetTableFieldValue(&struCursor, "ne_areaname"),
				GetTableFieldValue(&struCursor, "ne_countyname"),
				GetTableFieldValue(&struCursor, "NE_PROJECTNAME"),
				GetTableFieldValue(&struCursor, "ne_projecttype"),
				GetTableFieldValue(&struCursor, "NE_LON"),
				GetTableFieldValue(&struCursor, "NE_LON"),
				GetTableFieldValue(&struCursor, "NE_COVERGOALS"),
				GetTableFieldValue(&struCursor, "NE_WATCHSTATUS"),
				GetTableFieldValue(&struCursor, "NE_BACKUPPOWERSTATUS"),
				GetTableFieldValue(&struCursor, "NE_RIVALOVERLAY"),
				GetTableFieldValue(&struCursor, "NE_CONSTRUCTINGATTRIBUTE"),
				GetTableFieldValue(&struCursor, "NE_INVEST"),
				GetTableFieldValue(&struCursor, "NE_NAME"),
				GetTableFieldValue(&struCursor, "Ne_repeaterid"),
				GetTableFieldValue(&struCursor, "NE_DEVICEID"),
				GetTableFieldValue(&struCursor, "NE_devicetypename"),
				GetTableFieldValue(&struCursor, "NE_protocolname"),
				GetTableFieldValue(&struCursor, "NE_deviceproperty"),
				GetTableFieldValue(&struCursor, "NE_nettypename"),
				GetTableFieldValue(&struCursor, "NE_NETELNUM"),
				GetTableFieldValue(&struCursor, "NE_INSTALLPLACE"),
				GetTableFieldValue(&struCursor, "NE_COVEREDREGION"),
				GetTableFieldValue(&struCursor, "NE_DEVICEMODELID"),
				GetTableFieldValue(&struCursor, "NE_ISINDEPENDENTSOURCE"),
				GetTableFieldValue(&struCursor, "bs_cellname"),
				GetTableFieldValue(&struCursor, "NE_BASESTATIONID"),
				GetTableFieldValue(&struCursor, "NE_POWERTYPE"),
				GetTableFieldValue(&struCursor, "slv_Name"),
				GetTableFieldValue(&struCursor, "ds_Name"),
				GetTableFieldValue(&struCursor, "NE_OWNERNAME"),
				GetTableFieldValue(&struCursor, "NE_OWNERTEL"),
				GetTableFieldValue(&struCursor, "NE_companyname"),
				GetTableFieldValue(&struCursor, "NE_INTEGRATEDCOMP"),
				GetTableFieldValue(&struCursor, "NE_AGENTFACTORY"),
				GetTableFieldValue(&struCursor, "NE_OPENDATE"),
				GetTableFieldValue(&struCursor, "NE_FIRSTCHECK"),
				GetTableFieldValue(&struCursor, "NE_ENDCHECK"),
				GetTableFieldValue(&struCursor, "NE_FREETIME"),
				GetTableFieldValue(&struCursor, "NE_ISBACKUPPOWER"),
				GetTableFieldValue(&struCursor, "NE_BACKUPPOWERCOUNT"),
				GetTableFieldValue(&struCursor, "NE_ASSETNUMBER"),
				GetTableFieldValue(&struCursor, "NE_ANTTYPE"),
				GetTableFieldValue(&struCursor, "NE_ANTCOUNT"),
				GetTableFieldValue(&struCursor, "NE_ANTTYPE2"),
				GetTableFieldValue(&struCursor, "NE_ANTCOUNT2"),
				GetTableFieldValue(&struCursor, "NE_ANTTYPE3"),
				GetTableFieldValue(&struCursor, "NE_ANTCOUNT3"),
				GetTableFieldValue(&struCursor, "NE_TOWER"),
				GetTableFieldValue(&struCursor, "NE_TOWERHIGHT"),
				GetTableFieldValue(&struCursor, "NE_ANTHIGHT"),
				GetTableFieldValue(&struCursor, "NE_TOWERUSE"),
				GetTableFieldValue(&struCursor, "NE_MEMO"),
				GetTableFieldValue(&struCursor, "ctp_Name"),
				GetTableFieldValue(&struCursor, "NE_SERVERTELNUM"),
				GetTableFieldValue(&struCursor, "NE_LASTUPDATETIME"),
				GetTableFieldValue(&struCursor, "NE_DEVICEIP"),
				GetTableFieldValue(&struCursor, "NE_DEVICEPORT"),
				GetTableFieldValue(&struCursor, "NE_OMCIP"),
				GetTableFieldValue(&struCursor, "NE_OMCPORT"),
				GetTableFieldValue(&struCursor, "io_Name"),
				GetTableFieldValue(&struCursor, "NE_INSTALLATIONDATE"),
				GetTableFieldValue(&struCursor, "NE_CHANGEDDATE"),
				GetTableFieldValue(&struCursor, "NE_TRANSMITPOWER"),
				GetTableFieldValue(&struCursor, "NE_UPNODEDISTANCE"),
				GetTableFieldValue(&struCursor, "NE_LOGICNEARENDID"),
				GetTableFieldValue(&struCursor, "NE_LOGICFARENDID"),
				GetTableFieldValue(&struCursor, "res_name"),
				GetTableFieldValue(&struCursor, "NE_CURPROJECTNAME")
				);
		/* д���ļ� */
		fwrite(szBuffer, sizeof(szBuffer), 1, fp);
		
	} 
	fclose(fp); 
	return(0); 
}

/*
 * ������
 */
RESULT main(INT argc,PSTR argv[])
{
	STR szTemp[MAX_STR_LEN];
	
	fprintf(stderr,"\t��ӭʹ������ϵͳ(ÿ�ո澯��ʱ����)\n");

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
	
	ProcessRepeatInfo();
	
	CloseDatabase();
	
	fprintf(stderr,"���н���[%s]\n", GetSysDateTime());
	
	return NORMAL;
}


