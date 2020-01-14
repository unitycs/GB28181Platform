#pragma once
#include "SDPParser/SDPParser.h"
#include "Main/UnifiedMessage.h"

using namespace std;
// 有两个类使用
// RTSP消息类型
enum RTSPMsgType
{
	rmt_unknown = 0,
	rmt_response,
	rmt_announce,
	rmt_setparameter,
	rmt_maxtype
};

// Method define
#define RTSP_METHOD_DESCRIBE		_T("DESCRIBE")
#define RTSP_METHOD_ANNOUNCE		_T("ANNOUNCE")
#define RTSP_METHOD_GET_PARAMETER	_T("GET_PARAMETER")
#define RTSP_METHOD_OPTIONS			_T("OPTIONS")
#define RTSP_METHOD_PAUSE			_T("PAUSE")
#define RTSP_METHOD_PLAY			_T("PLAY")
#define RTSP_METHOD_RECORD			_T("RECORD")
#define RTSP_METHOD_REDIRECT		_T("REDIRECT")
#define RTSP_METHOD_SETUP			_T("SETUP")
#define RTSP_METHOD_SET_PARAMETER	_T("SET_PARAMETER")
#define RTSP_METHOD_TEARDOWN		_T("TEARDOWN")

// SDP type define
#define RTSP_TYPE_SEQUENCE			_T("CSeq")
#define RTSP_TYPE_USER_AGENT		_T("User-Agent")
#define RTSP_TYPE_SERVER			_T("Server")
#define RTSP_TYPE_PUBLIC			_T("Public")
#define RTSP_TYPE_TOKEN				_T("Token")
#define RTSP_TYPE_ACCEPT			_T("Accept")
#define RTSP_TYPE_CONTENT_BASE		_T("Content-Base")
#define RTSP_TYPE_CONTENT_TYPE		_T("Content-Type")
#define RTSP_TYPE_CONTENT_LENGTH	_T("Content-Length")
#define RTSP_TYPE_TRANSPORT			_T("Transport")
#define RTSP_TYPE_SESSION			_T("Session")
#define RTSP_TYPE_LAST_MODIFIED		_T("Last-Modified")
#define RTSP_TYPE_CACHE_CONTROL		_T("Cache-Control")
#define RTSP_TYPE_DATE				_T("Date")
#define RTSP_TYPE_EXPIRES			_T("Expires")
#define RTSP_TYPE_RANGE				_T("Range")
#define RTSP_TYPE_RTP_INFO			_T("RTP-Info")
#define RTSP_TYPE_REQUIRE			_T("Require")
#define RTSP_TYPE_SCALE				_T("Scale")

// String define
#define RTSP_STRING_EMPTY			_T("")
#define RTSP_STRING_NONE			_T("None")
#define RTSP_STRING_BLANK			_T(" ")
#define RTSP_STRING_COLON			_T(":")
#define RTSP_STRING_EQUALSSIGN		_T("=")
#define RTSP_STRING_BARS			_T("-")
#define RTSP_STRING_ENTER			_T("\r\n")
#define RTSP_STRING_BACKSLASH		_T("/")
#define RTSP_STRING_DOUBLEBACKSLASH	_T("//")
#define RTSP_STRING_POINT			_T(".")
#define RTSP_STRING_QUESTIONMARK	_T("?")
#define RTSP_STRING_SEMICOLON		_T(";")
#define RTSP_STRING_AND				_T("&")

// Other CString
#define RTSP_STRING_VERSION			_T("RTSP/1.0")
#define RTSP_STRING_SDP				_T("application/sdp")
#define RTSP_STRING_JSON			_T("application/json")
#define RTSP_STRING_PROTOCOL		_T("rtsp")
#define RTSP_STRING_REALSTREAM		_T("livestream")
#define RTSP_STRING_RECORDSTREAM	_T("recordstream")
#define RTSP_STRING_VDO				_T("vdo")
#define RTSP_STRING_STREAMID		_T("streamid")
#define RTSP_STRING_REQUIRE			_T("onvif-replay")
#define RTSP_STRING_UDPTRANSPORT	_T("RTP/AVP")
#define RTSP_STRING_TCPTRANSPORT	_T("RTP/AVP/TCP")
#define RTSP_STRING_CLIENTPORT		_T("client_port")
#define RTSP_STRING_DESTINATION		_T("destination")
#define RTSP_STRING_UNICAST			_T("unicast")
#define RTSP_STRING_CLIENTADDR		_T("clientaddr")
#define RTSP_STRING_PLAYTIME		_T("playtime")
#define RTSP_STRING_ENDTIME		    _T("endtime")
#define RTSP_STRING_USERAGENT		_T("GB28181Agent-PS")
#define RTSP_STRING_CLOCK			_T("clock")
#define RTSP_STRING_SSRC		    _T("ssrc")

#define RTSP_STRING_THIRD_CALL_RELA_STREAM	_T("thirdCallRealStream")
#define RTSP_STRING_BROADCAST_STREAM	_T("broadcastStream")

class RTSP_INFO_BASE
{
public:
	RTSP_INFO_BASE()
	{
		method = RTSP_STRING_EMPTY;
		url = RTSP_STRING_EMPTY;
		version = RTSP_STRING_VERSION;
		sequence = RTSP_STRING_EMPTY;
		user_agent = RTSP_STRING_EMPTY;
	}

	CString method;
	CString url;
	CString version;
	CString sequence;
	CString user_agent;
};

class RTSP_Unknown_Msg
{
public:
	RTSP_Unknown_Msg()
	{
		// 初始化位置序号
		int nIdx = 0;

		m_oHeadMap.SetAt(CString(RTSP_TYPE_SERVER).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_SEQUENCE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_PUBLIC).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_CONTENT_LENGTH).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_CONTENT_TYPE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_CONTENT_BASE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_LAST_MODIFIED).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_CACHE_CONTROL).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_DATE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_EXPIRES).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_TRANSPORT).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_SESSION).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_RANGE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_RTP_INFO).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_USER_AGENT).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_TOKEN).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_ACCEPT).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_REQUIRE).MakeLower(), nIdx++);
		m_oHeadMap.SetAt(CString(RTSP_TYPE_SCALE).MakeLower(), nIdx++);

		m_eMsgType = rmt_unknown;
	}
	~RTSP_Unknown_Msg() {}

protected:
	CString m_strVersion;		// RTSP协议版本

	CString m_strStatus;		// 应答状态

	CString m_strMethod;		// 指令方法

	CString m_strURL;			// 请求的地址

	RTSPMsgType m_eMsgType;		// 消息类型

	CString m_strHeader[18];	//0 server;
								//1 sequence;
								//2 public_info;
								//3 content_length;
								//4 content_type;
								//5 content_base;
								//6 last_modified;
								//7 cache_control;
								//8 date;
								//9 expires;
								//10 transport;
								//11 session;
								//12 range;
								//13 rtp_info;
								//14 user_agent;
								//15 token;
								//16 accept;
								//17 quire;

	SDPParser m_oSdpStack;		// SDP协议栈

	CMap<CString, LPCSTR, int, int> m_oHeadMap;	//RTSP消息头中字段值对应的序号

public:
	// 设置版本信息
	void SetVersion(CString &strVersion)
	{
		m_strVersion = strVersion;
		m_strVersion.Trim();
	}

	// 设置状态信息
	void SetStatus(CString &strStatus)
	{
		m_strStatus = strStatus;
		m_strStatus.Trim();
	}

	// 设置相应的字段值
	void SetFieldValue(const char *pszFieldName, CString &strFieldValue)
	{
		int nIdx = 0;
		CString strName = pszFieldName;
		strName.MakeLower();
		if (m_oHeadMap.Lookup(strName, nIdx))
		{
			m_strHeader[nIdx] = strFieldValue;
		}
	}

	// 解析SDP文件
	void ParseSDP(CString &strSDP)
	{
		m_oSdpStack.decode_sdp(strSDP);
	}

	// 取得解析后SDP信息
	SDPParser::SDP_STACK &GetSDPStack()
	{
		return m_oSdpStack.GetSDPStack();
	}

	// 设置RTSP消息类型
	void SetMsgType(RTSPMsgType eMsgType)
	{
		m_eMsgType = eMsgType;
	}

	// 取得RTSP消息类型
	RTSPMsgType GetMsgType()
	{
		return m_eMsgType;
	}

	// 设置方法名称
	void SetMethod(CString &strMethod)
	{
		m_strMethod = strMethod;
		if (0 == m_strMethod.CompareNoCase(RTSP_METHOD_SET_PARAMETER))
		{
			SetMsgType(rmt_setparameter);
		}
		else if (0 == m_strMethod.CompareNoCase(RTSP_METHOD_ANNOUNCE))
		{
			SetMsgType(rmt_announce);
		}
	}

	// 取得方法名称
	CString  &GetMethod()
	{
		return m_strMethod;
	}

	// 设置地址
	void SetURL(CString &strURL)
	{
		m_strURL = strURL;
	}

	// 取得地址
	CString &GetURL()
	{
		return m_strURL;
	}

	// 取得相应字段值
	int GetFieldValue(const char *strFieldName, char *szFieldValueBuf, int nBufLen)
	{
		int nIdx = 0;
		CString strName = strFieldName;
		if (m_oHeadMap.Lookup(strName.MakeLower(), nIdx))
		{
			Utils::StringCpy_s(szFieldValueBuf, nBufLen, m_strHeader[nIdx]);
			return 0;
		}
		szFieldValueBuf[0] = '\0';
		return -1;
	}

	// 取得相应字段值
	int GetFieldValue(const char *strFieldName, CString &strFieldValue)
	{
		int nIdx = 0;
		CString strName = strFieldName;
		if (m_oHeadMap.Lookup(strName.MakeLower(), nIdx))
		{
			strFieldValue = m_strHeader[nIdx];
			return 0;
		}
		return -1;
	}

	const char *GetStatus() const
	{
		return m_strStatus.GetString();
	}
};

class RTSP_ACK_BASE
{
public:
	RTSP_ACK_BASE()
	{
		version = RTSP_STRING_EMPTY;
		status = RTSP_STRING_EMPTY;
		server = RTSP_STRING_EMPTY;
		sequence = RTSP_STRING_EMPTY;
	}

	CString version;
	CString status;
	CString server;
	CString sequence;
};

class OPTION_INFO : public RTSP_INFO_BASE
{
public:
	OPTION_INFO()
	{
		method = RTSP_METHOD_OPTIONS;
	}
};

class OPTION_ACK_INFO : public RTSP_ACK_BASE
{
public:
	OPTION_ACK_INFO()
	{
		public_info = RTSP_STRING_EMPTY;
	}

	CString public_info;
};

class DESCRIBE_INFO : public RTSP_INFO_BASE
{
public:
	DESCRIBE_INFO()
	{
		method = RTSP_METHOD_DESCRIBE;
		token = RTSP_STRING_EMPTY;
		accept = RTSP_STRING_SDP;
		require = RTSP_STRING_EMPTY;
	}

	CString token;
	CString accept;
	CString require;
};

class DESCRIBE_ACK_INFO : public RTSP_ACK_BASE
{
public:
	DESCRIBE_ACK_INFO()
	{
		content_length = RTSP_STRING_EMPTY;
		content_type = RTSP_STRING_EMPTY;
		content_base = RTSP_STRING_EMPTY;
	}

	CString content_length;
	CString content_type;
	CString content_base;
	SDPParser::SDP_STACK sdp_stack;
};

class SETUP_INFO : public RTSP_INFO_BASE
{
public:
	SETUP_INFO()
	{
		method = RTSP_METHOD_SETUP;
		transport = RTSP_STRING_EMPTY;
		session = RTSP_STRING_EMPTY;
	}

	CString transport;
	CString session;
};

class SETUP_ACK_INFO : public RTSP_ACK_BASE
{
public:
	SETUP_ACK_INFO()
	{
		last_modified = RTSP_STRING_EMPTY;
		cache_control = RTSP_STRING_EMPTY;
		session = RTSP_STRING_EMPTY;
		date = RTSP_STRING_EMPTY;
		expires = RTSP_STRING_EMPTY;
		transport = RTSP_STRING_EMPTY;
	}

	CString last_modified;
	CString cache_control;
	CString session;
	CString date;
	CString expires;
	CString transport;
};

class PLAY_INFO : public RTSP_INFO_BASE
{
public:
	PLAY_INFO()
	{
		method = RTSP_METHOD_PLAY;
		session = RTSP_STRING_EMPTY;
		range = RTSP_STRING_EMPTY;
		scale = RTSP_STRING_EMPTY;
		speed = RTSP_STRING_EMPTY;
	}

	CString session;
	CString range;
	CString scale;
	CString speed;
};

class PLAY_ACK_INFO : public RTSP_ACK_BASE
{
public:
	PLAY_ACK_INFO()
	{
		session = RTSP_STRING_EMPTY;
		range = RTSP_STRING_EMPTY;
		rtp_info = RTSP_STRING_EMPTY;
	}

	CString session;
	CString range;
	CString rtp_info;
};

class GET_PARAMETER_INFO : public RTSP_INFO_BASE
{
public:
	GET_PARAMETER_INFO()
	{
		method = RTSP_METHOD_GET_PARAMETER;
		session = RTSP_STRING_EMPTY;
	}

	CString session;
};

class RECORD_INFO : public RTSP_INFO_BASE
{
public:
	RECORD_INFO()
	{
		method = RTSP_METHOD_RECORD;
		range = RTSP_STRING_EMPTY;
		user_agent = RTSP_STRING_NONE;
	}

	CString range;
	CString user_agent;
};

class TEARDOWN_INFO : public RTSP_INFO_BASE
{
public:
	TEARDOWN_INFO()
	{
		method = RTSP_METHOD_TEARDOWN;
		session = RTSP_STRING_EMPTY;
	}

	CString session;
};

class PAUSE_INFO : public RTSP_INFO_BASE
{
public:
	PAUSE_INFO()
	{
		method = RTSP_METHOD_PAUSE;
		session = RTSP_STRING_EMPTY;
		range = RTSP_STRING_EMPTY;
	}

	CString session;
	CString range;
};

class PAUSE_ACK_INFO : public RTSP_ACK_BASE
{
public:
	PAUSE_ACK_INFO()
	{
		session = RTSP_STRING_EMPTY;
		range = RTSP_STRING_EMPTY;
		rtp_info = RTSP_STRING_EMPTY;
	}

	CString session;
	CString range;
	CString rtp_info;
};

class RTSPParser
{
public:
	RTSPParser(void) = default;
	~RTSPParser(void) = default;

	// 编辑Option命令
	static void EncodeOption(const OPTION_INFO &info, CString * pstrValue);

	// 编辑Describe命令
	static void EncodeDescribe(const DESCRIBE_INFO &info, CString * pstrValue);

	// 编辑Setup命令
	static void EncodeSetup(const SETUP_INFO &info, CString * pstrValue);

	// 编辑Play命令
	static void EncodePlay(const PLAY_INFO &info, CString * pstrValue);

	// 编辑Pause命令
	static void EncodePause(const PAUSE_INFO &info, CString *pstrValue);

	// 编辑GetParameter命令
	static void EncodeGetParameter(const GET_PARAMETER_INFO &info, CString *pstrValue);

	// 编辑Record命令
	static void EncodeRecord(const RECORD_INFO &info, CString *pstrValue);

	// 编辑Teardown命令
	static void EncodeTeardown(const TEARDOWN_INFO &info, CString *pstrValue);

	// 解析RTSP命令
	static RTSPMsgType DecodeUnknowMsg(const char *pszContext, RTSP_Unknown_Msg &rtsp_unknown_msg);
};
