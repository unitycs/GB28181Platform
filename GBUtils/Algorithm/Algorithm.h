#pragma once
#include <string>
#include <time.h>

#ifdef GBUTILS_EXPORTS
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllexport)
#endif
#else
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllimport)
#endif
#endif

#define HASHLEN 16
typedef char HASH[HASHLEN];

#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

using namespace std;
#define IN
#define OUT


GBUTILS_API void DigestCalcHA1(IN const char *pszAlg, IN const char *pszUserName, IN const char *pszRealm, IN const char *pszPassword, IN const char *pszNonce, IN const char *pszCNonce, OUT HASHHEX SessionKey);
GBUTILS_API string GetStringBetween(string &strData, char *strFront, char *strBack, bool bIsInclude);
//void CvtHex (char *pBin, int nBinBufLen, char *pHex, int nHexBufLen);
void CvtHex(IN HASH Bin, OUT HASHHEX Hex);
GBUTILS_API int CreateNonce(string &strNoce, int pwdLen);
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
/* request-digest or response-digest */);