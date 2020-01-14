#include "./../StdAfx.h"
#include "QueryTask.h"



//char query_body[] = "<?xml version=\"1.0\"?>\r\n" 
//	"<Control>\r\n" 
//	"<CmdType>DeviceControl</CmdType>\r\n" 
//	"<SN>%d</SN>\r\n" 
//	"<DeviceID>34010000002000000001</DeviceID>\r\n" 
//	"<TeleBoot>Boot</TeleBoot>\r\n" 
//	"</Control>\r\n" ;



CQueryTask::CQueryTask(SOCKET sock, SOCKADDR_IN tSockAddr)
	: CTask(sock, tSockAddr)
{
}


CQueryTask::~CQueryTask(void)
{
}

void CQueryTask::Init()
{
	m_strHead = "MESSAGE sip:34020000002000000001@3402000000 SIP/2.0\r\n" 
		"Via:SIP/2.0/UDP 192.168.3.75:5060;branch=%s\r\n"  
		"From: <sip:34020000002000000001@3402000000>;tag=%s\r\n"  
		"To: <sip:34020000002000000001@3402000000>\r\n"  
		"Call-ID: %s\r\n"  
		"CSeq: %d MESSAGE\r\n"  
		"Content-Length: %d\r\n" 
		"Content-Type:  Application/MANSCDP+xml\r\n" 
		"Max-Forwards: 70\r\n\r\n";

	m_strBody = "<?xml version=\"1.0\" ?>\r\n" 
		"<Query>\r\n" 
		"<CmdType>Catalog</CmdType>\r\n" 
		"<SN>%d</SN>\r\n" 
		"<DeviceID>34010000002000000001</DeviceID>\r\n" 
		"</Query>\r\n" ;

	InitParam();

	// 注册工作线程
	RegisterProc(pfnSendProc, this, nConc);
}

// 释放占用内存
void CQueryTask::Cleanup()
{

}