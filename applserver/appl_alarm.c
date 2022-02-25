/*
 * ����: ����ϵͳӦ�÷�����(����澯)
 *
 * �޸ļ�¼:
 * ��־�� -		2008-10-18 ���� 
 */
 
#include <ebdgdl.h>
#include <applserver.h>

ALARMPARAMETER struAlarmPara;


RESULT InitAlarmPara(int nNeId)
{
    struAlarmPara.nNeId = nNeId;
    return NORMAL;
}

RESULT SaveAlarmLog(int nNeId)
{
    CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	UINT nAlarmLogId;
	//STR szAlarmObjList[1000];
	STR szOldAlarmObjId[20];
	STR szNewAlarmObjId[20];
	PSTR pszTemp;
	INT nAlarmId;
	TBINDVARSTRU struBindVar;
	
	memset(&struBindVar, 0, sizeof(struBindVar));

	struBindVar.struBind[0].nVarType = SQLT_STR;
	strcpy(struBindVar.struBind[0].VarValue.szValueChar, struAlarmPara.szAlarmObjId);
		
	struBindVar.nVarCount++;
		
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select * from ALM_ALARM where alm_ObjId = :v_0 ");
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%s]\n", szSql,  struAlarmPara.szAlarmObjId);
	if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
					  szSql, GetSQLErrorMessage());
        FreeCursor(&struCursor);
		return EXCEPTION;
	}

	if (strcmp(getenv("DATABASE"), "mysql") == 0)
	{
		if(GetMySqlSequence(&nAlarmLogId, "alm_alarmlog")!=NORMAL)
		{
			PrintErrorLog(DBG_HERE,"��ȡ��ѯ������ˮ�Ŵ���\n");
			return EXCEPTION;
		}
	}
	else
	{
		if(GetDbSequence(&nAlarmLogId, "SEQ_ALARMLOG")!=NORMAL)
		{
			PrintErrorLog(DBG_HERE,"��ȡ��ѯ������ˮ�Ŵ���\n");
			FreeCursor(&struCursor);
			return EXCEPTION;
		}
	}
	struAlarmPara.nAlarmLogId = nAlarmLogId;
	
	strcpy(struAlarmPara.szAlarmId ,GetTableFieldValue(&struCursor, "alm_AlarmId"));
	strcpy(struAlarmPara.szAlarmLevelId, GetTableFieldValue(&struCursor, "alm_LevelId"));
	strcpy(struAlarmPara.szAlarmName, GetTableFieldValue(&struCursor, "alm_Name"));
	//strcpy(struAlarmPara.alarmLevelName, GetTableFieldValue(&struCursor, "alm_LevelName"));
    FreeCursor(&struCursor);

    PrintDebugLog(DBG_HERE,"szAlarmObjList=[%s],szAlarmObjId[%s]\n", struAlarmPara.szAlarmObjList, struAlarmPara.szAlarmObjId);
    if ((pszTemp = strstr(struAlarmPara.szAlarmObjList, struAlarmPara.szAlarmObjId)) != NULL)
    {
        memset(szOldAlarmObjId, 0, sizeof(szOldAlarmObjId));
        memcpy(szOldAlarmObjId, pszTemp, strlen(struAlarmPara.szAlarmObjId)+2);
        sprintf(szNewAlarmObjId, "%s:1", struAlarmPara.szAlarmObjId);
        
        memset(&struBindVar, 0, sizeof(struBindVar));
		struBindVar.struBind[0].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[0].VarValue.szValueChar, szOldAlarmObjId);
		struBindVar.nVarCount++;
		
		struBindVar.struBind[1].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[1].VarValue.szValueChar, szNewAlarmObjId);
		struBindVar.nVarCount++;
		
		struBindVar.struBind[2].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[2].VarValue.szValueChar, GetSysDateTime());
		struBindVar.nVarCount++;
		
		struBindVar.struBind[3].nVarType = SQLT_INT;
		struBindVar.struBind[3].VarValue.nValueInt =  nNeId;
		struBindVar.nVarCount++;
		
        sprintf(szSql, "update ne_Element set ne_AlarmObjList=REPLACE(ne_AlarmObjList, :v_0, :v_1),ne_LastUpdateTime =to_date( :v_2,'yyyy-mm-dd hh24:mi:ss'),ne_IsAlarm = 1 where  ne_NeId= :v_3");
        
        //PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%s][%s][%s][%d]\n", szSql, szOldAlarmObjId, szNewAlarmObjId, GetSysDateTime(),  nNeId);
        if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	    {
		    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    return EXCEPTION;
	    }
	    PrintDebugLog(DBG_HERE,"Excute SQL[%s]\n",szSql);
	    CommitTransaction();
	    
    }
    
    snprintf(szSql, sizeof(szSql), "insert into  alm_AlarmLog (alg_AlarmLogId,alg_AlarmId,alg_NeId,alg_AlarmTime,alg_AlarmStatusId,alg_AlarmObjId, alg_AlarmInfo)"
             " VALUES(%d, %s, %d, to_date( '%s','yyyy-mm-dd hh24:mi:ss'), 1, '%s', '%s')", 
             struAlarmPara.nAlarmLogId, struAlarmPara.szAlarmId, struAlarmPara.nNeId, struAlarmPara.szAlarmTime, struAlarmPara.szAlarmObjId, struAlarmPara.szAlarmInfo);
    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
    if(ExecuteSQL(szSql) !=NORMAL) 
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	CommitTransaction();
    
    //���ݻ��ܴ���, �ۼ�1
    memset(szSql, 0, sizeof(szSql));
    nAlarmId = atoi(struAlarmPara.szAlarmId);
    if ((nAlarmId >=1 && nAlarmId <=20) 
    	|| (nAlarmId >=56 && nAlarmId <=71)
    	|| (nAlarmId >=137 && nAlarmId <=139)
    	|| (nAlarmId >=167 && nAlarmId <=170)
    	|| (nAlarmId >=187 && nAlarmId <=189)
    	|| nAlarmId == 22 
    	|| nAlarmId == 30 
    	|| nAlarmId == 31
    	|| nAlarmId == 158
    	|| nAlarmId == 185
    )
    {
    	memset(&struBindVar, 0, sizeof(struBindVar));
		struBindVar.struBind[0].nVarType = SQLT_INT;
		struBindVar.struBind[0].VarValue.nValueInt =  nNeId;
		struBindVar.nVarCount++;
		
		struBindVar.struBind[1].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[1].VarValue.szValueChar, GetSystemDate());
		struBindVar.nVarCount++;

    	snprintf(szSql, sizeof(szSql), "update alm_collect_report set ALARM_%d_TIMES=ALARM_%d_TIMES + 1 where ne_neid = :v_0 and collect_date = to_date(:v_1, 'yyyymmdd')", nAlarmId, nAlarmId);
		PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s][%d][%s]\n", szSql,	 nNeId, GetSystemDate());
		if(BindExecuteSQL(szSql, &struBindVar) != NORMAL)
		{
			PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
				szSql, GetSQLErrorMessage());
    	    return EXCEPTION;
		}
		CommitTransaction();
	}
	
 	
    return NORMAL;  
		
}


RESULT AlarmAutoConfirm(int nNeId)
{
    STR szSql[MAX_SQL_LEN];
    STR szEnable[10];
    STR szCondition[100];
    STR szTemp[10];
	//ȡ�Զ�ȷ������
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmAutoConfirm' and par_KeyName = 'Enabled'");
	GetSysParameter(szSql, szEnable);
	if (strcmp(szEnable, "TRUE") == 0)
	{
		sprintf(szSql, "par_SectionName ='AlarmAutoConfirm' and par_KeyName = 'Condition'");
		GetSysParameter(szSql, szCondition);
		sprintf(szTemp, "%s,", struAlarmPara.szAlarmId);
		if (strstr(szCondition, szTemp) == NULL)
		{
			return NORMAL;
		}
	    sprintf(szSql, "UPDATE alm_AlarmLog SET alg_ConfirmTime = to_date('%s','yyyy-mm-dd hh24:mi:ss'), alg_ConfirmName = '%s', alg_AlarmStatusId = 3,alg_ConfirmWay = 0 WHERE alg_AlarmLogId = %d",
	        GetSysDateTime(), "ϵͳ�Զ�ȷ��", struAlarmPara.nAlarmLogId);
	    if(ExecuteSQL(szSql) !=NORMAL ) 
	    {
		    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    return EXCEPTION;
	    }
	    CommitTransaction();
	    
	}
    return NORMAL;
}


RESULT AlarmAutoClear(int nNeId)
{
    STR szSql[MAX_SQL_LEN];
    STR szEnable[10];
    STR szCondition[100];
    STR szTemp[10];
	//ȡ�Զ��������
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmAutoClear' and par_KeyName = 'Enabled'");
	GetSysParameter(szSql, szEnable);
	if (strcmp(szEnable, "TRUE") == 0)
	{
		sprintf(szSql, "par_SectionName ='AlarmAutoClear' and par_KeyName = 'Condition'");
		GetSysParameter(szSql, szCondition);
		sprintf(szTemp, "%s,", struAlarmPara.szAlarmId);
		if (strstr(szCondition, szTemp) == NULL)
		{
			return NORMAL;
		}
	    sprintf(szSql, "UPDATE alm_AlarmLog SET alg_ClearTime = to_date( '%s','yyyy-mm-dd hh24:mi:ss'), alg_ClearUserName = '%s', alg_AlarmStatusId = 5 WHERE alg_AlarmLogId = %d",
	        GetSysDateTime(), "ϵͳ�Զ����", struAlarmPara.nAlarmLogId);
	    if(ExecuteSQL(szSql) !=NORMAL ) 
	    {
		    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    return EXCEPTION;
	    }
	    CommitTransaction();
	}
    return NORMAL;
}

RESULT AlarmCompress(int nNeId)
{
	CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	STR szEnable[10];
	INT nCount, nAlarmLogId;
	STR szCondition[2000];
	STR szTemp[100];
	TBINDVARSTRU struBindVar;
	
	//ȡ�澯ѹ������
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmCompress' and par_KeyName = 'Enabled'");
	//sprintf(szSql, "par_SectionName ='alarm_general_config' and par_KeyName = 'alarm_compres'");
	GetSysParameter(szSql, szEnable);
	//if (strcmp(szEnable, "true") == 0)
	if (strcmp(szEnable, "TRUE") == 0)
	{
		
		sprintf(szSql, "par_SectionName ='AlarmCompress' and par_KeyName = 'Condition'");
		GetSysParameter(szSql, szCondition);
		sprintf(szTemp, "%s,", struAlarmPara.szAlarmId);
		if (strstr(szCondition, szTemp) == NULL)
		{
			return NORMAL;
		}
		memset(&struBindVar, 0, sizeof(struBindVar));
		struBindVar.struBind[0].nVarType = SQLT_INT;
		struBindVar.struBind[0].VarValue.nValueInt = nNeId;
		struBindVar.nVarCount++;
		
		struBindVar.struBind[1].nVarType = SQLT_INT;
		struBindVar.struBind[1].VarValue.nValueInt = atoi(struAlarmPara.szAlarmId);
		struBindVar.nVarCount++;
		
		memset(szSql, 0, sizeof(szSql));
		sprintf(szSql,"select count(alg_NeId) as almcount from  alm_AlarmLog  where alg_NeId = :v_0 and  alg_AlarmId = :v_1 and alg_AlarmStatusId < 4");
	    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%d][%s]\n", szSql, nNeId, struAlarmPara.szAlarmId);
	    if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    				  szSql, GetSQLErrorMessage());
	    	return EXCEPTION;
	    }
	    if(FetchCursor(&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
	    				  szSql, GetSQLErrorMessage());
	    	FreeCursor(&struCursor);
	    	return EXCEPTION;
	    }
	    nCount = atoi(GetTableFieldValue(&struCursor, "almcount"));
	    FreeCursor(&struCursor);
	    //������2�����ϵļ�¼�Ž���ѹ��
	    if (nCount > 1)
	    {
	    	memset(szSql, 0, sizeof(szSql));
			sprintf(szSql,"select alg_AlarmLogId from  alm_AlarmLog  where alg_NeId = :v_0 and  alg_AlarmId = :v_1 and alg_AlarmStatusId < 4 and alg_compressState = 1");
	    	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql, nNeId, struAlarmPara.szAlarmId);
	    	if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    					  szSql, GetSQLErrorMessage());
	    		return EXCEPTION;
	    	}
	    	if(FetchCursor(&struCursor) == NORMAL)
	    	{
	    		nAlarmLogId = atoi(GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
	    		FreeCursor(&struCursor);
	    		
	    		memset(&struBindVar, 0, sizeof(struBindVar));
				struBindVar.struBind[0].nVarType = SQLT_INT;
				struBindVar.struBind[0].VarValue.nValueInt = nCount;
				struBindVar.nVarCount++;
				
				struBindVar.struBind[1].nVarType = SQLT_INT;
				struBindVar.struBind[1].VarValue.nValueInt = nAlarmLogId;
				struBindVar.nVarCount++;
				
				sprintf(szSql, "update alm_AlarmLog set alg_compressCount = :v_0 where alg_AlarmLogId = :v_1");
	        	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql,	 nCount, nAlarmLogId);
	        	if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	    		{
		    		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    		return EXCEPTION;
	    		}
	    		CommitTransaction();
	    		
	    	}
	    	else
	    	{
	    		FreeCursor(&struCursor);
	    		
	    		//�������һ��û������ѹ���ļ�¼��������Ϊ��ѹ����¼
	    		memset(szSql, 0, sizeof(szSql));
				sprintf(szSql,"select min(alg_AlarmLogId) as alg_AlarmLogId from alm_AlarmLog  where alg_NeId = :v_0 and  alg_AlarmId = :v_1 and alg_AlarmStatusId < 4 ");
	    		PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%d][%s]\n", szSql, nNeId, struAlarmPara.szAlarmId);
	    		if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	    		{
	    			PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    					  szSql, GetSQLErrorMessage());
	    			return EXCEPTION;
	    		}
	    		if(FetchCursor(&struCursor) != NORMAL)
	    		{
	    			PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n",  szSql);
        			FreeCursor(&struCursor);
	    			return EXCEPTION;
	    		}
	    		nAlarmLogId = atoi(GetTableFieldValue(&struCursor, "alg_AlarmLogId"));
	    		FreeCursor(&struCursor);
	    		
	    		memset(&struBindVar, 0, sizeof(struBindVar));
				struBindVar.struBind[0].nVarType = SQLT_INT;
				struBindVar.struBind[0].VarValue.nValueInt = nCount;
				struBindVar.nVarCount++;
				
				struBindVar.struBind[1].nVarType = SQLT_INT;
				struBindVar.struBind[1].VarValue.nValueInt = nAlarmLogId;
				struBindVar.nVarCount++;
				
	    		sprintf(szSql, "update alm_AlarmLog set alg_compressCount = :v_0 ,alg_compressState = 1 where alg_AlarmLogId = :v_1");
	        	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql, nCount, nAlarmLogId);
	        	if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	    		{
		    		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    		return EXCEPTION;
	    		}
	    		CommitTransaction();
	    	}
	    	memset(&struBindVar, 0, sizeof(struBindVar));
			struBindVar.struBind[0].nVarType = SQLT_INT;
			struBindVar.struBind[0].VarValue.nValueInt = nAlarmLogId;
			struBindVar.nVarCount++;
			
			struBindVar.struBind[1].nVarType = SQLT_INT;
			struBindVar.struBind[1].VarValue.nValueInt = nNeId;
			struBindVar.nVarCount++;
			
			struBindVar.struBind[2].nVarType = SQLT_INT;
			struBindVar.struBind[2].VarValue.nValueInt = atoi(struAlarmPara.szAlarmId);
			struBindVar.nVarCount++;
			
			struBindVar.struBind[3].nVarType = SQLT_INT;
			struBindVar.struBind[3].VarValue.nValueInt = nAlarmLogId;
			struBindVar.nVarCount++;
		
	        sprintf(szSql, "update alm_AlarmLog set alg_compressShowId = :v_0, alg_compressState = 0 where  alg_NeId = :v_1 and  alg_AlarmId = :v_2 and alg_AlarmStatusId < 4 and alg_AlarmLogId <> :v_3 and (alg_compressState <> 0  or alg_compressState is null)");
	    	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%d][%d][%s][%d]\n", szSql, nAlarmLogId, nNeId, struAlarmPara.szAlarmId, nAlarmLogId);	
	    	if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	    	{
		    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    	return EXCEPTION;
	    	}
	    	CommitTransaction();
	    	
	    }

	}
	
    return NORMAL;
}

//Ƶ���ϱ�
RESULT AlarmFrequent(int nMaintainLogId, PSTR pszCondition)
{
	CURSORSTRU struCursor;
    STR szSql[MAX_SQL_LEN];
    STR szCount[10];
    STR szHour[10];
    STR szEnable[10];
    INT nStartAlarmLogId, nCount;
    UINT nAlarmFrequentId;
    STR szStartTime[20], szStyle[100];
    STR szCondition[150];
    bufclr(szStyle);
    
    if (strcmp(pszCondition, "chkFrqAlarm") == 0)
    	strcpy(szStyle, "�¸澯");
    else if (strcmp(pszCondition, "chkFrqAlarmClear") == 0)
    	strcpy(szStyle, "�澯�ָ�");
    else if (strcmp(pszCondition, "chkFrqNeBulid") == 0)
    	strcpy(szStyle, "���ſ�վ�ϱ�");
    else if (strcmp(pszCondition, "chkFrqNeRevamp") == 0)
    	strcpy(szStyle, "����ϱ�");
    else if (strcmp(pszCondition, "chkFrqNeRebuild") == 0)
    	strcpy(szStyle, "�޸��ϱ�");
    else if (strcmp(pszCondition, "chkFrqNeCheck") == 0)
    	strcpy(szStyle, "Ѳ���ϱ�");

	//ȡƵ���澯����
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='Frequency' and par_KeyName = 'Enabled'");
	GetSysParameter(szSql, szEnable);
  	
	if(strcmp(szEnable, "TRUE") == 0)
	{	
		sprintf(szSql, "par_SectionName ='Frequency' and par_KeyName = 'Condition'");
		GetSysParameter(szSql, szCondition);
		if (strstr(szCondition, pszCondition) == NULL)
		{			
			return NORMAL;
		} 		
	    sprintf(szSql, "par_SectionName = 'Frequency' and par_KeyName = 'Hour'");
	    GetSysParameter(szSql, szHour);
	    sprintf(szSql, "par_SectionName = 'Frequency' and par_KeyName = 'Count'");
	    GetSysParameter(szSql, szCount);
  	    
	    if ((strcmp(szHour, "0") == 0) || (strcmp(szHour, "") == 0)
	        || (strcmp(szCount, "0") == 0) || (strcmp(szCount, "") == 0))
	    {
	        PrintErrorLog(DBG_HERE,"��ȡƵ���澯������ʱ�����\n");
	        return EXCEPTION;
	    }
  	    
	    bufclr(szHour);
	    memcpy(szHour, GetSystemTime(), 2);
	    memset(szSql, 0, sizeof(szSql));
    
	    if (atoi(szHour) < 12)
			sprintf(szSql,"SELECT count(mnt_NeId) as v_count from man_MaintainLog where  mnt_Style = '%s' and mnt_NeId = %d  and mnt_EventTime >= to_date('%s000000', 'yyyymmddhh24miss') and mnt_EventTime <= to_date('%s115959', 'yyyymmddhh24miss')", 
	            szStyle, struAlarmPara.nNeId,  GetSystemDate(), GetSystemDate());
	    else
	    	sprintf(szSql,"SELECT count(mnt_NeId) as v_count from man_MaintainLog where  mnt_Style = '%s' and mnt_NeId = %d  and mnt_EventTime >= to_date('%s120000', 'yyyymmddhh24miss') and mnt_EventTime <= to_date('%s235959', 'yyyymmddhh24miss')", 
	            szStyle, struAlarmPara.nNeId,  GetSystemDate(), GetSystemDate());
	    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	    if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    				  szSql, GetSQLErrorMessage());
	    	return EXCEPTION;
	    }
	    if(FetchCursor(&struCursor) != NORMAL)
	    {
	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
	    				  szSql, GetSQLErrorMessage());
	    	FreeCursor(&struCursor);
	    	return EXCEPTION;
	    }
	    nCount = atoi(GetTableFieldValue(&struCursor, "v_count"));
	    FreeCursor(&struCursor);
	    
	    if (nCount >= atoi(szCount))
	    {
	    	memset(szSql, 0, sizeof(szSql));
	    	if (atoi(szHour) < 12)
				sprintf(szSql,"SELECT  afq_AlarmFrequentId, afq_Count from  alm_AlarmFrequent WHERE afq_NeId = %d  and afq_StartTime >= to_date('%s000000', 'yyyymmddhh24miss') and afq_StartTime <= to_date('%s115959', 'yyyymmddhh24miss')", 
	    	        struAlarmPara.nNeId, GetSystemDate(), GetSystemDate());
	    	else
	    		sprintf(szSql,"SELECT  afq_AlarmFrequentId, afq_Count from  alm_AlarmFrequent WHERE afq_NeId = %d  and afq_StartTime >= to_date('%s120000', 'yyyymmddhh24miss') and afq_StartTime <= to_date('%s235959', 'yyyymmddhh24miss')", 
	    	        struAlarmPara.nNeId, GetSystemDate(), GetSystemDate());
	    	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	    	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	    	{
	    		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    					  szSql, GetSQLErrorMessage());
	    		return EXCEPTION;
	    	}
	    	if(FetchCursor(&struCursor) == NORMAL)
	    	{
	    		nAlarmFrequentId = atoi(GetTableFieldValue(&struCursor, "afq_AlarmFrequentId"));
	    		FreeCursor(&struCursor);
				sprintf(szSql, "update alm_AlarmFrequent set afq_Count = %d,afq_EndTime = to_date('%s', 'yyyy-mm-dd hh24:mi:ss'),afq_EndAlarmLogId = %d where afq_NeId = %d and afq_AlarmFrequentId = %d",
	        		 nCount, GetSysDateTime(), nMaintainLogId, struAlarmPara.nNeId, nAlarmFrequentId);
	    	}
	    	else
	    	{
	    		FreeCursor(&struCursor);
	    		memset(szSql, 0, sizeof(szSql));
	    		if (atoi(szHour) < 12)
	    			sprintf(szSql,"SELECT  mnt_MaintainLogId, to_char(mnt_EventTime, 'yyyy-mm-dd hh24:mi:ss') as mnt_EventTime from man_MaintainLog where  mnt_Style = '%s' and mnt_NeId = %d  and mnt_EventTime >= to_date('%s000000', 'yyyymmddhh24miss') and mnt_EventTime <= to_date('%s115959', 'yyyymmddhh24miss') order by mnt_MaintainLogId", 
	    	        	szStyle, struAlarmPara.nNeId,  GetSystemDate(), GetSystemDate());
	    	    else
	    	    	sprintf(szSql,"SELECT  mnt_MaintainLogId, to_char(mnt_EventTime, 'yyyy-mm-dd hh24:mi:ss') as mnt_EventTime from man_MaintainLog where  mnt_Style = '%s' and mnt_NeId = %d  and mnt_EventTime >= to_date('%s120000', 'yyyymmddhh24miss') and mnt_EventTime <= to_date('%s235959', 'yyyymmddhh24miss') order by mnt_MaintainLogId", 
	    	        	szStyle, struAlarmPara.nNeId,  GetSystemDate(), GetSystemDate());
	    	    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	    	    if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	    	    {
	    	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
	    					  szSql, GetSQLErrorMessage());
	    		    return EXCEPTION;
	    	    }
	    	    if(FetchCursor(&struCursor) != NORMAL)
	    	    {
	    	    	PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
	    				  szSql, GetSQLErrorMessage());
	    			FreeCursor(&struCursor);
	    			return EXCEPTION;
	    	    }
	    	    nStartAlarmLogId = atoi(GetTableFieldValue(&struCursor, "mnt_MaintainLogId"));
	    	    strcpy(szStartTime, GetTableFieldValue(&struCursor, "mnt_EventTime"));
	    	    FreeCursor(&struCursor);
	    	    
	    		GetDbSerial(&nAlarmFrequentId, "alm_AlarmFrequent");
	    		snprintf(szSql, sizeof(szSql),
	    			"insert into  alm_AlarmFrequent(afq_AlarmFrequentId,afq_NeId,afq_Count,afq_StartAlarmLogId,afq_StartTime,afq_EndAlarmLogId,afq_EndTime,afq_End)"
         			" VALUES(%d, %d,%d, %d, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'),%d, to_date('%s', 'yyyy-mm-dd hh24:mi:ss'),0)",
	        		nAlarmFrequentId,  struAlarmPara.nNeId, nCount, nStartAlarmLogId, szStartTime, nMaintainLogId, GetSysDateTime());
	    	}
	    	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	    	if(ExecuteSQL(szSql) !=NORMAL ) 
	    	{
		    	PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    	return EXCEPTION;
	    	}
	    	CommitTransaction();
	    }
	    /*
		if (CallAlarmFrequency(nMaintainLogId, struAlarmPara.nNeId, szHour, szCount) != NORMAL)
		{
		    PrintErrorLog(DBG_HERE,"ִ�и澯Ƶ���ϱ��洢���̴���\n");
	        return EXCEPTION;
		}
		*/
	}
	
    return NORMAL;
}

RESULT SaveAlarmTransferLog(int nPersonId, PSTR pszMobile)
{
	CURSORSTRU struCursor;
    STR szSql[MAX_SQL_LEN];
	UINT nTransferLogId;		//��ȡ���
	STR szMsgCont[500];
	
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select ne_name, ne_cellid, ne_basestationid from ne_element  where ne_neid = %d", struAlarmPara.nNeId);
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
	if(SelectTableRecord(szSql, &struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
		FreeCursor(&struCursor);
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if (struAlarmPara.bIsNewAlarm == BOOLTRUE)
		sprintf(szMsgCont, "%s,��վ��%s,С����%s,%s,�봦��", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_name")),
			GetTableFieldValue(&struCursor, "ne_basestationid"), GetTableFieldValue(&struCursor, "ne_cellid"), struAlarmPara.szAlarmName);
	else
		sprintf(szMsgCont, "%s,��վ��%s,С����%s,%s�Ѿ��ָ���", TrimAllSpace(GetTableFieldValue(&struCursor, "ne_name")),
			GetTableFieldValue(&struCursor, "ne_basestationid"), GetTableFieldValue(&struCursor, "ne_cellid"), struAlarmPara.szAlarmName);
	FreeCursor(&struCursor);
	
	
	GetDbSerial(&nTransferLogId, "alarmtransferlog");
	snprintf(szSql, sizeof(szSql), "insert into  alm_alarmtransferlog (atl_transferlogid,atl_personid,atl_time, atl_moble,atl_interface, atl_alarmlogid, atl_neid, atl_alarmid, atl_alarmstate)"
             " VALUES(%d, %d, to_date( '%s','yyyy-mm-dd hh24:mi:ss'), '%s',  '%s', %d, %d, %s, 0)", 
             nTransferLogId, nPersonId, GetSysDateTime(), pszMobile, szMsgCont, struAlarmPara.nAlarmLogId, struAlarmPara.nNeId, struAlarmPara.szAlarmId);
    PrintDebugLog(DBG_HERE, "ִ��SQL���[%s]\n", szSql);
    if(ExecuteSQL(szSql) !=NORMAL) 
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	CommitTransaction();
	return NORMAL;
}

RESULT TransferAlarm()
{
	CURSORSTRU struCursor;
    STR szSql[MAX_SQL_LEN];
    STR szTime[10];
    STR szEnable[10];
    INT nPersonId;
    STR szTemp[20], szMobile[20];
    STR szCondition[100];
    
    //ȡ�澯ǰת����
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmTransfer' and par_KeyName = 'Enabled'");
	GetSysParameter(szSql, szEnable);
	if(strcmp(szEnable, "TRUE") != 0)
	{
		return EXCEPTION;
	}
	//�жϸ澯ʱ���
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmTransfer' and par_KeyName = 'Time'");
	GetSysParameter(szSql, szTime);
	if(strcmp(szTime, "24") != 0 && strlen(szTime) == 4)
	{
		time_t struTimeNow;
		struct tm struTmNow;
		STR szBeginTime[3],szEndTime[3];
		STR szAlarmBeginTime[30],szAlarmEndTime[30];
		
		strncpy(szBeginTime, szTime, 2);
		strncpy(szEndTime, szTime+2, 2);
		
		time(&struTimeNow);
		struTmNow=*localtime(&struTimeNow);
		
        snprintf(szAlarmBeginTime, sizeof(szAlarmBeginTime), "%04d-%02d-%02d %s:00:00", 
			struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szBeginTime);
		snprintf(szAlarmEndTime, sizeof(szAlarmEndTime), "%04d-%02d-%02d %s:00:00", 
			struTmNow.tm_year + 1900, struTmNow.tm_mon + 1, struTmNow.tm_mday, szEndTime);
		
		PrintDebugLog(DBG_HERE,"�澯ʱ���[%s][%s]\n",szAlarmBeginTime,szAlarmEndTime);
        if ((int)time(NULL) < (int)MakeITimeFromSTime(szAlarmBeginTime) || 
        	(int)time(NULL) > (int)MakeITimeFromSTime(szAlarmEndTime))
        {
        	return EXCEPTION;
        }
	}
	
	//�ж��Ƿ����ظ��澯��¼
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select atl_TransferLogId from alm_alarmtransferlog where atl_neId= %d and atl_alarmId = %s and to_char(atl_time,'yyyy-mm-dd') = to_char(sysdate,'yyyy-mm-dd')",
		struAlarmPara.nNeId, struAlarmPara.szAlarmId);
	PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) == NORMAL)
	{
		FreeCursor(&struCursor);
		return EXCEPTION;
	}
	FreeCursor(&struCursor);
	
	//�����Ƿ��и澯ǰת���ֻ���
	memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select use_userid, use_mobile,use_alarmturncondition from pre_user  where use_mobileturn = -1");
	PrintDebugLog(DBG_HERE,"ִ��SQL���[%s]\n",szSql);
	if(SelectTableRecord(szSql,&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		return EXCEPTION;
	}
	while(FetchCursor(&struCursor) == NORMAL)
	{
		nPersonId = atoi(GetTableFieldValue(&struCursor,"use_userid"));
		strcpy(szMobile, GetTableFieldValue(&struCursor,"use_mobile"));
		if (strlen(szMobile) != 11) continue;
		strcpy(szCondition, TrimAllSpace(GetTableFieldValue(&struCursor,"use_alarmturncondition")));
		sprintf(szTemp, "%s,", struAlarmPara.szAlarmId);
		if (strlen(szCondition) > 1 && strstr(szCondition, szTemp) == NULL) 
			continue;
		else
			SaveAlarmTransferLog(nPersonId, szMobile);
	}
	FreeCursor(&struCursor);
	
	
    return NORMAL;
}



RESULT DealNewAlarm(PXMLSTRU pstruXml)
{
    int nResult; // ÿ���Ƿ�ִ�гɹ�
    char szAlarmTime[30];
    
    memset(&struAlarmPara, 0, sizeof(ALARMPARAMETER));
    struAlarmPara.bIsNewAlarm = BOOLTRUE;
    struAlarmPara.nNeId = atoi(DemandStrInXmlExt(pstruXml, "<omc>/<��Ԫ���>"));
    //struAlarmPara.nNeId = nNeId;
    strcpy(struAlarmPara.szAlarmObjId, DemandStrInXmlExt(pstruXml, "<omc>/<�澯����>"));
    //strcpy(struAlarmPara.szAlarmObjId, pszAlarmObjId);
    strcpy(struAlarmPara.szAlarmObjList, DemandStrInXmlExt(pstruXml, "<omc>/<�澯�б�>"));
    //strcpy(struAlarmPara.szAlarmObjList, pszAlarmObjList);
    strcpy(szAlarmTime, DemandStrInXmlExt(pstruXml, "<omc>/<�澯ʱ��>"));
    if (strlen(szAlarmTime) == 19 && strncmp(szAlarmTime, GetSysDateTime(), 10) == 0)
		strcpy(struAlarmPara.szAlarmTime, szAlarmTime);
    else
    	strcpy(struAlarmPara.szAlarmTime, GetSysDateTime());
	//��ƽǿ�ȸ澯��Ϣ
    if (strcmp(struAlarmPara.szAlarmObjId, "0704") == 0)
    {
    	sprintf(struAlarmPara.szAlarmInfo, "%s, ����С����=%s, ��ƽǿ��=%s", struAlarmPara.szAlarmTime,
    		DemandStrInXmlExt(pstruXml, "<omc>/<����С��>"), DemandStrInXmlExt(pstruXml, "<omc>/<��ƽǿ��>"));
    }
    //��ӵ��澯��־��alm_AlarmLog��
	nResult = SaveAlarmLog(struAlarmPara.nNeId);
	if (nResult == EXCEPTION)
	{
		PrintErrorLog(DBG_HERE,"��ӵ��澯��־��alm_AlarmLog�͸�����Ԫ�澯�б����\n");
		return EXCEPTION;
	}
	/*
	//�澯�Զ�ȷ��
	nResult = AlarmAutoConfirm(struAlarmPara.nNeId);
	if (nResult == EXCEPTION)
	{
	    PrintErrorLog(DBG_HERE,"�澯�Զ�ȷ�Ϸ�������\n");
		return EXCEPTION;
	}
	struAlarmPara.nAlarmAutoConfirmSuccess = nResult;

	//�澯�Զ����
	nResult = AlarmAutoClear(struAlarmPara.nNeId);
	if (nResult == EXCEPTION)
	{
	    PrintErrorLog(DBG_HERE,"�澯�Զ������������\n");
		return EXCEPTION;
	}
	struAlarmPara.nAlarmAutoClearSuccess = nResult;

	if (nResult == NORMAL)//�澯����Զ�����ɹ�������ִ�и澯ѹ��
	*/
	{
		//�澯ѹ��
		nResult = AlarmCompress(struAlarmPara.nNeId);
		if (nResult == EXCEPTION)
		{
		    PrintErrorLog(DBG_HERE,"�澯ѹ����������\n");
		    return EXCEPTION;
		}
	}
	

	//�澯ǰת
	/**********
	nResult = TransferAlarm();
	if (nResult == EXCEPTION)
	{
		return EXCEPTION;
	}
	************/
	return NORMAL;
	
}
//�澯�ָ�
RESULT AlarmComeback(PXMLSTRU pstruXml)
{
    CURSORSTRU struCursor;
	STR szSql[MAX_SQL_LEN];
	STR szOldAlarmObjId[20];
	STR szNewAlarmObjId[20];
	char szAlarmTime[30];
	PSTR pszTemp;
	TBINDVARSTRU struBindVar;
	
	memset(&struAlarmPara, 0, sizeof(ALARMPARAMETER));
	struAlarmPara.bIsNewAlarm = BOOLFALSE;
    struAlarmPara.nNeId = atoi(DemandStrInXmlExt(pstruXml, "<omc>/<��Ԫ���>"));
    strcpy(struAlarmPara.szAlarmObjId, DemandStrInXmlExt(pstruXml, "<omc>/<�澯����>"));
    strcpy(struAlarmPara.szAlarmObjList, DemandStrInXmlExt(pstruXml, "<omc>/<�澯�б�>"));
    //strcpy(struAlarmPara.szAlarmObjList, pszAlarmObjList);
    strcpy(szAlarmTime, DemandStrInXmlExt(pstruXml, "<omc>/<�澯ʱ��>"));
    if (strlen(szAlarmTime) == 19 && strncmp(szAlarmTime, GetSysDateTime(), 10) == 0)
		strcpy(struAlarmPara.szAlarmTime, szAlarmTime);
    else
    	strcpy(struAlarmPara.szAlarmTime, GetSysDateTime());
    	
	memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_STR;
	strcpy(struBindVar.struBind[0].VarValue.szValueChar, struAlarmPara.szAlarmObjId);
	struBindVar.nVarCount++;
	
    memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "select * from ALM_ALARM where alm_ObjId = :v_0 ");
	PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%s]\n", szSql, struAlarmPara.szAlarmObjId);
	if(BindSelectTableRecord(szSql, &struCursor, &struBindVar) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]����, ��ϢΪ[%s]\n", \
					  szSql, GetSQLErrorMessage());
		return EXCEPTION;
	}
	if(FetchCursor(&struCursor) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���[%s]û���ҵ���¼\n", \
					  szSql, GetSQLErrorMessage());
        FreeCursor(&struCursor);
		return EXCEPTION;
	}
	
	strcpy(struAlarmPara.szAlarmId ,GetTableFieldValue(&struCursor, "alm_AlarmId"));
	strcpy(struAlarmPara.szAlarmLevelId , GetTableFieldValue(&struCursor, "alm_LevelId"));
	strcpy(struAlarmPara.szAlarmName, GetTableFieldValue(&struCursor, "alm_Name"));
	//strcpy(struAlarmPara.alarmLevelName, GetTableFieldValue(&struCursor, "alm_LevelName"));
    FreeCursor(&struCursor);
    
    memset(&struBindVar, 0, sizeof(struBindVar));
	struBindVar.struBind[0].nVarType = SQLT_STR;
	strcpy(struBindVar.struBind[0].VarValue.szValueChar, struAlarmPara.szAlarmTime);
	struBindVar.nVarCount++;
	
	struBindVar.struBind[1].nVarType = SQLT_INT;
	struBindVar.struBind[1].VarValue.nValueInt =  struAlarmPara.nNeId;
	struBindVar.nVarCount++;
			
	struBindVar.struBind[2].nVarType = SQLT_INT;
	struBindVar.struBind[2].VarValue.nValueInt = atoi(struAlarmPara.szAlarmId);
	struBindVar.nVarCount++;
	
    memset(szSql, 0, sizeof(szSql));
    snprintf(szSql, sizeof(szSql), 
        "UPDATE alm_AlarmLog SET alg_ClearTime = to_date(:v_0,'yyyy-mm-dd hh24:mi:ss'), alg_AlarmStatusId = 7 ,alg_compressCount = null,alg_compressShowId = null,"
    	"alg_compressState = null WHERE alg_NeId = :v_1 and alg_alarmId = :v_2 and alg_AlarmStatusId < 4 ");
    
    PrintDebugLog(DBG_HERE, "��ʼִ��SQL[%s][%s][%d][%s]\n", szSql, struAlarmPara.szAlarmTime, struAlarmPara.nNeId, struAlarmPara.szAlarmId);
	if(BindExecuteSQL(szSql, &struBindVar) != NORMAL)
	{
		PrintErrorLog(DBG_HERE, "ִ��SQL���ʧ��[%s][%s]\n",
			szSql, GetSQLErrorMessage());
        return EXCEPTION;
	}
	CommitTransaction();
	if ((pszTemp = strstr(struAlarmPara.szAlarmObjList, struAlarmPara.szAlarmObjId)) != NULL)
    {
		memset(szOldAlarmObjId, 0, sizeof(szOldAlarmObjId));
        memcpy(szOldAlarmObjId, pszTemp, strlen(struAlarmPara.szAlarmObjId)+2);
        sprintf(szNewAlarmObjId, "%s:0", struAlarmPara.szAlarmObjId);
        
        memset(&struBindVar, 0, sizeof(struBindVar));
		struBindVar.struBind[0].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[0].VarValue.szValueChar, szOldAlarmObjId);
		struBindVar.nVarCount++;
		
		struBindVar.struBind[1].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[1].VarValue.szValueChar, szNewAlarmObjId);
		struBindVar.nVarCount++;
		
		struBindVar.struBind[2].nVarType = SQLT_STR;
		strcpy(struBindVar.struBind[2].VarValue.szValueChar, GetSysDateTime());
		struBindVar.nVarCount++;
		
		struBindVar.struBind[3].nVarType = SQLT_INT;
		struBindVar.struBind[3].VarValue.nValueInt =  struAlarmPara.nNeId;
		struBindVar.nVarCount++;
				
	
        sprintf(szSql, "update ne_Element set ne_AlarmObjList=REPLACE(ne_AlarmObjList, :v_0, :v_1),ne_LastUpdateTime =to_date( :v_2,'yyyy-mm-dd hh24:mi:ss'),ne_IsAlarm = 0 where  ne_NeId=:v_3");
        
        PrintDebugLog(DBG_HERE, "ִ��SQL���[%s][%s][%s][%s][%d]\n", szSql, szOldAlarmObjId, szNewAlarmObjId, GetSysDateTime(),  struAlarmPara.nNeId);
        if(BindExecuteSQL(szSql, &struBindVar) !=NORMAL ) 
	    {
		    PrintErrorLog(DBG_HERE,"ִ��SQL���[%s]ʧ��[%s]\n",szSql,GetSQLErrorMessage());
		    return EXCEPTION;
	    }
	    CommitTransaction();
	    
	}
	
       
    /********    
    memset(szSql, 0, sizeof(szSql));
	sprintf(szSql, "par_SectionName ='AlarmTransfer' and par_KeyName = 'chkSet3'");
	GetSysParameter(szSql, szEnable);
	if(strcmp(szEnable, "TRUE") == 0)
	{
		TransferAlarm();
	}
	********/
    
    return NORMAL;
}
