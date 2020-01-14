#include "StdAfx.h"
#include "LoginHandler.h"
#include "./Common/SipParser.h"


CLoginHandler::CLoginHandler(void):m_sock(NULL)
{
}


CLoginHandler::~CLoginHandler(void)
{
}

void CLoginHandler::Init(SOCKET sock)
{
	m_sock = sock;

	//if(!pModule->m_ip.IsEmpty() && 0 != pModule->m_port)
	//{
	//	sAddr.sin_family = AF_INET;
	//	sAddr.sin_addr.S_un.S_addr = inet_addr(pModule->m_ip.GetString());
	//	sAddr.sin_port = htons(pModule->m_port);
	//	bind(m_sock, (struct sockaddr*)&sAddr, sizeof(sAddr));
	//}
}

void CLoginHandler::WaitLogin(SOCKADDR_IN &sAddr)
{
	char		buf[4096];
	timeval		timeout;
	int			nRecv = sizeof(SOCKADDR_IN);
	fd_set		fdset;
	auto bIsComplated = false;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	while(!bIsComplated)
	{
		// 等待登录应答
		FD_ZERO(&fdset);
		FD_SET(m_sock, &fdset);
		auto ret = select(0, &fdset, nullptr, nullptr, &timeout);
		if(0 == ret)
		{
			continue;
		}
		if (SOCKET_ERROR == ret)
		{
			WSAGetLastError();

			// 通信失败
			printf("等待登录请求应答通信故障，线程退出！\n");
			break;
		}

		// 接收登录应答
		ret = recvfrom(m_sock, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr *>(&sAddr), &nRecv);
		if(0 >= ret)
		{
			WSAGetLastError();
			break;
		}

		//strcpy(buf, "REGISTER sip:34020000002000000001@3402000000 SIP/2.0\r\n"
		//			"Via: SIP/2.0/UDP 192.168.3.135:5060;rport;branch=z9hG4bK1967686311\r\n"
		//			"From: <sip:34020000001180000002@3402000000>;tag=1753065625\r\n" 
		//			"To: <sip:34020000001180000002@3402000000>\r\n" 
		//			"Call-ID: 1481588382\r\n" 
		//			"CSeq: 7 REGISTER\r\n" 
		//			"Contact: <sip:34020000001180000002@192.168.3.135:5060>\r\n" 
		//			"Max-Forwards: 70\r\n" 
		//			"User-Agent: Embedded Net DVR/DVS\r\n" 
		//			"Expires: 3600\r\n" 
		//			"Content-Length: 0\r\n");

		// 解析接收的消息
		buf[ret] = 0;
		CString strMsg = buf;
		// 这是注册消息
		if(0 <= strMsg.Find("REGISTER"))
		{
			// 取得Via行
			CString strVia;
			CSipParser::get_line("Via", strMsg, strVia);

			// 取得From行
			CString strFrom;
			CSipParser::get_line("From", strMsg, strFrom);

			// 取得To行 
			CString strTo;
			CSipParser::get_line("To", strMsg, strTo);

			// 取得Call-ID行
			CString strCallID;
			CSipParser::get_line("Call-ID", strMsg, strCallID);

			// 取得CSeq行
			CString strCSeq;
			CSipParser::get_line("CSeq", strMsg, strCSeq);

			CString strAnswer;
			// 鉴权
			if(0 > strMsg.Find("Authorization"))
			{
				strAnswer.Format(RegisterAnswerUnauthorized, strVia, strFrom, strTo, strCallID, strCSeq);	
			}
			else
			{
				strAnswer.Format(RegisterAnswerOK, strVia, strFrom, strTo, strCallID, strCSeq);
				bIsComplated = true;
			}
			sendto(m_sock, strAnswer.GetString(), strAnswer.GetLength(), 0, reinterpret_cast<const SOCKADDR *>(&sAddr), sizeof(sAddr));
		}
	}
}