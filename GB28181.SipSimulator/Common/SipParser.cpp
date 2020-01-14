#include "./../StdAfx.h"
#include "SipParser.h"


CSipParser::CSipParser(void)
{
	
}


CSipParser::~CSipParser(void)
{
}

void CSipParser::gen_call_id(char * pID) 
{
	gen_rand_charset(pID, CALLID_LEN-9);
	strcat_s(pID, CALLID_LEN, "@0.0.0.0");
}

void CSipParser::gen_tag(char * pTag) 
{
	gen_rand_charset(pTag, TAG_LEN);
}

void CSipParser::gen_branch(char * pBranch) 
{
	memcpy(pBranch, "z9hG4bK", 7);
	gen_rand_charset(pBranch+7, BRANCH_LEN-7);
}

void CSipParser::parse_rtag(char * buf, char * pRTag) 
{
	throw  new std::exception("not implemention");
}

void CSipParser::get_line(const char *pszFeildName, const CString &strMsg, CString &strLine) 
{
	auto nPos = strMsg.Find(pszFeildName);
	if(0 <= nPos)
	{
		strLine = strMsg.Right(strMsg.GetLength()-nPos);
		nPos = strLine.Find("\r");
		if(0 < nPos)
		{
			strLine = strLine.Left(nPos);
		}
	}
}

 void CSipParser::gen_rand_charset(char *pBuf, int nBufLen)
{
	int i;
	auto count = 0;
	char str[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E',
		'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
		'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	int nStrSize = sizeof(str);
	static UINT nSeed = 0;
	//srand((unsigned)time(NULL));
	srand(nSeed++);

	nBufLen--;
	while (count < nBufLen) {
		i = rand() % nStrSize;
		pBuf[count] = str[i];
		count++;
	}	
	
	pBuf[nBufLen] = 0;
}