/*
 * ����: ��ʱ������ͷ�ļ�
 * 
 * �޸ļ�¼:
 * 2008-11-18    ��־��   - ����
 */
 
#ifndef	__NORTHSEND_H__
#define	__NORTHSEND_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>

#define	MAX_TIMESVR_NUM	100
#define	SLEEP_TIMESVR_TIME	30

#define	TASKRUNING_STATE	0   //��������
#define	NORMAL_STATE	2       //��������
#define	PAUSE_STATE		3       //���������ͣ

/*
 * �澯���붨��
 */
#define ALM_MOS_ID  		162     //����MOSֵ�͸澯
#define ALM_FTPDOWN_ID		163     //FTP�������ʵ͸澯
#define ALM_MMS_ID			164     //���Ŷ˵��˷��ͳɹ��ʵ͸澯
#define ALM_FTPCELL_ID		165     //FTP����ʱС����ѡƵ���澯


typedef struct tagYIYANGSTRU
{
	STR szAlarmLogId[20+1];
	STR szNeId[20+1];
	STR szNeType[100+1];           //         ��Ԫ����
	STR szNeName[200+1];           //         ��Ԫ����
	STR szNeVendor[100+1];         //       ��Ԫ����

	STR szAlarmId[100+1];          //   ���ܸ澯ID������ϵͳ���_�澯ID��澯������澯IDΪͬһ�������ҹ�˾���в���ΨһID�ż���
	STR szAlarmTitle[200+1];       //     �澯���⣺���澯����
	STR szAlarmCreateTime[19+1];  //   �澯����ʱ��	��ʽΪ����-��-�� Сʱ:��:�룬��Ϊ4λ����СʱΪ24Сʱ�ƣ���ʽYYYY-MM-DD HH:MM:SS
	STR szAlarmClearTime[19+1];    //�澯���ʱ��
	STR szAlarmLevel[20+1];       //     �澯����
	STR szAlarmType[30+1] ;       //      �澯���ͣ����߸澯
	STR szAlarmObjId[9+1];         //
	STR szStandardAlarmName[200+1]; //��׼�澯��
	STR szProbableCauseTxt[200+1] ; //����ԭ��
	
	STR szAlarmLocation[100+1];    //  �澯��λ��
		
	STR szAlarmStatusId[1+1];      //    �澯״̬��0����Ԫ�Զ���� 1���Զ������2���ֹ������
	
	
	STR szSiteId[20+1];				//��վС����
	STR szSiteName[100+1];			//��վ����
	
	STR szSystemName[19+1] ;      //   ϵͳ���ƣ���ϵͳ��ƣ��μ���¼A�����ֶ���д������������ϵͳ��
	STR szSystemVendor[19+1];     //   ϵͳ�������ң�����άͨ�š�
	STR szSystemLevel[20+1];	   //   ϵͳ�ȼ�
	STR szSystemNo[20+1];			// ϵͳ���
	
	STR szAlarmRegion[100];
	STR szAlarmCounty[100];
	
	STR szExtendInfo[100+1] ;      //     ��չ��Ϣ��������չ
}YIYANGSTRU;


#endif
