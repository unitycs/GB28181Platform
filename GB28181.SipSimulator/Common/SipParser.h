#pragma once
class CSipParser
{
public:
	CSipParser(void);
	virtual ~CSipParser(void);
	//sip parser
	static void gen_call_id(char * pID) ;
	static void gen_tag(char * pTag) ;
	static void gen_branch(char * pBranch) ;
	static void parse_rtag(char * buf, char * pRTag);
	static void get_line(const char *pszFeildName, const CString &strMsg, CString &strLine);
	static void gen_rand_charset(char *pBuf, int nBufLen);


private:
	static const int TAG_LEN =9;
	static const int CALLID_LEN=26;
	static const int BRANCH_LEN=17;

};

