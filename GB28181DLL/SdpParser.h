#pragma once

#include <list>
#include <string>

using namespace std;

// SDP type define

// Version: <version>
#define SDP_TYPE_VERSION		'v'
// Owner: <username> <session id> <version> <network type> <address type> <address>
#define SDP_TYPE_OWNER			'o'
// Session: <session name>
#define SDP_TYPE_SESSION_NAME	's'
// *Session description: <session description>
#define SDP_TYPE_SESSION_DESP	'i'
// *URL: <url>
#define SDP_TYPE_URL			'u'
// *Email: <email address>
#define SDP_TYPE_EMAIL			'e'
// *Phone: <phone number>
#define SDP_TYPE_PHONE			'p'
// *Connect information: <network type> <address type> <connection address>
#define SDP_TYPE_CONNECT_INFO	'c'
// *Bandwidth information: <modifier>:<bandwidth-value>
#define SDP_TYPE_BANDWIDTH		'b'
// Time the session is active: <start time> <stop time>
#define SDP_TYPE_ACTIVE_TIME	't'
// *Zero or more repeat times: <repeat interval> <active duration> <list of offsets from start-time>
#define SDP_TYPE_REPEAT_TIME	'r'
// *Time zone adjustments: <adjustment time> <offset> <adjustment time> <offset> ...
#define SDP_TYPE_ADJUSTMENTS_TIME		'z'
// *Encryption key: <method>/<method>:<encryption key>
#define SDP_TYPE_KEY			'k'
// *Zero or more session attribute lines: <attribute>/<attribute>:<value>
#define SDP_TYPE_ATTRIBUTE		'a'
// Media name and transport address: <media> <port> <transport> <fmt list>
#define SDP_TYPE_MEDIA			'm'

// String define
#define SDP_STRING_EMPTY		_T("")
#define SDP_STRING_BLANK		_T(" ")
#define SDP_STRING_COLON		_T(":")
#define SDP_STRING_EQUALS_SIGN	_T("=")
#define SDP_STRING_BARS			_T("-")
#define SDP_STRING_ENTER		_T("\r\n")
#define SDP_STRING_BACKSLASH	_T("/")

class SDP_INFO_BASE
{
public:
	SDP_INFO_BASE()
	{
		type_value = SDP_STRING_EMPTY;
		separator = SDP_STRING_EMPTY;
	}

	_tstring type_value;
	_tstring separator;
};

class VERSION_INFO : public SDP_INFO_BASE
{
public:
	VERSION_INFO()
	{
		type_value = SDP_TYPE_VERSION;
		version = SDP_STRING_EMPTY;
	}

	_tstring version;

	_tstring encode()
	{
		if (version.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + version + SDP_STRING_ENTER;
	}
};

class CONNECT_INFO : public SDP_INFO_BASE
{
public:
	CONNECT_INFO()
	{
		type_value = SDP_TYPE_CONNECT_INFO;
		separator = SDP_STRING_BLANK;
		network_type = SDP_STRING_EMPTY;
		address_type = SDP_STRING_EMPTY;
		address = SDP_STRING_EMPTY;
	}

	_tstring network_type;
	_tstring address_type;
	_tstring address;

	_tstring encode()
	{
		if (network_type.empty() || address_type.empty() || address.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + network_type
			+ separator + address_type + separator + address + SDP_STRING_ENTER;
	}
};

class OWNER_INFO : public SDP_INFO_BASE
{
public:
	OWNER_INFO()
	{
		type_value = SDP_TYPE_OWNER;
		separator = SDP_STRING_BLANK;
		user_name = SDP_STRING_EMPTY;
		session_id = SDP_STRING_EMPTY;
		version = SDP_STRING_EMPTY;
	}

	_tstring user_name;
	_tstring session_id;
	_tstring version;
	CONNECT_INFO connect_info;

	_tstring encode()
	{
		if (user_name.empty() || session_id.empty() || version.empty()
			|| connect_info.network_type.empty() || connect_info.address_type.empty() || connect_info.address.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + user_name + separator + session_id + separator + version
			+ separator + connect_info.network_type + separator + connect_info.address_type + separator
			+ connect_info.address + SDP_STRING_ENTER;
	}
};

class SESSION_NAME : public SDP_INFO_BASE
{
public:
	SESSION_NAME()
	{
		type_value = SDP_TYPE_SESSION_NAME;
	}

	_tstring name;

	_tstring encode()
	{
		if (name.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + name + SDP_STRING_ENTER;
	}
};

class SESSION_DESP : public SDP_INFO_BASE
{
public:
	SESSION_DESP()
	{
		type_value = SDP_TYPE_SESSION_DESP;
	}

	_tstring description;

	_tstring encode()
	{
		if (description.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + description + SDP_STRING_ENTER;
	}
};

class URL_INFO : public SDP_INFO_BASE
{
public:
	URL_INFO()
	{
		type_value = SDP_TYPE_URL;
		url = SDP_STRING_EMPTY;
	}

	_tstring url;

	_tstring encode()
	{
		if (url.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + url + SDP_STRING_ENTER;
	}
};

class EMAIL_INFO : public SDP_INFO_BASE
{
public:
	EMAIL_INFO()
	{
		type_value = SDP_TYPE_EMAIL;
		email = SDP_STRING_EMPTY;
	}

	_tstring email;

	_tstring encode()
	{
		if (email.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + email + SDP_STRING_ENTER;
	}
};

class PHONE_INFO : public SDP_INFO_BASE
{
public:
	PHONE_INFO()
	{
		type_value = SDP_TYPE_PHONE;
		phone = SDP_STRING_EMPTY;
	}

	_tstring phone;

	_tstring encode()
	{
		if (phone.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + phone + SDP_STRING_ENTER;
	}
};

class BANDWIDTH_INFO : public SDP_INFO_BASE
{
public:
	BANDWIDTH_INFO()
	{
		type_value = SDP_TYPE_BANDWIDTH;
		separator = SDP_STRING_COLON;
		modifier = SDP_STRING_EMPTY;
		value = SDP_STRING_EMPTY;
	}

	_tstring modifier;
	_tstring value;

	_tstring encode()
	{
		if (modifier.empty() || value.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + modifier + separator + value + SDP_STRING_ENTER;
	}
};

class ACTIVE_TIME : public SDP_INFO_BASE
{
public:
	ACTIVE_TIME()
	{
		type_value = SDP_TYPE_ACTIVE_TIME;
		separator = SDP_STRING_BLANK;
		start = SDP_STRING_EMPTY;
		stop = SDP_STRING_EMPTY;
	}

	_tstring start;
	_tstring stop;

	_tstring encode()
	{
		if (start.empty() || stop.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + start + separator + stop + SDP_STRING_ENTER;
	}
};

class REPEAT_TIME : public SDP_INFO_BASE
{
public:
	REPEAT_TIME()
	{
		type_value = SDP_TYPE_REPEAT_TIME;
		separator = SDP_STRING_BLANK;
		repeat = SDP_STRING_EMPTY;
		active = SDP_STRING_EMPTY;
		offset = SDP_STRING_EMPTY;
	}

	_tstring repeat;
	_tstring active;
	_tstring offset;

	_tstring encode()
	{
		if (repeat.empty() || active.empty() || offset.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + repeat + separator + active + separator + offset + SDP_STRING_ENTER;
	}
};

class ADJUSTMENT_TIME : public SDP_INFO_BASE
{
public:
	ADJUSTMENT_TIME()
	{
		type_value = SDP_TYPE_ADJUSTMENTS_TIME;
		separator = SDP_STRING_BLANK;
		time = SDP_STRING_EMPTY;
		offset = SDP_STRING_EMPTY;
	}

	_tstring time;
	_tstring offset;

	_tstring encode()
	{
		if (time.empty() || offset.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return time + separator + offset;
	}
};
typedef list<ADJUSTMENT_TIME> ADJUSTMENT_TIME_LIST;

class ENCRYPTION_KEY : public SDP_INFO_BASE
{
public:
	ENCRYPTION_KEY()
	{
		type_value = SDP_TYPE_KEY;
		separator = SDP_STRING_COLON;
		method = SDP_STRING_EMPTY;
		key = SDP_STRING_EMPTY;
	}

	_tstring method;
	_tstring key;

	_tstring encode()
	{
		if (method.empty())
		{
			return SDP_STRING_EMPTY;
		}
		else if (!key.empty())
		{
			return type_value + SDP_STRING_EQUALS_SIGN + method + separator + key + SDP_STRING_ENTER;
		}
		else
		{
			return type_value + SDP_STRING_EQUALS_SIGN + method + SDP_STRING_ENTER;
		}
	}
};
typedef list<ENCRYPTION_KEY> ENCRYPTION_KEY_LIST;

class SESSION_ATTRIBUTE : public SDP_INFO_BASE
{
public:
	SESSION_ATTRIBUTE()
	{
		type_value = SDP_TYPE_ATTRIBUTE;
		separator = SDP_STRING_COLON;
		attribute = SDP_STRING_EMPTY;
		value = SDP_STRING_EMPTY;
	}

	_tstring attribute;
	_tstring value;

	_tstring encode()
	{
		if (attribute.empty() || value.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + attribute + separator + value + SDP_STRING_ENTER;
	}
};
typedef list<SESSION_ATTRIBUTE> SESSION_ATTRIBUTE_LIST;

class MEDIA_INFO : public SDP_INFO_BASE
{
public:
	MEDIA_INFO()
	{
		type_value = SDP_TYPE_MEDIA;
		separator = SDP_STRING_BLANK;
		media = SDP_STRING_EMPTY;
		port = SDP_STRING_EMPTY;
		transport = SDP_STRING_EMPTY;
		fmtlist = SDP_STRING_EMPTY;
	}

	_tstring media;
	_tstring port;
	_tstring transport;
	_tstring fmtlist;

	_tstring encode()
	{
		if (media.empty() || port.empty() || transport.empty() || fmtlist.empty())
		{
			return SDP_STRING_EMPTY;
		}

		return type_value + SDP_STRING_EQUALS_SIGN + media + separator + port + separator + transport
			+ separator + fmtlist + SDP_STRING_ENTER;
	}
};
typedef list<MEDIA_INFO> MEDIA_INFO_LIST;

class SDP_STACK
{
public:
	SDP_STACK()
	{
	}

	VERSION_INFO version_info;
	OWNER_INFO owner_info;
	SESSION_NAME session_name;
	SESSION_DESP session_desp;
	URL_INFO url_info;
	EMAIL_INFO email_info;
	PHONE_INFO phone_info;
	CONNECT_INFO connect_info;
	BANDWIDTH_INFO bandwidth_info;
	ACTIVE_TIME active_time;
	REPEAT_TIME repeat_time;
	ADJUSTMENT_TIME_LIST adjustment_time_list;
	ENCRYPTION_KEY_LIST encryption_key_list;
	SESSION_ATTRIBUTE_LIST attribute_list;
	MEDIA_INFO_LIST media_list;
};

class SdpParser
{
public:
	SdpParser(void);
	SdpParser(_tstring sdp_buffer);
	~SdpParser(void);

	SDP_STACK decode_sdp(const _tstring sdp_buffer);
	SDP_STACK decode_sdp();

	int encode_sdp(const SDP_STACK &sdp_stack, TCHAR * sdp_buffer, int &buffer_size);

private:
	_tstring m_strSdpBuffer;
	SDP_STACK m_sdpStack;

	void Decode();
	void Classify(const _tstring strLine);
	_tstring GetStringByToken(const _tstring strLine, const _tstring strToken, _tstring &strTail);

	OWNER_INFO GetOwner(const _tstring strLine);
	CONNECT_INFO GetConnect(const _tstring strLine);
	BANDWIDTH_INFO GetBandWidth(const _tstring strLine);
	ACTIVE_TIME GetActiveTime(const _tstring strLine);
	REPEAT_TIME GetRepeatTime(const _tstring strLine);
	ADJUSTMENT_TIME_LIST GetAdjustmentTime(const _tstring strLine);
	ENCRYPTION_KEY GetEncryptionKey(const _tstring strLine);
	SESSION_ATTRIBUTE GetSessionAttribute(const _tstring strLine);
	MEDIA_INFO GetMedia(const _tstring strLine);
};


