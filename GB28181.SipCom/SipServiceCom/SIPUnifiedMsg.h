#pragma once

#include "Memory/MemPoolUnit.h"

struct CSIPUnifiedMsg : public CMemPoolUnit
{
	CSIPUnifiedMsg() {

	}


	virtual ~CSIPUnifiedMsg()
	{

	}

	void * pObject;

};

