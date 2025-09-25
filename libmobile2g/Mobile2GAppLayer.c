// Mobile2GAppLayerA.cpp: implementation of the CMobile2GAppLayerA class.
//
//////////////////////////////////////////////////////////////////////

#include <ebdgdl.h>
#include "mobile2g.h"

static BYTE m_Pdu[4096];
static int m_Len;




//����Ӧ�ò�Э��
int DecodeAppLayer(BYTEARRAY *pPack ,DECODE_OUT* pDecodeOut,OBJECTSTRU* ObjList)
{
    int i, nObjCont=0;
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
    if ((pDecodeOut->MAPLayer.CommandFalg == 0x02 || pDecodeOut->MAPLayer.CommandFalg==0x03) &&
    	pDecodeOut->NPLayer.NPJHFlag >= 0x80)
    {
    	PrintErrorLog(DBG_HERE,"��ѯ�����÷���NP�㽻����־[%d]ʧ��\n", pDecodeOut->NPLayer.NPJHFlag);
        return EXCEPTION;
    }

    pDecodeOut->MAPLayer.ErrorId=0;
    pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//�ο����ؽ����


    //ȥ�������ʾ��Ӧ���־
    m_Len-=2;
    nIndex+=2;

    while(m_Len - 4>=0)
    {
        //���󳤶�
        memset(&Item, 0, sizeof(OBJECTSTRU));
        Item.OL = m_Pdu[nIndex]-M2G_MONITOROBJID_SIZE-M2G_MONITOROBJLEN_SIZE;

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
        WORD MapId = ReadWORD(&m_Pdu[nIndex]);
        /*if(MapId==0x074B)
        {
            m_Len-=2;
            nIndex+=2;
            //����
            //������	CellID	BSIC	BCCH	RxLev	CellID(N1~N6)	BSIC(N1~N6)	BCCH(N1~N6)	RxLev(N1~N6)
            //����		uint2	uint1	uint2	sint1	uint2*6			uint1*6		uint2*6		sint1*6
            //����		2		1		2		1		12				6			12			6
            BYTE *p = &m_Pdu[nIndex];

            //CellID uint2
            memset(&Item, 0, sizeof(OBJECTSTRU));
            Item.MapID = 0x050C;
            Item.OC[0] = *p++;
            Item.OC[1] = *p++;
            Item.OL=2;
            memcpy(&ObjList[0], &Item, sizeof(OBJECTSTRU));
            pDecodeOut->MAPLayer.ObjCount+=1;

            //BSIC uint1
            memset(&Item, 0, sizeof(OBJECTSTRU));
            Item.MapID = 0x0509;
            Item.OC[0] = *p++;
            Item.OL=1;
            memcpy(&ObjList[1], &Item, sizeof(OBJECTSTRU));
            pDecodeOut->MAPLayer.ObjCount+=1;

            //BCCH uint2
            memset(&Item, 0, sizeof(OBJECTSTRU));
            Item.MapID = 0x050A;
            Item.OC[0] = *p++;
            Item.OC[1] = *p++;
            Item.OL=2;
            memcpy(&ObjList[2], &Item, sizeof(OBJECTSTRU));
            pDecodeOut->MAPLayer.ObjCount+=1;

            //RxLev sint1
            memset(&Item, 0, sizeof(OBJECTSTRU));
            Item.MapID = 0x050B;
            Item.OC[0] = *p++;
            Item.OL=1;
            memcpy(&ObjList[3], &Item, sizeof(OBJECTSTRU));
            pDecodeOut->MAPLayer.ObjCount+=1;

            //CellID(N1~N6)
            for(i=0; i<6; i++)
            {
                memset(&Item, 0, sizeof(OBJECTSTRU));
                //CellID uint2
                Item.MapID = 0x0710+i;
                Item.OC[0] = *p++;
                Item.OC[1] = *p++;
                Item.OL=2;
                memcpy(&ObjList[i+4], &Item, sizeof(OBJECTSTRU));
                pDecodeOut->MAPLayer.ObjCount+=1;
            }

            //BSIC(N1~N6)
            for(i=0; i<6; i++)
            {
                memset(&Item, 0, sizeof(OBJECTSTRU));
                Item.MapID = 0x0716+i;
                Item.OC[0] = *p++;
                Item.OL=1;
                memcpy(&ObjList[i+4+6], &Item, sizeof(OBJECTSTRU));
                pDecodeOut->MAPLayer.ObjCount+=1;
            }

            //BCCH(N1~N6) uint2
            for(i=0; i<6; i++)
            {
                memset(&Item, 0, sizeof(OBJECTSTRU));
                Item.MapID = 0x071C+i;
                Item.OC[0] = *p++;
                Item.OC[1] = *p++;
                Item.OL=2;
                memcpy(&ObjList[i+4+6+6], &Item, sizeof(OBJECTSTRU));
                pDecodeOut->MAPLayer.ObjCount+=1;
            }

            //RxLev sint1
            for(i=0; i<6; i++)
            {
                memset(&Item, 0, sizeof(OBJECTSTRU));
                Item.MapID = 0x0722+i;
                Item.OC[0] = *p++;
                Item.OL=1;
                memcpy(&ObjList[i+4+6+6+6], &Item, sizeof(OBJECTSTRU));
                pDecodeOut->MAPLayer.ObjCount+=1;
            }
            //���buf 
            //m_Pdu.RemoveAll();
            memset(m_Pdu, 0, sizeof(m_Pdu));
            m_Len = 0;
        }
        else*/
        {
            Item.MapID = MapId;
            //RemoveAt2(m_Pdu, 0, 2);
            m_Len-=2;
            nIndex+=2;

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

int EncodeAppLayer(int CommandType, int AnswerType, OBJECTSTRU *ObjList, int ObjSize, BYTEARRAY *pPack)
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
        m_Pdu[index] = Item.OL+M2G_MONITOROBJID_SIZE+M2G_MONITOROBJLEN_SIZE;
        index++;
        //��ض�����
        m_Pdu[index] = LOBYTE(Item.MapID);
        index++;
        m_Pdu[index] = HIBYTE(Item.MapID);
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


/* struct of MCP:B */
typedef struct _OBJECT_B
{   
    int OL;
    int MapID;
    char OC[256];
}OBJECTSTRU_B;

/**
 * Decode MCP:B
 */
int DecodeAppLayerB(BYTEARRAY *pPack ,DECODE_OUT* pDecodeOut,OBJECTSTRU* ObjList)
{
    int nObjCont=0;
    int nRet = M2G_SUCCESS;
    int nIndex=0;
    OBJECTSTRU_B Item;

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

    /* check returnFlag */
    if(m_RetFlag!=0x00 && m_RetFlag!=0xFF && m_RetFlag!=0x01)
    {
        pDecodeOut->MAPLayer.ErrorId=1;
        pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//�ο����ؽ����

        PrintErrorLog(DBG_HERE,"���ؽ����[%d]ʧ��\n",m_RetFlag);
        return EXCEPTION;
    }

    pDecodeOut->MAPLayer.ErrorId=0;
    pDecodeOut->MAPLayer.ErrorCode=m_RetFlag;//�ο����ؽ����

    /* ignore the CommandFlag and ReturnFlag */
    m_Len-=2;
    nIndex+=2;

    while(m_Len - 4>=0)
    {
        memset(&Item, 0, sizeof(OBJECTSTRU));
        Item.OL = ReadWORD(&m_Pdu[nIndex]) - 2 - 2; /* exclude the length of fields 'object length' and 'object id' */

        if(Item.OL<1||(m_Len<Item.OL+3))
        {
            pDecodeOut->MAPLayer.ErrorId=1;
            pDecodeOut->MAPLayer.ErrorCode=M2G_LEN_ERR;//�ο����ؽ����
            PrintErrorLog(DBG_HERE,"���Ĺ���[%d]ʧ��,ʵ��[%d],����[%d],������[%d]\n", Item.OL, m_Len, nIndex, pDecodeOut->MAPLayer.ObjCount);
            return -1;
        }
        m_Len -= 2;
        nIndex += 2;

        Item.MapID = ReadWORD(&m_Pdu[nIndex]);

        m_Len-=2;
        nIndex+=2;

        /* fetch Object content */
        memcpy(Item.OC, &m_Pdu[nIndex], Item.OL);

        m_Len -= Item.OL;
        nIndex += Item.OL;

        //add to objectlist
        memcpy(&ObjList[nObjCont], &Item, sizeof(OBJECTSTRU));
        nObjCont++;
        //add objectcount
        pDecodeOut->MAPLayer.ObjCount+=1;
    }

    return nRet;
}

/**
 * Encode MCP:B
 */
int EncodeAppLayerB(int CommandType, int AnswerType, OBJECTSTRU *ObjList, int ObjSize, BYTEARRAY *pPack)
{
    int i, j;
    int index=0;
    int nRet = M2G_SUCCESS;
    OBJECTSTRU_B Item;

    memset(m_Pdu, 0, sizeof(m_Pdu));
    //�����ʶ
    m_Pdu[index] = CommandType;
    index++;
    //�����־
    m_Pdu[index] = AnswerType;
    index++;

    for( i=0; i<ObjSize; i++)
    {
        //PrintDebugLog(DBG_HERE, "MapID=[%X]\n", ObjList[i].MapID);
        //PrintHexDebugLog("OC=", ObjList[i].OC, ObjList[i].OL);
        memcpy(&Item, &ObjList[i], sizeof(OBJECTSTRU));

        //��ض��󳤶�=����+��ų���
        m_Pdu[index] = (Item.OL + 2 + 2) & 0xFF;
        m_Pdu[index+1] = (Item.OL + 2 + 2) & 0xFF00 >> 8;
        index += 2;

        //��ض�����
        m_Pdu[index] = LOBYTE(Item.MapID);
        index++;
        m_Pdu[index] = HIBYTE(Item.MapID);
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


