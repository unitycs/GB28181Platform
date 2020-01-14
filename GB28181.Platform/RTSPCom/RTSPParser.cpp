#include "StdAfx.h"
#include "RTSPParser.h"


// 编辑Option命令
void RTSPParser::EncodeOption(const OPTION_INFO &info, CString *pstrValue)
{
	OPTION_INFO infoTemp = info;

	CString strOption = infoTemp.method + RTSP_STRING_BLANK + infoTemp.url + RTSP_STRING_BLANK + infoTemp.version + RTSP_STRING_ENTER;
	*pstrValue += strOption;

	if (!infoTemp.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + infoTemp.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!infoTemp.user_agent.IsEmpty())
	{
		CString strUserAgent = RTSP_TYPE_USER_AGENT;
		strUserAgent += RTSP_STRING_COLON;
		strUserAgent += RTSP_STRING_BLANK + infoTemp.user_agent + RTSP_STRING_ENTER;
		*pstrValue += strUserAgent;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Describe命令
void RTSPParser::EncodeDescribe(const DESCRIBE_INFO &info, CString *pstrValue)
{
	DESCRIBE_INFO infoTemp = info;
	*pstrValue = RTSP_STRING_EMPTY;

	CString strDescribe = infoTemp.method + RTSP_STRING_BLANK + infoTemp.url + RTSP_STRING_BLANK + infoTemp.version + RTSP_STRING_ENTER;
	*pstrValue += strDescribe;

	if (!infoTemp.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + infoTemp.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!infoTemp.token.IsEmpty())
	{
		CString strToken = RTSP_TYPE_TOKEN;
		strToken += RTSP_STRING_COLON;
		strToken += RTSP_STRING_BLANK + infoTemp.token + RTSP_STRING_ENTER;
		*pstrValue += strToken;
	}

	if (!infoTemp.require.IsEmpty())
	{
		CString strAccept = RTSP_TYPE_REQUIRE;
		strAccept += RTSP_STRING_COLON;
		strAccept += RTSP_STRING_BLANK + infoTemp.require + RTSP_STRING_ENTER;
		*pstrValue += strAccept;
	}

	if (!infoTemp.accept.IsEmpty())
	{
		CString strAccept = RTSP_TYPE_ACCEPT;
		strAccept += RTSP_STRING_COLON;
		strAccept += RTSP_STRING_BLANK + infoTemp.accept + RTSP_STRING_ENTER;
		*pstrValue += strAccept;
	}

	if (!infoTemp.user_agent.IsEmpty())
	{
		CString strUserAgent = RTSP_TYPE_USER_AGENT;
		strUserAgent += RTSP_STRING_COLON;
		strUserAgent += RTSP_STRING_BLANK + infoTemp.user_agent + RTSP_STRING_ENTER;
		*pstrValue += strUserAgent;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Setup命令
void RTSPParser::EncodeSetup(const SETUP_INFO &info, CString *pstrValue)
{
	SETUP_INFO infoTemp = info;
	*pstrValue = RTSP_STRING_EMPTY;

	CString strSetup = infoTemp.method + RTSP_STRING_BLANK + infoTemp.url + RTSP_STRING_BLANK + infoTemp.version + RTSP_STRING_ENTER;
	*pstrValue += strSetup;

	if (!infoTemp.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + infoTemp.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!infoTemp.transport.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_TRANSPORT;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + infoTemp.transport + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	if (!infoTemp.session.IsEmpty())
	{
		CString strSession = RTSP_TYPE_SESSION;
		strSession += RTSP_STRING_COLON;
		strSession += RTSP_STRING_BLANK + infoTemp.session + RTSP_STRING_ENTER;
		*pstrValue += strSession;
	}

	if (!infoTemp.user_agent.IsEmpty())
	{
		CString strUserAgent = RTSP_TYPE_USER_AGENT;
		strUserAgent += RTSP_STRING_COLON;
		strUserAgent += RTSP_STRING_BLANK + infoTemp.user_agent + RTSP_STRING_ENTER;
		*pstrValue += strUserAgent;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Play命令
void RTSPParser::EncodePlay(const PLAY_INFO &info, CString *pstrValue)
{
	PLAY_INFO infoTemp = info;
	*pstrValue = RTSP_STRING_EMPTY;

	CString strPlay = infoTemp.method + RTSP_STRING_BLANK + infoTemp.url + RTSP_STRING_BLANK + infoTemp.version + RTSP_STRING_ENTER;
	*pstrValue += strPlay;

	if (!infoTemp.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + infoTemp.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!infoTemp.session.IsEmpty())
	{
		CString strSession = RTSP_TYPE_SESSION;
		strSession += RTSP_STRING_COLON;
		strSession += RTSP_STRING_BLANK + infoTemp.session + RTSP_STRING_ENTER;
		*pstrValue += strSession;
	}

	if (!infoTemp.scale.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_SCALE;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + infoTemp.scale + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	if (!infoTemp.speed.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_RANGE;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + infoTemp.speed + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	if (!infoTemp.range.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_RANGE;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + infoTemp.range + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	if (!infoTemp.user_agent.IsEmpty())
	{
		CString strUserAgent = RTSP_TYPE_USER_AGENT;
		strUserAgent += RTSP_STRING_COLON;
		strUserAgent += RTSP_STRING_BLANK + infoTemp.user_agent + RTSP_STRING_ENTER;
		*pstrValue += strUserAgent;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Pause命令
void RTSPParser::EncodePause(const PAUSE_INFO &info, CString *pstrValue)
{
	PAUSE_INFO infoTemp = info;
	*pstrValue = RTSP_STRING_EMPTY;

	CString strPlay = infoTemp.method + RTSP_STRING_BLANK + infoTemp.url + RTSP_STRING_BLANK + infoTemp.version + RTSP_STRING_ENTER;
	*pstrValue += strPlay;

	if (!infoTemp.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + infoTemp.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!infoTemp.session.IsEmpty())
	{
		CString strSession = RTSP_TYPE_SESSION;
		strSession += RTSP_STRING_COLON;
		strSession += RTSP_STRING_BLANK + infoTemp.session + RTSP_STRING_ENTER;
		*pstrValue += strSession;
	}

	if (!infoTemp.range.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_RANGE;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + infoTemp.range + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	if (!infoTemp.user_agent.IsEmpty())
	{
		CString strUserAgent = RTSP_TYPE_USER_AGENT;
		strUserAgent += RTSP_STRING_COLON;
		strUserAgent += RTSP_STRING_BLANK + infoTemp.user_agent + RTSP_STRING_ENTER;
		*pstrValue += strUserAgent;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑GetParameter命令
void RTSPParser::EncodeGetParameter(const GET_PARAMETER_INFO &info, CString *pstrValue)
{
	*pstrValue = RTSP_STRING_EMPTY;

	CString strPlay = info.method + RTSP_STRING_BLANK + info.url + RTSP_STRING_BLANK + info.version + RTSP_STRING_ENTER;
	*pstrValue += strPlay;

	if (!info.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + info.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!info.session.IsEmpty())
	{
		CString strSession = RTSP_TYPE_SESSION;
		strSession += RTSP_STRING_COLON;
		strSession += RTSP_STRING_BLANK + info.session + RTSP_STRING_ENTER;
		*pstrValue += strSession;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Record命令
void RTSPParser::EncodeRecord(const RECORD_INFO &info, CString *pstrValue)
{
	*pstrValue = RTSP_STRING_EMPTY;

	CString strPlay = info.method + RTSP_STRING_BLANK + info.url + RTSP_STRING_BLANK + info.version + RTSP_STRING_ENTER;
	*pstrValue += strPlay;

	if (!info.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + info.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!info.range.IsEmpty())
	{
		CString strTransport = RTSP_TYPE_RANGE;
		strTransport += RTSP_STRING_COLON;
		strTransport += RTSP_STRING_BLANK + info.range + RTSP_STRING_ENTER;
		*pstrValue += strTransport;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 编辑Teardown命令
void RTSPParser::EncodeTeardown(const TEARDOWN_INFO &info, CString *pstrValue)
{
	*pstrValue = RTSP_STRING_EMPTY;

	CString strPlay = info.method + RTSP_STRING_BLANK + info.url + RTSP_STRING_BLANK + info.version + RTSP_STRING_ENTER;
	*pstrValue += strPlay;

	if (!info.sequence.IsEmpty())
	{
		CString strSeq = RTSP_TYPE_SEQUENCE;
		strSeq += RTSP_STRING_COLON;
		strSeq += RTSP_STRING_BLANK + info.sequence + RTSP_STRING_ENTER;
		*pstrValue += strSeq;
	}

	if (!info.session.IsEmpty())
	{
		CString strSession = RTSP_TYPE_SESSION;
		strSession += RTSP_STRING_COLON;
		strSession += RTSP_STRING_BLANK + info.session + RTSP_STRING_ENTER;
		*pstrValue += strSession;
	}

	*pstrValue += RTSP_STRING_ENTER;
}

// 解析RTSP命令
RTSPMsgType RTSPParser::DecodeUnknowMsg(const char *pszContext, RTSP_Unknown_Msg &rtsp_unknown_msg)
{
	CString strTempContext = pszContext;
	rtsp_unknown_msg.SetMsgType(rmt_response);
	//
	strTempContext.Trim(" \r\n") += "\r\n";

	int nPos = strTempContext.Find(RTSP_STRING_ENTER);
	if (0 > nPos) return rmt_unknown;

	auto strFirstLine = strTempContext.Left(nPos);
	strTempContext = strTempContext.Right(strTempContext.GetLength() - (nPos + 2));

	// response消息
	if (0 == strFirstLine.Find("RTSP"))
	{
		nPos = strFirstLine.Find(RTSP_STRING_BLANK);
		if (0 > nPos)
		{
			rtsp_unknown_msg.SetStatus(strFirstLine);
		}
		else
		{
			auto strVersion = strFirstLine.Left(nPos);
			rtsp_unknown_msg.SetVersion(strVersion);
			auto strStatus = strFirstLine.Right(strFirstLine.GetLength() - (nPos + 1));
			rtsp_unknown_msg.SetStatus(strStatus);
		}
	}
	// request消息
	else
	{
		// 解析首行
		nPos = strFirstLine.Find(RTSP_STRING_BLANK);
		if (0 > nPos)
		{
			return rmt_unknown;
		}
		else
		{
			// 取得Method字段
			CString strMethod = strFirstLine.Left(nPos);
			rtsp_unknown_msg.SetMethod(strMethod);
			strFirstLine = strFirstLine.Right(strFirstLine.GetLength() - (nPos + 1));
		}

		nPos = strFirstLine.Find(RTSP_STRING_BLANK);
		// 没有URL字段
		if (0 > nPos)
		{
			// 取得version字段
			CString strVersion = strFirstLine.Right(strFirstLine.GetLength() - (nPos + 1));
			rtsp_unknown_msg.SetVersion(strVersion);
			nPos = 0;
		}
		// 有URL字段
		else
		{
			// 取得URL字段
			CString strURL = strFirstLine.Left(nPos);
			rtsp_unknown_msg.SetURL(strURL);

			// 取得version字段
			CString strVersion = strFirstLine.Right(strFirstLine.GetLength() - (nPos + 1));
			rtsp_unknown_msg.SetVersion(strVersion);
		}
	}
	CString strSegment = RTSP_STRING_EMPTY;
	CString strFieldName = RTSP_STRING_EMPTY;
	CString strFieldValue = RTSP_STRING_EMPTY;
	// 解析首行之下的其他头字段
	while (-1 < nPos)
	{
		nPos = strTempContext.Find(RTSP_STRING_ENTER);
		if (0 > nPos)
		{
			// 无包体
			return rtsp_unknown_msg.GetMsgType();
		}
		//如果发现当前行是空行(只有回车换行),则表明下面内容是SDP或者是JSON内容
		else if (0 == nPos)
		{
			// 有包体
			char szContentType[MAX_PATH];
			rtsp_unknown_msg.GetFieldValue(RTSP_TYPE_CONTENT_TYPE, szContentType, MAX_PATH);

			// 解析content内容
			if (0 == strcmp(szContentType, RTSP_STRING_SDP))
			{
				auto strSdp = strTempContext.Right(strTempContext.GetLength() - (nPos + 2));
				rtsp_unknown_msg.ParseSDP(strSdp);
			}
			else if (0 == strcmp(szContentType, RTSP_STRING_JSON))
			{
				auto strJson = strTempContext.Right(strTempContext.GetLength() - (nPos + 2));
				int nSSPos = strJson.Find("stream-state");
				int nEndPos = strJson.Find("end");
				if (0 >= nSSPos || 0 >= nEndPos || nSSPos > nEndPos)
					return rmt_unknown;
			}
			else
			{
				return rmt_unknown;
			}

			return rtsp_unknown_msg.GetMsgType();
		}

		strSegment = strTempContext.Left(nPos);
		strTempContext = strTempContext.Right(strTempContext.GetLength() - (nPos + 2));
		//strTempContext.Trim();

		nPos = strSegment.Find(RTSP_STRING_COLON);
		if (0 > nPos)
		{
			return rmt_unknown;
		}
		else
		{
			strFieldName = strSegment.Left(nPos);
			strFieldValue = strSegment.Right(strSegment.GetLength() - (nPos + 1));
			strFieldValue.Trim();
		}
		rtsp_unknown_msg.SetFieldValue(strFieldName, strFieldValue);
	}

	return rtsp_unknown_msg.GetMsgType();
}