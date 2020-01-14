#pragma once

static _ATL_FUNC_INFO EventDelegate = { CC_STDCALL, VT_EMPTY, 2, {VT_VARIANT,VT_DISPATCH} };

#include <string>
#include <unordered_map>
#include <unordered_set>
using namespace std;

enum tagEC_EVENT_TYPE
{
	EET_OFFLINE_EVENT = 0,
	EET_ONLINE_EVENT = 1,
	EET_MESSAGE_ARRIVED = 2
};


enum _EC_EVENT_RESULT :INT
{
	FALSE_FAIELD = 0,
	TRUE_OK = 1,
	TRUE_DUP = 2
};


class DevicesInfoMgr;
class CSDKCom;

class ECClient :
	public IDispEventImpl<1, ECClient, &DIID_IConnectionManagerEvents, &LIBID_ECConnectionManager, 1, 0>
{
	typedef IDispEventImpl<0, ECClient, &DIID_IConnectionManagerEvents, &LIBID_ECConnectionManager, 1, 0> DispECEvent;
public:
	ECClient(void) = default;
	virtual ~ECClient(void);


	BOOL CheckConection(CString strECGUID);
	BOOL AddServer(LPCTSTR lpstrServerIP, long nServerPort, LPCTSTR lpstrServerID);
	void RemoveServer(LPCTSTR lpstrServerID);

	INT Connect(LPCTSTR lpstrName, LPCTSTR lpstrPassword, LPCTSTR lpstrServerID);
	void DisConnect(LPCTSTR lpstrServerID);

	INT  ListenTo(LPCTSTR lpstrDeviceID, BOOL bListenTo, LPCTSTR lpstrServerID);

	BEGIN_SINK_MAP(ECClient)
		SINK_ENTRY_INFO(1, DIID_IConnectionManagerEvents, 0, OnOfflineEvent, &EventDelegate)
		SINK_ENTRY_INFO(1, DIID_IConnectionManagerEvents, 1, OnOnlineEvent, &EventDelegate)
		SINK_ENTRY_INFO(1, DIID_IConnectionManagerEvents, 2, OnMessageArrived, &EventDelegate)
	END_SINK_MAP()

	void __stdcall OnOfflineEvent(VARIANT bstrGUID, IDispatch FAR* bstrDetail);
	void __stdcall OnOnlineEvent(VARIANT bstrGUID, IDispatch FAR* bstrDetail);
	void __stdcall OnMessageArrived(VARIANT bstrGUID, IDispatch FAR* bstrDetail);

	BOOL PostCommand(CString &sCommandContent, CString &sServerDeviceID) const;

	void SetParent(DevicesInfoMgr *pParent)
	{
		m_pParent = pParent;
	}
private:

	typedef struct _EC_ConnetionInfo_
	{
		LPCTSTR pszECIP;
		bool b_Connected = false;
		std::unordered_set<LPCTSTR> pszDeviceSets;
	}EC_CONNETION;

	IConnectionManagerProvider *m_pConnectionManagerProvider;
	DevicesInfoMgr *m_pParent;
	unordered_map<std::string, EC_CONNETION> m_Connections;


};
