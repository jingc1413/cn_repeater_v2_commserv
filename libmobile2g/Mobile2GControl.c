// Mobile2GControl.cpp: implementation of the CMobile2GControl class.
//
//////////////////////////////////////////////////////////////////////
#include <ebdgdl.h>
#include "mobile2g.h"



///////////////////////////////////////////////////////////
//Decode()
//功能描述:2G协议解析函数
//参数:
//  CommType:   协议类型
//  pPack:      待解析的参数
//  pDecodeOut: 解析的各层结果
//  ObjList:    对象列表
//返回值:       解析结果  M2G_SUCCESS:表示成功 
//作者:xqj
///////////////////////////////////////////////////////////
int Decode_2G(int CommType, BYTEARRAY *pPack , DECODE_OUT *pDecodeOut, OBJECTSTRU *ObjList)
{
	int nRet = M2G_SUCCESS;
	
    memset((char*)pDecodeOut,0,sizeof(DECODE_OUT));
    pDecodeOut->APLayer.ErrorId=1;
	pDecodeOut->APLayer.ErrorCode=M2G_LEN_ERR;//默认设置为接入层长度错

    pDecodeOut->NPLayer.NetFlag = -1000;//默认
    if(pPack->Len< 16) //首先满足最小包需求
    {
        PrintErrorLog(DBG_HERE,"报文过短[%d]失败\n",pPack->Len);
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

//2G协议组包模块
//参数
//   协议类型  CommType M2G_RS232   M2G_DATA   M2G_SMS  M2G_TCPIP       
//   站点编号  stuRepeaterInfos
//   设备编号
//   通信包标识号 NetFlag = 流水号       //只有当报警回复或者[其它]回复的时候用到
//   CommandType 命令类型
//   UPCOMMAND= 1;             //设备主动告警（上报）--回复
//	 const int QUERYCOMMAND= 2;//查询
//	 const int SETCOMMAND	= 3; //设置
//   OBJECTSTRU* objList;          //对象列表  如果命令标示=UPCOMMAND 则objList=NULL
int Encode_2G(int CommType, int CommandType, int NetFlag, REPEATERINFO*  stuRepeaterInfo, OBJECTSTRU* ObjList,int objSize, BYTEARRAY *pPack )
{
    int nRet = M2G_SUCCESS;
	
	//更改成三维工程命令  add by qgl 2008-04-03
	//CMobile2GAppLayerA AppLayerA;
    if(CommandType==QUERYCOMMAND)//查询
	{
        nRet=EncodeAppLayer(QUERYCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==SETCOMMAND)//设置
	{
		nRet=EncodeAppLayer(SETCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	//增加工厂模式2008.1.9 begin
	else if(CommandType==FCTPRMQRY)//工厂参数查询
	{
	    nRet=EncodeAppLayer(FCTPRMQRY,0xFF,ObjList,objSize,pPack);

	}
	else if(CommandType==FCTPRMSET)//工厂参数设置
	{
		nRet=EncodeAppLayer(FCTPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMQRY)//工程参数查询
	{
		nRet=EncodeAppLayer(PRJPRMQRY,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMSET)//工程参数设置
	{
		nRet=EncodeAppLayer(PRJPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==FACTORYMODE)//进入工厂模式
	{
        nRet=EncodeAppLayer(FACTORYMODE,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==0x10) /* update mode */
	{
        nRet=EncodeAppLayer(0x10,0xFF,ObjList,objSize,pPack);
	}
	//工厂模式 end
	else //(CommandType==RELAPSECOMMAND) //报警等信息恢复
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


//参数
//   协议类型  CommType M2G_RS232   M2G_DATA   M2G_SMS  M2G_TCPIP       
//   站点编号  stuRepeaterInfos
//   设备编号
//   通信包标识号 NetFlag = 流水号       //只有当报警回复或者[其它]回复的时候用到
//   CommandType 命令类型
//   UPCOMMAND= 1;             //设备主动告警（上报）--回复
//	 const int QUERYCOMMAND= 2;//查询
//	 const int SETCOMMAND	= 3; //设置
//   OBJECTSTRU* objList;          //对象列表  如果命令标示=UPCOMMAND 则objList=NULL
int Encode_Das(int CommType, int CommandType, int NetFlag, REPEATERINFO*  stuRepeaterInfo, OBJECTSTRU* ObjList,int objSize, BYTEARRAY *pPack )
{
    int nRet = M2G_SUCCESS;
	
	//更改成三维工程命令  add by qgl 2008-04-03
	//CMobile2GAppLayerA AppLayerA;
    if(CommandType==QUERYCOMMAND)//查询
	{
        nRet=EncodeDasAppLayer(QUERYCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==SETCOMMAND)//设置
	{
		nRet=EncodeDasAppLayer(SETCOMMAND,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType == 0xB2){
		nRet=EncodeDasAppLayer(0xB2,0xFF,ObjList,objSize,pPack);
	}
	//增加工厂模式2008.1.9 begin
	else if(CommandType==FCTPRMQRY)//工厂参数查询
	{
	    nRet=EncodeDasAppLayer(FCTPRMQRY,0xFF,ObjList,objSize,pPack);

	}
	else if(CommandType==FCTPRMSET)//工厂参数设置
	{
		nRet=EncodeDasAppLayer(FCTPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMQRY)//工程参数查询
	{
		nRet=EncodeDasAppLayer(PRJPRMQRY,0xFF,ObjList,objSize,pPack);
	}
	else if(CommandType==PRJPRMSET)//工程参数设置
	{
		nRet=EncodeDasAppLayer(PRJPRMSET,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==FACTORYMODE)//进入工厂模式
	{
        nRet=EncodeDasAppLayer(FACTORYMODE,0xFF,ObjList,objSize,pPack);
	}
	else if (CommandType==0x10) /* update mode */
	{
        nRet=EncodeDasAppLayer(0x10,0xFF,ObjList,objSize,pPack);
	}
	//工厂模式 end
	else //(CommandType==RELAPSECOMMAND) //报警等信息恢复
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
