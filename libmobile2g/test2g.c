#include <ebdgdl.h>
#include "mobile2g.h"


int strHexToInt(char* strSource) 
{ 
    int nTemp=0; 
    int i,j,len,flen; 

    len = strlen(strSource); 
    flen = --len; 
    for(i = 0; i <= len; i++) 
    { 
        if(strSource[i] > 'g' || strSource[i] < '0' || ( strSource[i] > '9' && strSource[i] < 'A' ) ) 
        { 
            PrintErrorLog(DBG_HERE,"请输入正确的16进制字符串!输入错误 [%s]\n", strSource);
            return -1; 
        } 
        else 
        { 
            int nDecNum; 
            switch(strSource[i]) 
            { 
                case 'a': 
                case 'A': nDecNum = 10; break; 
                case 'b': 
                case 'B': nDecNum = 11; break; 
                case 'c': 
                case 'C': nDecNum = 12; break; 
                case 'd': 
                case 'D': nDecNum = 13; break; 
                case 'e': 
                case 'E': nDecNum = 14; break; 
                case 'f': 
                case 'F': nDecNum = 15; break; 
                case '0': 
                case '1': 
                case '2': 
                case '3': 
                case '4': 
                case '5': 
                case '6': 
                case '7': 
                case '8': 
                case '9': nDecNum = strSource[i] - '0'; break; 
                default: return 0; 
            } 
        
            for(j = flen; j > 0; j-- ) 
            { 
                nDecNum *= 16; 
            } 
            flen--; 
            nTemp += nDecNum; 
        
        } 
    } 
    return nTemp;
}


char *HexToAsc(char *dest, char *src, int src_len)
{
    int i;
    char szTemp[10];

    for(i = 0; i < src_len/2; i ++)
    {
        bufclr(szTemp);
        memcpy(szTemp, src, 2);
        printf("[%s]", szTemp);
        dest[i]=strHexToInt(szTemp);
        src += 2;
    }
    return dest;
}



int main()
{
    char szHexStr[4096];
    char szMapData[1024];
    char szMsgCont[2048];
    char szTemp[10];
	DECODE_OUT Decodeout;
	REPEATERINFO struRepeInfo;
	OBJECTSTRU struObject[100];
	int n2G_QB;
	int objsize, i;
    BYTEARRAY struPack;
    INT nConnectFd;
    int iRet;
    
    PSTR pszSeperateStr[100];  /* 分割字符数组*/
    char szParam[1000];
    
    bufclr(szHexStr);
    bufclr(szMsgCont);
    //7E03010100150DFF0080800101FF044101C8A50900020102000300040005000A00100018002000D007D107010102011101120113011401150120013001310133013601370150016001610162016301640165016601010204020107010304030407070708070A070B070C070D070E070F07A007A107A207A307A407B007B107B207B307B407B507B607B707B807B907BA07BB07BC07BD07BE07BF07C007C107C207C307C407C507C607E107E207E307E407E507E607E707AA287E
    strcpy(szHexStr, "20217E03010100150DFF0080800101FF044101200A500120090226110011050B0708000401030004040300040407010508051458052807CC0104070500050C057F4B04090514050A053E00040B05CC043B0730043C0730051007F3470416070C051C075900042207CE045007000456070005110700000417073B051D075700042307BA0451070004570700051207F49404180721051E075000042407B804520700045807000513072F9A0419070F051F075B00042507B804530700045907000514070000041A07000520075500042607B004540700045A07000515074C6F041B07000521074D00042707AF04550700045B07005E7D307E20");
    HexToAsc(szMsgCont, szHexStr, strlen(szHexStr));
    PrintHexDebugLog("转换为", szMsgCont, strlen(szHexStr)/2);
    
    if((nConnectFd = CreateConnectSocket("127.0.0.1", 9999, 30)) < 0)
	{
		PrintErrorLog(DBG_HERE, \
			"同应用服务程序建立连接错误,请确信applserv已经启动\n");
		return EXCEPTION;
	}
	if(SendSocketNoSync(nConnectFd, szMsgCont, strlen(szHexStr)/2, 30) != NORMAL)
	{
	    	PrintErrorLog(DBG_HERE, "返回渠道应答报文错误!\n");
	    	return EXCEPTION;
	}
	char szCaReqBuffer[1024];
	bufclr(szCaReqBuffer);
	iRet = RecvSocketNoSync(nConnectFd, szCaReqBuffer, sizeof(szCaReqBuffer)-1, 30);
	if (iRet < 0)
	{
	    	PrintErrorLog(DBG_HERE, "接收渠道请求报文[%s]失败\n", szCaReqBuffer);
	    	return EXCEPTION;
	}
	close(nConnectFd);
	PrintHexDebugLog("应答", szCaReqBuffer, iRet);
	 
    struPack.pPack = szMsgCont;
	struPack.Len = strlen(szHexStr)/2;
	
	
    if (Decode_2G(M2G_TCPIP, &struPack, &Decodeout, struObject) != NORMAL)
    {
    	printf("解析设备内容错误\n");
		return -1; 
    }
    
    PrintDebugLog(DBG_HERE, "解析2G协议成功,协议类型[%02d],承载协议类型[%02d],错误代码[%d],站点编号[%d],设备编号[%d],网络标识[%d],对象数[%d]\n",
        Decodeout.APLayer.ProtocolType,  Decodeout.APLayer.NPType, Decodeout.APLayer.ErrorCode, 
        Decodeout.NPLayer.structRepeater.RepeaterId, Decodeout.NPLayer.structRepeater.DeviceId,
        Decodeout.NPLayer.NetFlag, Decodeout.MAPLayer.ObjCount);
    
    
    
    for(i=0; i< Decodeout.MAPLayer.ObjCount; i++)
    {
        PrintDebugLog(DBG_HERE, "MapID=[%04X]\n", struObject[i].MapID);
        PrintHexDebugLog("OC=", struObject[i].OC, struObject[i].OL);
    }
    
    /*
    strcpy(szParam, "0201 0204 0701 0301 0304 0704 0707 0708 070A 070B");
    
    Decodeout.MAPLayer.ObjCount = SeperateStringWithChar(szParam, ' ',pszSeperateStr, 100);
    for(i=0; i< Decodeout.MAPLayer.ObjCount; i++)
    {
        memset(&struObject[i], 0, sizeof(OBJECTSTRU));
		struObject[i].OL = 1;
		struObject[i].MapID = strHexToInt(pszSeperateStr[i]);
		
        PrintDebugLog(DBG_HERE, "MapID=[%X]\n", struObject[i].MapID);
        PrintHexDebugLog("OC=", struObject[i].OC, struObject[i].OL);
    }
    */
    /*
    n2G_QB = Decodeout.NPLayer.NetFlag;
    struRepeInfo.DeviceId= Decodeout.NPLayer.structRepeater.DeviceId;
	struRepeInfo.RepeaterId= Decodeout.NPLayer.structRepeater.RepeaterId;
	
	//对象数
	objsize = Decodeout.MAPLayer.ObjCount;	
    memset(szMsgCont, 0, sizeof(szMsgCont));
    
	if (Encode_2G(M2G_SMS, QUERYCOMMAND, n2G_QB, &struRepeInfo, struObject,  objsize, &struPack) != NORMAL)
	{
	    printf("解析设备内容错误\n");
		//return -1; 
	}
	*/
	
	     int nUpQB=Decodeout.NPLayer.NetFlag;
	     int nObjCount=1; //回复报文为1
	     
	     memset(szMsgCont, 0, sizeof(szMsgCont));
	     struPack.pPack = szMsgCont;
	     struPack.Len = 0;
	     memset(&struObject, 0, sizeof(OBJECTSTRU));
	     struObject[0].MapID=0x0150;
	     struObject[0].OL=0x07;
	     strcpy(szMapData, GetSysDateTime());
	     bufclr(szTemp);
	     strncpy(szTemp, szMapData, 2);
	     printf("szTemp[%s]\n", szTemp);
         struObject[0].OC[0] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+2, 2);
         printf("szTemp[%s]\n", szTemp);
         struObject[0].OC[1] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+5, 2);
         printf("szTemp[%s]\n", szTemp);
         struObject[0].OC[2] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+8, 2);
         struObject[0].OC[3] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+11, 2);
         struObject[0].OC[4] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+14, 2);
         struObject[0].OC[5] = strHexToInt(szTemp);
         bufclr(szTemp);
         strncpy(szTemp, szMapData+17, 2);
         struObject[0].OC[6] = strHexToInt(szTemp);
         
	     
	     if (Encode_2G(M2G_TCPIP, RELAPSECOMMAND, nUpQB, &Decodeout.NPLayer.structRepeater, struObject, nObjCount, &struPack) != NORMAL)
	     {
	         PrintErrorLog(DBG_HERE, "回复主动上报打包错误\n");
		     return EXCEPTION; 
	     }
	     PrintHexDebugLog("OC=", szMsgCont, struPack.Len);
	     
	//printf("设备内容为[%s]\n", szMsgCont);
	
	return 0;
}


