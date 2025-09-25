// Mobile2GIntoLayerC.c: implementation of the CMobile2GIntoLayerC class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"

//定义起止标志
const BYTE ILC_STX				= 0x7E;
const BYTE ILC_ETX				= 0x7E;
//协议类型
const BYTE ILC_PROTOCOLTYPE		= 0x03;
//承载协议类型
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
	
	//对所有数据转义
	//用0x5E，0x5D来代替0x5E；用0x5E，0x7D来代替0x7E
	memset(szPdu, 0, sizeof(szPdu));
	szPdu[0]=m_Pdu[0];//起始标志
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
	szPdu[j]=m_Pdu[i];//结束标志
	j++;
	
	//清除递进来的数据
    memset(Pack->pPack, 0, j+1);
	Pack->Len = j;
	memcpy(Pack->pPack, szPdu, j);
	
	//Pack->Copy(m_Pdu);
	return TRUE;
}


//解析转义
BOOL UnEsc_c(BYTEARRAY *Pack)
{
	//对除起始标志和结束标志外的所有数据解析转义
	//用0x5E，0x5D来代替0x5E；用0x5E，0x7D来代替0x7E
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
	
    //PrintHexDebugLog("解析接入层报文", m_Pdu, m_Len);
	return TRUE;
}



int EncodeIntoLayerC(BYTEARRAY *pPack)
{
    int nRet = M2G_SUCCESS;
	char szPdu[4096];
	
	//协议类型
	szPdu[0] = ILC_PROTOCOLTYPE;
	//承载协议类型
	szPdu[1] = ILC_NPPROTOCOLTYPE;
	//数据单元
	memcpy(szPdu + 2, pPack->pPack, pPack->Len);
	
	//
	WORD crc = Get_CRC(pPack->Len + 2, szPdu);
	
	memset(m_Pdu, 0, sizeof(m_Pdu));

	// 	起始标志
	m_Pdu[0] = ILC_STX;
    memcpy(m_Pdu + 1, szPdu, pPack->Len + 2);
    m_Len = pPack->Len + 3;
    
	// 	校验单元
	m_Pdu[m_Len] = LOBYTE(crc);
	m_Len++;
	m_Pdu[m_Len] = HIBYTE(crc);
	m_Len++;
	//结束标志
	m_Pdu[m_Len] = ILC_ETX;
	m_Len++;
	
	memset(pPack->pPack, 0, m_Len +1);
    pPack->Len = m_Len;
    memcpy(pPack->pPack, m_Pdu, m_Len);
    //PrintHexDebugLog("打包接入层", m_Pdu, m_Len);
    
	if(!Esc_c(pPack))
		return M2G_ENCODE_ERR;
	
	return nRet;
}

//解析接入层C协议
int DecodeIntoLayerC(BYTEARRAY *pPack, DECODE_OUT *pDecodeOut, OBJECTSTRU *ObjList)
{
    int nRet = M2G_SUCCESS;
	if(pPack->Len<10)
		return M2G_OTHER_ERR;
    
    memset(m_Pdu, 0, sizeof(m_Pdu));
	m_Len = 0;
	
    //先进行转义处理并去掉STX和ETX
	if(!UnEsc_c(pPack))
	{
	    PrintErrorLog(DBG_HERE,"转义错误[%s]\n",pPack->pPack);
		return -1;
	}

	//检查CRC错
	WORD crc = m_Pdu[m_Len-2] + m_Pdu[m_Len-1]*0x100;
	//PrintDebugLog(DBG_HERE,"原始crc值[%d]\n", crc);
	//去掉CRC位
	m_Pdu[m_Len-1]='\0';
	m_Pdu[m_Len-2]='\0';
	m_Len-=2;

	if (crc != Get_CRC(m_Len, m_Pdu))
	{
		//pInfo->SetLastErr("接入层B: CRC校验错!");
		PrintErrorLog(DBG_HERE,"接入层C:CRC校验错[%d]\n",crc);
		return M2G_CRC_ERR;
	}
	
	//协议类型
	pDecodeOut->APLayer.ProtocolType = m_Pdu[0];
	//RemoveAt(m_Pdu, 0);
    m_Len-=1;
	//承载协议类型
	pDecodeOut->APLayer.NPType = m_Pdu[1];
	//RemoveAt(m_Pdu, 0);
    m_Len-=1;
    
	    
	//解析正常
	pDecodeOut->APLayer.ErrorId=0;
	pDecodeOut->APLayer.ErrorCode=0;

	//传入网路层进行解析
	BYTEARRAY pOut;
	pOut.pPack = &m_Pdu[2];
	pOut.Len = m_Len;
	nRet = DecodeNetworkLayer(&pOut, pDecodeOut, ObjList);
	
	return nRet;
}


