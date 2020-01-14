#pragma once
#include "Module.h"
#include "./../TaskBase/Task.h"

class CQueryTask
	: public CModule, public CTask
{
public:
	CQueryTask(SOCKET sock, SOCKADDR_IN tSockAddr);
	~CQueryTask(void);

	virtual void Init() override;
	virtual void Cleanup() override;
};

