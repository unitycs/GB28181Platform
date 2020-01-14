#include "StdAfx.h"
#include "IUAS.h"

int IUAS::SendGetwayAlive(void)
{
	return 0;
}


int IUAS::KeepAlive()
{
	return 0;
}


const char* drag_zoom_string(BYTE type)
{
	static const char* STRING_MAP[] =
	{
		NULL,
		"DragZoomIn",
		"DragZoomOut"
	};
	return STRING_MAP[type];
}

