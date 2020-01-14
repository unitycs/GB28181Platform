#pragma once
const char RegisterAnswerUnauthorized[] = "SIP/2.0 401 Unauthorized\r\n"
	"%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n"
	"WWW-Authenticate: Digest realm=\"3401000000\",nonce=\"b4f7ea506c2485a1\"\r\n"
	"Content-Length: 0\r\n";

const char RegisterAnswerOK[] = "SIP/2.0 200 OK\r\n"
	"%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n"
	"Date: 2014-03-13T15:49:17.087\r\n"
	"Content-Length: 0\r\n";

class CLoginHandler
{
public:
	CLoginHandler(void);
	~CLoginHandler(void);

	void Init(SOCKET sock);
protected:
	SOCKET m_sock;
public:
	void WaitLogin(SOCKADDR_IN &sAddr);
//	SOCKET GetSocket();
};

