#include "StdAfx.h"
#include "SdpParser.h"
#include "tchar.h"

SdpParser::SdpParser(void)
{
}

SdpParser::SdpParser( _tstring sdp_buffer )
{
	m_strSdpBuffer = sdp_buffer;

	Decode();
}


SdpParser::~SdpParser(void)
{
}

SDP_STACK SdpParser::decode_sdp()
{
	return m_sdpStack;
}

SDP_STACK SdpParser::decode_sdp(const _tstring sdp_buffer)
{
	Decode();
	return m_sdpStack;
}

int SdpParser::encode_sdp(const SDP_STACK & sdp_stack, TCHAR * sdp_buffer, int &buffer_size)
{
	SDP_STACK stackTemp = sdp_stack;
	int iBufferSize = 0;
	_tstring strSdp;

	if (!stackTemp.version_info.encode().empty())
	{
		strSdp = stackTemp.version_info.encode();
	}

	if (!stackTemp.owner_info.encode().empty())
	{
		strSdp += stackTemp.owner_info.encode();
	}

	if (!stackTemp.session_name.encode().empty())
	{
		strSdp += stackTemp.session_name.encode();
	}

	if (!stackTemp.session_desp.encode().empty())
	{
		strSdp += stackTemp.session_desp.encode();
	}

	if (!stackTemp.url_info.encode().empty())
	{
		strSdp += stackTemp.url_info.encode();
	}

	if (!stackTemp.email_info.encode().empty())
	{
		strSdp += stackTemp.email_info.encode();
	}

	if (!stackTemp.phone_info.encode().empty())
	{
		strSdp += stackTemp.phone_info.encode();
	}

	if (!stackTemp.connect_info.encode().empty())
	{
		strSdp += stackTemp.connect_info.encode();
	}

	if (!stackTemp.bandwidth_info.encode().empty())
	{
		strSdp += stackTemp.bandwidth_info.encode();
	}

	if (!stackTemp.active_time.encode().empty())
	{
		strSdp += stackTemp.active_time.encode();
	}

	if (!stackTemp.repeat_time.encode().empty())
	{
		strSdp += stackTemp.repeat_time.encode();
	}

	ADJUSTMENT_TIME_LIST::iterator timeBegin = stackTemp.adjustment_time_list.begin();
	ADJUSTMENT_TIME_LIST::iterator timeEnd = stackTemp.adjustment_time_list.end();
	ADJUSTMENT_TIME time;
	_tstring strTime = time.type_value;
	strTime += SDP_STRING_EQUALS_SIGN;
	while (timeBegin != timeEnd)
	{
		time = *timeBegin;
		if (!time.encode().empty())
		{
			strTime += time.encode();
		}
		timeBegin++;
		if (timeBegin != timeEnd)
		{
			strTime += SDP_STRING_BLANK;
		}
		else
		{
			strTime += SDP_STRING_ENTER;
		}
	}
	strSdp += strTime;

	ENCRYPTION_KEY_LIST::iterator keyBegin = stackTemp.encryption_key_list.begin();
	ENCRYPTION_KEY_LIST::iterator keyEnd = stackTemp.encryption_key_list.end();
	while (keyBegin != keyEnd)
	{
		ENCRYPTION_KEY key = *keyBegin;
		if (!key.encode().empty())
		{
			strSdp += key.encode();
		}
		keyBegin++;
	}

	SESSION_ATTRIBUTE_LIST::iterator attrBegin = stackTemp.attribute_list.begin();
	SESSION_ATTRIBUTE_LIST::iterator attrEnd = stackTemp.attribute_list.end();
	while (attrBegin != attrEnd)
	{
		SESSION_ATTRIBUTE attrib = *attrBegin;
		if (!attrib.encode().empty())
		{
			strSdp += attrib.encode();
		}
		attrBegin++;
	}

	MEDIA_INFO_LIST::iterator mediaBegin = stackTemp.media_list.begin();
	MEDIA_INFO_LIST::iterator mediaEnd = stackTemp.media_list.end();
	while (mediaBegin != mediaEnd)
	{
		MEDIA_INFO media = *mediaBegin;
		if (!media.encode().empty())
		{
			strSdp += media.encode();
		}
		mediaBegin++;
	}

	iBufferSize = strSdp.size();

	if (0 == buffer_size)
	{
		buffer_size = iBufferSize;
	}
	else
	{
		_tcscpy_s(sdp_buffer, buffer_size, strSdp.c_str());
	}

	return iBufferSize;
}

void SdpParser::Decode()
{
	_tstring strSdpTemp = m_strSdpBuffer;

	int iPos = 0;
	while (_tstring::npos != iPos)
	{
		_tstring strLine = _T("");
		iPos = strSdpTemp.find(_T("\r\n"));
		if (_tstring::npos != iPos)
		{
			strLine = strSdpTemp.substr(0, iPos);
			strSdpTemp = strSdpTemp.substr(iPos + 2);
		}
		else
		{
			strLine = strSdpTemp;
		}

		Classify(strLine);
	}
}

void SdpParser::Classify(const _tstring strLine)
{
	_tstring strTempLine = strLine;
	int iPos = strTempLine.find(_T("="));
	if (_tstring::npos == iPos)
	{
		return;
	}

	TCHAR *pType = (TCHAR *)strTempLine.c_str();
	TCHAR cType = (TCHAR)pType[0];

	strTempLine = strTempLine.substr(iPos + 1);

	switch(cType)
	{
	case SDP_TYPE_VERSION:
		{
			m_sdpStack.version_info.version = strTempLine;
		}
		break;
	case SDP_TYPE_OWNER:
		{
			m_sdpStack.owner_info = GetOwner(strTempLine);
		}
		break;
	case SDP_TYPE_SESSION_NAME:
		{
			m_sdpStack.session_name.name = strTempLine;
		}
		break;
	case SDP_TYPE_SESSION_DESP:
		{
			m_sdpStack.session_desp.description = strTempLine;
		}
		break;
	case SDP_TYPE_URL:
		{
			m_sdpStack.url_info.url = strTempLine;
		}
		break;
	case SDP_TYPE_EMAIL:
		{
			m_sdpStack.email_info.email = strTempLine;
		}
		break;
	case SDP_TYPE_PHONE:
		{
			m_sdpStack.phone_info.phone = strTempLine;
		}
		break;
	case SDP_TYPE_CONNECT_INFO:
		{
			//取首个c行的数据
			if(m_sdpStack.connect_info.address.empty())
				m_sdpStack.connect_info = GetConnect(strTempLine);
		}
		break;
	case SDP_TYPE_BANDWIDTH:
		{
			m_sdpStack.bandwidth_info = GetBandWidth(strTempLine);
		}
		break;
	case SDP_TYPE_ACTIVE_TIME:
		{
			m_sdpStack.active_time = GetActiveTime(strTempLine);
		}
		break;
	case SDP_TYPE_REPEAT_TIME:
		{
			m_sdpStack.repeat_time = GetRepeatTime(strTempLine);
		}
		break;
	case SDP_TYPE_ADJUSTMENTS_TIME:
		{
			m_sdpStack.adjustment_time_list = GetAdjustmentTime(strTempLine);
		}
		break;
	case SDP_TYPE_KEY:
		{
			ENCRYPTION_KEY key = GetEncryptionKey(strTempLine);
			m_sdpStack.encryption_key_list.push_back(key);
		}
		break;
	case SDP_TYPE_ATTRIBUTE:
		{
			SESSION_ATTRIBUTE attrib = GetSessionAttribute(strTempLine);
			m_sdpStack.attribute_list.push_back(attrib);
		}
		break;
	case SDP_TYPE_MEDIA:
		{
			MEDIA_INFO info = GetMedia(strTempLine);
			m_sdpStack.media_list.push_back(info);
		}
		break;
	default:
		break;
	}
}

_tstring SdpParser::GetStringByToken(const _tstring strLine, const _tstring strToken, _tstring &strTail)
{
	_tstring strValue = _T("");

	int iTokenSize = strToken.size();
	int iPos = strLine.find(strToken);
	if (_tstring::npos != iPos)
	{
		strValue = strLine.substr(0, iPos);
		strTail = strLine.substr(iPos + iTokenSize);
	}
	else
	{
		strValue = strLine;
		strTail = _T("");
	}

	return strValue;
}

OWNER_INFO SdpParser::GetOwner(const _tstring strLine)
{
	_tstring strTempLine = strLine;
	OWNER_INFO info;

	int i = 0;
	_tstring strToken = info.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				info.user_name = strValue;
			}
			break;
		case 1:
			{
				info.session_id = strValue;
			}
			break;
		case 2:
			{
				info.version = strValue;
			}
			break;
		case 3:
			{
				info.connect_info.network_type = strValue;
			}
			break;
		case 4:
			{
				info.connect_info.address_type = strValue;
			}
			break;
		case 5:
			{
				info.connect_info.address = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return info;
}

CONNECT_INFO SdpParser::GetConnect(const _tstring strLine)
{
	CONNECT_INFO info;
	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = info.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				info.network_type = strValue;
			}
			break;
		case 1:
			{
				info.address_type = strValue;
			}
			break;
		case 2:
			{
				info.address = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return info;
}

BANDWIDTH_INFO SdpParser::GetBandWidth(const _tstring strLine)
{
	BANDWIDTH_INFO info;
	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = info.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				info.modifier = strValue;
			}
			break;
		case 1:
			{
				info.value = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return info;
}

ACTIVE_TIME SdpParser::GetActiveTime(const _tstring strLine)
{
	ACTIVE_TIME active;
	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = active.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				active.start = strValue;
			}
			break;
		case 1:
			{
				active.stop = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return active;
}

REPEAT_TIME SdpParser::GetRepeatTime(const _tstring strLine)
{
	REPEAT_TIME repeat;

	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = repeat.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				repeat.repeat = strValue;
			}
			break;
		case 1:
			{
				repeat.active = strValue;
			}
			break;
		case 2:
			{
				repeat.offset = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return repeat;
}

ADJUSTMENT_TIME_LIST SdpParser::GetAdjustmentTime(const _tstring strLine)
{
	ADJUSTMENT_TIME_LIST adjustment_list;
	ADJUSTMENT_TIME adjustment;

	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = adjustment.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				adjustment.time = strValue;
			}
			break;
		case 1:
			{
				adjustment.offset = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);

		if (2 == i)
		{
			adjustment_list.push_back(adjustment);
			i = 0;
		}
	}

	return adjustment_list;
}

ENCRYPTION_KEY SdpParser::GetEncryptionKey(const _tstring strLine)
{
	ENCRYPTION_KEY key;

	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = key.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				key.method = strValue;
			}
			break;
		case 1:
			{
				key.key = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return key;
}

SESSION_ATTRIBUTE SdpParser::GetSessionAttribute(const _tstring strLine)
{
	SESSION_ATTRIBUTE attribute;

	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = attribute.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				attribute.attribute = strValue;
			}
			break;
		case 1:
			{
				attribute.value = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return attribute;
}

MEDIA_INFO SdpParser::GetMedia(const _tstring strLine)
{
	MEDIA_INFO info;

	_tstring strTempLine = strLine;

	int i = 0;
	_tstring strToken = info.separator;
	_tstring strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	while (!strValue.empty())
	{
		switch(i)
		{
		case 0:
			{
				info.media = strValue;
			}
			break;
		case 1:
			{
				info.port = strValue;
			}
			break;
		case 2:
			{
				info.transport = strValue;
			}
			break;
		case 3:
			{
				info.fmtlist = strValue;
			}
			break;
		default:
			break;
		}

		i++;
		strValue = GetStringByToken(strTempLine, strToken, strTempLine);
	}

	return info;
}