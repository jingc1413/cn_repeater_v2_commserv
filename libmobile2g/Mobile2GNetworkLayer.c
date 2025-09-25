// Mobile2GNetworkLayer.cpp: implementation of the CMobile2GNetworkLayer class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"


static BYTE m_Pdu[4096];
static int m_Len;



//网络层解析
int DecodeNetworkLayer(BYTEARRAY *pPack ,DECODE_OUT* pDecodeOut,OBJECTSTRU* ObjList)
{
	int nRet = M2G_SUCCESS;
	int nIndex=0;

	memset(m_Pdu, 0, sizeof(m_Pdu));
	m_Len = 0;

	memcpy(m_Pdu, pPack->pPack, pPack->Len);
	m_Len =  pPack->Len;
    
	if(m_Len<=9)
		return M2G_OTHER_ERR;

	//地址单元
	pDecodeOut->NPLayer.structRepeater.RepeaterId = ReadDWORD(m_Pdu);
    m_Len-=4;
	nIndex+=4;
	
	pDecodeOut->NPLayer.structRepeater.DeviceId = m_Pdu[nIndex];
    m_Len-=1;
    nIndex+=1;
    
	//通信包标识号
	pDecodeOut->NPLayer.NetFlag=ReadWORD(&m_Pdu[nIndex]);
    m_Len-=2;
    nIndex+=2;
 
	//交互标志
	pDecodeOut->NPLayer.NPJHFlag = m_Pdu[nIndex];
	//RemoveAt(m_Pdu, 0);
    m_Len-=1;
    nIndex+=1;

	//应用协议标识
	pDecodeOut->NPLayer.APID = m_Pdu[nIndex];
	//RemoveAt(m_Pdu, 0);
	m_Len-=1;
    nIndex+=1;
    
	//表明此层解析正常
	pDecodeOut->NPLayer.ErrorId=0;
	pDecodeOut->NPLayer.ErrorCode=0;

	if(pDecodeOut->NPLayer.APID==0x03)// DAS协议
	{
		BYTEARRAY pOut;
	    pOut.pPack = &m_Pdu[nIndex];
	    pOut.Len = m_Len;
		nRet = DecodeDasAppLayer(&pOut, pDecodeOut, ObjList);
	}
	else  
	{
		//CMobile2GAppLayerA Layer;
		//nRet = Layer.Decode(&m_Pdu, pDecodeOut, ObjList);
		//传入网路层进行解析
		//PrintHexDebugLog("解析网络层报文", &m_Pdu[nIndex], m_Len);
	    BYTEARRAY pOut;
	    pOut.pPack = &m_Pdu[nIndex];
	    pOut.Len = m_Len;
		nRet = DecodeAppLayer(&pOut, pDecodeOut, ObjList);
	}	

	return nRet;
}

int EncodeNetworkLayer(int NetFlag,REPEATERINFO* stuRepterinfo,BYTEARRAY* pPack, int CommandType)
{
	int nRet = M2G_SUCCESS;

	//m_Pdu.RemoveAll();
	memset(m_Pdu, 0, sizeof(m_Pdu));
	
	//站点编号 ,低字节在前
	m_Pdu[0]=LOBYTE(LOWORD(stuRepterinfo->RepeaterId));
	m_Pdu[1]=HIBYTE(LOWORD(stuRepterinfo->RepeaterId));
	m_Pdu[2]=LOBYTE(HIWORD(stuRepterinfo->RepeaterId));
	m_Pdu[3]=HIBYTE(HIWORD(stuRepterinfo->RepeaterId));
	//设备编号
	m_Pdu[4]=stuRepterinfo->DeviceId;

	//通信包标识号

	m_Pdu[5] = LOBYTE(NetFlag);
	m_Pdu[6] = HIBYTE(NetFlag);
	
	//NP层交互标志
	if(CommandType==RELAPSECOMMAND)
		m_Pdu[7] = 0x00; 
	else
		m_Pdu[7] = 0x80;

	//应用协议标识(APID)
    m_Pdu[8] = 0x01;

	//应用层数据单元（PDU）
	//m_Pdu.Append(*pPack);
	memcpy(m_Pdu + 9, pPack->pPack, pPack->Len);
	
	int nPackLen = pPack->Len + 9;
	memset(pPack->pPack, 0, nPackLen+1);
    
    memcpy(pPack->pPack, m_Pdu, nPackLen);
    pPack->Len = nPackLen;
    
    //PrintHexDebugLog("打包网络层", m_Pdu, nPackLen);

	return nRet;
}

int EncodeDasNetworkLayer(int NetFlag,REPEATERINFO* stuRepterinfo,BYTEARRAY* pPack, int CommandType)
{
	int nRet = M2G_SUCCESS;

	//m_Pdu.RemoveAll();
	memset(m_Pdu, 0, sizeof(m_Pdu));
	
	//站点编号 ,低字节在前
	m_Pdu[0]=LOBYTE(LOWORD(stuRepterinfo->RepeaterId));
	m_Pdu[1]=HIBYTE(LOWORD(stuRepterinfo->RepeaterId));
	m_Pdu[2]=LOBYTE(HIWORD(stuRepterinfo->RepeaterId));
	m_Pdu[3]=HIBYTE(HIWORD(stuRepterinfo->RepeaterId));
	//设备编号
	m_Pdu[4]=stuRepterinfo->DeviceId;

	//通信包标识号

	m_Pdu[5] = LOBYTE(NetFlag);
	m_Pdu[6] = HIBYTE(NetFlag);
	
	//NP层交互标志
	if(CommandType==RELAPSECOMMAND)
		m_Pdu[7] = 0x00; 
	else
		m_Pdu[7] = 0x80;

	//应用协议标识(APID) 2013.7.15
    m_Pdu[8] = 0x03;

	//应用层数据单元（PDU）
	//m_Pdu.Append(*pPack);
	memcpy(m_Pdu + 9, pPack->pPack, pPack->Len);
	
	int nPackLen = pPack->Len + 9;
	memset(pPack->pPack, 0, nPackLen+1);
    
    memcpy(pPack->pPack, m_Pdu, nPackLen);
    pPack->Len = nPackLen;
    
    //PrintHexDebugLog("打包网络层", m_Pdu, nPackLen);

	return nRet;
}
