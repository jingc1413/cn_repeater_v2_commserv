// Mobile2GControl.cpp: implementation of the CMobile2GControl class.
//
//////////////////////////////////////////////////////////////////////
#include <ebdgdl.h>
#include "mobile2g.h"



///////////////////////////////////////////////////////////
//Decode()
//��������:2GЭ���������
//����:
//  CommType:   Э������
//  pPack:      �������Ĳ���
//  pDecodeOut: �����ĸ�����
//  ObjList:    �����б�
//����ֵ:       �������  M2G_SUCCESS:��ʾ�ɹ� 
//����:xqj
///////////////////////////////////////////////////////////
int Decode_2G(int CommType, BYTEARRAY *pPack , DECODE_OUT *pDecodeOut, OBJECTSTRU *ObjList)
{
	int nRet = M2G_SUCCESS;
	
    memset((char*)pDecodeOut,0,sizeof(DECODE_OUT));
    pDecodeOut->APLayer.ErrorId=1;
	pDecodeOut->APLayer.ErrorCode=M2G_LEN_ERR;//Ĭ������Ϊ����㳤�ȴ�

    pDecodeOut->NPLayer.NetFlag = -1000;//Ĭ��
    if(pPack->Len< 16) //����������С������
    {
        PrintErrorLog(DBG_HERE,"���Ĺ���[%d]ʧ��\n",pPack->Len);
		return M2G_OTHER_ERR;
    }

	if(CommType==M2G_SMS)
	{
		//CMobile2GIntoLayerB Layer;
		nRet = DecodeIntoLayerB(pPack, pDecodeOut, ObjList);
	}
	else if(CommType==M2G_TCPIP)
	{
	    nRet = DecodeIntoLayerC(pPack, pDecodeOut, ObjList);
	}

	return nRet;

}

//2GЭ�����ģ��
//����
//   Э������  CommType M2G_RS232   M2G_DATA   M2G_SMS  M2G_TCPIP       
//   վ����  stuRepeaterInfos
//   �豸���
//   ͨ�Ű���ʶ�� NetFlag = ��ˮ��       //ֻ�е������ظ�����[����]�ظ���ʱ���õ�
//   CommandType ��������
//   UPCOMMAND= 1;             //�豸�����澯���ϱ���--�ظ�
//	 const int QUERYCOMMAND= 2;//��ѯ
//	 const int SETCOMMAND	= 3; //����
//   OBJECTSTRU* objList;          //�����б�  ��������ʾ=UPCOMMAND ��objList=NULL
int Encode_2G(int CommType, int CommandType, int NetFlag, REPEATERINFO*  stuRepeaterInfo, OBJECTSTRU* ObjList,int objSize, BYTEARRAY *pPack )
{
    int nRet = M2G_SUCCESS;
	
	//���ĳ���ά��������  add by qgl 2008-04-03
	//CMobile2GAppLayerA AppLayerA;
    if(CommandType==QUERYCOMMAND)//��ѯ
	{
        nRet=EncodeAppLayer(QUERYCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==SETCOMMAND)//����
	{
		nRet=EncodeAppLayer(SETCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	//���ӹ���ģʽ2008.1.9 begin
	else if(CommandType==FCTPRMQRY)//����������ѯ
	{
	    nRet=EncodeAppLayer(FCTPRMQRY,0xFF,ObjList,objSize,pPack);

	}
	else if(CommandType==FCTPRMSET)//������������
	{
		nRet=EncodeAppLayer(FCTPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMQRY)//���̲�����ѯ
	{
		nRet=EncodeAppLayer(PRJPRMQRY,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMSET)//���̲�������
	{
		nRet=EncodeAppLayer(PRJPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==FACTORYMODE)//���빤��ģʽ
	{
        nRet=EncodeAppLayer(FACTORYMODE,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==0x10) /* update mode */
	{
        nRet=EncodeAppLayer(0x10,0xFF,ObjList,objSize,pPack);
	}
	//����ģʽ end
	else //(CommandType==RELAPSECOMMAND) //��������Ϣ�ָ�
	{
		nRet=EncodeAppLayer(UPCOMMAND,0x00,ObjList,objSize,pPack);
	}
    
	if( (nRet==M2G_SUCCESS)&&(pPack!=NULL) )
	{
        //CMobile2GNetworkLayer NetLayer;
		nRet=EncodeNetworkLayer(NetFlag,stuRepeaterInfo,pPack,CommandType);
		if( (nRet==M2G_SUCCESS)&&(pPack!=NULL) )
		{
			if(CommType==M2G_SMS)
			{
				//CMobile2GIntoLayerB Layer;
				nRet = EncodeIntoLayerB(pPack);
			}
			else if(CommType==M2G_TCPIP)
			{
			    nRet = EncodeIntoLayerC(pPack);
			}
		}
	}
    else
		return M2G_ENCODE_ERR;

	return nRet;
}


//����
//   Э������  CommType M2G_RS232   M2G_DATA   M2G_SMS  M2G_TCPIP       
//   վ����  stuRepeaterInfos
//   �豸���
//   ͨ�Ű���ʶ�� NetFlag = ��ˮ��       //ֻ�е������ظ�����[����]�ظ���ʱ���õ�
//   CommandType ��������
//   UPCOMMAND= 1;             //�豸�����澯���ϱ���--�ظ�
//	 const int QUERYCOMMAND= 2;//��ѯ
//	 const int SETCOMMAND	= 3; //����
//   OBJECTSTRU* objList;          //�����б�  ��������ʾ=UPCOMMAND ��objList=NULL
int Encode_Das(int CommType, int CommandType, int NetFlag, REPEATERINFO*  stuRepeaterInfo, OBJECTSTRU* ObjList,int objSize, BYTEARRAY *pPack )
{
    int nRet = M2G_SUCCESS;
	
	//���ĳ���ά��������  add by qgl 2008-04-03
	//CMobile2GAppLayerA AppLayerA;
    if(CommandType==QUERYCOMMAND)//��ѯ
	{
        nRet=EncodeDasAppLayer(QUERYCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==SETCOMMAND)//����
	{
		nRet=EncodeDasAppLayer(SETCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType == 0xB2){
		nRet=EncodeDasAppLayer(0xB2,0xFF,ObjList,objSize,pPack);
	}
	//���ӹ���ģʽ2008.1.9 begin
	else if(CommandType==FCTPRMQRY)//����������ѯ
	{
	    nRet=EncodeDasAppLayer(FCTPRMQRY,0xFF,ObjList,objSize,pPack);

	}
	else if(CommandType==FCTPRMSET)//������������
	{
		nRet=EncodeDasAppLayer(FCTPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMQRY)//���̲�����ѯ
	{
		nRet=EncodeDasAppLayer(PRJPRMQRY,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMSET)//���̲�������
	{
		nRet=EncodeDasAppLayer(PRJPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==FACTORYMODE)//���빤��ģʽ
	{
        nRet=EncodeDasAppLayer(FACTORYMODE,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==0x10) /* update mode */
	{
        nRet=EncodeDasAppLayer(0x10,0xFF,ObjList,objSize,pPack);
	}
	//����ģʽ end
	else //(CommandType==RELAPSECOMMAND) //��������Ϣ�ָ�
	{
		nRet=EncodeDasAppLayer(UPCOMMAND,0x00,ObjList,objSize,pPack);
	}
    
	if( (nRet==M2G_SUCCESS)&&(pPack!=NULL) )
	{
        //CMobile2GNetworkLayer NetLayer;
		nRet=EncodeDasNetworkLayer(NetFlag,stuRepeaterInfo,pPack,CommandType);
		if( (nRet==M2G_SUCCESS)&&(pPack!=NULL) )
		{
			if(CommType==M2G_SMS)
			{
				//CMobile2GIntoLayerB Layer;
				nRet = EncodeIntoLayerB(pPack);
			}
			else if(CommType==M2G_TCPIP)
			{
			    nRet = EncodeIntoLayerC(pPack);
			}
		}
	}
    else
		return M2G_ENCODE_ERR;

	return nRet;
}
