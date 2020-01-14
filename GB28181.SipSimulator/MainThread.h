#pragma once

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID);

enum TASKID
{
	DEVICE_INFO_QUERY = 1,
	PLAYBACK,
	PLAYBACK_CTRL,
	PLAY_VIDEO
	//SETIPPORT = 9
};

struct TASK_DESC
{
	TASKID		id;
	char		name[32];
};