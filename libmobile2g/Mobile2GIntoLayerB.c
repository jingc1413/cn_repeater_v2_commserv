// Mobile2GIntoLayerB.cpp: implementation of the CMobile2GIntoLayerB class.
//
//////////////////////////////////////////////////////////////////////
#include <ebdgdl.h>
#include "mobile2g.h"
//#include "crc.h"


//定义起止标志
const BYTE ILB_STX				= '!';
const BYTE ILB_ETX				= '!';
//协议类型
const BYTE ILB_PROTOCOLTYPE		= 0x02;
//承载协议类型
const BYTE ILB_NPPROTOCOLTYPE	= 0x01;

static BYTE m_Pdu[4096];
static int m_Len;

#define CRC_PRESET		0x0000
#define CRC_POLYNOM		0x1021


static unsigned short crc;


//现在直放站用的CRC的算法 2002/8/27
WORD Get_CRC(WORD count, BYTE *p)
{
	int i, j;
	WORD word;
	crc = CRC_PRESET;
	BYTE* bt;
	//bt = new BYTE[count];
	bt = (BYTE *)malloc(count);
	for(i=0;i<count;i++)
		bt[i]=*(p+i);
	BYTE *pp=bt;
	for(i=0; i<count; i++, pp++)
	{
		for(j=0; j<8; j++)
		{
			word = crc^(((WORD)*pp)<<8);
			crc <<= 1;
			if((word & 0x8000)==0x8000)
			{
				crc ^= CRC_POLYNOM;
			}
			*pp <<=1;
		}
	}

	free(bt);
	//PrintDebugLog(DBG_HERE,"产生返回crc值[%d]\n", crc);
	return crc;
}


//ASCII码拆分处理(ASCII专程成HEX)
BOOL Esc(BYTEARRAY *Pack)
{
 	//对除起始标志和结束标志外的所有数据转义
	int len = (Pack->Len-2)*2+2;
	BYTE pdu[len+1];
    
    memset(pdu, 0, len+1);
       
	BYTE Hivalue,Lovalue;
	int i, j;
	for(i=1, j=1; i<m_Len-1; i++)
	{
		Hivalue=m_Pdu[i]/16;
        Lovalue=m_Pdu[i]%16;
        Hivalue=Hivalue<10?Hivalue+48:Hivalue+55;
		Lovalue=Lovalue<10?Lovalue+48:Lovalue+55;
		pdu[j++]=Hivalue;
		pdu[j++]=Lovalue;
	}
	pdu[0] = ILB_STX;
	pdu[j] = ILB_ETX;
	
	//清除递进来的数据
    memset(Pack->pPack, 0, len);
	Pack->Len = len;
	memcpy(Pack->pPack, pdu, len);
	PrintDebugLog(DBG_HERE, "生成2G协议报文[%s]\n", Pack->pPack);
	return TRUE;
}
//反向ASCII码拆分处理
//Pack 待解析的数据
//m_Pdu 为反向后的结果
BOOL UnEsc(BYTEARRAY *Pack)
{

	//m_Pdu[0] = Pack->pPack[0];

	int Hivalue,Lovalue,temp;
	int i, j;
	for(i=1,j=0; i<Pack->Len-1; i++)
	{
		temp=Pack->pPack[i];
		Hivalue=(temp>=48&&temp<=57)?temp-48:temp-55;
		temp=Pack->pPack[i+1];
	 	Lovalue=(temp>=48&&temp<=57)?temp-48:temp-55;
		m_Pdu[j] = (Hivalue*16+Lovalue);
		j++;
		i++;
	}
	//m_Pdu[j] = Pack->pPack[Pack->Len-1];

	m_Len = j;
    
    //PrintHexDebugLog("解析接入层报文", m_Pdu, m_Len);
	return TRUE;
}

int DecodeIntoLayerB(BYTEARRAY *pPack, DECODE_OUT *pDecodeOut, OBJECTSTRU *ObjList)
{   
    int nRet = M2G_SUCCESS;
	if(pPack->Len<10)
		return M2G_OTHER_ERR;
    
    memset(m_Pdu, 0, sizeof(m_Pdu));
	m_Len = 0;
	
	if(pPack->pPack[0]!=ILB_STX)
	{
		return -1;		//M2G_DECODE_ERR;
	}
	//先转义,并去掉STX和ETX
    if(!UnEsc(pPack))
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
		PrintErrorLog(DBG_HERE,"接入层B:CRC校验错[%d]\n",crc);
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



int EncodeIntoLayerB(BYTEARRAY *pPack)
{
    int nRet = M2G_SUCCESS;
    
    /****做CRC校验*****/
	char szPdu[4096];

	//协议类型
	szPdu[0] = ILB_PROTOCOLTYPE;
	//承载协议类型
	szPdu[1] = ILB_NPPROTOCOLTYPE;
	memcpy(szPdu + 2, pPack->pPack, pPack->Len);
		
	WORD crc = Get_CRC(pPack->Len + 2, szPdu);
	
    memset(m_Pdu, 0, sizeof(m_Pdu));
	// 	起始标志
	m_Pdu[0] = ILB_STX;
    memcpy(m_Pdu + 1, szPdu, pPack->Len + 2);
    m_Len = pPack->Len + 3;
    
	// 	校验单元
	m_Pdu[m_Len] = LOBYTE(crc);
	m_Len++;
	m_Pdu[m_Len] = HIBYTE(crc);
	m_Len++;
	//结束标志
	m_Pdu[m_Len] = ILB_ETX;
	m_Len++;
    
    memset(pPack->pPack, 0, m_Len +1);
    pPack->Len = m_Len;
    memcpy(pPack->pPack, m_Pdu, m_Len);
    //PrintHexDebugLog("打包接入层", m_Pdu, m_Len);
    
	//ASCII拆分处理
	Esc(pPack);

	//拷贝结果
	//pPack->Copy(Pack);
	
	return nRet;
}
