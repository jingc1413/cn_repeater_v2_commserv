// Mobile2GAppLayerA.cpp: implementation of the CMobile2GAppLayerA class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"

static BYTE m_Pdu[4096];
static int m_Len;
	



//����Ӧ�ò�Э��
int DecodeDasAppLayer(BYTEARRAY *pPack ,DECODE_OUT* pDecodeOut,OBJECTSTRU* ObjList)
{
    int nObjCont=0;
    int nRet = M2G_SUCCESS;
    int nIndex=0;
    OBJECTSTRU Item;
    
    memset(m_Pdu, 0, sizeof(m_Pdu));
	m_Len = 0;
	
	memcpy(m_Pdu, pPack->pPack, pPack->Len);
	m_Len = pPack->Len;
    
    //PrintHexDebugLog("����Ӧ�ò㱨��", m_Pdu, m_Len);
    
	if(m_Len<2)
	{
	    PrintErrorLog(DBG_HERE,"���Ĺ���[%d]ʧ��\n",m_Len);
		return M2G_OTHER_ERR;
    }
	//�����ʶ
	pDecodeOut->MAPLayer.CommandFalg = m_Pdu[0];

	//���ؽ����
	BYTE m_RetFlag = m_Pdu[1];
    pDecodeOut->MAPLayer.AnswerFlag=m_RetFlag;

	//�ж��Ƿ��б�Ҫ���¼�������
	if(m_RetFlag!=M2G_SUCCESS&&m_RetFlag!=0xFF&&m_RetFlag!=0x01) //
	{

    	pDecodeOut->MAPLayer.ErrorId=1;
		pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//�ο����ؽ����
		
        PrintErrorLog(DBG_HERE,"���ؽ����[%d]ʧ��\n",m_RetFlag);
		return EXCEPTION;
	}
	
	pDecodeOut->MAPLayer.ErrorId=0;
	pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//�ο����ؽ����
	

	//ȥ�������ʾ��Ӧ���־
    m_Len-=2;
    nIndex+=2;

	while(m_Len - 5>=0)
	{
		//���󳤶�
		memset(&Item, 0, sizeof(OBJECTSTRU));
		Item.OL = m_Pdu[nIndex]-DAS_MONITOROBJLEN_SIZE-DAS_MONITOROBJID_SIZE;

		if(Item.OL<1||(m_Len<Item.OL+3))
		{
            pDecodeOut->MAPLayer.ErrorId=1;
		    pDecodeOut->MAPLayer.ErrorCode=M2G_LEN_ERR;//�ο����ؽ����
		    PrintErrorLog(DBG_HERE,"���Ĺ���[%d]ʧ��,ʵ��[%d],����[%d],������[%d]\n", Item.OL, m_Len, nIndex, pDecodeOut->MAPLayer.ObjCount);
			return -1;
		}
		//RemoveAt(m_Pdu, 0);
		m_Len-=1;
        nIndex+=1;
        
		//������
		//���ڶ�ʱ�ϱ��������⴦�� 
		DWORD MapId = ReadDWORD(&m_Pdu[nIndex]);
		{
			//MAPID��Ϊ4λ 2013.7.25
			Item.MapID = MapId;
			//RemoveAt2(m_Pdu, 0, 4);
            m_Len-=4;
            nIndex+=4;
		
			//��������
			memcpy(Item.OC, &m_Pdu[nIndex], Item.OL);

			//RemoveAt2(m_Pdu, 0, Item.OL);
			m_Len-=Item.OL;
			nIndex+=Item.OL;
			
			//add to objectlist
			memcpy(&ObjList[nObjCont], &Item, sizeof(OBJECTSTRU));
			nObjCont++;
			//add objectcount
			pDecodeOut->MAPLayer.ObjCount+=1;
		}
	}

	return nRet;
}

int EncodeDasAppLayer(int CommandType, int AnswerType, OBJECTSTRU *ObjList, int ObjSize, BYTEARRAY *pPack)
{
    int i, j;
	int index=0;
	int nRet = M2G_SUCCESS;
	OBJECTSTRU Item;

	memset(m_Pdu, 0, sizeof(m_Pdu));
	//�����ʶ
	m_Pdu[index] = (char)CommandType;
	index++;
	//�����־
	m_Pdu[index] = (char)AnswerType;
	index++;
	

	//����һ�����⴦��
	//�������ݵ�Ԫ����
	for( i=0; i<ObjSize; i++)
	{
	    //PrintDebugLog(DBG_HERE, "MapID=[%X]\n", ObjList[i].MapID);
        //PrintHexDebugLog("OC=", ObjList[i].OC, ObjList[i].OL);
		memcpy(&Item, &ObjList[i], sizeof(OBJECTSTRU));
		
		//��ض��󳤶�=����+��ų���
		m_Pdu[index] = Item.OL+DAS_MONITOROBJID_SIZE+DAS_MONITOROBJLEN_SIZE;
		index++;
		//��ض����� 2013.7.25
		m_Pdu[index] = LOBYTE(LOWORD(Item.MapID));
		index++;
		m_Pdu[index] = HIBYTE(LOWORD(Item.MapID));
		index++;
		m_Pdu[index] = LOBYTE(HIWORD(Item.MapID));
		index++;		
		m_Pdu[index] = HIBYTE(HIWORD(Item.MapID));
		index++;
		//��ض�������
		for(j=0; j<Item.OL; j++)
		{
			m_Pdu[index] = Item.OC[j];
			index++;		
		}

	}
    memcpy(pPack->pPack, m_Pdu, index);
    pPack->Len = index;
    
    //PrintHexDebugLog("���Ӧ�ò�", m_Pdu, index);
    return nRet;
}
