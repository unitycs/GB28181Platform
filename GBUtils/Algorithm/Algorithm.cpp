#include "Algorithm.h"
#include "md5.h"
GBUTILS_API int CreateNonce(string &strNoce, int pwdLen)
{
	//    const int maxNum = 61;
	int i;
	int count = 0;
	char str[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
			'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E',
			'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
			'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	int nStrSize = sizeof(str);
	srand(static_cast<unsigned>(time(nullptr)));
	while (count < pwdLen) {
		i = rand() % nStrSize;
		//i = 0;

		strNoce += str[i];
		count++;
	}
	return 0;
}

void CvtHex(IN HASH Bin, OUT HASHHEX Hex)
{
	unsigned short i;
	unsigned char j;

	for (i = 0; i < HASHLEN; i++) {
		j = (Bin[i] >> 4) & 0xf;
		if (j <= 9)
			Hex[i * 2] = (j + '0');
		else
			Hex[i * 2] = (j + 'a' - 10);
		j = Bin[i] & 0xf;
		if (j <= 9)
			Hex[i * 2 + 1] = (j + '0');
		else
			Hex[i * 2 + 1] = (j + 'a' - 10);
	};
	Hex[HASHHEXLEN] = '\0';
}


GBUTILS_API string GetStringBetween(string &strData, char *strFront, char *strBack, bool bIsInclude)
{
	string strTmp;

	int nPos_L = strData.find(strFront);
	int nPos_R = strData.find(strBack);
	if (0 > nPos_L || 0 > nPos_R || nPos_L >= nPos_R)
		return strTmp;

	if (false == bIsInclude)
		strTmp = strData.substr(nPos_L + strlen(strFront), nPos_R - nPos_L - strlen(strBack) + 1);
	else
		strTmp = strData.substr(nPos_L, nPos_R - nPos_L + strlen(strBack));

	return strTmp;
}

GBUTILS_API void DigestCalcHA1(IN const char *pszAlg, IN const char *pszUserName, IN const char *pszRealm, IN const char *pszPassword, IN const char *pszNonce, IN const char *pszCNonce, OUT HASHHEX SessionKey)
{
	MD5CryptoPrivoder::MD5_CTX Md5Ctx;
	HASH HA1;
	MD5CryptoPrivoder provider;
	provider.MD5Init(&Md5Ctx);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszUserName, (unsigned int)strlen(pszUserName));
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszRealm, (unsigned int)strlen(pszRealm));
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszPassword, (unsigned int)strlen(pszPassword));
	provider.MD5Final((unsigned char *)HA1, &Md5Ctx);
	if ((pszAlg != NULL) && strcmp(pszAlg, "md5-sess") == 0) {
		provider.MD5Init(&Md5Ctx);
		provider.MD5Update(&Md5Ctx, (unsigned char *)HA1, HASHLEN);
		provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		provider.MD5Update(&Md5Ctx, (unsigned char *)pszNonce, (unsigned int)strlen(pszNonce));
		provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		provider.MD5Update(&Md5Ctx, (unsigned char *)pszCNonce, (unsigned int)strlen(pszCNonce));
		provider.MD5Final((unsigned char *)HA1, &Md5Ctx);
	}
	CvtHex(HA1, SessionKey);
}

GBUTILS_API void DigestCalcResponse(IN HASHHEX HA1,     /* H(A1) */
	IN const char *pszNonce,    /* nonce from server */
	IN const char *pszNonceCount,       /* 8 hex digits */
	IN const char *pszCNonce,   /* client nonce */
	IN const char *pszQop,      /* qop-value: "", "auth", "auth-int" */
	IN int Aka, /* Calculating AKAv1-MD5 response */
	IN const char *pszMethod,   /* method from the request */
	IN const char *pszDigestUri,        /* requested URL */
	IN HASHHEX HEntity, /* H(entity body) if qop="auth-int" */
	OUT HASHHEX Response
/* request-digest or response-digest */)
{
	MD5CryptoPrivoder::MD5_CTX Md5Ctx;
	HASH HA2;
	HASH RespHash;
	HASHHEX HA2Hex;
	MD5CryptoPrivoder provider;
	/* calculate H(A2) */
	provider.MD5Init(&Md5Ctx);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszMethod, (unsigned int)strlen(pszMethod));
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszDigestUri, (unsigned int)strlen(pszDigestUri));

	if (pszQop == NULL || 0 == strlen(pszQop)) {
		goto auth_withoutqop;
	}
	else if (0 == strcmp(pszQop, "auth-int")) {
		goto auth_withauth_int;
	}
	else if (0 == strcmp(pszQop, "auth")) {
		goto auth_withauth;
	}

auth_withoutqop:
	provider.MD5Final((unsigned char *)HA2, &Md5Ctx);
	CvtHex(HA2, HA2Hex);

	/* calculate response */
	provider.MD5Init(&Md5Ctx);
	provider.MD5Update(&Md5Ctx, (unsigned char *)HA1, HASHHEXLEN);
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszNonce, (unsigned int)strlen(pszNonce));
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);

	goto end;

auth_withauth_int:

	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)HEntity, HASHHEXLEN);

auth_withauth:
	provider.MD5Final((unsigned char *)HA2, &Md5Ctx);
	CvtHex(HA2, HA2Hex);

	/* calculate response */
	provider.MD5Init(&Md5Ctx);
	provider.MD5Update(&Md5Ctx, (unsigned char *)HA1, HASHHEXLEN);
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	provider.MD5Update(&Md5Ctx, (unsigned char *)pszNonce, (unsigned int)strlen(pszNonce));
	provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	if (Aka == 0) {
		provider.MD5Update(&Md5Ctx, (unsigned char *)pszNonceCount, (unsigned int)strlen(pszNonceCount));
		provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		provider.MD5Update(&Md5Ctx, (unsigned char *)pszCNonce, (unsigned int)strlen(pszCNonce));
		provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		provider.MD5Update(&Md5Ctx, (unsigned char *)pszQop, (unsigned int)strlen(pszQop));
		provider.MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	}
end:
	provider.MD5Update(&Md5Ctx, (unsigned char *)HA2Hex, HASHHEXLEN);
	provider.MD5Final((unsigned char *)RespHash, &Md5Ctx);
	CvtHex(RespHash, Response);
}