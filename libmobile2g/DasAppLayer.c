// Mobile2GAppLayerA.cpp: implementation of the CMobile2GAppLayerA class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"

static BYTE m_Pdu[4096];
static int m_Len;
	



//解析应用层协议
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
    
    //PrintHexDebugLog("解析应用层报文", m_Pdu, m_Len);
    
	if(m_Len<2)
	{
	    PrintErrorLog(DBG_HERE,"报文过短[%d]失败\n",m_Len);
		return M2G_OTHER_ERR;
    }
	//命令标识
	pDecodeOut->MAPLayer.CommandFalg = m_Pdu[0];

	//返回结果码
	BYTE m_RetFlag = m_Pdu[1];
    pDecodeOut->MAPLayer.AnswerFlag=m_RetFlag;

	//判断是否有必要往下继续解析
	if(m_RetFlag!=M2G_SUCCESS&&m_RetFlag!=0xFF&&m_RetFlag!=0x01) //
	{

    	pDecodeOut->MAPLayer.ErrorId=1;
		pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//参考返回结果码
		
        PrintErrorLog(DBG_HERE,"返回结果码[%d]失败\n",m_RetFlag);
		return EXCEPTION;
	}
	
	pDecodeOut->MAPLayer.ErrorId=0;
	pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//参考返回结果码
	

	//去掉命令标示和应答标志
    m_Len-=2;
    nIndex+=2;

	while(m_Len - 5>=0)
	{
		//对象长度
		memset(&Item, 0, sizeof(OBJECTSTRU));
		Item.OL = m_Pdu[nIndex]-DAS_MONITOROBJLEN_SIZE-DAS_MONITOROBJID_SIZE;

		if(Item.OL<1||(m_Len<Item.OL+3))
		{
            pDecodeOut->MAPLayer.ErrorId=1;
		    pDecodeOut->MAPLayer.ErrorCode=M2G_LEN_ERR;//参考返回结果码
		    PrintErrorLog(DBG_HERE,"报文过长[%d]失败,实际[%d],索引[%d],对象数[%d]\n", Item.OL, m_Len, nIndex, pDecodeOut->MAPLayer.ObjCount);
			return -1;
		}
		//RemoveAt(m_Pdu, 0);
		m_Len-=1;
        nIndex+=1;
        
		//对象标号
		//对于定时上报进行例外处理 
		DWORD MapId = ReadDWORD(&m_Pdu[nIndex]);
		{
			//MAPID改为4位 2013.7.25
			Item.MapID = MapId;
			//RemoveAt2(m_Pdu, 0, 4);
            m_Len-=4;
            nIndex+=4;
		
			//对象内容
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
	//命令标识
	m_Pdu[index] = (char)CommandType;
	index++;
	//命令标志
	m_Pdu[index] = (char)AnswerType;
	index++;
	

	//还少一步特殊处理
	//设置数据单元内容
	for( i=0; i<ObjSize; i++)
	{
	    //PrintDebugLog(DBG_HERE, "MapID=[%X]\n", ObjList[i].MapID);
        //PrintHexDebugLog("OC=", ObjList[i].OC, ObjList[i].OL);
		memcpy(&Item, &ObjList[i], sizeof(OBJECTSTRU));
		
		//监控对象长度=内容+标号长度
		m_Pdu[index] = Item.OL+DAS_MONITOROBJID_SIZE+DAS_MONITOROBJLEN_SIZE;
		index++;
		//监控对象标号 2013.7.25
		m_Pdu[index] = LOBYTE(LOWORD(Item.MapID));
		index++;
		m_Pdu[index] = HIBYTE(LOWORD(Item.MapID));
		index++;
		m_Pdu[index] = LOBYTE(HIWORD(Item.MapID));
		index++;		
		m_Pdu[index] = HIBYTE(HIWORD(Item.MapID));
		index++;
		//监控对象内容
		for(j=0; j<Item.OL; j++)
		{
			m_Pdu[index] = Item.OC[j];
			index++;		
		}

	}
    memcpy(pPack->pPack, m_Pdu, index);
    pPack->Len = index;
    
    //PrintHexDebugLog("打包应用层", m_Pdu, index);
    return nRet;
}
