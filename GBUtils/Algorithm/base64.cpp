#include "base64.h"
// 从双字中取单字节
#define B0(a) (a & 0xFF)
#define B1(a) (a >> 8 & 0xFF)
#define B2(a) (a >> 16 & 0xFF)
#define B3(a) (a >> 24 & 0xFF)

inline int BASE64Cryptor::GetB64Index(char ch)
{
	int index = -1;
	if (ch >= 'A' && ch <= 'Z')
	{
		index = ch - 'A';
	}
	else if (ch >= 'a' && ch <= 'z')
	{
		index = ch - 'a' + 26;
	}
	else if (ch >= '0' && ch <= '9')
	{
		index = ch - '0' + 52;
	}
	else if (ch == '+')
	{
		index = 62;
	}
	else if (ch == '/')
	{
		index = 63;
	}

	return index;
}

inline char BASE64Cryptor::GetB64Char(int index)
{
	const char szBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (index >= 0 && index < 64)
		return szBase64Table[index];

	return '=';
}

// 编码后的长度一般比原文多占1/3的存储空间，请保证base64code有足够的空间
inline int BASE64Cryptor::Base64Encode(char * base64code, const char * src, int src_len)
{
	if (src_len == 0)
		src_len = strlen(src);

	int len = 0;
	auto tmpsrc = const_cast<char*>(src);
	unsigned char* psrc = reinterpret_cast<unsigned char*>(tmpsrc);
	char * p64 = base64code;
	int i;
	for (i = 0; i < src_len - 3; i += 3)
	{
		unsigned long ulTmp = *reinterpret_cast<unsigned long*>(psrc);
		register int b0 = GetB64Char((B0(ulTmp) >> 2) & 0x3F);
		register int b1 = GetB64Char((B0(ulTmp) << 6 >> 2 | B1(ulTmp) >> 4) & 0x3F);
		register int b2 = GetB64Char((B1(ulTmp) << 4 >> 2 | B2(ulTmp) >> 6) & 0x3F);
		register int b3 = GetB64Char((B2(ulTmp) << 2 >> 2) & 0x3F);
		*reinterpret_cast<unsigned long*>(p64) = b0 | b1 << 8 | b2 << 16 | b3 << 24;
		len += 4;
		p64 += 4;
		psrc += 3;
	}

	// 处理最后余下的不足3字节的饿数据
	if (i < src_len)
	{
		int rest = src_len - i;
		unsigned long ulTmp = 0;
		for (int j = 0; j < rest; ++j)
		{
			*(reinterpret_cast<unsigned char*>(&ulTmp) + j) = *psrc++;
		}

		p64[0] = GetB64Char((B0(ulTmp) >> 2) & 0x3F);
		p64[1] = GetB64Char((B0(ulTmp) << 6 >> 2 | B1(ulTmp) >> 4) & 0x3F);
		p64[2] = rest > 1 ? GetB64Char((B1(ulTmp) << 4 >> 2 | B2(ulTmp) >> 6) & 0x3F) : '=';
		p64[3] = rest > 2 ? GetB64Char((B2(ulTmp) << 2 >> 2) & 0x3F) : '=';
		p64 += 4;
		len += 4;
	}

	*p64 = '\0';

	return len;
}

// 解码后的长度一般比原文少用占1/4的存储空间，请保证buf有足够的空间
inline int BASE64Cryptor::Base64Decode(char * buf, const char * base64code, int src_len)
{
	if (src_len == 0)
		src_len = strlen(base64code);

	int len = 0;
	auto tmpbase64code = const_cast<char*>(base64code);
	unsigned char* psrc = reinterpret_cast<unsigned char*>(tmpbase64code);
	char * pbuf = buf;
	int i;
	for (i = 0; i < src_len - 4; i += 4)
	{
		unsigned long ulTmp = *reinterpret_cast<unsigned long*>(psrc);

		register int b0 = (GetB64Index(static_cast<char>(B0(ulTmp))) << 2 | GetB64Index(static_cast<char>(B1(ulTmp))) << 2 >> 6) & 0xFF;
		register int b1 = (GetB64Index(static_cast<char>(B1(ulTmp))) << 4 | GetB64Index(static_cast<char>(B2(ulTmp))) << 2 >> 4) & 0xFF;
		register int b2 = (GetB64Index(static_cast<char>(B2(ulTmp))) << 6 | GetB64Index(static_cast<char>(B3(ulTmp))) << 2 >> 2) & 0xFF;

		*reinterpret_cast<unsigned long*>(pbuf) = b0 | b1 << 8 | b2 << 16;
		psrc += 4;
		pbuf += 3;
		len += 3;
	}

	// 处理最后余下的不足4字节的饿数据
	if (i < src_len)
	{
		int rest = src_len - i;
		unsigned long ulTmp = 0;
		for (int j = 0; j < rest; ++j)
		{
			*(reinterpret_cast<unsigned char*>(&ulTmp) + j) = *psrc++;
		}

		register int b0 = (GetB64Index(static_cast<char>(B0(ulTmp))) << 2 | GetB64Index(static_cast<char>(B1(ulTmp))) << 2 >> 6) & 0xFF;
		*pbuf++ = static_cast<char>(b0);
		len++;

		if ('=' != B1(ulTmp) && '=' != B2(ulTmp))
		{
			register int b1 = (GetB64Index(static_cast<char>(B1(ulTmp))) << 4 | GetB64Index(static_cast<char>(B2(ulTmp))) << 2 >> 4) & 0xFF;
			*pbuf++ = static_cast<char>(b1);
			len++;
		}

		if ('=' != B2(ulTmp) && '=' != B3(ulTmp))
		{
			register int b2 = (GetB64Index(static_cast<char>(B2(ulTmp))) << 6 | GetB64Index(static_cast<char>(B3(ulTmp))) << 2 >> 2) & 0xFF;
			*pbuf++ = static_cast<char>(b2);
			len++;
		}
	}

	*pbuf = '\0';

	return len;
}

inline std::string BASE64Cryptor::Decode(const char * src_bytes, int src_len)
{
	char buf_KEY[260] = { 0 };
	Base64Decode(buf_KEY, src_bytes, src_len);
	return buf_KEY;
}

inline std::string BASE64Cryptor::Encode(const char * src_bytes, int src_len)
{
	char buf_IV[260] = { 0 };
	Base64Encode(buf_IV, src_bytes, src_len);
	return buf_IV;
}