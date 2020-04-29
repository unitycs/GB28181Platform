#include "./../StdAfx.h"
#include "InviteTask.h"
#include <WinSock2.h>

//char CInviteTask::bye_req[] = "";

CInviteTask::CInviteTask(SOCKET sock, SOCKADDR_IN tSockAddr)
	: CTask(sock, tSockAddr)
{
}


CInviteTask::~CInviteTask(void)
{
}

void CInviteTask::Init()
{
	m_strHead = "INVITE sip:34020000001320000001@3402000000 SIP/2.0\n"
		"Call-ID: %s@0.0.0.0\n"
		"CSeq: 1 INVITE\n"
		"From: <sip:34010000002000000001@3401000000>;tag=%s\n"
		"To: <sip:34020000001320000001@3402000000>\n"
		"Max-Forwards: 70\n"
		"Contact: \"34010000002000000001\" <sip:192.168.3.82:5060>\n"
		"Subject: 34020000001320000001:1-4-1364436954760,34010000002000000001:1\n"
		"Content-Type: application/sdp\n"
		"Route: <sip:34020000001320000001@192.168.3.75:5060;lr>\n"
		"Via: SIP/2.0/UDP 192.168.3.82:5060;branch=%s\n"
		"Content-Length: 253\n"
		"\n"
		"v=0\n"
		"o=34020000002020000001 0 0 IN IP4 192.168.3.82\n"
		"s=Playback\n"
		"u=34020000001320000001:3\n"
		"c=IN IP4 192.168.3.82\n"
		"t=1364436626 1364436723\n"
		"m=video 6000 RTP/AVP 96 98 97\n"
		"a=recvonly\n"
		"a=rtpmap:96 PS/90000\n"
		"a=rtpmap:98 H264/90000\n"
		"a=rtpmap:97 MPEG4/90000\n";

	m_strBody = "ACK sip:34020000002000000001@192.168.3.75:5060 SIP/2.0"
		"Call-ID: %s@0.0.0.0"
		"CSeq: 1 ACK"
		"From: <sip:34010000002000000001@3401000000>;tag=%s"
		"To: <sip:34020000001320000001@3402000000>;tag=%s"
		"Max-Forwards: 70"
		"Via: SIP/2.0/UDP 192.168.3.82:5060;branch=%s"
		"Content-Length: 0";

	InitParam();

	// 注册工作线程
	RegisterProc(pfnSendProc, this, nConc);
}

// 释放占用内存
void CInviteTask::Cleanup()
{

}

//UINT AFX_CDECL CInviteTask::pfnLogProc( LPVOID lParam )
//{
//	CInviteTask	* pModule = reinterpret_cast<CInviteTask *>(lParam);
//	int				nMaxPerSec = pModule->nMaxPerSec;
//	int				nMaxCount = pModule->nMaxCount;
//	int				nSent = 0;
//	int				nRecv = 0;
//	int				ret;
//	SOCKADDR_IN		sAddr;
//	SOCKET			sock;
//	fd_set			m_set;
//	timeval			timeout;
//
//	char			CALL_ID[64];
//	char			TAG[128];
//	char			BRANCH[128];
//	char			RTAG[128];
//	char			buf[4096];
//	LARGE_INTEGER	liPerfFreq	= {0};
//	LARGE_INTEGER	liPerfCur	= {0};
//	int				nOweSum		= 0;
//	int				msSum		= nMaxPerSec / 1000.0;		//每毫秒发送的个数
//
//	// 初始化通信参数
//	timeout.tv_sec = 5;
//	timeout.tv_usec = 0;
//	gen_call_id(CALL_ID);
//	gen_tag(TAG);
//	gen_branch(BRANCH);
//
//	sock = pModule->m_sock;
//
//
//	// 计时开始
//	QueryPerformanceFrequency(&liPerfFreq); 
//
//	// 进入发送循环
//	for (int i = 0; i < nMaxCount; i++)
//	{
//		QueryPerformanceCounter(&liPerfCur);
//		int nStartTime = (int)(liPerfCur.QuadPart * 10000 / liPerfFreq.QuadPart);
//
//		// 初始化本次事务信息
//		sprintf_s(buf, invite_req, CALL_ID, TAG, BRANCH);
//
//		// 发送登录消息
//		sendto(sock, buf, strlen(buf), 0, (const SOCKADDR *) &sAddr, sizeof(sAddr));
//		
//		// 等待登录应答
//		FD_ZERO(&m_set);
//		FD_SET(sock, &m_set);
//		ret = select(0, &m_set, NULL, NULL, &timeout);
//		if (ret == 0)
//		{
//			// 超时无应答
//			printf("等待登录请求应答超过%d秒钟，线程退出！\n", timeout.tv_sec);
//			break;
//		}
//		else if (ret = SOCKET_ERROR)
//		{
//			// 通信失败
//			printf("等待登录请求应答通信故障，线程退出！\n");
//			break;
//		}
//
//		// 接收登录应答
//		recvfrom(sock, buf, sizeof(buf), 0, (SOCKADDR *) &sAddr, &nRecv);
//		parse_rtag(buf, RTAG);
//
//		// 发送登录应答确认
//		sprintf_s(buf, invite_ack, CALL_ID, TAG, RTAG, BRANCH);
//		sendto(sock, buf, strlen(buf), 0, (const SOCKADDR *) &sAddr, sizeof(sAddr));
//
//		QueryPerformanceCounter(&liPerfCur);
//		int nEndTime = (int)(liPerfCur.QuadPart * 10000 / liPerfFreq.QuadPart);
//
//		if(0 == nOweSum && 5 < nEndTime - nStartTime)
//		{
//			Sleep(1);
//
//			QueryPerformanceCounter(&liPerfCur);
//			int nCurTime = (int)(liPerfCur.QuadPart* 10000 / liPerfFreq.QuadPart);
//
//			// Sleep(1)实际休眠时间会超过1毫米
//			// 取得实际的休眠时间，计算此时间内应该工作的次数
//			nOweSum = ((nCurTime - nEndTime) * msSum) / 10.0;
//		}
//		else
//		{
//			nOweSum--;
//			nOweSum = 0 > nOweSum ? 0 : nOweSum;
//			continue;
//		}
//	}
//
//	return 0;
//}
//
