#pragma once
#include "smartsocket.h"
class CSmartSocketClient :
	public CSmartSocket
{
public:
	CSmartSocketClient(void);
	~CSmartSocketClient(void);
	CSmartSocketClient::CSmartSocketClient(CUACTCP *pParent);

	UINT ThreadPro();
};

