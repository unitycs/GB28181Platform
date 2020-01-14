#include "StdAfx.h"
#include "tinyXML/tinyxml2.h"
#include "BodyBuilder.h"
#include "Common/common.h"
#include "Main/UnifiedMessage.h"

int CBodyBuilder::Init()
{
	return 0;
}

// 创建Keepalive的SIP数据包
int CBodyBuilder::CreateKeepaliveBody(char *pszKeepaliveBuf, int nBufLen, const char *pszSN, const char *pszDevID)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Notify>\r\n"
		"<CmdType>Keepalive</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strEndTag = "</DeviceID>\r\n"
		"<Status>OK</Status>\r\n"
		"</Notify>\r\n";

	int nSNLen = strlen(pszSN);
	if (nBufLen < strVersion.GetLength() + strSNTag.GetLength() + strEndTag.GetLength() + nSNLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	// 攒保活数据包的包体
	int nOffset = 0;
	memcpy(pszKeepaliveBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszKeepaliveBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszKeepaliveBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszKeepaliveBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszKeepaliveBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);
	return 0;
}

int CBodyBuilder::CreateAlarmBodyNew(char *pszRecordBuf, int nBufLen, const char *pszSN, const char *pszDeviceID,							// 设备ID
	const char *alarmPriority,
	const char *alarmMethod,
	const char *alarmTime,
	const char *alarmDescription,
	const char *longitude,
	const char *latitude,
	const char *alarmType,
	const char *alarmStatus)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Notify>\r\n"
		"<CmdType>Alarm</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceTag = "</DeviceID>\r\n";

	static CString strAlarmPriorityTagS = "<AlarmPriority>";
	static CString strAlarmPriorityTagE = "</AlarmPriority>\r\n";

	static CString strAlarmMethodTagS = "<AlarmMethod>";
	static CString strAlarmMethodTagE = "</AlarmMethod>\r\n";

	static CString strAlarmTimeTagS = "<AlarmTime>";
	static CString strAlarmTimeTagE = "</AlarmTime>\r\n";

	static CString strAlarmDescriptionTagS = "<AlarmDescription>";
	static CString strAlarmDescriptionTagE = "</AlarmDescription>\r\n"
		"<Longitude>";

	static CString strLongitudeTag = "</Longitude>\r\n"
		"<Latitude>";

	static CString strLatitudeTag = "</Latitude>\r\n"
		"<AlarmType>";

	static CString strAlarmTypeTag = "</AlarmType>\r\n"
		"<AlarmStatus>";

	static CString strAlarmStatusTag = "</AlarmStatus>\r\n"
		"</Notify>\r\n";
	static int nStaitcCharLen = strVersion.GetLength() + strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strAlarmPriorityTagS.GetLength() +
		strAlarmPriorityTagE.GetLength() +
		strAlarmMethodTagS.GetLength() +
		strAlarmMethodTagE.GetLength() +
		strAlarmTimeTagS.GetLength() +
		strAlarmTimeTagE.GetLength() +
		strAlarmDescriptionTagS.GetLength() +
		strAlarmDescriptionTagE.GetLength() +
		strLatitudeTag.GetLength() +
		strLongitudeTag.GetLength() +
		strAlarmTypeTag.GetLength() +
		strAlarmStatusTag.GetLength();

	if (nBufLen <= nStaitcCharLen)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加Item标签
	memcpy(pszRecordBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();

	if (pszSN)
	{
		memcpy(pszRecordBuf + nOffset, pszSN, strlen(pszSN));
		nOffset += strlen(pszSN);
	}

	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	/* memcpy(pszRecordBuf+nOffset, strAlarmPriorityTagE.GetString(), strAlarmPriorityTagE.GetLength());
	 nOffset += strAlarmPriorityTagE.GetLength();*/

	if (pszDeviceID && '\0' != pszDeviceID[0])
	{
		// 添加设备ID
		memcpy(pszRecordBuf + nOffset, pszDeviceID, ID_BUF_LEN);
		nOffset += ID_LEN;

		// 添加设备ID标签
	}
	memcpy(pszRecordBuf + nOffset, strDeviceTag.GetString(), strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();

	memcpy(pszRecordBuf + nOffset, strAlarmPriorityTagS.GetString(), strAlarmPriorityTagS.GetLength());
	nOffset += strAlarmPriorityTagS.GetLength();

	if (alarmPriority)
	{
		memcpy(pszRecordBuf + nOffset, alarmPriority, strlen(alarmPriority));
		nOffset += strlen(alarmPriority);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmPriorityTagE.GetString(), strAlarmPriorityTagE.GetLength());
	nOffset += strAlarmPriorityTagE.GetLength();

	memcpy(pszRecordBuf + nOffset, strAlarmMethodTagS.GetString(), strAlarmMethodTagS.GetLength());
	nOffset += strAlarmMethodTagS.GetLength();

	if (alarmMethod)
	{
		memcpy(pszRecordBuf + nOffset, alarmMethod, strlen(alarmMethod));
		nOffset += strlen(alarmMethod);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmMethodTagE.GetString(), strAlarmMethodTagE.GetLength());
	nOffset += strAlarmMethodTagE.GetLength();

	memcpy(pszRecordBuf + nOffset, strAlarmTimeTagS.GetString(), strAlarmTimeTagS.GetLength());
	nOffset += strAlarmTimeTagS.GetLength();

	if (alarmTime)
	{
		memcpy(pszRecordBuf + nOffset, alarmTime, strlen(alarmTime));
		nOffset += strlen(alarmTime);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmTimeTagE.GetString(), strAlarmTimeTagE.GetLength());
	nOffset += strAlarmTimeTagE.GetLength();

	//strAlarmDescriptionTagS
	memcpy(pszRecordBuf + nOffset, strAlarmDescriptionTagS.GetString(), strAlarmDescriptionTagS.GetLength());
	nOffset += strAlarmDescriptionTagS.GetLength();

	if (alarmDescription)
	{
		memcpy(pszRecordBuf + nOffset, alarmDescription, strlen(alarmDescription));
		nOffset += strlen(alarmDescription);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmDescriptionTagE.GetString(), strAlarmDescriptionTagE.GetLength());
	nOffset += strAlarmDescriptionTagE.GetLength();

	if (longitude)
	{
		memcpy(pszRecordBuf + nOffset, longitude, strlen(longitude));
		nOffset += strlen(longitude);
	}

	// 添加Secrecy标签 strLatitudeTag
	memcpy(pszRecordBuf + nOffset, strLongitudeTag.GetString(), strLongitudeTag.GetLength());
	nOffset += strLongitudeTag.GetLength();

	if (latitude)
	{
		memcpy(pszRecordBuf + nOffset, latitude, strlen(latitude));
		nOffset += strlen(latitude);
	}

	memcpy(pszRecordBuf + nOffset, strLatitudeTag.GetString(), strLatitudeTag.GetLength());
	nOffset += strLatitudeTag.GetLength();

	if (alarmType)
	{
		memcpy(pszRecordBuf + nOffset, alarmType, strlen(alarmType));
		nOffset += strlen(alarmType);
	}
	// 添加Type
	memcpy(pszRecordBuf + nOffset, strAlarmTypeTag.GetString(), strAlarmTypeTag.GetLength());
	nOffset += strAlarmTypeTag.GetLength();

	if (alarmStatus)
	{
		memcpy(pszRecordBuf + nOffset, alarmStatus, strlen(alarmStatus));
		nOffset += strlen(alarmStatus);
	}

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strAlarmStatusTag.GetString(), strAlarmStatusTag.GetLength() + 1);
	//nBufLen = nOffset + strAlarmStatusTag.GetLength() + 1;

	return 0;
}

int CBodyBuilder::createSubscribeResponse(char *pRecordBuf, int nBufLen, const char *pszSn, const char *pszDeviceId, char *cmdType, char *result)
{
	char *xmlStr = "<?xml version=\"1.0\" ?><Response>\r\n<CmdType>%s</CmdType>\r\n<SN>%s</SN>\r\n<DeviceID>%s</DeviceID>\r\n<Result>%s</Result>\r\n</Response>\r\n";
	sprintf_s(pRecordBuf, nBufLen, xmlStr, cmdType, pszSn, pszDeviceId, result);
	return 1;
}
// 创建报价上报的数据包体
int CBodyBuilder::CreateAlarmBody(char *pszAlarmBuf, int nBufLen, const char *pszSN,
	const char *pszDevID, const char *pszDescription, BYTE nLevel, const char *pszTime) const
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Notify>\r\n"
		"<CmdType>Alarm</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<AlarmPriority>";

	static CString strPriorityTag = "</AlarmPriority>\r\n"
		"<AlarmMethod>2</AlarmMethod>\r\n"
		"<AlarmDescription>";

	static CString strDescriptionTag = "</AlarmDescription>\r\n"
		"<AlarmTime>";
	static CString strEndTag = "</AlarmTime>\r\n"
		"</Notify>\r\n";

	char szGrade[8];
	_itoa_s(nLevel, szGrade, 10);

	// 取得参数字符串长度
	int nDescriptionLen = strlen(pszDescription);
	int nSNLen = strlen(pszSN);
	int nGradeLen = strlen(szGrade);
	int nTimeLen = strlen(pszTime);

	// 判断缓存是否够大
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDescriptionTag.GetLength() +
		strPriorityTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen +
		ID_BUF_LEN +
		nGradeLen +
		nDescriptionLen)
		return ERROR_NOACCESS;

	// 攒报警数据包的包体
	int nOffset = 0;
	memcpy(pszAlarmBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszAlarmBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszAlarmBuf + nOffset, strDeviceIDTag, strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, szGrade, nGradeLen);
	nOffset += nGradeLen;
	memcpy(pszAlarmBuf + nOffset, strPriorityTag, strPriorityTag.GetLength());
	nOffset += strPriorityTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszDescription, nDescriptionLen);
	nOffset += nDescriptionLen;
	memcpy(pszAlarmBuf + nOffset, strDescriptionTag, strDescriptionTag.GetLength());
	nOffset += strDescriptionTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszTime, nTimeLen);
	nOffset += nTimeLen;
	memcpy(pszAlarmBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);
	return 0;
}

int CBodyBuilder::AddStatusReportBody(char* pszAlarmBuf, int& nBufLen, const char* pszDevID, const char* pszName, const char* pszParentID, const char* pszStatus)
{
	static CString strItemTag = "<Item>\r\n"
		"<DeviceID>";
	static CString strDeviceIDTag = "</DeviceID>\r\n";
	static CString strNameTag = "<Name>";
	static CString strNameEndTag = "</Name>\r\n";
	static CString strEventTag = "<Event>";
	static CString strEventEndTag = "</Event>\r\n";
	static CString strItemEndTag = "</Item>\r\n";

	static CString strParentIDTag = "<ParentID>";
	static CString strParentIDEndTag = "</ParentID>\r\n";

	// 取得参数字符串长度
	int nEventLen = 0;
	if (pszStatus)
		nEventLen = strlen(pszStatus);
	int nNameLen = 0;
	if (pszName)
		nNameLen = strlen(pszName);
	int nIDLen = strlen(pszDevID);

	int nParentIDLen = 0;
	if (pszParentID)
		nParentIDLen = strlen(pszParentID);

	// 判断缓存是否够大
	if (nBufLen <
		strItemTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strEventTag.GetLength() +
		strEventEndTag.GetLength() +
		ID_BUF_LEN +
		nNameLen +
		nEventLen)
		return ERROR_NOACCESS;

	// 攒报警数据包的包体
	int nOffset = 0;
	memcpy(pszAlarmBuf, strItemTag.GetString(), strItemTag.GetLength());
	nOffset += strItemTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszDevID, nIDLen);
	nOffset += nIDLen;
	memcpy(pszAlarmBuf + nOffset, strDeviceIDTag, strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	if (0 < nNameLen)
	{
		memcpy(pszAlarmBuf + nOffset, strNameTag, strNameTag.GetLength());
		nOffset += strNameTag.GetLength();
		memcpy(pszAlarmBuf + nOffset, pszName, nNameLen);
		nOffset += nNameLen;
		memcpy(pszAlarmBuf + nOffset, strNameEndTag, strNameEndTag.GetLength());
		nOffset += strNameEndTag.GetLength();
	}
	if (0 < nEventLen)
	{
		memcpy(pszAlarmBuf + nOffset, strEventTag, strEventTag.GetLength());
		nOffset += strEventTag.GetLength();
		memcpy(pszAlarmBuf + nOffset, pszStatus, nEventLen);
		nOffset += nEventLen;
		memcpy(pszAlarmBuf + nOffset, strEventEndTag, strEventEndTag.GetLength());
		nOffset += strEventEndTag.GetLength();
	}
	if (0 < nParentIDLen) {
		memcpy(pszAlarmBuf + nOffset, strParentIDTag, strParentIDTag.GetLength());
		nOffset += strParentIDTag.GetLength();
		memcpy(pszAlarmBuf + nOffset, pszParentID, nParentIDLen);
		nOffset += nParentIDLen;
		memcpy(pszAlarmBuf + nOffset, strParentIDEndTag, strParentIDEndTag.GetLength());
		nOffset += strParentIDEndTag.GetLength();
	}

	memcpy(pszAlarmBuf + nOffset, strItemEndTag, strItemEndTag.GetLength());
	nOffset += strItemEndTag.GetLength();
	nBufLen = nOffset;
	return 0;
}

int CBodyBuilder::CreateInviteAnswerBody(char  *pszSDPBuf,
	int nBufLen,
	const char *pszIP,
	const char *pszPort,
	const char *pszOperateType,
	const char *pszActiveType,
	const char *pszStartTime,
	const char *pszEndTime,
	const char *pszID,
	const char *pszPassWord,
	const char *pszSSRC
)
{
	static CString strID = "v=0\r\n"
		"o=";

	static CString strIP = " 0 0 IN IP4 ";

	static CString strDevType = "\r\n"
		"s=";

	static CString strIP2 = "\r\n"
		"c=IN IP4 ";

	static CString strTime = "\r\n"
		"t=";

	static CString strPort = "\r\n"
		"m=video ";

	static CString strActiveType = " RTP/AVP 96\r\n"
		"a=";

	static CString strName = "\r\n"
		"a=rtpmap:96 PS/90000\r\n"
		"a=username:";

	static CString strPassword = "\r\n"
		"a=password:";

	static CString strSSRC = "\r\n"
		"y=";

	static CString strEnd = "\r\n"
		"f=\r\n";

	static int nTagLen = strID.GetLength() + strIP.GetLength() +
		strDevType.GetLength() + strIP2.GetLength() +
		strTime.GetLength() + 1 + strPort.GetLength() +
		strActiveType.GetLength() + strName.GetLength() +
		strPassword.GetLength() + strSSRC.GetLength() +
		strEnd.GetLength();

	// 取得参数字符串长度
	int nIPLen = strlen(pszIP);
	int nPortLen = strlen(pszPort);
	int nOperateTypeLen = strlen(pszOperateType);
	int nActiveTypeLen = strlen(pszActiveType);
	int nSTimeLen = strlen(pszStartTime);
	int nETimeLen = strlen(pszEndTime);

	int nPWLen = 0;
	int nSSRCLen = 0;

	if (pszPassWord)
		nPWLen = strlen(pszPassWord);

	if (pszSSRC)
		nSSRCLen = strlen(pszSSRC);

	// 判断缓存是否够大
	if (nBufLen <= nTagLen + nIPLen + nPortLen + nOperateTypeLen +
		nActiveTypeLen + nSTimeLen + nETimeLen + nPWLen + nSSRCLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pszSDPBuf + nOffset, strID, strID.GetLength());
	nOffset += strID.GetLength();

	memcpy(pszSDPBuf + nOffset, pszID, ID_LEN);
	nOffset += ID_LEN;

	memcpy(pszSDPBuf + nOffset, strIP, strIP.GetLength());
	nOffset += strIP.GetLength();

	memcpy(pszSDPBuf + nOffset, pszIP, nIPLen);
	nOffset += nIPLen;

	memcpy(pszSDPBuf + nOffset, strDevType, strDevType.GetLength());
	nOffset += strDevType.GetLength();

	memcpy(pszSDPBuf + nOffset, pszOperateType, nOperateTypeLen);
	nOffset += nOperateTypeLen;

	memcpy(pszSDPBuf + nOffset, strIP2, strIP2.GetLength());
	nOffset += strIP2.GetLength();

	memcpy(pszSDPBuf + nOffset, pszIP, nIPLen);
	nOffset += nIPLen;

	memcpy(pszSDPBuf + nOffset, strTime, strTime.GetLength());
	nOffset += strTime.GetLength();

	memcpy(pszSDPBuf + nOffset, pszStartTime, nSTimeLen);
	nOffset += nSTimeLen;

	memcpy(pszSDPBuf + nOffset, " ", 1);
	nOffset += 1;

	memcpy(pszSDPBuf + nOffset, pszEndTime, nETimeLen);
	nOffset += nETimeLen;

	memcpy(pszSDPBuf + nOffset, strPort, strPort.GetLength());
	nOffset += strPort.GetLength();

	memcpy(pszSDPBuf + nOffset, pszPort, nPortLen);
	nOffset += nPortLen;

	memcpy(pszSDPBuf + nOffset, strActiveType, strActiveType.GetLength());
	nOffset += strActiveType.GetLength();

	memcpy(pszSDPBuf + nOffset, pszActiveType, nActiveTypeLen);
	nOffset += nActiveTypeLen;

	memcpy(pszSDPBuf + nOffset, strName, strName.GetLength());
	nOffset += strName.GetLength();

	memcpy(pszSDPBuf + nOffset, pszID, ID_LEN);
	nOffset += ID_LEN;

	if (0 < nPWLen)
	{
		memcpy(pszSDPBuf + nOffset, strPassword, strPassword.GetLength());
		nOffset += strPassword.GetLength();

		memcpy(pszSDPBuf + nOffset, pszPassWord, nPWLen);
		nOffset += nPWLen;
	}

	if (0 < nSSRCLen)
	{
		memcpy(pszSDPBuf + nOffset, strSSRC, strSSRC.GetLength());
		nOffset += strSSRC.GetLength();

		memcpy(pszSDPBuf + nOffset, pszSSRC, nSSRCLen);
		nOffset += nSSRCLen;
	}

	memcpy(pszSDPBuf + nOffset, strEnd, strEnd.GetLength() + 1);
	//nOffset += strEnd.GetLength();

	return 0;
}

//a=filesize: a=downloadspeed: f=v/////a/1/8/1
int CBodyBuilder::CreateInviteAnswerBody(char  *pszSDPBuf, int nBufLen,
	const char *pszIP, const char *pszPort,
	const char *pszOperateType, const char *pszActiveType,
	const char *pszStartTime, const char *pszEndTime,
	const char *pszID, const char *pszPassWord,
	const char *pszSSRC, int transType, int mediaType, const char *pszFileSize) const
{
	static CString strID = "v=0\r\n"
		"o=";

	static CString strIP = " 0 0 IN IP4 ";

	static CString strDevType = "\r\n"
		"s=";

	static CString strIP2 = "\r\n"
		"c=IN IP4 ";

	static CString strTime = "\r\n"
		"t=";

	static CString strPort = "\r\n"
		"m=video ";

	static CString strActiveType = " RTP/AVP 96\r\n"
		"a=";

	static CString strPakageType = "\r\n"
		"a=rtpmap:96 PS/90000";

	static CString strName = "\r\n"
		"a=username:";

	static CString strPassword = "\r\n"
		"a=password:";

	static CString strSSRC = "\r\n"
		"y=";

	static CString strEnd = "\r\n"
		"f=\r\n";

	static int nTagLen = strID.GetLength() + strIP.GetLength() +
		strDevType.GetLength() + strIP2.GetLength() +
		strTime.GetLength() + 1 + strPort.GetLength() +
		strActiveType.GetLength() + strName.GetLength() +
		strPassword.GetLength() + strSSRC.GetLength() +
		strEnd.GetLength() + strPakageType.GetLength();

	// 取得参数字符串长度
	int nIPLen = strlen(pszIP);
	int nPortLen = strlen(pszPort);
	int nOperateTypeLen = strlen(pszOperateType);
	int nActiveTypeLen = strlen(pszActiveType);
	int nSTimeLen = strlen(pszStartTime);
	int nETimeLen = strlen(pszEndTime);

	int nPWLen = 0;
	int nSSRCLen = 0;

	if (pszPassWord)
		nPWLen = strlen(pszPassWord);

	if (pszSSRC)
		nSSRCLen = strlen(pszSSRC);

	// 判断缓存是否够大
	if (nBufLen <= nTagLen + nIPLen + nPortLen + nOperateTypeLen +
		nActiveTypeLen + nSTimeLen + nETimeLen + nPWLen + nSSRCLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pszSDPBuf + nOffset, strID, strID.GetLength());
	nOffset += strID.GetLength();

	memcpy(pszSDPBuf + nOffset, pszID, ID_LEN);
	nOffset += ID_LEN;

	memcpy(pszSDPBuf + nOffset, strIP, strIP.GetLength());
	nOffset += strIP.GetLength();

	memcpy(pszSDPBuf + nOffset, pszIP, nIPLen);
	nOffset += nIPLen;

	memcpy(pszSDPBuf + nOffset, strDevType, strDevType.GetLength());
	nOffset += strDevType.GetLength();

	memcpy(pszSDPBuf + nOffset, pszOperateType, nOperateTypeLen);
	nOffset += nOperateTypeLen;

	memcpy(pszSDPBuf + nOffset, strIP2, strIP2.GetLength());
	nOffset += strIP2.GetLength();

	memcpy(pszSDPBuf + nOffset, pszIP, nIPLen);
	nOffset += nIPLen;

	memcpy(pszSDPBuf + nOffset, strTime, strTime.GetLength());
	nOffset += strTime.GetLength();

	memcpy(pszSDPBuf + nOffset, pszStartTime, nSTimeLen);
	nOffset += nSTimeLen;

	memcpy(pszSDPBuf + nOffset, " ", 1);
	nOffset += 1;

	memcpy(pszSDPBuf + nOffset, pszEndTime, nETimeLen);
	nOffset += nETimeLen;

	/*  CString mAndaInfo = "\r\nm=video %s RTP/AVP 96\r\n \
						   a=sendOnly%s\r\n\
						   a=rtpmap:96 PS/90000\r\n \
						   a=username:%s\r\n"*/
	CString mAndaInfo = "\r\nm=%s %s %s 96\r\na=%s";

	CString transTypeStr = "RTP/AVP";
	if (transType == 1) //tcp
	{
		transTypeStr = "TCP/RTP/AVP";
	}

	CString mediaTypeStr = "video";
	if (mediaType == 1)
	{
		mediaTypeStr = "audio";
	}

	char tmpBuf[512] = { 0 };
	sprintf_s(tmpBuf, static_cast<LPCSTR>(mAndaInfo), static_cast<LPCSTR>(mediaTypeStr), pszPort, static_cast<LPCSTR>(transTypeStr), pszActiveType);

	int tempBufLen = strlen(tmpBuf);
	memcpy(pszSDPBuf + nOffset, tmpBuf, tempBufLen);
	nOffset += tempBufLen;

	/*  memcpy(pszSDPBuf + nOffset, strPort, strPort.GetLength());
	  nOffset += strPort.GetLength();

	  memcpy(pszSDPBuf + nOffset, pszPort, nPortLen);
	  nOffset += nPortLen;

	  memcpy(pszSDPBuf + nOffset, strActiveType, strActiveType.GetLength());
	  nOffset += strActiveType.GetLength();

	  memcpy(pszSDPBuf + nOffset, pszActiveType, nActiveTypeLen);
	  nOffset += nActiveTypeLen;
	  */

	memcpy(pszSDPBuf + nOffset, strPakageType, strPakageType.GetLength());
	nOffset += strPakageType.GetLength();

	memcpy(pszSDPBuf + nOffset, strName, strName.GetLength());
	nOffset += strName.GetLength();

	memcpy(pszSDPBuf + nOffset, pszID, ID_LEN);
	nOffset += ID_LEN;

	if (0 < nPWLen)
	{
		memcpy(pszSDPBuf + nOffset, strPassword, strPassword.GetLength());
		nOffset += strPassword.GetLength();

		memcpy(pszSDPBuf + nOffset, pszPassWord, nPWLen);
		nOffset += nPWLen;
	}

	int strFileSizeLen = strlen(pszFileSize);
	if (strFileSizeLen > 0)
	{
		int fileSizeStrLen = strlen(pszFileSize);
		CString strFileSize = "\r\na=filesize:";
		memcpy(pszSDPBuf + nOffset, strFileSize, strFileSize.GetLength());
		nOffset += strFileSize.GetLength();

		memcpy(pszSDPBuf + nOffset, pszFileSize, fileSizeStrLen);
		nOffset += fileSizeStrLen;
	}

	if (0 < nSSRCLen)
	{
		memcpy(pszSDPBuf + nOffset, strSSRC, strSSRC.GetLength());
		nOffset += strSSRC.GetLength();

		memcpy(pszSDPBuf + nOffset, pszSSRC, nSSRCLen);
		nOffset += nSSRCLen;
	}

	memcpy(pszSDPBuf + nOffset, strEnd, strEnd.GetLength() + 1);
	// nOffset += strEnd.GetLength();

	/* if(mediaType == 1)
	 {
		 CString audioF = "=v/////a/1/8/1";
		 memcpy(pszSDPBuf + nOffset, audioF, audioF.GetLength());
		 nOffset += audioF.GetLength();
	 }
	 else
	 {
		 CString fStr = "v/1/5/25/1/8000a///\r\n";
		 memcpy(pszSDPBuf + nOffset - 2, fStr, fStr.GetLength());
		 nOffset += fStr.GetLength() - 2;
	 }*/

	return 0;
}

int CBodyBuilder::CreateRecordHead(char *pszRecordBuf,
	int &nBufLen,
	const char *pstrSN,
	const char *pstrDeviceID,
	const char *pstrDeviceName,
	int nSumNum,
	int nListNum)
{
	static CString strVersionTag = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>RecordInfo</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<Name>";

	static CString strNameTag = "</Name>\r\n"
		"<SumNum>";

	static CString strSumNumTag = "</SumNum>\r\n"
		"<RecordList Num=\"";

	static CString strEnterTag = "\">\r\n";
	static int nStaitcCharLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strNameTag.GetLength() +
		strSumNumTag.GetLength();

	CString strSumNum;
	CString strListNum;
	strSumNum.Format("%d", nSumNum);
	strListNum.Format("%d", nListNum);
	int nSNLen = strlen(pstrSN);
	int nNameLen = strlen(pstrDeviceName);

	if (nBufLen <= nStaitcCharLen + strSumNum.GetLength() + strListNum.GetLength() + nSNLen + ID_BUF_LEN + nNameLen)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加version头
	memcpy(pszRecordBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	// 添加SN
	memcpy(pszRecordBuf + nOffset, pstrSN, nSNLen);
	nOffset += nSNLen;

	// 添加SN Tag
	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	// 添加DeviceID
	memcpy(pszRecordBuf + nOffset, pstrDeviceID, ID_LEN);
	nOffset += ID_LEN;

	// 添加DeviceID Tag
	memcpy(pszRecordBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	// 添加Device Name
	memcpy(pszRecordBuf + nOffset, pstrDeviceName, nNameLen);
	nOffset += nNameLen;

	// 添加Device Name Tag
	memcpy(pszRecordBuf + nOffset, strNameTag.GetString(), strNameTag.GetLength());
	nOffset += strNameTag.GetLength();

	// 添加SumNum
	memcpy(pszRecordBuf + nOffset, strSumNum.GetString(), strSumNum.GetLength());
	nOffset += strSumNum.GetLength();

	// 添加SumNum Tag
	memcpy(pszRecordBuf + nOffset, strSumNumTag.GetString(), strSumNumTag.GetLength());
	nOffset += strSumNumTag.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strListNum.GetString(), strListNum.GetLength());
	nOffset += strListNum.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strEnterTag.GetString(), strEnterTag.GetLength());
	nBufLen = nOffset + strEnterTag.GetLength();

	return 0;
}

int CBodyBuilder::AddRecordBody(char *pszRecordBuf,
	int &nBufLen,
	const char *pszDeviceID,
	const char *pstrName,
	const char *pszStartTime,
	const char *pszEndTime,
	const char *pszAddress,
	const char *pszFilePath,
	int	  nRecordType,
	int   nSecrecy)
{
	char szRcordType[][8] = { "all","manual","alarm","time" };

	static CString strItemTag = "<Item>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n";

	static CString strNameTagS = "<Name>";
	static CString strNameTagE = "</Name>\r\n";

	static CString strFilePathTagS = "<FilePath>";
	static CString strFilePathTagE = "</FilePath>\r\n";

	static CString strAddressTagS = "<Address>";
	static CString strAddressTagE = "</Address>\r\n";

	static CString strStartTimeTagS = "<StartTime>";
	static CString strStartTimeTagE = "</StartTime>\r\n"
		"<EndTime>";

	static CString strEndTimeTag = "</EndTime>\r\n"
		"<Secrecy>";

	static CString strSecrecyTag = "</Secrecy>\r\n"
		"<Type>";

	static CString strTypeTag = "</Type>\r\n"
		"</Item>\r\n";
	static int nStaitcCharLen = strItemTag.GetLength() +
		strDeviceTag.GetLength() +
		strNameTagS.GetLength() +
		strNameTagE.GetLength() +
		strFilePathTagS.GetLength() +
		strFilePathTagE.GetLength() +
		strAddressTagS.GetLength() +
		strAddressTagE.GetLength() +
		strStartTimeTagS.GetLength() +
		strStartTimeTagE.GetLength() +
		strEndTimeTag.GetLength() +
		strSecrecyTag.GetLength() +
		strTypeTag.GetLength();

	int nNameLen = 0;
	int nAddressLen = 0;
	int nFilePathLen = 0;
	if (pstrName)
		nNameLen = strlen(pstrName);
	if (pszAddress)
		nAddressLen = strlen(pszAddress);
	if (pszFilePath)
		nFilePathLen = strlen(pszFilePath);

	if (nBufLen <= nStaitcCharLen +
		ID_BUF_LEN +
		nNameLen +
		TIME_BUF_LEN * 2 +
		nAddressLen +
		nFilePathLen +
		RECORD_TYPE_BUF_LEN + 1)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加Item标签
	memcpy(pszRecordBuf + nOffset, strItemTag.GetString(), strItemTag.GetLength());
	nOffset += strItemTag.GetLength();

	if (pszDeviceID && '\0' != pszDeviceID[0])
	{
		// 添加设备ID
		memcpy(pszRecordBuf + nOffset, pszDeviceID, ID_BUF_LEN);
		nOffset += ID_LEN;

		// 添加设备ID标签
		memcpy(pszRecordBuf + nOffset, strDeviceTag.GetString(), strDeviceTag.GetLength());
		nOffset += strDeviceTag.GetLength();
	}

	if (pstrName && '\0' != pstrName[0])
	{
		// 添加Name开始标签
		memcpy(pszRecordBuf + nOffset, strNameTagS.GetString(), strNameTagS.GetLength());
		nOffset += strNameTagS.GetLength();

		// 添加设备Name
		memcpy(pszRecordBuf + nOffset, pstrName, nNameLen);
		nOffset += nNameLen;

		// 添加Name结束标签
		memcpy(pszRecordBuf + nOffset, strNameTagE.GetString(), strNameTagE.GetLength());
		nOffset += strNameTagE.GetLength();
	}

	if (pszFilePath && '\0' != pszFilePath[0])
	{
		// 添加FilePath开始标签
		memcpy(pszRecordBuf + nOffset, strFilePathTagS.GetString(), strFilePathTagS.GetLength());
		nOffset += strFilePathTagS.GetLength();

		// 添加FilePath
		memcpy(pszRecordBuf + nOffset, pszFilePath, nFilePathLen);
		nOffset += nFilePathLen;

		// 添加FilePath结束标签
		memcpy(pszRecordBuf + nOffset, strFilePathTagE.GetString(), strFilePathTagE.GetLength());
		nOffset += strFilePathTagE.GetLength();
	}

	if (pszAddress && '\0' != pszAddress[0])
	{
		// 添加address标签
		memcpy(pszRecordBuf + nOffset, strAddressTagS.GetString(), strAddressTagS.GetLength());
		nOffset += strAddressTagS.GetLength();

		// 添加address
		memcpy(pszRecordBuf + nOffset, pszAddress, nAddressLen);
		nOffset += nAddressLen;

		// 添加address标签
		memcpy(pszRecordBuf + nOffset, strAddressTagE.GetString(), strAddressTagE.GetLength());
		nOffset += strAddressTagE.GetLength();
	}

	if (pszStartTime && '\0' != pszStartTime[0])
	{
		// 添加StartTime标签
		memcpy(pszRecordBuf + nOffset, strStartTimeTagS.GetString(), strStartTimeTagS.GetLength());
		nOffset += strStartTimeTagS.GetLength();

		// 添加StartTime
		memcpy(pszRecordBuf + nOffset, pszStartTime, TIME_BUF_LEN);
		nOffset += TIME_LEN;

		// 添加StartTime标签
		memcpy(pszRecordBuf + nOffset, strStartTimeTagE.GetString(), strStartTimeTagE.GetLength());
		nOffset += strStartTimeTagE.GetLength();
	}

	if (pszEndTime && '\0' != pszEndTime[0])
	{
		// 添加EndTime
		memcpy(pszRecordBuf + nOffset, pszEndTime, TIME_BUF_LEN);
		nOffset += TIME_LEN;

		// 添加EndTime标签
		memcpy(pszRecordBuf + nOffset, strEndTimeTag.GetString(), strEndTimeTag.GetLength());
		nOffset += strEndTimeTag.GetLength();
	}

	// 添加Secrecy
	pszRecordBuf[nOffset] = static_cast<char>(nSecrecy + 48);
	nOffset++;

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strSecrecyTag.GetString(), strSecrecyTag.GetLength());
	nOffset += strSecrecyTag.GetLength();

	// 添加Type
	memcpy(pszRecordBuf + nOffset, szRcordType[nRecordType], strlen(szRcordType[nRecordType]));
	nOffset += strlen(szRcordType[nRecordType]);

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strTypeTag.GetString(), strTypeTag.GetLength());
	nBufLen = nOffset + strTypeTag.GetLength();
	return 0;
}

int CBodyBuilder::AddRecordBodyShengtong(char *pszRecordBuf,
	int &nBufLen,
	const char *pszDeviceID,
	const char *pstrName,
	const char *pszStartTime,
	const char *pszEndTime,
	const char *pszAddress,
	const char *pszFilePath,
	const char *pszFileSize,
	const char *pszRecorderID,
	int	  nRecordType,
	int   nSecrecy)
{
	char szRcordType[][8] = { "all","manual","alarm","time" };

	static CString strItemTag = "<Item>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n";

	static CString strNameTagS = "<Name>";
	static CString strNameTagE = "</Name>\r\n";

	static CString strFilePathTagS = "<FilePath>";
	static CString strFilePathTagE = "</FilePath>\r\n";

	static CString strFileSizeTagS = "<FileSize>";
	static CString strFileSizeTagE = "</FileSize>\r\n";

	static CString strRecorderIDTagS = "<RecorderID>";
	static CString strRecorderIDTagE = "</RecorderID>\r\n";

	static CString strAddressTagS = "<Address>";
	static CString strAddressTagE = "</Address>\r\n";

	static CString strStartTimeTagS = "<StartTime>";
	static CString strStartTimeTagE = "</StartTime>\r\n"
		"<EndTime>";

	static CString strEndTimeTag = "</EndTime>\r\n"
		"<Secrecy>";

	static CString strSecrecyTag = "</Secrecy>\r\n"
		"<Type>";

	static CString strTypeTag = "</Type>\r\n"
		"</Item>\r\n";
	static int nStaitcCharLen = strItemTag.GetLength() +
		strDeviceTag.GetLength() +
		strNameTagS.GetLength() +
		strNameTagE.GetLength() +
		strFilePathTagS.GetLength() +
		strFilePathTagE.GetLength() +

		strFileSizeTagS.GetLength() +
		strFileSizeTagE.GetLength() +

		strRecorderIDTagS.GetLength() +
		strRecorderIDTagE.GetLength() +

		strAddressTagS.GetLength() +
		strAddressTagE.GetLength() +
		strStartTimeTagS.GetLength() +
		strStartTimeTagE.GetLength() +
		strEndTimeTag.GetLength() +
		strSecrecyTag.GetLength() +
		strTypeTag.GetLength();

	int nNameLen = 0;
	int nAddressLen = 0;
	int nFilePathLen = 0;
	int nFileSieLen = 0;
	int nRecordIdLen = 0;
	if (pstrName)
		nNameLen = strlen(pstrName);
	if (pszAddress)
		nAddressLen = strlen(pszAddress);
	if (pszFilePath)
		nFilePathLen = strlen(pszFilePath);

	if (pszFileSize)
	{
		nFileSieLen = strlen(pszFileSize);
	}

	if (pszRecorderID)
	{
		nRecordIdLen = strlen(pszRecorderID);
	}

	if (nBufLen <= nStaitcCharLen +
		ID_BUF_LEN +
		nNameLen +
		TIME_BUF_LEN * 2 +
		nAddressLen +
		nFilePathLen +
		nFileSieLen +
		nRecordIdLen +
		RECORD_TYPE_BUF_LEN + 1)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加Item标签
	memcpy(pszRecordBuf + nOffset, strItemTag.GetString(), strItemTag.GetLength());
	nOffset += strItemTag.GetLength();

	if (pszDeviceID && '\0' != pszDeviceID[0])
	{
		// 添加设备ID
		memcpy(pszRecordBuf + nOffset, pszDeviceID, ID_BUF_LEN);
		nOffset += ID_LEN;

		// 添加设备ID标签
		memcpy(pszRecordBuf + nOffset, strDeviceTag.GetString(), strDeviceTag.GetLength());
		nOffset += strDeviceTag.GetLength();
	}

	//  if(pstrName && '\0' != pstrName[0])
	{
		// 添加Name开始标签
		memcpy(pszRecordBuf + nOffset, strNameTagS.GetString(), strNameTagS.GetLength());
		nOffset += strNameTagS.GetLength();

		// 添加设备Name
		if (pstrName && '\0' != pstrName[0])
		{
			memcpy(pszRecordBuf + nOffset, pstrName, nNameLen);
			nOffset += nNameLen;
		}

		// 添加Name结束标签
		memcpy(pszRecordBuf + nOffset, strNameTagE.GetString(), strNameTagE.GetLength());
		nOffset += strNameTagE.GetLength();
	}

	//   if(pszFilePath && '\0' != pszFilePath[0])
	{
		// 添加FilePath开始标签
		memcpy(pszRecordBuf + nOffset, strFilePathTagS.GetString(), strFilePathTagS.GetLength());
		nOffset += strFilePathTagS.GetLength();

		// 添加FilePath
		if (pszFilePath && '\0' != pszFilePath[0])
		{
			memcpy(pszRecordBuf + nOffset, pszFilePath, nFilePathLen);
			nOffset += nFilePathLen;
		}

		// 添加FilePath结束标签
		memcpy(pszRecordBuf + nOffset, strFilePathTagE.GetString(), strFilePathTagE.GetLength());
		nOffset += strFilePathTagE.GetLength();
	}

	//  if(pszFileSize && '\0' != pszFileSize[0])
	{
		// 添加FilePath开始标签
		memcpy(pszRecordBuf + nOffset, strFileSizeTagS.GetString(), strFileSizeTagS.GetLength());
		nOffset += strFileSizeTagS.GetLength();

		// 添加FilePath
		if (pszFileSize && '\0' != pszFileSize[0])
		{
			memcpy(pszRecordBuf + nOffset, pszFileSize, nFileSieLen);
			nOffset += nFileSieLen;
		}

		// 添加FilePath结束标签
		memcpy(pszRecordBuf + nOffset, strFileSizeTagE.GetString(), strFileSizeTagE.GetLength());
		nOffset += strFileSizeTagE.GetLength();
	}

	//  if(pszFileSize && '\0' != pszFileSize[0])
	{
		// 添加FilePath开始标签
		memcpy(pszRecordBuf + nOffset, strRecorderIDTagS.GetString(), strRecorderIDTagS.GetLength());
		nOffset += strRecorderIDTagS.GetLength();

		// 添加FilePath
		if (pszRecorderID && '\0' != pszRecorderID[0])
		{
			memcpy(pszRecordBuf + nOffset, pszRecorderID, nRecordIdLen);
			nOffset += nRecordIdLen;
		}

		// 添加FilePath结束标签
		memcpy(pszRecordBuf + nOffset, strRecorderIDTagE.GetString(), strRecorderIDTagE.GetLength());
		nOffset += strRecorderIDTagE.GetLength();
	}

	//  if(pszAddress && '\0' != pszAddress[0])
	{
		// 添加address标签
		memcpy(pszRecordBuf + nOffset, strAddressTagS.GetString(), strAddressTagS.GetLength());
		nOffset += strAddressTagS.GetLength();

		// 添加address
		if (pszAddress && '\0' != pszAddress[0])
		{
			memcpy(pszRecordBuf + nOffset, pszAddress, nAddressLen);
			nOffset += nAddressLen;
		}

		// 添加address标签
		memcpy(pszRecordBuf + nOffset, strAddressTagE.GetString(), strAddressTagE.GetLength());
		nOffset += strAddressTagE.GetLength();
	}

	if (pszStartTime && '\0' != pszStartTime[0])
	{
		// 添加StartTime标签
		memcpy(pszRecordBuf + nOffset, strStartTimeTagS.GetString(), strStartTimeTagS.GetLength());
		nOffset += strStartTimeTagS.GetLength();

		// 添加StartTime
		memcpy(pszRecordBuf + nOffset, pszStartTime, TIME_BUF_LEN);
		nOffset += TIME_LEN;

		// 添加StartTime标签
		memcpy(pszRecordBuf + nOffset, strStartTimeTagE.GetString(), strStartTimeTagE.GetLength());
		nOffset += strStartTimeTagE.GetLength();
	}

	if (pszEndTime && '\0' != pszEndTime[0])
	{
		// 添加EndTime
		memcpy(pszRecordBuf + nOffset, pszEndTime, TIME_BUF_LEN);
		nOffset += TIME_LEN;

		// 添加EndTime标签
		memcpy(pszRecordBuf + nOffset, strEndTimeTag.GetString(), strEndTimeTag.GetLength());
		nOffset += strEndTimeTag.GetLength();
	}

	// 添加Secrecy
	pszRecordBuf[nOffset] = static_cast<char>(nSecrecy + 48);
	nOffset++;

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strSecrecyTag.GetString(), strSecrecyTag.GetLength());
	nOffset += strSecrecyTag.GetLength();

	// 添加Type
	memcpy(pszRecordBuf + nOffset, szRcordType[nRecordType], strlen(szRcordType[nRecordType]));
	nOffset += strlen(szRcordType[nRecordType]);

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strTypeTag.GetString(), strTypeTag.GetLength());
	nBufLen = nOffset + strTypeTag.GetLength();
	return 0;
}

// 开始生成录像文件信息xml文件头
int CBodyBuilder::CreateAlarmRecordHead(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,						// IN-缓存长度，OUT-缓存已填充长度
	const char *pstrSN,					// SN号
	const char *pstrDeviceID,			// 设备ID
	const char *pstrDeviceName,			// 设备Name
	int nSumNum,						// 录像总数
	int nListNum)						// 当前xml中包含的录像个数
{
	static CString strVersionTag = "<?xml version=\"1.0\" ?>\r\n"
		"<Response>\r\n"
		"<CmdType>Alarm</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<Result>";

	static CString strNameTag = "</Result>\r\n"
		"<SumNum>";

	static CString strSumNumTag = "</SumNum>\r\n"
		"<AlarmList Num=\"";

	static CString strEnterTag = "\">\r\n";
	static int nStaitcCharLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strNameTag.GetLength() +
		strSumNumTag.GetLength();

	CString strSumNum;
	CString strListNum;
	strSumNum.Format("%d", nSumNum);
	strListNum.Format("%d", nListNum);
	int nSNLen = strlen(pstrSN);
	int nNameLen = strlen(pstrDeviceName);

	if (nBufLen <= nStaitcCharLen + strSumNum.GetLength() + strListNum.GetLength() + nSNLen + ID_BUF_LEN + nNameLen)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加version头
	memcpy(pszRecordBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	// 添加SN
	memcpy(pszRecordBuf + nOffset, pstrSN, nSNLen);
	nOffset += nSNLen;

	// 添加SN Tag
	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	// 添加DeviceID
	memcpy(pszRecordBuf + nOffset, pstrDeviceID, ID_LEN);
	nOffset += ID_LEN;

	// 添加DeviceID Tag
	memcpy(pszRecordBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	// 添加Device Name
	memcpy(pszRecordBuf + nOffset, pstrDeviceName, nNameLen);
	nOffset += nNameLen;

	// 添加Device Name Tag
	memcpy(pszRecordBuf + nOffset, strNameTag.GetString(), strNameTag.GetLength());
	nOffset += strNameTag.GetLength();

	// 添加SumNum
	memcpy(pszRecordBuf + nOffset, strSumNum.GetString(), strSumNum.GetLength());
	nOffset += strSumNum.GetLength();

	// 添加SumNum Tag
	memcpy(pszRecordBuf + nOffset, strSumNumTag.GetString(), strSumNumTag.GetLength());
	nOffset += strSumNumTag.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strListNum.GetString(), strListNum.GetLength());
	nOffset += strListNum.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strEnterTag.GetString(), strEnterTag.GetLength());
	nBufLen = nOffset + strEnterTag.GetLength();

	return 0;
}

// 添加xml body
int CBodyBuilder::AddAlarmRecordBody(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
	const char *pszDeviceID,							// 设备ID
	const char *alarmPriority,
	const char *alarmMethod,
	const char *alarmTime,
	const char *alarmDescription,
	const char *longitude,
	const char *latitude,
	const char *alarmType,
	const char *alarmStatus)
{
	//char szRcordType[][8] =	{"all","manual","alarm","time"};

	static CString strItemTag = "<Item>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n";

	static CString strAlarmPriorityTagS = "<AlarmPriority>";
	static CString strAlarmPriorityTagE = "</AlarmPriority>\r\n";

	static CString strAlarmMethodTagS = "<AlarmMethod>";
	static CString strAlarmMethodTagE = "</AlarmMethod>\r\n";

	static CString strAlarmTimeTagS = "<AlarmTime>";
	static CString strAlarmTimeTagE = "</AlarmTime>\r\n";

	static CString strAlarmDescriptionTagS = "<AlarmDescription>";
	static CString strAlarmDescriptionTagE = "</AlarmDescription>\r\n"
		"<Longitude>";

	static CString strLongitudeTag = "</Longitude>\r\n"
		"<Latitude>";

	static CString strLatitudeTag = "</Latitude>\r\n";

	static CString  strAlarmTypeTagS = "<AlarmType>";
	static CString strAlarmTypeTagE = "</AlarmType>\r\n";

	static CString strAlarmStatusTagS = "<AlarmStatus>";
	static CString strAlarmStatusTagE = "</AlarmStatus>\r\n";
	static CString  strEnd = "</Item>\r\n";

	static int nStaitcCharLen = strItemTag.GetLength() +
		strDeviceTag.GetLength() +
		strAlarmPriorityTagS.GetLength() +
		strAlarmPriorityTagE.GetLength() +
		strAlarmMethodTagS.GetLength() +
		strAlarmMethodTagE.GetLength() +
		strAlarmTimeTagS.GetLength() +
		strAlarmTimeTagE.GetLength() +
		strAlarmDescriptionTagS.GetLength() +
		strAlarmDescriptionTagE.GetLength() +
		strLatitudeTag.GetLength() +
		strLongitudeTag.GetLength() +
		strAlarmTypeTagS.GetLength() +
		strAlarmTypeTagE.GetLength() +
		strAlarmStatusTagS.GetLength() +
		strAlarmStatusTagE.GetLength() +
		strEnd.GetLength();

	if (nBufLen <= nStaitcCharLen)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加Item标签
	memcpy(pszRecordBuf + nOffset, strItemTag.GetString(), strItemTag.GetLength());
	nOffset += strItemTag.GetLength();

	if (pszDeviceID && '\0' != pszDeviceID[0])
	{
		// 添加设备ID
		memcpy(pszRecordBuf + nOffset, pszDeviceID, ID_BUF_LEN);
		nOffset += ID_LEN;

		// 添加设备ID标签
		memcpy(pszRecordBuf + nOffset, strDeviceTag.GetString(), strDeviceTag.GetLength());
		nOffset += strDeviceTag.GetLength();
	}

	memcpy(pszRecordBuf + nOffset, strAlarmPriorityTagS.GetString(), strAlarmPriorityTagS.GetLength());
	nOffset += strAlarmPriorityTagS.GetLength();

	if (alarmPriority)
	{
		memcpy(pszRecordBuf + nOffset, alarmPriority, strlen(alarmPriority));
		nOffset += strlen(alarmPriority);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmPriorityTagE.GetString(), strAlarmPriorityTagE.GetLength());
	nOffset += strAlarmPriorityTagE.GetLength();

	memcpy(pszRecordBuf + nOffset, strAlarmMethodTagS.GetString(), strAlarmMethodTagS.GetLength());
	nOffset += strAlarmMethodTagS.GetLength();

	if (alarmMethod)
	{
		memcpy(pszRecordBuf + nOffset, alarmMethod, strlen(alarmMethod));
		nOffset += strlen(alarmMethod);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmMethodTagE.GetString(), strAlarmMethodTagE.GetLength());
	nOffset += strAlarmMethodTagE.GetLength();

	memcpy(pszRecordBuf + nOffset, strAlarmTimeTagS.GetString(), strAlarmTimeTagS.GetLength());
	nOffset += strAlarmTimeTagS.GetLength();

	if (alarmTime)
	{
		memcpy(pszRecordBuf + nOffset, alarmTime, strlen(alarmTime));
		nOffset += strlen(alarmTime);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmTimeTagE.GetString(), strAlarmTimeTagE.GetLength());
	nOffset += strAlarmTimeTagE.GetLength();

	//strAlarmDescriptionTagS
	memcpy(pszRecordBuf + nOffset, strAlarmDescriptionTagS.GetString(), strAlarmDescriptionTagS.GetLength());
	nOffset += strAlarmDescriptionTagS.GetLength();

	if (alarmDescription)
	{
		memcpy(pszRecordBuf + nOffset, alarmDescription, strlen(alarmDescription));
		nOffset += strlen(alarmDescription);
	}

	memcpy(pszRecordBuf + nOffset, strAlarmDescriptionTagE.GetString(), strAlarmDescriptionTagE.GetLength());
	nOffset += strAlarmDescriptionTagE.GetLength();

	// 添加Secrecy标签 strLatitudeTag
	memcpy(pszRecordBuf + nOffset, strLongitudeTag.GetString(), strLongitudeTag.GetLength());
	nOffset += strLongitudeTag.GetLength();

	memcpy(pszRecordBuf + nOffset, longitude, strlen(longitude));
	nOffset += strlen(longitude);

	memcpy(pszRecordBuf + nOffset, strLatitudeTag.GetString(), strLatitudeTag.GetLength());
	nOffset += strLatitudeTag.GetLength();

	memcpy(pszRecordBuf + nOffset, latitude, strlen(latitude));
	nOffset += strlen(latitude);

	if (alarmType && '\0' != alarmType[0])
	{
		memcpy(pszRecordBuf + nOffset, strAlarmTypeTagS.GetString(), strAlarmTypeTagS.GetLength());
		nOffset += strAlarmTypeTagS.GetLength();

		if (alarmType)
		{
			memcpy(pszRecordBuf + nOffset, alarmType, strlen(alarmType));
			nOffset += strlen(alarmType);
		}

		memcpy(pszRecordBuf + nOffset, strAlarmTypeTagE.GetString(), strAlarmTypeTagE.GetLength());
		nOffset += strAlarmTypeTagE.GetLength();
	}

	if (alarmStatus && '\0' != alarmStatus[0])
	{
		memcpy(pszRecordBuf + nOffset, strAlarmStatusTagS.GetString(), strAlarmStatusTagS.GetLength());
		nOffset += strAlarmStatusTagS.GetLength();

		if (alarmType)
		{
			memcpy(pszRecordBuf + nOffset, alarmStatus, strlen(alarmStatus));
			nOffset += strlen(alarmStatus);
		}

		memcpy(pszRecordBuf + nOffset, strAlarmStatusTagE.GetString(), strAlarmStatusTagE.GetLength());
		nOffset += strAlarmStatusTagE.GetLength();
	}

	memcpy(pszRecordBuf + nOffset, strEnd, strlen(strEnd));
	nOffset += strlen(strEnd);

	nBufLen = nOffset;

	return 0;
}

// 完成xml
int CBodyBuilder::CompleteAlarmRecord(char *pRecordBuf, int nBufLen)
{
	static CString strAlarmRecordEndTag = "</AlarmList>\r\n"
		"</Response>\r\n";

	if (nBufLen <= strAlarmRecordEndTag.GetLength())
		return ERROR_NOACCESS;

	// 一定把'\0'也拷贝过去
	memcpy(pRecordBuf, strAlarmRecordEndTag.GetString(), strAlarmRecordEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CompleteRecord(char *pszRecordBuf, int nBufLen)
{
	static CString strRecordEndTag = "</RecordList>\r\n"
		"</Response>\r\n";

	if (nBufLen <= strRecordEndTag.GetLength())
		return ERROR_NOACCESS;

	// 一定把'\0'也拷贝过去
	memcpy(pszRecordBuf, strRecordEndTag.GetString(), strRecordEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateCatalogHead(char *pszRecordBuf,
	int &nBufLen,
	const char *pstrSN,
	const char *pstrDeviceID,
	int nSumNum,
	int nListNum)
{
	static CString strVersionTag = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>Catalog</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<SumNum>";

	static CString strSumNumTag = "</SumNum>\r\n"
		"<DeviceList Num=\"";

	static CString strEnterTag = "\">\r\n";
	static int nStaitcCharLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strSumNumTag.GetLength();

	CString strSumNum;
	CString strListNum;
	strSumNum.Format("%d", nSumNum);
	strListNum.Format("%d", nListNum);
	int nSNLen = strlen(pstrSN);

	if (nBufLen <= nStaitcCharLen + strSumNum.GetLength() + strListNum.GetLength() + nSNLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加version头
	memcpy(pszRecordBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	// 添加SN
	memcpy(pszRecordBuf + nOffset, pstrSN, nSNLen);
	nOffset += nSNLen;

	// 添加SN Tag
	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	// 添加DeviceID
	memcpy(pszRecordBuf + nOffset, pstrDeviceID, ID_LEN);
	nOffset += ID_LEN;

	// 添加DeviceID Tag
	memcpy(pszRecordBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	// 添加SumNum
	memcpy(pszRecordBuf + nOffset, strSumNum.GetString(), strSumNum.GetLength());
	nOffset += strSumNum.GetLength();

	// 添加SumNum Tag
	memcpy(pszRecordBuf + nOffset, strSumNumTag.GetString(), strSumNumTag.GetLength());
	nOffset += strSumNumTag.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strListNum.GetString(), strListNum.GetLength());
	nOffset += strListNum.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strEnterTag.GetString(), strEnterTag.GetLength() + 1);
	nBufLen = nOffset + strEnterTag.GetLength();

	return 0;
}

int CBodyBuilder::CreateCatalogHead(char* pszRecordBuf, int& nBufLen, const char* pszDataType, const char* pstrSN, const char* pstrDeviceID, int nSumNum, int nListNum)
{
	CString strVersionTag;
	strVersionTag.Format("<?xml version=\"1.0\" ?>\r\n"
		"<%s>\r\n"
		"<CmdType>Catalog</CmdType>\r\n"
		"<SN>", pszDataType);

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<SumNum>";

	static CString strSumNumTag = "</SumNum>\r\n"
		"<DeviceList Num=\"";

	static CString strEnterTag = "\">\r\n";
	static int nStaitcCharLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strSumNumTag.GetLength();

	CString strSumNum;
	CString strListNum;
	strSumNum.Format("%d", nSumNum);
	strListNum.Format("%d", nListNum);
	int nSNLen = strlen(pstrSN);
	int nDeviceIDLen = strlen(pstrDeviceID);

	if (nBufLen <= nStaitcCharLen + strSumNum.GetLength() + strListNum.GetLength() + nSNLen + nDeviceIDLen)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加version头
	memcpy(pszRecordBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	// 添加SN
	memcpy(pszRecordBuf + nOffset, pstrSN, nSNLen);
	nOffset += nSNLen;

	// 添加SN Tag
	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	// 添加DeviceID
	memcpy(pszRecordBuf + nOffset, pstrDeviceID, nDeviceIDLen);
	nOffset += nDeviceIDLen;

	// 添加DeviceID Tag
	memcpy(pszRecordBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	// 添加SumNum
	memcpy(pszRecordBuf + nOffset, strSumNum.GetString(), strSumNum.GetLength());
	nOffset += strSumNum.GetLength();

	// 添加SumNum Tag
	memcpy(pszRecordBuf + nOffset, strSumNumTag.GetString(), strSumNumTag.GetLength());
	nOffset += strSumNumTag.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strListNum.GetString(), strListNum.GetLength());
	nOffset += strListNum.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strEnterTag.GetString(), strEnterTag.GetLength() + 1);
	nBufLen = nOffset + strEnterTag.GetLength();

	return 0;
}

// 添加Catalog xml body

int CBodyBuilder::AddCatalogBody(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
	void *pszCatalog)
{
	if (pszCatalog == nullptr)
	{
		return ERROR_NOACCESS;
	}

	CatalogItem* catalog = static_cast<CatalogItem*>(pszCatalog);

	const int nodeNum = 38;
	const int nodeInfoNum = 4;

	CString strNode[nodeNum] = {
		"DeviceID",     catalog->GetDeviceID(),
		"Name",         catalog->GetName(),
		"Manufacturer", catalog->GetManufacturer(),
		"Model",        catalog->GetModel(),
		"Owner",        catalog->GetOwner(),
		"CivilCode",    catalog->GetCivilCode(),
		"Block",        catalog->GetBlock(),
		"Address",      catalog->GetAddress(),
		"Parental",     catalog->GetParental(),
		"ParentID",     catalog->GetParentID(),
		"SafetyWay",    catalog->GetSafetyWay(),
		"RegisterWay",  catalog->GetRegisterWay(),
		"Certifiable",  catalog->GetCertifiable(),
		"ErrCode",      catalog->GetErrcode(),
		"Secrecy",      catalog->GetSecrecy(),
		"Status",       catalog->GetStatus(),
		"Longitude",    catalog->GetLongitude(),
		"Latitude",     catalog->GetLatitude(),
		 "OperateType",  catalog->GetOperateType(),
	};

	CString strNodeInfo[nodeInfoNum] = {
	   "PTZType",          catalog->GetPTZType(),
	   "PositionType",     catalog->GetPositionType(),
	};

	auto doc = new XMLDocument();
	auto root = doc->NewElement("Item");
	doc->InsertEndChild(root);

	for (auto i = 0; i < nodeNum; i += 2)
	{
		auto node = doc->NewElement(strNode[i]);
		node->SetText(strNode[i + 1]);
		root->InsertEndChild(node);
	}

	auto info = doc->NewElement("Info");
	root->InsertEndChild(info);

	for (auto i = 0; i < nodeInfoNum; i += 2)
	{
		auto node = doc->NewElement(strNodeInfo[i]);
		node->SetText(strNodeInfo[i + 1]);
		info->InsertEndChild(node);
	}

	XMLPrinter printer;
	doc->Accept(&printer);

	if (printer.CStrSize() >= nBufLen)
	{
		return ERROR_NOACCESS;
	}
	else
	{
		nBufLen = printer.CStrSize();
	}
	memcpy(pszRecordBuf, printer.CStr(), printer.CStrSize());

	delete doc;

	return 0;
}

// 添加Catalog xml body
int CBodyBuilder::AddCatalogBody(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
	const char *pszDeviceID,							// 设备ID
	const char *pszEvent,
	const char *pstrName,
	const char *pszManufacturer,
	const char *pszModel,
	const char *pszOwner,
	const char *pszCivilCode,
	const char *pszAddress,
	const char *pszParental,
	const char *pszSafetyWay,
	const char *pszRegisterWay,
	const char *pszSecrecy,
	const char *pszStatus,
	const char *pszParentID,
	const char *pszPTZType)
{
	static CString strItemTag = "<Item>\r\n"
		"<DeviceID>";

	static CString strDeviceTag = "</DeviceID>\r\n";
	static CString strEventTag = "<Event>";
	static CString strEventEndTag = "</Event>\r\n";
	static CString strNameTag = "<Name>";

	static CString strNameEndTag = "</Name>\r\n"
		"<Manufacturer>";

	static CString strManufacturerTag = "</Manufacturer>\r\n"
		"<Model>";

	static CString strModelTag = "</Model>\r\n"
		"<Owner>";

	static CString strOwnerTag = "</Owner>\r\n"
		"<CivilCode>";

	static CString strCivilCodeTag = "</CivilCode>\r\n"
		"<Address>";

	static CString strAddressTag = "</Address>\r\n"
		"<ParentID>";

	static CString strParentIDTag = "</ParentID>\r\n"
		"<Parental>";

	static CString strParentalTag = "</Parental>\r\n"
		"<SafetyWay>";

	static CString strSafetyWayTag = "</SafetyWay>\r\n"
		"<RegisterWay>";

	static CString strRegisterWayTag = "</RegisterWay>\r\n"
		"<Secrecy>";

	static CString strSecrecyTag = "</Secrecy>\r\n"
		"<Status>";

	static CString strStatusTag = "</Status>\r\n";

	static CString strInforTag = "<Info>\r\n"
		"<PTZType>";
	static CString strPTZTypeTag = "</PTZType>\r\n"
		"</Info>\r\n";

	static CString strItemEndTag = "</Item>\r\n";

	static int nStaitcCharLen = strItemTag.GetLength() +
		strDeviceTag.GetLength() +
		strEventTag.GetLength() +
		strEventEndTag.GetLength() +
		strNameTag.GetLength() +
		strNameEndTag.GetLength() +
		strManufacturerTag.GetLength() +
		strModelTag.GetLength() +
		strOwnerTag.GetLength() +
		strCivilCodeTag.GetAllocLength() +
		strAddressTag.GetLength() +
		strParentalTag.GetLength() +
		strSafetyWayTag.GetLength() +
		strRegisterWayTag.GetLength() +
		strSecrecyTag.GetLength() +
		strParentIDTag.GetLength() +
		strInforTag.GetLength() +
		strPTZTypeTag.GetLength() +
		strItemEndTag.GetLength();

	int nEventLen = 0;
	if (pszEvent)
		nEventLen = strlen(pszEvent);
	int nNameLen = strlen(pstrName);
	int nManufacturerLen = strlen(pszManufacturer);
	int nModelLen = strlen(pszModel);
	int nOwnerLen = strlen(pszOwner);
	int nCivilCodeLen = strlen(pszCivilCode);
	int nAddressLen = strlen(pszAddress);
	int nStatusLen = strlen(pszStatus);
	int nParentID = strlen(pszParentID);

	if (nBufLen <= nStaitcCharLen +
		ID_BUF_LEN +
		nEventLen +
		nNameLen +
		nManufacturerLen +
		nModelLen +
		nOwnerLen +
		nCivilCodeLen +
		nAddressLen +
		nStatusLen +
		nParentID + 5)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加Item标签
	memcpy(pszRecordBuf + nOffset, strItemTag.GetString(), strItemTag.GetLength());
	nOffset += strItemTag.GetLength();

	// 添加设备ID
	memcpy(pszRecordBuf + nOffset, pszDeviceID, ID_LEN);
	nOffset += ID_LEN;

	// 添加设备ID标签
	memcpy(pszRecordBuf + nOffset, strDeviceTag.GetString(), strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();

	if (0 < nEventLen)
	{
		// 添加Event
		memcpy(pszRecordBuf + nOffset, strEventTag, strEventTag.GetLength());
		nOffset += strEventTag.GetLength();

		// 添加Event
		memcpy(pszRecordBuf + nOffset, pszEvent, nEventLen);
		nOffset += nEventLen;

		// 添加Event
		memcpy(pszRecordBuf + nOffset, strEventEndTag, strEventEndTag.GetLength());
		nOffset += strEventEndTag.GetLength();
	}
	// 添加Name标签
	memcpy(pszRecordBuf + nOffset, strNameTag.GetString(), strNameTag.GetLength());
	nOffset += strNameTag.GetLength();

	// 添加设备Name
	memcpy(pszRecordBuf + nOffset, pstrName, nNameLen);
	nOffset += nNameLen;

	// 添加Name标签
	memcpy(pszRecordBuf + nOffset, strNameEndTag.GetString(), strNameEndTag.GetLength());
	nOffset += strNameEndTag.GetLength();

	// 添加Manufacturer
	memcpy(pszRecordBuf + nOffset, pszManufacturer, nManufacturerLen);
	nOffset += nManufacturerLen;

	// 添加Manufacturer标签
	memcpy(pszRecordBuf + nOffset, strManufacturerTag.GetString(), strManufacturerTag.GetLength());
	nOffset += strManufacturerTag.GetLength();

	// 添加Model
	memcpy(pszRecordBuf + nOffset, pszModel, nModelLen);
	nOffset += nModelLen;

	// 添加Model标签
	memcpy(pszRecordBuf + nOffset, strModelTag.GetString(), strModelTag.GetLength());
	nOffset += strModelTag.GetLength();

	// 添加Owner
	memcpy(pszRecordBuf + nOffset, pszOwner, nOwnerLen);
	nOffset += nOwnerLen;

	// 添加Owner标签
	memcpy(pszRecordBuf + nOffset, strOwnerTag.GetString(), strOwnerTag.GetLength());
	nOffset += strOwnerTag.GetLength();

	// 添加CivilCode
	memcpy(pszRecordBuf + nOffset, pszCivilCode, nCivilCodeLen);
	nOffset += nCivilCodeLen;

	// 添加CivilCode标签
	memcpy(pszRecordBuf + nOffset, strCivilCodeTag.GetString(), strCivilCodeTag.GetLength());
	nOffset += strCivilCodeTag.GetLength();

	// 添加Address
	memcpy(pszRecordBuf + nOffset, pszAddress, nAddressLen);
	nOffset += nAddressLen;

	// 添加Address标签
	memcpy(pszRecordBuf + nOffset, strAddressTag.GetString(), strAddressTag.GetLength());
	nOffset += strAddressTag.GetLength();

	if (0 < nParentID)
	{
		memcpy(pszRecordBuf + nOffset, pszParentID, nParentID);
		nOffset += nParentID;
	}

	memcpy(pszRecordBuf + nOffset, strParentIDTag.GetString(), strParentIDTag.GetLength());
	nOffset += strParentIDTag.GetLength();

	// 添加Parental
	if (0 == pszParental[0])
		pszRecordBuf[nOffset] = '0';
	else
		pszRecordBuf[nOffset] = pszParental[0];
	nOffset++;

	// 添加Parental标签
	memcpy(pszRecordBuf + nOffset, strParentalTag.GetString(), strParentalTag.GetLength());
	nOffset += strParentalTag.GetLength();

	// 添加SafetyWay
	pszRecordBuf[nOffset] = pszSafetyWay[0];
	nOffset++;

	// 添加SafetyWay标签
	memcpy(pszRecordBuf + nOffset, strSafetyWayTag.GetString(), strSafetyWayTag.GetLength());
	nOffset += strSafetyWayTag.GetLength();

	// 添加RegisterWay
	pszRecordBuf[nOffset] = pszRegisterWay[0];
	nOffset++;

	// 添加RegisterWay标签
	memcpy(pszRecordBuf + nOffset, strRegisterWayTag.GetString(), strRegisterWayTag.GetLength());
	nOffset += strRegisterWayTag.GetLength();

	// 添加Secrecy
	pszRecordBuf[nOffset] = pszSecrecy[0];
	nOffset++;

	// 添加Secrecy标签
	memcpy(pszRecordBuf + nOffset, strSecrecyTag.GetString(), strSecrecyTag.GetLength());
	nOffset += strSecrecyTag.GetLength();

	// 添加Status
	memcpy(pszRecordBuf + nOffset, pszStatus, nStatusLen);
	nOffset += nStatusLen;

	// 添加Status标签
	memcpy(pszRecordBuf + nOffset, strStatusTag.GetString(), strStatusTag.GetLength());
	nOffset += strStatusTag.GetLength();

	if (0 != pszPTZType[0] && '0' != pszPTZType[0])
	{
		memcpy(pszRecordBuf + nOffset, strInforTag.GetString(), strInforTag.GetLength());
		nOffset += strInforTag.GetLength();

		memcpy(pszRecordBuf + nOffset, pszPTZType, 1);
		nOffset++;

		memcpy(pszRecordBuf + nOffset, strPTZTypeTag.GetString(), strPTZTypeTag.GetLength());
		nOffset += strPTZTypeTag.GetLength();
	}
	memcpy(pszRecordBuf + nOffset, strItemEndTag.GetString(), strItemEndTag.GetLength());
	nBufLen = nOffset + strItemEndTag.GetLength();

	return 0;
}

int CBodyBuilder::CompleteCatalog(char *pszRecordBuf, int nBufLen, const char *pszDataType)
{
	CString strRecordEndTag;
	strRecordEndTag.Format("</DeviceList>\r\n"
		"</%s>\r\n", pszDataType);

	if (nBufLen <= strRecordEndTag.GetLength())
		return ERROR_NOACCESS;

	// 一定把'\0'也拷贝过去
	memcpy(pszRecordBuf, strRecordEndTag.GetString(), strRecordEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CompleteCatalog(char *pszRecordBuf, int nBufLen)
{
	static CString strCatalogEndTag = "</DeviceList>\r\n"
		"</Response>\r\n";
	CLog::Log(DEVINFO, LL_NORMAL, " %s  nBufLen = %d  strCatalogEndTag len = %d \r\n", __FUNCTION__, nBufLen, strCatalogEndTag.GetLength());
	if (nBufLen <= strCatalogEndTag.GetLength())
		return ERROR_NOACCESS;

	// 一定把'\0'也拷贝过去
	memcpy(pszRecordBuf, strCatalogEndTag.GetString(), strCatalogEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateNotifyCatalogHead(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
	const char *pstrSN,									// SN号
	const char *pstrDeviceID,							// 设备ID
	int nSumNum,										// 设备总数
	int nListNum)										// 当前xml中包含的设备个数
{
	CString strVersionTag = "<?xml version=\"1.0\" ?>\r\n"
		"<Notify>\r\n"
		"<CmdType>Catalog</CmdType>\r\n"
		"<SN>";

	CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	CString strDeviceIDTag = "</DeviceID>\r\n"
		"<SumNum>";

	CString strSumNumTag = "</SumNum>\r\n"
		"<DeviceList Num=\"";

	CString strEnterTag = "\">\r\n";
	int nStaitcCharLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strSumNumTag.GetLength();

	CString strSumNum;
	CString strListNum;
	strSumNum.Format("%d", nSumNum);
	strListNum.Format("%d", nListNum);
	int nSNLen = strlen(pstrSN);

	if (nBufLen <= nStaitcCharLen + strSumNum.GetLength() + strListNum.GetLength() + nSNLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	int nOffset = 0;

	// 添加version头
	memcpy(pszRecordBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	// 添加SN
	memcpy(pszRecordBuf + nOffset, pstrSN, nSNLen);
	nOffset += nSNLen;

	// 添加SN Tag
	memcpy(pszRecordBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	// 添加DeviceID
	memcpy(pszRecordBuf + nOffset, pstrDeviceID, ID_LEN);
	nOffset += ID_LEN;

	// 添加DeviceID Tag
	memcpy(pszRecordBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	// 添加SumNum
	memcpy(pszRecordBuf + nOffset, strSumNum.GetString(), strSumNum.GetLength());
	nOffset += strSumNum.GetLength();

	// 添加SumNum Tag
	memcpy(pszRecordBuf + nOffset, strSumNumTag.GetString(), strSumNumTag.GetLength());
	nOffset += strSumNumTag.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strListNum.GetString(), strListNum.GetLength());
	nOffset += strListNum.GetLength();

	// 添加List Num
	memcpy(pszRecordBuf + nOffset, strEnterTag.GetString(), strEnterTag.GetLength() + 1);
	nBufLen = nOffset + strEnterTag.GetLength();

	return 0;
}

// 添加Catalog xml body
int CBodyBuilder::AddNotifyCatalogBody(char *pszRecordBuf,					// xml文件的缓存
	int &nBufLen,										// IN-缓存长度，OUT-缓存已填充长度
	void *pszCatalog)
{
	if (pszCatalog == nullptr)
	{
		return ERROR_NOACCESS;
	}

	CatalogItem* catalog = static_cast<CatalogItem*>(pszCatalog);

	const int nodeNum = 52;
	//    const int nodeInfoNum = 16;

	CString strNotifyCatalogNode[nodeNum] = {
		"DeviceID",     catalog->GetDeviceID(),
		"Name",         catalog->GetName(),
		"Manufacturer", catalog->GetManufacturer(),
		"Model",        catalog->GetModel(),
		"Owner",        catalog->GetOwner(),
		"CivilCode",    catalog->GetCivilCode(),
		"Block",        catalog->GetBlock(),
		"Address",      catalog->GetAddress(),
		"Parental",     catalog->GetParental(),
		"ParentID",     catalog->GetParentID(),
		"SafetyWay",    catalog->GetSafetyWay(),
		"RegisterWay",  catalog->GetRegisterWay(),
		"CertNum",      catalog->GetCertNum(),
		"Certifiable",  catalog->GetCertifiable(),
		"ErrCode",      catalog->GetErrcode(),
		"EndTime",      catalog->GetEndTime(),
		"Secrecy",      catalog->GetSecrecy(),
		"IPAddress",    catalog->GetIPAddress(),
		"Port",         catalog->GetPort(),
		"Password",     catalog->GetPassword(),
		"Status",       catalog->GetStatus(),
		"Longitude",    catalog->GetLongitude(),
		"Latitude",     catalog->GetLatitude(),
		//     "ResType",      catalog->GetResType(),
			 "RecLocation",  catalog->GetRecLocation(),
			 "Operatetype",  catalog->GetOperateType(),
			 //      "PlayUrl",      catalog->GetPlayUrl()
				   "ChannelNum",   catalog->m_strChnNum,
	};

	/*  CString strNotifyCatalogNodeInfo[nodeInfoNum] = {
		  "PTZType",          catalog->GetPTZType(),
		  "PositionType",     catalog->GetPositionType(),
		  "RoomType",         catalog->GetRoomType(),
		  "UseType",          catalog->GetUseType(),
		  "SupplyLightType",  catalog->GetSupplyLightType(),
		  "DirectionType",    catalog->GetDirectionType(),
		  "Resolution",       catalog->GetResolution(),
		  "BusinessGroupID",  catalog->GetBusinessGroupID()
	  };*/

	auto doc = new XMLDocument();
	auto root = doc->NewElement("Item");
	doc->InsertEndChild(root);

	for (int i = 0; i < nodeNum; i += 2)
	{
		auto node = doc->NewElement(strNotifyCatalogNode[i]);
		node->SetText(strNotifyCatalogNode[i + 1]);
		root->InsertEndChild(node);
	}

	/*TiXmlElement *info = new TiXmlElement("Info");
	root->LinkEndChild(info);

	for(int i = 0; i < nodeInfoNum; i+=2)
	{
	TiXmlElement *node = new TiXmlElement(strNotifyCatalogNodeInfo[i]);
	TiXmlText *val = new TiXmlText(strNotifyCatalogNodeInfo[i+1]);
	info->LinkEndChild(node);
	node->LinkEndChild(val);
	}*/

	XMLPrinter printer;
	doc->Accept(&printer);

	if (printer.CStrSize() >= nBufLen)
	{
		return ERROR_NOACCESS;
	}
	nBufLen = printer.CStrSize();

	memcpy(pszRecordBuf, printer.CStr(), printer.CStrSize());

	delete doc;

	return 0;
}

// 完成xml
int CBodyBuilder::CompleteNotifyCatalog(char *pRecordBuf, int nBufLen)
{
	static CString strNotifyCatalogEndTag = "</DeviceList>\r\n"
		"</Notify>";
	CLog::Log(DEVINFO, LL_NORMAL, " %s  nBufLen = %d  strCatalogEndTag len = %d \r\n", __FUNCTION__, nBufLen, strNotifyCatalogEndTag.GetLength());
	if (nBufLen <= strNotifyCatalogEndTag.GetLength())
		return ERROR_NOACCESS;

	// 一定把'\0'也拷贝过去
	memcpy(pRecordBuf, strNotifyCatalogEndTag.GetString(), strNotifyCatalogEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateStatusBody(char *pStatusBuf, int &nBufLen,
	const char *pszSN,
	const char *pszDeviceID,
	const char *pszResult,
	const char *pszOnlien,
	const char *pszStatus,
	const char *pszEncode,
	const char *pszRecord,
	const char *pszTime,
	int nAlarmSum,
	bool bIsEncoder)
{
	static CString strVersionTag = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>DeviceStatus</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<Result>";

	static CString strResultTag = "</Result>\r\n"
		"<Online>";

	static CString strOnlienTag = "</Online>\r\n"
		"<Status>";

	static CString strStatusTag = "</Status>\r\n";
	static CString strEncodeTag1 = "<Encode>";
	////ON
	static CString strEncodeTag2 = "</Encode>\r\n"
		"<Record>";
	////OFF
	static CString strRecordTag = "</Record>\r\n";
	static CString strTime = "<DeviceTime>";

	static CString strTimeTag = "</DeviceTime>\r\n";

	static CString strNum = "<Alarmstatus Num=\"";

	static CString strNumTag = "\">\r\n";

	//static CString strAlarmTag =	"</Alarmstatus>\r\n";
	static CString strResponseTag = "</Response>\r\n";

	static int nHeadTagLen = strVersionTag.GetLength() +
		strSNTag.GetLength() +
		strDeviceIDTag.GetLength() +
		strResultTag.GetLength() +
		strOnlienTag.GetLength() +
		strStatusTag.GetLength() +
		strTimeTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	int nOnlineLen = strlen(pszOnlien);
	int nStatusLen = strlen(pszStatus);
	int nEncodeLen = strlen(pszEncode);
	int nRecordLen = strlen(pszRecord);

	CString strAlarmNum;
	strAlarmNum.Format("%d", nAlarmSum);

	// 有alarm通道
	if (bIsEncoder)
	{
		// 长度检查
		if (nBufLen <= nHeadTagLen +
			strEncodeTag1.GetLength() +
			strEncodeTag2.GetLength() +
			strRecordTag.GetLength() +
			strTime.GetLength() + strTimeTag.GetLength() +
			strNum.GetLength() + strNumTag.GetLength() + strAlarmNum.GetLength() +
			nSNLen + nResultLen + nOnlineLen + nStatusLen +
			nEncodeLen + nRecordLen +
			ID_LEN + TIME_LEN)
			return ERROR_NOACCESS;
	}
	// 解码器没有alarm通道
	else
	{
		// 长度检查
		if (nBufLen <= nHeadTagLen + strTime.GetLength() +
			strTimeTag.GetLength() + strResponseTag.GetLength() +
			nSNLen + nResultLen + nOnlineLen + nStatusLen +
			ID_LEN + TIME_LEN)
			return ERROR_NOACCESS;
	}

	int nOffset = 0;
	memcpy(pStatusBuf + nOffset, strVersionTag.GetString(), strVersionTag.GetLength());
	nOffset += strVersionTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;

	memcpy(pStatusBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszDeviceID, ID_LEN);
	nOffset += ID_LEN;

	memcpy(pStatusBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;

	memcpy(pStatusBuf + nOffset, strResultTag.GetString(), strResultTag.GetLength());
	nOffset += strResultTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszOnlien, nOnlineLen);
	nOffset += nOnlineLen;

	memcpy(pStatusBuf + nOffset, strOnlienTag.GetString(), strOnlienTag.GetLength());
	nOffset += strOnlienTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszStatus, nStatusLen);
	nOffset += nStatusLen;

	memcpy(pStatusBuf + nOffset, strStatusTag.GetString(), strStatusTag.GetLength());
	nOffset += strStatusTag.GetLength();

	//// 编码器
	if (bIsEncoder)
	{
		memcpy(pStatusBuf + nOffset, strEncodeTag1.GetString(), strEncodeTag1.GetLength());
		nOffset += strEncodeTag1.GetLength();

		memcpy(pStatusBuf + nOffset, pszEncode, nEncodeLen);
		nOffset += nEncodeLen;

		memcpy(pStatusBuf + nOffset, strEncodeTag2.GetString(), strEncodeTag2.GetLength());
		nOffset += strEncodeTag2.GetLength();

		memcpy(pStatusBuf + nOffset, pszRecord, nRecordLen);
		nOffset += nRecordLen;

		memcpy(pStatusBuf + nOffset, strRecordTag.GetString(), strRecordTag.GetLength());
		nOffset += strRecordTag.GetLength();
	}

	memcpy(pStatusBuf + nOffset, strTime.GetString(), strTime.GetLength());
	nOffset += strTime.GetLength();

	memcpy(pStatusBuf + nOffset, pszTime, TIME_LEN);
	nOffset += TIME_LEN;

	memcpy(pStatusBuf + nOffset, strTimeTag.GetString(), strTimeTag.GetLength());
	nOffset += strTimeTag.GetLength();

	// 编码器的通道
	if (bIsEncoder)
	{
		memcpy(pStatusBuf + nOffset, strNum.GetString(), strNum.GetLength());
		nOffset += strNum.GetLength();

		memcpy(pStatusBuf + nOffset, strAlarmNum.GetString(), strAlarmNum.GetLength());
		nOffset += strAlarmNum.GetLength();

		memcpy(pStatusBuf + nOffset, strNumTag.GetString(), strNumTag.GetLength() + 1);
		nOffset += strNumTag.GetLength();
	}
	// 解码器
	else
	{
		memcpy(pStatusBuf + nOffset, strResponseTag.GetString(), strResponseTag.GetLength() + 1);
		nOffset += strResponseTag.GetLength();
	}

	nBufLen = nOffset;
	return 0;
}

int CBodyBuilder::AddAlarmToStatusBody(char *pStatusBuf, int &nBufLen,
	const char *pszAlarmID, const char *pszDutyStatus)
{
	static CString strItem = "<Item>\r\n"
		"<DeviceID>";

	static CString strAlarmIDTag = "</DeviceID>\r\n"
		"<DutyStatus>";

	static CString strDutyTag = "</DutyStatus>\r\n"
		"</Item>\r\n";

	static int nTagLen = strItem.GetLength() + strAlarmIDTag.GetLength() + strDutyTag.GetLength();

	int nAlarmLen = strlen(pszAlarmID);
	int nDutyStatus = strlen(pszDutyStatus);

	// 缓存大小检测
	if (nBufLen <= nTagLen + nAlarmLen + nDutyStatus)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pStatusBuf + nOffset, strItem, strItem.GetLength());
	nOffset += strItem.GetLength();

	memcpy(pStatusBuf + nOffset, pszAlarmID, nAlarmLen);
	nOffset += nAlarmLen;

	memcpy(pStatusBuf + nOffset, strAlarmIDTag, strAlarmIDTag.GetLength());
	nOffset += strAlarmIDTag.GetLength();

	memcpy(pStatusBuf + nOffset, pszDutyStatus, nDutyStatus);
	nOffset += nDutyStatus;

	memcpy(pStatusBuf + nOffset, strDutyTag, strDutyTag.GetLength());
	nOffset += strDutyTag.GetLength();

	nBufLen = nOffset;
	return 0;
}

int CBodyBuilder::ComplateStatusBody(char *pStatusBuf, int nBufLen)
{
	static CString strAlarmTag = "</Alarmstatus>\r\n";
	static CString strResponseTag = "</Response>\r\n";

	static int nEndTagLen = strAlarmTag.GetLength() + strResponseTag.GetLength();

	// 缓存大小检测
	if (nBufLen <= nEndTagLen)
		return ERROR_NOACCESS;

	memcpy(pStatusBuf, strAlarmTag.GetString(), strAlarmTag.GetLength());
	memcpy(pStatusBuf + strAlarmTag.GetLength(), strResponseTag.GetString(), strResponseTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreatePropertyBody(char *pPropertyBuf, int nBufLen,
	const char *pszSN,
	const char *pszID,
	const char *pszResult,
	const char *pszDevType,
	const char *pszManufacturer,
	const char *pszModel,
	const char *pszFirmware,
	const char *pszMaxCamera,
	const char *pszMaxAlarm,
	bool bIsEncoder)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>DeviceInfo</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strIDTag = "</DeviceID>\r\n"
		"<Result>";

	static CString strResultTag = "</Result>\r\n"
		"<DeviceType>";

	static CString strDevTypeTag = "</DeviceType>\r\n"
		"<Manufacturer>";

	static CString strManufacturerTag = "</Manufacturer>\r\n"
		"<Model>";

	static CString strModelTag = "</Model>\r\n"
		"<Firmware>";

	static CString strFirmwareTag = "</Firmware>\r\n";
	static CString strMaxCameraTag = "<MaxCamera>";

	static CString strMaxCameraTag2 = "</MaxCamera>\r\n"
		"<MaxAlarm>";

	static CString strMaxAlarmTag = "</MaxAlarm>\r\n"
		"</Response>\r\n";

	static CString strMaxOutTag = "<MaxOut>";

	static CString strMaxOutTag2 = "</MaxOut>\r\n"
		"</Response>\r\n";

	static const int nTagLen = strVersion.GetLength() + strSNTag.GetLength() +
		strIDTag.GetLength() + strResultTag.GetLength() +
		strDevTypeTag.GetLength() + strManufacturerTag.GetLength() +
		strModelTag.GetLength() + strFirmwareTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	int nDevTypeLen = strlen(pszDevType);
	int nManufacturerLen = strlen(pszManufacturer);
	int nModelLen = strlen(pszModel);
	int nFirmwareLen = strlen(pszFirmware);
	int nMaxAlarmLen = strlen(pszMaxAlarm);
	int nMaxCameraLen = strlen(pszMaxCamera);

	// 编码器
	if (bIsEncoder)
	{
		// 缓存大小检测
		if (nBufLen <= nTagLen + nMaxCameraLen + strMaxCameraTag.GetLength() + strMaxAlarmTag.GetLength() +
			nSNLen + nResultLen + nDevTypeLen + nManufacturerLen +
			nModelLen + nFirmwareLen + ID_LEN + strMaxCameraTag2.GetLength() + nMaxAlarmLen)
			return ERROR_NOACCESS;
	}
	// 解码器
	else
	{
		// 缓存大小检测
		if (nBufLen <= nTagLen + nMaxCameraLen + strMaxOutTag.GetLength() + strMaxOutTag2.GetLength() +
			nSNLen + nResultLen + nDevTypeLen + nManufacturerLen + nModelLen + nFirmwareLen + ID_LEN + nMaxAlarmLen)
			return ERROR_NOACCESS;
	}

	int nOffset = 0;
	memcpy(pPropertyBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();

	memcpy(pPropertyBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;

	memcpy(pPropertyBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszID, ID_LEN);
	nOffset += ID_LEN;

	memcpy(pPropertyBuf + nOffset, strIDTag.GetString(), strIDTag.GetLength());
	nOffset += strIDTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;

	memcpy(pPropertyBuf + nOffset, strResultTag.GetString(), strResultTag.GetLength());
	nOffset += strResultTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszDevType, nDevTypeLen);
	nOffset += nDevTypeLen;

	memcpy(pPropertyBuf + nOffset, strDevTypeTag.GetString(), strDevTypeTag.GetLength());
	nOffset += strDevTypeTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszManufacturer, nManufacturerLen);
	nOffset += nManufacturerLen;

	memcpy(pPropertyBuf + nOffset, strManufacturerTag.GetString(), strManufacturerTag.GetLength());
	nOffset += strManufacturerTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszModel, nModelLen);
	nOffset += nModelLen;

	memcpy(pPropertyBuf + nOffset, strModelTag.GetString(), strModelTag.GetLength());
	nOffset += strModelTag.GetLength();

	memcpy(pPropertyBuf + nOffset, pszFirmware, nFirmwareLen);
	nOffset += nFirmwareLen;

	memcpy(pPropertyBuf + nOffset, strFirmwareTag.GetString(), strFirmwareTag.GetLength());
	nOffset += strFirmwareTag.GetLength();

	// 编码器
	if (bIsEncoder)
	{
		memcpy(pPropertyBuf + nOffset, strMaxCameraTag.GetString(), strMaxCameraTag.GetLength());
		nOffset += strMaxCameraTag.GetLength();

		memcpy(pPropertyBuf + nOffset, pszMaxCamera, nMaxCameraLen);
		nOffset += nMaxCameraLen;

		memcpy(pPropertyBuf + nOffset, strMaxCameraTag2.GetString(), strMaxCameraTag2.GetLength());
		nOffset += strMaxCameraTag2.GetLength();

		memcpy(pPropertyBuf + nOffset, pszMaxAlarm, nMaxAlarmLen);
		nOffset += nMaxAlarmLen;

		memcpy(pPropertyBuf + nOffset, strMaxAlarmTag.GetString(), strMaxAlarmTag.GetLength() + 1);
	}
	// 解码器
	else
	{
		memcpy(pPropertyBuf + nOffset, strMaxOutTag.GetString(), strMaxOutTag.GetLength());
		nOffset += strMaxOutTag.GetLength();

		memcpy(pPropertyBuf + nOffset, pszMaxCamera, nMaxCameraLen);
		nOffset += nMaxCameraLen;

		memcpy(pPropertyBuf + nOffset, strMaxOutTag2.GetString(), strMaxOutTag2.GetLength() + 1);
	}

	return 0;
}

int CBodyBuilder::CreateMediaStatusBody(char *pMediaStatusBuf, int nBufLen,
	const char *pszSN,
	const char *pszID)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Notify>\r\n"
		"<CmdType>MediaStatus</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<NotifyType>121</ NotifyType>\r\n"
		"</Notify>\r\n";

	static const int nTagLen = strVersion.GetLength() + strSNTag.GetLength() + strDeviceIDTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nIDLen = strlen(pszID);
	if (nBufLen <= nTagLen + nSNLen + nIDLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pMediaStatusBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();

	memcpy(pMediaStatusBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;

	memcpy(pMediaStatusBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();

	memcpy(pMediaStatusBuf + nOffset, pszID, nIDLen);
	nOffset += nIDLen;

	memcpy(pMediaStatusBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength() + 1);

	return 0;
}

// 创建设备控制应答的SIP数据包
int CBodyBuilder::CreateControlResponseBody(char *pszResponseBuf, int nBufLen, const char *pszSN, const char *pszDevID, const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n"
		"<Result>";
	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen + nResultLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	// 攒保活数据包的包体
	int nOffset = 0;
	memcpy(pszResponseBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszResponseBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszResponseBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszResponseBuf + nOffset, strDeviceTag, strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;
	memcpy(pszResponseBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateDecoderDivisonResponseBody(char *pszResponseBuf, int nBufLen,
	const char *pszSN,
	const char *pszDevID,
	const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>DecoderDivision</CmdType>\r\n"
		"<SN>";
	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n"
		"<Result>";
	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen + nResultLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	// 攒保活数据包的包体
	int nOffset = 0;
	memcpy(pszResponseBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszResponseBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszResponseBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszResponseBuf + nOffset, strDeviceTag, strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;
	memcpy(pszResponseBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateStopPlayUrlResponseBody(char *pszResponseBuf, int nBufLen,
	const char *pszSN,
	const char *pszDevID,
	const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>StopPlayUrl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n"
		"<Result>";
	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen + nResultLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	// 攒保活数据包的包体
	int nOffset = 0;
	memcpy(pszResponseBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszResponseBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszResponseBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszResponseBuf + nOffset, strDeviceTag, strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;
	memcpy(pszResponseBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateDeviceConfigResponse(char *pszResponseBuf, int nBufLen, const char *pszSN, const char *pszDevID, const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>DeviceConfig</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";
	static CString strDeviceTag = "</DeviceID>\r\n"
		"<Result>";
	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen + nResultLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	// 攒保活数据包的包体
	int nOffset = 0;
	memcpy(pszResponseBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszResponseBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszResponseBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszDevID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pszResponseBuf + nOffset, strDeviceTag, strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();
	memcpy(pszResponseBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;
	memcpy(pszResponseBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateConfigSearchHead(char *pSearchBuf, int &nBufLen,
	const char *pszSN,
	const char *pszDeviceID,
	const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>ConfigDownload</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceTag = "</DeviceID>\r\n"
		"<Result>";

	static CString strEndTag = "</Result>\r\n";

	int nSNLen = strlen(pszSN);
	int nResultLen = strlen(pszResult);
	if (nBufLen < strVersion.GetLength() +
		strSNTag.GetLength() +
		strDeviceTag.GetLength() +
		strEndTag.GetLength() +
		nSNLen + nResultLen + ID_BUF_LEN)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pSearchBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pSearchBuf + nOffset, strSNTag, strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszDeviceID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pSearchBuf + nOffset, strDeviceTag, strDeviceTag.GetLength());
	nOffset += strDeviceTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;
	memcpy(pSearchBuf + nOffset, strEndTag, strEndTag.GetLength() + 1);
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddBasicParamConfig(char *pSearchBuf, int &nBufLen,
	const char *pName,
	const char *pDeviceID,
	const char *pServerID,
	const char *pServerIP,
	const char *pServerPort,
	const char *pDomainName,
	const char *pExpiration,
	const char *pPasswd,
	const char *pHeartBeat,
	const char *pHeartBeatCount)
{
	static CString strNameTag = "<BasicParam>\r\n"
		"<Name>\r\n";

	static CString strIDTag = "</Name>\r\n"
		"<DeviceID>\r\n";

	static CString strSipIDTag = "</DeviceID>\r\n"
		"<SIPServerID>\r\n";

	static CString strSipIPTag = "</SIPServerID>\r\n"
		"<SIPServerIP>\r\n";

	static CString strSipPortTag = "</SIPServerIP>\r\n"
		"<SIPServerPort>\r\n";

	static CString strDomainTag = "</SIPServerPort>\r\n"
		"<DomainName>\r\n";

	static CString strExpirationTag = "</DomainName>\r\n"
		"<Expiration>\r\n";

	static CString strPasswdTag = "</Expiration>\r\n"
		"<Password>\r\n";

	static CString strHeartBeat = "</Password>\r\n"
		"<HeartBeatInterval>\r\n";

	static CString strHeartBeatCount = "</HeartBeatInterval>\r\n"
		"<HeartBeatCount>\r\n";

	static CString strEndTag = "</HeartBeatCount>\r\n"
		"</BasicParam>\r\n";

	int nNameLen = strlen(pName);
	int nIPLen = strlen(pServerIP);
	int nPortLen = strlen(pServerPort);
	int nDomainLen = strlen(pDomainName);
	int nExpiryLen = strlen(pExpiration);
	int nPasswdLen = strlen(pPasswd);
	int nHeartBeatLen = strlen(pHeartBeat);
	int nHBeatCountLen = strlen(pHeartBeatCount);
	if (nBufLen < strNameTag.GetLength() +
		strIDTag.GetLength() +
		strSipIDTag.GetLength() +
		strSipIPTag.GetLength() +
		strSipPortTag.GetLength() +
		strDomainTag.GetLength() +
		strExpirationTag.GetLength() +
		strPasswdTag.GetLength() +
		strHeartBeat.GetLength() +
		strHeartBeatCount.GetLength() +
		strEndTag.GetLength() +
		nNameLen + ID_BUF_LEN + ID_BUF_LEN +
		nIPLen + nPortLen + nDomainLen +
		nExpiryLen + nPasswdLen + nHeartBeatLen +
		nHBeatCountLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strNameTag.GetString(), strNameTag.GetLength());
	nOffset += strNameTag.GetLength();
	memcpy(pSearchBuf + nOffset, pName, nNameLen);
	nOffset += nNameLen;
	memcpy(pSearchBuf + nOffset, strIDTag.GetString(), strIDTag.GetLength());
	nOffset += strIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pDeviceID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pSearchBuf + nOffset, strSipIDTag.GetString(), strSipIDTag.GetLength());
	nOffset += strSipIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pServerID, ID_LEN);
	nOffset += ID_LEN;
	memcpy(pSearchBuf + nOffset, strSipIPTag.GetString(), strSipIPTag.GetLength());
	nOffset += strSipIPTag.GetLength();
	memcpy(pSearchBuf + nOffset, pServerIP, nIPLen);
	nOffset += nIPLen;
	memcpy(pSearchBuf + nOffset, strSipPortTag.GetString(), strSipPortTag.GetLength());
	nOffset += strSipPortTag.GetLength();
	memcpy(pSearchBuf + nOffset, pServerPort, nPortLen);
	nOffset += nPortLen;
	memcpy(pSearchBuf + nOffset, strDomainTag.GetString(), strDomainTag.GetLength());
	nOffset += strDomainTag.GetLength();
	memcpy(pSearchBuf + nOffset, pDomainName, nDomainLen);
	nOffset += nDomainLen;
	memcpy(pSearchBuf + nOffset, strExpirationTag.GetString(), strExpirationTag.GetLength());
	nOffset += strExpirationTag.GetLength();
	memcpy(pSearchBuf + nOffset, pExpiration, nExpiryLen);
	nOffset += nExpiryLen;
	memcpy(pSearchBuf + nOffset, strPasswdTag.GetString(), strPasswdTag.GetLength());
	nOffset += strPasswdTag.GetLength();
	memcpy(pSearchBuf + nOffset, pPasswd, nPasswdLen);
	nOffset += nPasswdLen;
	memcpy(pSearchBuf + nOffset, strHeartBeat.GetString(), strHeartBeat.GetLength());
	nOffset += strHeartBeat.GetLength();
	memcpy(pSearchBuf + nOffset, pHeartBeat, nHeartBeatLen);
	nOffset += nHeartBeatLen;
	memcpy(pSearchBuf + nOffset, strHeartBeatCount.GetString(), strHeartBeatCount.GetLength());
	nOffset += strHeartBeatCount.GetLength();
	memcpy(pSearchBuf + nOffset, pHeartBeatCount, nHBeatCountLen);
	nOffset += nHBeatCountLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddVideoParanOpt(char *pSearchBuf, int &nBufLen,
	const char *pVideoFormat,
	const char *pResolution,
	const char *pFramRate,
	const char *pBitRateType,
	const char *pVideoBitRate,
	const char *pDownloadSpeed)
{
	static CString strVideoFormatTag = "<VideoParamOpt>\r\n"
		"<VideoFormatOpt>\r\n";

	static CString strResolutionTag = "</VideoFormatOpt>\r\n"
		"<ResolutionOpt>\r\n";

	static CString strFrameRateTag = "</ResolutionOpt>\r\n"
		"<FrameRateOpt>\r\n";

	static CString strBitrateTypeTag = "</FrameRateOpt>\r\n"
		"<BitRateTypeOpt>\r\n";

	static CString strVideoBitRateTag = "</BitRateTypeOpt>\r\n"
		"<VideoBitRateOpt>\r\n";

	static CString strDownloadSpeedTag = "</VideoBitRateOpt>\r\n"
		"<DownloadSpeedOpt>\r\n";

	static CString strEndTag = "</DownloadSpeedOpt>\r\n"
		"</VideoParamOpt>\r\n";

	int nVideoFormatLen = strlen(pVideoFormat);
	int nResolutionLen = strlen(pResolution);
	int nFramRateLen = strlen(pFramRate);
	int nBitRateTypeLen = strlen(pBitRateType);
	int nVideoBitRateLen = strlen(pVideoBitRate);
	int nDownloadSpeedLen = strlen(pDownloadSpeed);
	if (nBufLen < strVideoFormatTag.GetLength() +
		strResolutionTag.GetLength() +
		strFrameRateTag.GetLength() +
		strBitrateTypeTag.GetLength() +
		strVideoBitRateTag.GetLength() +
		strDownloadSpeedTag.GetLength() +
		strEndTag.GetLength() +
		nVideoFormatLen + nResolutionLen + nFramRateLen +
		nBitRateTypeLen + nVideoBitRateLen + nDownloadSpeedLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strVideoFormatTag.GetString(), strVideoFormatTag.GetLength());
	nOffset += strVideoFormatTag.GetLength();
	memcpy(pSearchBuf + nOffset, pVideoFormat, nVideoFormatLen);
	nOffset += nVideoFormatLen;
	memcpy(pSearchBuf + nOffset, strResolutionTag.GetString(), strResolutionTag.GetLength());
	nOffset += strResolutionTag.GetLength();
	memcpy(pSearchBuf + nOffset, pResolution, nResolutionLen);
	nOffset += nResolutionLen;
	memcpy(pSearchBuf + nOffset, strFrameRateTag.GetString(), strFrameRateTag.GetLength());
	nOffset += strFrameRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pFramRate, nFramRateLen);
	nOffset += nFramRateLen;
	memcpy(pSearchBuf + nOffset, strBitrateTypeTag.GetString(), strBitrateTypeTag.GetLength());
	nOffset += strBitrateTypeTag.GetLength();
	memcpy(pSearchBuf + nOffset, pBitRateType, nBitRateTypeLen);
	nOffset += nBitRateTypeLen;
	memcpy(pSearchBuf + nOffset, strVideoBitRateTag.GetString(), strVideoBitRateTag.GetLength());
	nOffset += strVideoBitRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pVideoBitRate, nVideoBitRateLen);
	nOffset += nVideoBitRateLen;
	memcpy(pSearchBuf + nOffset, strDownloadSpeedTag.GetString(), strDownloadSpeedTag.GetLength());
	nOffset += strDownloadSpeedTag.GetLength();
	memcpy(pSearchBuf + nOffset, pDownloadSpeed, nDownloadSpeedLen);
	nOffset += nDownloadSpeedLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddVideoParamConfigHead(char *pSearchBuf, int &nBufLen, int nNum)
{
	static CString strHeadTag = "<VideoParamConfig Num=\"";

	static CString strEndTag = "\">\r\n";

	CString strNum;
	strNum.Format("%d", nNum);

	if (nBufLen < strNum.GetLength() +
		strHeadTag.GetLength() +
		strEndTag.GetLength())
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strHeadTag.GetString(), strHeadTag.GetLength());
	nOffset += strHeadTag.GetLength();
	memcpy(pSearchBuf + nOffset, strNum.GetString(), strNum.GetLength());
	nOffset += strNum.GetLength();
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddVideoParamConfig(char *pSearchBuf, int &nBufLen,
	const char *pStreamName,
	const char *pVideoFormat,
	const char *pResolution,
	const char *pFrameRate,
	const char *pBitRateType,
	const char *pVideoBitRate)
{
	static CString strStreamNameTag = "<Item>\r\n"
		"<StreamName>\r\n";

	static CString strVideoFormatTag = "</StreamName>\r\n"
		"<VideoFormat>\r\n";

	static CString strResolutionTag = "</VideoFormat>\r\n"
		"<Resolution>\r\n";

	static CString strFrameRateTag = "</Resolution>\r\n"
		"<FrameRate>\r\n";

	static CString strBitrateTypeTag = "</FrameRate>\r\n"
		"<BitRateType>\r\n";

	static CString strVideoBitRateTag = "</BitRateType>\r\n"
		"<VideoBitRate>\r\n";

	static CString strEndTag = "</VideoBitRate>\r\n"
		"</Item>\r\n";

	int nVideoFormatLen = strlen(pVideoFormat);
	int nResolutionLen = strlen(pResolution);
	int nFramRateLen = strlen(pFrameRate);
	int nBitRateTypeLen = strlen(pBitRateType);
	int nVideoBitRateLen = strlen(pVideoBitRate);
	int nStreamNameLen = strlen(pStreamName);
	if (nBufLen < strVideoFormatTag.GetLength() +
		strResolutionTag.GetLength() +
		strFrameRateTag.GetLength() +
		strBitrateTypeTag.GetLength() +
		strVideoBitRateTag.GetLength() +
		strStreamNameTag.GetLength() +
		strEndTag.GetLength() +
		nVideoFormatLen + nResolutionLen + nFramRateLen +
		nBitRateTypeLen + nVideoBitRateLen + nStreamNameLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strStreamNameTag.GetString(), strStreamNameTag.GetLength());
	nOffset += strStreamNameTag.GetLength();
	memcpy(pSearchBuf + nOffset, pStreamName, nStreamNameLen);
	nOffset += nStreamNameLen;
	memcpy(pSearchBuf, strVideoFormatTag.GetString(), strVideoFormatTag.GetLength());
	nOffset += strVideoFormatTag.GetLength();
	memcpy(pSearchBuf + nOffset, pVideoFormat, nVideoFormatLen);
	nOffset += nVideoFormatLen;
	memcpy(pSearchBuf + nOffset, strResolutionTag.GetString(), strResolutionTag.GetLength());
	nOffset += strResolutionTag.GetLength();
	memcpy(pSearchBuf + nOffset, pResolution, nResolutionLen);
	nOffset += nResolutionLen;
	memcpy(pSearchBuf + nOffset, strFrameRateTag.GetString(), strFrameRateTag.GetLength());
	nOffset += strFrameRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pFrameRate, nFramRateLen);
	nOffset += nFramRateLen;
	memcpy(pSearchBuf + nOffset, strBitrateTypeTag.GetString(), strBitrateTypeTag.GetLength());
	nOffset += strBitrateTypeTag.GetLength();
	memcpy(pSearchBuf + nOffset, pBitRateType, nBitRateTypeLen);
	nOffset += nBitRateTypeLen;
	memcpy(pSearchBuf + nOffset, strVideoBitRateTag.GetString(), strVideoBitRateTag.GetLength());
	nOffset += strVideoBitRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pVideoBitRate, nVideoBitRateLen);
	nOffset += nVideoBitRateLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::CompleteVideoParamConfig(char *pSearchBuf, int &nBufLen)
{
	static CString strResponseTag = "</VideoParamConfig>\r\n";

	static int nEndTagLen = strResponseTag.GetLength();

	if (nBufLen <= nEndTagLen)
		return ERROR_NOACCESS;

	memcpy(pSearchBuf, strResponseTag.GetString(), strResponseTag.GetLength());

	nBufLen = nEndTagLen;

	return 0;
}

int CBodyBuilder::AddAudioParamOpt(char *pSearchBuf, int &nBufLen,
	const char *pAudioFormat,
	const char *pAudioBitRate,
	const char*pSamplingRate)
{
	static CString strAudioFormatTag = "<Item>\r\n"
		"<AudioFormatOpt>\r\n";

	static CString strAudioBitRateTag = "</AudioFormatOpt>\r\n"
		"<AudioBitRateOpt>\r\n";

	static CString strSamplingRateTag = "</AudioBitRateOpt>\r\n"
		"<SamplingRateOpt>\r\n";

	static CString strEndTag = "</SamplingRateOpt>\r\n"
		"</Item>\r\n";

	int nAudioFormatLen = strlen(pAudioFormat);
	int nAudioBitRateLen = strlen(pAudioBitRate);
	int nSamplingRateLen = strlen(pSamplingRate);
	if (nBufLen < strAudioFormatTag.GetLength() +
		strAudioBitRateTag.GetLength() +
		strSamplingRateTag.GetLength() +
		strEndTag.GetLength() +
		nAudioFormatLen + nAudioBitRateLen + nSamplingRateLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strAudioFormatTag.GetString(), strAudioFormatTag.GetLength());
	nOffset += strAudioFormatTag.GetLength();
	memcpy(pSearchBuf + nOffset, pAudioFormat, nAudioFormatLen);
	nOffset += nAudioFormatLen;
	memcpy(pSearchBuf + nOffset, strAudioBitRateTag.GetString(), strAudioBitRateTag.GetLength());
	nOffset += strAudioBitRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pAudioBitRate, nAudioBitRateLen);
	nOffset += nAudioBitRateLen;
	memcpy(pSearchBuf + nOffset, strSamplingRateTag.GetString(), strSamplingRateTag.GetLength());
	nOffset += strSamplingRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pSamplingRate, nSamplingRateLen);
	nOffset += nSamplingRateLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddAudioParamConfigHead(char *pSearchBuf, int &nBufLen, int nNum)
{
	static CString strHeadTag = "<AudioParamConfig Num=\"";

	static CString strEndTag = "\">\r\n";

	CString strNum;
	strNum.Format("%d", nNum);

	if (nBufLen < strNum.GetLength() +
		strHeadTag.GetLength() +
		strEndTag.GetLength())
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf, strHeadTag.GetString(), strHeadTag.GetLength());
	nOffset += strHeadTag.GetLength();
	memcpy(pSearchBuf + nOffset, strNum.GetString(), strNum.GetLength());
	nOffset += strNum.GetLength();
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::AddAudioParamConfig(char *pSearchBuf, int &nBufLen,
	const char *pStreamName,
	const char *pAudioFormat,
	const char *pAudioBitRate,
	const char*pSamplingRate)
{
	static CString strStreamNameTag = "<Item>\r\n"
		"<StreamName>\r\n";

	static CString strAudioFormatTag = "</StreamName>\r\n"
		"<AudioFormat>\r\n";

	static CString strSamplingRateTag = "</AudioFormat>\r\n"
		"<SamplingRate>\r\n";

	static CString strAudioBitRateTag = "</SamplingRate>\r\n"
		"<AudioBitRate>\r\n";

	static CString strEndTag = "</AudioBitRate>\r\n"
		"</Item>\r\n";

	int nStreamNameLen = strlen(pStreamName);
	int nAudioFormatLen = strlen(pAudioFormat);
	int nAudioBitRateLen = strlen(pAudioBitRate);
	int nSamplingRateLen = strlen(pSamplingRate);
	if (nBufLen < strStreamNameTag.GetLength() +
		strAudioFormatTag.GetLength() +
		strAudioBitRateTag.GetLength() +
		strSamplingRateTag.GetLength() +
		strEndTag.GetLength() +
		nStreamNameLen + nAudioFormatLen +
		nAudioBitRateLen + nSamplingRateLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strStreamNameTag.GetString(), strStreamNameTag.GetLength());
	nOffset += strStreamNameTag.GetLength();
	memcpy(pSearchBuf + nOffset, pStreamName, nStreamNameLen);
	nOffset += nStreamNameLen;
	memcpy(pSearchBuf, strAudioFormatTag.GetString(), strAudioFormatTag.GetLength());
	nOffset += strAudioFormatTag.GetLength();
	memcpy(pSearchBuf + nOffset, pAudioFormat, nAudioFormatLen);
	nOffset += nAudioFormatLen;
	memcpy(pSearchBuf + nOffset, strSamplingRateTag.GetString(), strSamplingRateTag.GetLength());
	nOffset += strSamplingRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pSamplingRate, nSamplingRateLen);
	nOffset += nSamplingRateLen;
	memcpy(pSearchBuf + nOffset, strAudioBitRateTag.GetString(), strAudioBitRateTag.GetLength());
	nOffset += strAudioBitRateTag.GetLength();
	memcpy(pSearchBuf + nOffset, pAudioBitRate, nAudioBitRateLen);
	nOffset += nAudioBitRateLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	nOffset += strEndTag.GetLength();

	nBufLen = nOffset;

	return 0;
}

int CBodyBuilder::CompleteAudioParamConfig(char *pSearchBuf, int &nBufLen)
{
	static CString strResponseTag = "</AudioParamConfig>\r\n";

	static int nEndTagLen = strResponseTag.GetLength();

	if (nBufLen <= nEndTagLen)
		return ERROR_NOACCESS;

	memcpy(pSearchBuf, strResponseTag.GetString(), strResponseTag.GetLength());

	nBufLen = nEndTagLen;

	return 0;
}

int CBodyBuilder::CompleteConfigSearch(char *pSearchBuf, int nBufLen)
{
	static CString strResponseTag = "</Response>\r\n";

	static int nEndTagLen = strResponseTag.GetLength();

	if (nBufLen <= nEndTagLen)
		return ERROR_NOACCESS;

	memcpy(pSearchBuf, strResponseTag.GetString(), strResponseTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateRealPlayUrlResponse(char *pSearchBuf, int nBufLen,
	const char *pszSN,
	const char *pszID,
	const char *pszChannelID,
	const char *pszPlayUrl)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>RealPlayUrl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<DecoderChannelID>";

	static CString strChanIDTag = "</DecoderChannelID>"
		"<PlayUrl>";

	static CString strEndTag = "</PlayUrl>\r\n"
		"</Response>\r\n";

	static const int nTagLen = strVersion.GetLength() +
		strSNTag.GetLength() + strDeviceIDTag.GetLength() +
		strChanIDTag.GetLength() + strEndTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nIDLen = strlen(pszID);
	int nUrlLen = strlen(pszPlayUrl);
	int nChanIDLen = strlen(pszChannelID);

	if (nBufLen <= nTagLen + nSNLen + nIDLen + nUrlLen + nChanIDLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pSearchBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pSearchBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszID, nIDLen);
	nOffset += nIDLen;
	memcpy(pSearchBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszChannelID, nChanIDLen);
	nOffset += nChanIDLen;
	memcpy(pSearchBuf + nOffset, strChanIDTag.GetString(), strChanIDTag.GetLength());
	nOffset += strChanIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszPlayUrl, nUrlLen);
	nOffset += nUrlLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateRealPlayUrlResponseByContrl(char *pSearchBuf, int nBufLen,
	const char *pszSN,
	const char *pszID,
	const char *pszChannelID,
	const char *pszPlayUrl,
	const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>RealPlayUrl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<DecoderChannelID>";

	static CString strChanIDTag = "</DecoderChannelID>"
		"<PlayUrl>";

	static CString strResultTag = "</PlayUrl>\r\n"
		"<Result>";

	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	static const int nTagLen = strVersion.GetLength() +
		strSNTag.GetLength() + strDeviceIDTag.GetLength() +
		strChanIDTag.GetLength() + strResultTag.GetLength() + strEndTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nIDLen = strlen(pszID);
	int nUrlLen = strlen(pszPlayUrl);
	int nChanIDLen = strlen(pszChannelID);
	int nResultLen = strlen(pszResult);

	if (nBufLen <= nTagLen + nSNLen + nIDLen + nUrlLen + nChanIDLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pSearchBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pSearchBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszID, nIDLen);
	nOffset += nIDLen;
	memcpy(pSearchBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszChannelID, nChanIDLen);
	nOffset += nChanIDLen;
	memcpy(pSearchBuf + nOffset, strChanIDTag.GetString(), strChanIDTag.GetLength());
	nOffset += strChanIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszPlayUrl, nUrlLen);
	nOffset += nUrlLen;

	memcpy(pSearchBuf + nOffset, strResultTag.GetString(), strResultTag.GetLength());
	nOffset += strResultTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;

	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreatePlayBackUrlResponse(char *pSearchBuf, int nBufLen,
	const char *pszSN,
	const char *pszID,
	const char *pszPlayUrl)
{
	static CString strVersion = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>PlayBackUrl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<PlayUrl>";

	static CString strEndTag = "</PlayUrl>\r\n"
		"</Response>\r\n";

	static const int nTagLen = strVersion.GetLength() + strSNTag.GetLength() + strDeviceIDTag.GetLength() + strEndTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nIDLen = strlen(pszID);
	int nUrlLen = strlen(pszPlayUrl);
	if (nBufLen <= nTagLen + nSNLen + nIDLen + nUrlLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pSearchBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pSearchBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszID, nIDLen);
	nOffset += nIDLen;
	memcpy(pSearchBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszPlayUrl, nUrlLen);
	nOffset += nUrlLen;
	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreatePlayBackUrlResponseByControl(char *pSearchBuf, int nBufLen,
	const char *pszSN,
	const char *pszID,
	const char *pszPlayUrl,
	const char *pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>PlayBackUrl</CmdType>\r\n"
		"<SN>";

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";

	static CString strDeviceIDTag = "</DeviceID>\r\n"
		"<PlayUrl>";

	static CString strResultTag = "</PlayUrl>\r\n"
		"<Result>";

	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	static const int nTagLen = strVersion.GetLength() + strSNTag.GetLength() + strDeviceIDTag.GetLength() + strResultTag.GetLength() + strEndTag.GetLength();

	int nSNLen = strlen(pszSN);
	int nIDLen = strlen(pszID);
	int nUrlLen = strlen(pszPlayUrl);
	int nResultLen = strlen(pszResult);

	if (nBufLen <= nTagLen + nSNLen + nIDLen + nUrlLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pSearchBuf + nOffset, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pSearchBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pSearchBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszID, nIDLen);
	nOffset += nIDLen;
	memcpy(pSearchBuf + nOffset, strDeviceIDTag.GetString(), strDeviceIDTag.GetLength());
	nOffset += strDeviceIDTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszPlayUrl, nUrlLen);
	nOffset += nUrlLen;

	memcpy(pSearchBuf + nOffset, strResultTag.GetString(), strResultTag.GetLength());
	nOffset += strResultTag.GetLength();
	memcpy(pSearchBuf + nOffset, pszResult, nResultLen);
	nOffset += nResultLen;

	memcpy(pSearchBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength() + 1);

	return 0;
}

int CBodyBuilder::CreateBroadCastXml(char *pszBroadcastReponseBuf, CString& sourceId, CString& targetId, CString& sn, CString& result) const
{
	static CString xml = XML_HEAD_WITH_DECLARATION_ENCODING_GB2312
		"<Response>\r\n"
		"<CmdType>Broadcast</CmdType>\r\n"
		"<SN>1</SN>\r\n"
		"<SourceID></SourceID>\r\n"
		"<TargetID></TargetID>\r\n"
		"<Result></Result>\r\n"
		"</Response>";

	XMLDocument bodyDocument;
	bodyDocument.Parse(xml);

	XMLElement * pRootElement = bodyDocument.RootElement();
	if (NULL == pRootElement)
		return -1;

	// <Query>、<Response>等
	CString strTypeName = pRootElement->Value();
	auto pElement = pRootElement->FirstChildElement("SN");
	pElement->SetText(sn);

	pElement = pRootElement->FirstChildElement("SourceID");
	pElement->SetText(sourceId);

	pElement = pRootElement->FirstChildElement("TargetID");
	pElement->SetText(targetId);

	pElement = pRootElement->FirstChildElement("Result");
	pElement->SetText(result);

	XMLPrinter printer;
	bodyDocument.Accept(&printer);
	strcpy_s(pszBroadcastReponseBuf, BIG_FILE_BUF_LEN, printer.CStr());
	return 0;
}

int CBodyBuilder::parseBroadCastXml(char *xml, CString& sourceId, CString& targetId, CString& sn)
{
	XMLDocument bodyDocument;
	bodyDocument.Parse(xml);

	XMLElement * pRootElement = bodyDocument.RootElement();
	if (NULL == pRootElement)
		return -1;

	// <Query>、<Response>等
	CString strTypeName = pRootElement->Value();
	auto pElement = pRootElement->FirstChildElement("SN");
	if (pElement)
	{
		sn = pElement->GetText();
	}

	pElement = pRootElement->FirstChildElement("SourceID");
	if (pElement)
	{
		sourceId = pElement->GetText();
	}

	pElement = pRootElement->FirstChildElement("TargetID");
	if (pElement)
	{
		targetId = pElement->GetText();
	}

	return 1;
}

int CBodyBuilder::CreateSubscribeResBody(char *pszAlarmBuf, int nBufLen, const char *pszSN, const char *pszDevID, const char* pszResult)
{
	static CString strVersion = "<?xml version=\"1.0\"?>\r\n"
		"<Response>\r\n"
		"<CmdType>Catalog</CmdType>\r\n"
		"<SN>";		//17430

	static CString strSNTag = "</SN>\r\n"
		"<DeviceID>";//34020000002000000001

	static CString strResultTag = "</DeviceID>\r\n"
		"<Result>"; //OK

	static CString strEndTag = "</Result>\r\n"
		"</Response>\r\n";

	int nSNLen = strlen(pszSN);
	int nDevIDLen = strlen(pszDevID);
	int nStatusLen = strlen(pszResult);

	if (strVersion.GetLength() + strSNTag.GetLength() + strEndTag.GetLength() + strResultTag.GetLength() +
		nSNLen + nDevIDLen + nStatusLen >= nBufLen)
		return ERROR_NOACCESS;

	int nOffset = 0;
	memcpy(pszAlarmBuf, strVersion.GetString(), strVersion.GetLength());
	nOffset += strVersion.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszSN, nSNLen);
	nOffset += nSNLen;
	memcpy(pszAlarmBuf + nOffset, strSNTag.GetString(), strSNTag.GetLength());
	nOffset += strSNTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszDevID, nDevIDLen);
	nOffset += nDevIDLen;
	memcpy(pszAlarmBuf + nOffset, strResultTag.GetString(), strResultTag.GetLength());
	nOffset += strResultTag.GetLength();
	memcpy(pszAlarmBuf + nOffset, pszResult, nStatusLen);
	nOffset += nStatusLen;
	memcpy(pszAlarmBuf + nOffset, strEndTag.GetString(), strEndTag.GetLength());
	return 0;
}