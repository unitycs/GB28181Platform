#pragma once
#include "Main/UnifiedMessage.h"

typedef enum XMLBodyType
{
	XBT_Control = 0,
	XBT_Notify,
	XBT_Query,
	XBT_Response,
	XBT_ERROR
}XMLBodyType_t;

typedef enum CatalogEventType
{
	CET_NONE = -1,
	CET_ON = 0,
	CET_OFF,
	CET_VLOST,
	CET_DEFECT,
	CET_ADD,
	CET_DEL,
	CET_UPDATE,
	CET_ERROR
}CatalogEventType_t;

#define BODY_NEWLINE  "\r\n"
const TCHAR* BodyTypeString[] = { "Control", "Notify", "Query", "Response" };
const TCHAR* CatalogEventTypeString[] = { "ON", "OFF", "VLOST", "DEFECT", "ADD", "DEL", "UPDATE" };
const CString str_NewLine = "\r\n";
const CString str_XmlHeader = "<?xml version=\"1.0\"?>";
const CString str_XmlHeader_GB2312 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>";
const CString str_XmlHeader_UTF8 = "<?xml version=\"1.0\" encoding = \"utf-8\"?>";
const CString str_CmdType = str_NewLine + "<CmdType>%s</CmdType>";
const CString str_SN = str_NewLine + "<SN>%s</SN>";
const CString str_DeviceList = str_NewLine + "<DeviceList Num=\"%d\">";
const CString str_Item = str_NewLine + "<Item/>";
const CString str_DeviceID = str_NewLine + "<DeviceID>%s</DeviceID>";
const CString str_ParentID = str_NewLine + "<ParentID>%s</ParentID>";
const CString str_BusinessGroupID = str_NewLine + "<BusinessGroupID>%s</BusinessGroupID>";
const CString str_Result = str_NewLine + "<Result>%s</Result>";
const CString str_Manufacturer = str_NewLine + "<Manufacturer>Honeywell</Manufacturer>";
const CString str_Model = str_NewLine + "<Model>%s</Model>"; //maybe platform
const CString str_Firmware = str_NewLine + "<Firmware>%s</Firmware>"; //Hon Ver1.0
const CString str_SumNum = str_NewLine + "<SumNum>%d</SumNum>";
const CString str_Owner = str_NewLine + "<Owner>%s</Owner>"; // Default Owner
const CString str_CivilCode = str_NewLine + "<CivilCode>%s</CivilCode>";
const CString str_Address = str_NewLine + "<Address>%s</Address>";
const CString str_RegisterWay = str_NewLine + "<RegisterWay>%d</RegisterWay>"; // 1 or 0 default 1
const CString str_Secrecy = str_NewLine + "<Secrecy>%d</Secrecy>";// 1 or 0 default 0

// Body XML包体抽象类,不能实例此类
class CXMLBodyContentBase
{
public:
	virtual ~CXMLBodyContentBase() = default;

	virtual void GetContent(CString& content) = 0;
	void SetDeviceID(const char* devid) { m_deviceID = devid; }
	void SetSN(const char* sn) { m_SN = sn; }
protected:
	CString m_deviceID;
	CString m_SN;
};

class CDeviceInfoContent
	: public CXMLBodyContentBase
{
public:
	CDeviceInfoContent(const char*devid, const char*sn, CDevProperty* prop)
	{
		m_deviceID = devid;
		m_SN = sn;
		m_pProperty = prop;
	}
	~CDeviceInfoContent() = default;
public:
	void SetProperty(CDevProperty* prop) { m_pProperty = prop; }
	void GetContent(CString& content)
	{
		CString infoBody;
		infoBody.Format(
			str_CmdType +
			str_SN +
			str_DeviceID +
			str_Result +
			str_Manufacturer +
			str_Model +
			str_Firmware,
			"DeviceInfo", m_SN, m_deviceID, "OK",
			m_pProperty->GetManufacturer(),
			m_pProperty->GetModel(),
			m_pProperty->GetFirmware());
		content = std::move(infoBody);
	}
private:
	CDevProperty* m_pProperty;
};

// 平台目录Body
class CPlatformCatalogContent
	: public CXMLBodyContentBase
{
public:
	CPlatformCatalogContent(const char*devid, const char*sn, const char*cv, int nSum) :m_nSum(nSum)
	{
		m_deviceID = devid;
		m_SN = sn;
		m_civilCode = cv;
		m_strName = "HON_Platform";
	}
	CPlatformCatalogContent(const char* pszName, const char*devid, const char*sn, const char*cv, int nSum) :m_nSum(nSum)
	{
		m_deviceID = devid;
		m_SN = sn;
		m_civilCode = cv;
		m_strName = (pszName == nullptr ? "HON_Platform" : pszName);
	}

	~CPlatformCatalogContent() {}

	void SetCivilCode(const char* civilcode) { m_civilCode = civilcode; }
	void GetContent(CString& content) override
	{
		TCHAR* node =
			_T("<CmdType>Catalog</CmdType>"BODY_NEWLINE
				"<SN>%s</SN>"BODY_NEWLINE
				"<DeviceID>%s</DeviceID>"BODY_NEWLINE
				"<SumNum>%d</SumNum>"BODY_NEWLINE
				"<DeviceList Num=\"1\">"BODY_NEWLINE
				"<Item>"BODY_NEWLINE
				"<DeviceID>%s</DeviceID>"BODY_NEWLINE
				"<Name>%s</Name>"BODY_NEWLINE
				"<Manufacturer>Honeywell</Manufacturer>"BODY_NEWLINE
				"<Model>Platform</Model>"BODY_NEWLINE
				"<Owner>Owner</Owner>"BODY_NEWLINE
				"<CivilCode>%s</CivilCode>"BODY_NEWLINE
				"<Address>Address</Address>"BODY_NEWLINE
				"<RegisterWay>1</RegisterWay>"BODY_NEWLINE
				"<Secrecy>0</Secrecy>"BODY_NEWLINE
				"</Item>"BODY_NEWLINE
				"</DeviceList>"BODY_NEWLINE);
		TCHAR myBody[2048] = { 0 };
		sprintf_s(myBody, node, m_SN, m_deviceID, m_nSum, m_deviceID, m_strName, m_civilCode);
		content = myBody;
	}
private:
	int m_nSum;
	CString m_civilCode;
	CString m_strName;
};

// 多Item目录Body,不实例此类
class CMultiCatalog
	: public CXMLBodyContentBase
{
public:
	explicit CMultiCatalog(int m_n_sum)
		: m_nSum(m_n_sum)
	{
	}
	void AddCatalog(CatalogItem catalog) { m_catalogList.AddTail(catalog); }
	void GetContent(CString& content) override
	{
		CString strItems;
		int nDevListCount = m_catalogList.GetCount();

		GetItems(strItems);

		TCHAR* node =
			_T("<CmdType>Catalog</CmdType>"BODY_NEWLINE
				"<SN>%s</SN>"BODY_NEWLINE
				"<DeviceID>%s</DeviceID>"BODY_NEWLINE
				"<SumNum>%d</SumNum>"BODY_NEWLINE
				"<DeviceList Num=\"%d\">"BODY_NEWLINE
				"%s" // items
				"</DeviceList>"BODY_NEWLINE);
		TCHAR myBody[2048] = { 0 };
		sprintf_s(myBody, node, m_SN, m_deviceID, m_nSum, nDevListCount, strItems);
		content = myBody;
	}
protected:
	CList<CatalogItem> m_catalogList;
	int m_nSum;
	virtual void GetItems(CString& catalog) = 0;
};

// 虚拟目录Body
class CVirtualCatalogContent
	: public CMultiCatalog
{
public:
	CVirtualCatalogContent(const char*devid, const char*sn, int nSum) : CMultiCatalog(nSum)
	{
		m_nSum = nSum;
		m_deviceID = devid;
		m_SN = sn;
	}
	~CVirtualCatalogContent() {}
protected:
	void GetItems(CString& catalog) override
	{
		CString businessID(_T("44000000002150000001"));
		TCHAR* itemFmt =
			_T("<Item>"BODY_NEWLINE
				"<DeviceID>%s</DeviceID>"BODY_NEWLINE
				"<Name>%s</Name>"BODY_NEWLINE
				"<ParentID>%s</ParentID>"BODY_NEWLINE
				"<BusinessGroupID>%s</BusinessGroupID>"BODY_NEWLINE
				"</Item>"BODY_NEWLINE);
		TCHAR myItems[2048] = { 0 };
		int offset = 0;
		//		int nDevListCount = m_catalogList.GetCount();
		auto pos = m_catalogList.GetHeadPosition();
		while (pos)
		{
			auto tmpCatalog = m_catalogList.GetNext(pos);
			TCHAR tmpItem[512] = { 0 };
			sprintf_s(tmpItem, itemFmt, tmpCatalog.GetDeviceID(), tmpCatalog.GetName(), m_deviceID, businessID); // catalog.GetBusinessGroupID()
			int tmpLen = strlen(tmpItem);
			memcpy(myItems + offset, tmpItem, tmpLen + 1);
			offset += tmpLen;
		}

		catalog = myItems;
	}
};

// 设备目录Body
class CDeviceCatalog
	: public CMultiCatalog
{
public:
	CDeviceCatalog(const char*devid, const char*sn, int nSum) : CMultiCatalog(nSum)
	{
		m_nSum = nSum;
		m_deviceID = devid;
		m_SN = sn;
		m_eveType = CET_NONE;
	}
	~CDeviceCatalog() {}

	void SetEvent(CatalogEventType_t eve) { m_eveType = eve; }
protected:
	void GetItems(CString& catalog) override
	{
		switch (m_eveType)
		{
		case CET_ADD:
		case CET_UPDATE:
			GetUpdateItem(catalog, TRUE);
			break;
		case CET_ON:
		case CET_OFF:
		case CET_VLOST:
		case CET_DEFECT:
		case CET_DEL:
			GetStatusItem(catalog);
			break;
		default:
			GetUpdateItem(catalog, FALSE);
		}
	}
private:
	BOOL IsEvent() const
	{
		return (m_eveType > CET_NONE && m_eveType < CET_ERROR);
	}
	void GetUpdateItem(CString& item, BOOL bEvent)
	{
		CString eveSection;

		if ((m_eveType == CET_ADD || m_eveType == CET_UPDATE) && bEvent)
		{
			eveSection.Append("<Event>");
			eveSection.Append(CatalogEventTypeString[m_eveType]);
			eveSection.Append("</Event>");
			eveSection.Append(BODY_NEWLINE);
		}

		//	int nDevListCount = m_catalogList.GetCount();
		auto pos = m_catalogList.GetHeadPosition();
		CString itemsResult;
		while (pos)
		{
			auto catalog = m_catalogList.GetNext(pos);
			CString strNode[] = {
				"DeviceID", catalog.GetDeviceID(),
				"Name", catalog.GetName(),
				"Manufacturer", catalog.GetManufacturer(),
				"Model", catalog.GetModel(),
				"Owner", catalog.GetOwner(),
				"CivilCode", catalog.GetCivilCode(),
				"Block", catalog.GetBlock(),
				"Address", catalog.GetAddress(),
				"Parental", catalog.GetParental(),
				"ParentID", catalog.GetParentID(),
				"SafetyWay", catalog.GetSafetyWay(),
				"RegisterWay", catalog.GetRegisterWay(),
				"Certifiable", catalog.GetCertifiable(),
				"ErrCode", catalog.GetErrcode(),
				"Secrecy", catalog.GetSecrecy(),
				"Status", catalog.GetStatus(),
				"Longitude", catalog.GetLongitude(),
				"Latitude", catalog.GetLatitude(),
				//"OperateType", catalog.GetOperateType()
			};

			CString strNodeInfo[] = {
				"PTZType", catalog.GetPTZType(),
				//"PositionType", catalog.GetPositionType(),
			};
			int nodeLen = sizeof(strNode) / sizeof(strNode[0]);
			int nodeInfoLen = sizeof(strNodeInfo) / sizeof(strNodeInfo[0]);

			itemsResult.Append("<Item>");
			itemsResult.Append(BODY_NEWLINE);
			TCHAR* devFmt = _T("<%s>%s</%s>"BODY_NEWLINE);
			for (auto i = 0; i < nodeLen - 1;)
			{
				TCHAR tmpItem[128] = { 0 };
				sprintf_s(tmpItem, devFmt, strNode[i], strNode[i + 1], strNode[i]);
				itemsResult.Append(tmpItem);
				i += 2;
			}
			if (bEvent) itemsResult.Append(eveSection);

			itemsResult.Append("<Info>");
			itemsResult.Append(BODY_NEWLINE);
			for (auto i = 0; i < nodeInfoLen - 1;)
			{
				TCHAR tmpItem[128] = { 0 };
				sprintf_s(tmpItem, devFmt, strNodeInfo[i], strNodeInfo[i + 1], strNodeInfo[i]);
				itemsResult.Append(tmpItem);
				i += 2;
			}
			itemsResult.Append("</Info>");
			itemsResult.Append(BODY_NEWLINE);

			itemsResult.Append("</Item>");
			itemsResult.Append(BODY_NEWLINE);
		}

		item = itemsResult;
	}
	void GetStatusItem(CString& item)
	{
		CString eveSection;
		ASSERT(m_eveType > CET_NONE && m_eveType < CET_ERROR);
		TCHAR* devFmt;// = _T("<%s>%s</%s>"BODY_NEWLINE);

		//int nDevListCount = m_catalogList.GetCount();
		auto pos = m_catalogList.GetHeadPosition();
		CString itemsResult;
		while (pos)
		{
			auto catalog = m_catalogList.GetNext(pos);
			CString strNode[] = {
				"DeviceID", catalog.GetDeviceID(),
				"Event", CatalogEventTypeString[m_eveType]
			};

			int nodeLen = sizeof(strNode) / sizeof(strNode[0]);

			itemsResult.Append("<Item>");
			itemsResult.Append(BODY_NEWLINE);
			devFmt = _T("<%s>%s</%s>"BODY_NEWLINE);
			for (auto i = 0; i < nodeLen - 1;)
			{
				TCHAR tmpItem[128] = { 0 };
				sprintf_s(tmpItem, devFmt, strNode[i], strNode[i + 1], strNode[i]);
				itemsResult.Append(tmpItem);
				i += 2;
			}

			itemsResult.Append("</Item>");
			itemsResult.Append(BODY_NEWLINE);
		}

		item = itemsResult;
	}
private:
	CatalogEventType_t m_eveType;
};

// Body生成器
// Usage:
//  CPlatformCatalogContent cnt("42000000012000000001", "17", "4200000", 10);
//	CXMLBodyBuilder::CreateBody(CONTROL, cnt, xxx, xxx);
class BodyBuilder
{
public:
	BodyBuilder() {}
	~BodyBuilder() {}
	static void CreateContent(XMLBodyType_t bdType, CXMLBodyContentBase* body, char* pBodyBuff, int& nBuffLen)
	{
		ASSERT(bdType >= 0 && bdType < XBT_ERROR);
		CString bodyContent;
		body->GetContent(bodyContent);
		TCHAR* chFormat =
			_T("<?xml version=\"1.0\" encoding=\"gb2312\"?>"BODY_NEWLINE
				"<%s>"BODY_NEWLINE
				"%s" // where you have to fill by CBodyContentBase
				"</%s>"BODY_NEWLINE);
		TCHAR myBody[2048] = { 0 };
		sprintf_s(myBody, chFormat, BodyTypeString[bdType], bodyContent, BodyTypeString[bdType]);
		nBuffLen = strlen(myBody);
		memcpy(pBodyBuff, myBody, nBuffLen + 1);
	}
};
