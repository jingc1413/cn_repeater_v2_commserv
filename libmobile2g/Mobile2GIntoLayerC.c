// Mobile2GIntoLayerC.c: implementation of the CMobile2GIntoLayerC class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"

//������ֹ��־
const BYTE ILC_STX				= 0x7E;
const BYTE ILC_ETX				= 0x7E;
//Э������
const BYTE ILC_PROTOCOLTYPE		= 0x03;
//����Э������
const BYTE ILC_NPPROTOCOLTYPE	= 0x01;

static BYTE m_Pdu[4096];
static int m_Len;

#define CRC_PRESET		0x0000
#define CRC_POLYNOM		0x1021




BOOL Esc_c(BYTEARRAY *Pack)
{
    int len = Pack->Len;
	BYTE szPdu[4096];
	int i, j;
	
	//����������ת��
	//��0x5E��0x5D������0x5E����0x5E��0x7D������0x7E
	memset(szPdu, 0, sizeof(szPdu));
	szPdu[0]=m_Pdu[0];//��ʼ��־
	for(i=1, j=1; i<len-1; i++, j++)
	{
		if(m_Pdu[i]==0x5E)
		{
		    szPdu[j]=0x5E;
		    j++;
		    szPdu[j]=0x5D;
		}
		else if(m_Pdu[i]==0x7E)
		{
		    szPdu[j]=0x5E;
		    j++;
		    szPdu[j]=0x7D;
		}
		else
		{
		    szPdu[j]=m_Pdu[i];
		}

	}
	szPdu[j]=m_Pdu[i];//������־
	j++;
	
	//����ݽ���������
    memset(Pack->pPack, 0, j+1);
	Pack->Len = j;
	memcpy(Pack->pPack, szPdu, j);
	
	//Pack->Copy(m_Pdu);
	return TRUE;
}


//����ת��
BOOL UnEsc_c(BYTEARRAY *Pack)
{
	//�Գ���ʼ��־�ͽ�����־����������ݽ���ת��
	//��0x5E��0x5D������0x5E����0x5E��0x7D������0x7E
	//m_Pdu.Copy(*Pack);
    int i, j;
	for(i=1, j=0; i<Pack->Len-1; i++, j++)
	{
		if(Pack->pPack[i]==0x5E && Pack->pPack[i+1]==0x5D)
		{
		    m_Pdu[j] = Pack->pPack[i];
			//m_Pdu.RemoveAt(i+1);
			i++;
		}
		else if(Pack->pPack[i]==0x5E && Pack->pPack[i+1]==0x7D)
		{
		    m_Pdu[j] = 0x7E;
			//m_Pdu.RemoveAt(i+1);
			i++;
		}
		else
		{
		    m_Pdu[j] = Pack->pPack[i];
		}

	}
	m_Len = j;
	
    //PrintHexDebugLog("��������㱨��", m_Pdu, m_Len);
	return TRUE;
}



int EncodeIntoLayerC(BYTEARRAY *pPack)
{
    int nRet = M2G_SUCCESS;
	char szPdu[4096];
	
	//Э������
	szPdu[0] = ILC_PROTOCOLTYPE;
	//����Э������
	szPdu[1] = ILC_NPPROTOCOLTYPE;
	//���ݵ�Ԫ
	memcpy(szPdu + 2, pPack->pPack, pPack->Len);
	
	//
	WORD crc = Get_CRC(pPack->Len + 2, szPdu);
	
	memset(m_Pdu, 0, sizeof(m_Pdu));

	// 	��ʼ��־
	m_Pdu[0] = ILC_STX;
    memcpy(m_Pdu + 1, szPdu, pPack->Len + 2);
    m_Len = pPack->Len + 3;
    
	// 	У�鵥Ԫ
	m_Pdu[m_Len] = LOBYTE(crc);
	m_Len++;
	m_Pdu[m_Len] = HIBYTE(crc);
	m_Len++;
	//������־
	m_Pdu[m_Len] = ILC_ETX;
	m_Len++;
	
	memset(pPack->pPack, 0, m_Len +1);
    pPack->Len = m_Len;
    memcpy(pPack->pPack, m_Pdu, m_Len);
    //PrintHexDebugLog("��������", m_Pdu, m_Len);
    
	if(!Esc_c(pPack))
		return M2G_ENCODE_ERR;
	
	return nRet;
}

//���������CЭ��
int DecodeIntoLayerC(BYTEARRAY *pPack, DECODE_OUT *pDecodeOut, OBJECTSTRU *ObjList)
{
    int nRet = M2G_SUCCESS;
	if(pPack->Len<10)
		return M2G_OTHER_ERR;
    
    memset(m_Pdu, 0, sizeof(m_Pdu));
	m_Len = 0;
	
    //�Ƚ���ת�崦��ȥ��STX��ETX
	if(!UnEsc_c(pPack))
	{
	    PrintErrorLog(DBG_HERE,"ת�����[%s]\n",pPack->pPack);
		return -1;
	}

	//���CRC��
	WORD crc = m_Pdu[m_Len-2] + m_Pdu[m_Len-1]*0x100;
	//PrintDebugLog(DBG_HERE,"ԭʼcrcֵ[%d]\n", crc);
	//ȥ��CRCλ
	m_Pdu[m_Len-1]='\0';
	m_Pdu[m_Len-2]='\0';
	m_Len-=2;

	if (crc != Get_CRC(m_Len, m_Pdu))
	{
		//pInfo->SetLastErr("�����B: CRCУ���!");
		PrintErrorLog(DBG_HERE,"�����C:CRCУ���[%d]\n",crc);
		return M2G_CRC_ERR;
	}
	
	//Э������
	pDecodeOut->APLayer.ProtocolType = m_Pdu[0];
	//RemoveAt(m_Pdu, 0);
    m_Len-=1;
	//����Э������
	pDecodeOut->APLayer.NPType = m_Pdu[1];
	//RemoveAt(m_Pdu, 0);
    m_Len-=1;
    
	    
	//��������
	pDecodeOut->APLayer.ErrorId=0;
	pDecodeOut->APLayer.ErrorCode=0;

	//������·����н���
	BYTEARRAY pOut;
	pOut.pPack = &m_Pdu[2];
	pOut.Len = m_Len;
	nRet = DecodeNetworkLayer(&pOut, pDecodeOut, ObjList);
	
	return nRet;
}


