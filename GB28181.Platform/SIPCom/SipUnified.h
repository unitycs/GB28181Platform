#pragma once
#include "SIPCom/SipRecordBase.h"
#include "SIPCom/BodyBuilder.h"
#include "Common/Utils.h"
#include "Common/AppSetting.h"
#include "Json/json.hpp"
#include <fstream>
using namespace std;
class SipUnified
{
public:
	SipUnified();
	virtual ~SipUnified();
	// 生成新的SN号
	static int GenerateReportSN(CString &strSN);
	// 时间格式转换
	static void GB28181TimeToDATE(const char *pszTime, DATE &dateTime);

	// 时间格式转换
	static void DATEToGB28181Time(const char *pszTime, DATE &dateTime);
};

class DeviceBasicObject
{
public:
	DeviceBasicObject() : needAddRecordCount(0)
	{}

	virtual ~DeviceBasicObject() {}
	typedef struct InfoContext
	{
		InfoContext() : posRecordMap(nullptr), nSurplus(0), nRawDevSum(0), nPerSum(1)
		{
		}
		POSITION posRecordMap;			// 在CMap中的位置
		int		 nSurplus;				// 还能合成xml文件的数量
		int		 nRawDevSum;			// 未统计的设备数
		int		 nPerSum;				// 每次发送个数
	}InfoContext_t;

	void SetDeviceID(const char *pszDeviceID)
	{
		m_strDeviceID = pszDeviceID;
	}

	const  CString & GetDeviceID()
	{
		return m_strDeviceID;
	}
	void SetDataType(const char *pszDataType)
	{
		m_strDataType = pszDataType;
	}
	const  CString & GetDataType()
	{
		return m_strDataType;
	}

	void SetGBType(GBDevice_T eGBDevType)
	{
		m_eGBDevType = eGBDevType;
	}

	const GBDevice_T & GetGBType()
	{
		return m_eGBDevType;
	}

	CString			m_strDeviceID;
protected:
	// 设备类型
	GBDevice_T      m_eGBDevType;
	CBodyBuilder	m_oBodyBuilder;
	CString			m_strDataType;


public:
	int needAddRecordCount;
};

// 静态的设备信息
// 从配置文件中读取
class CDevProperty
{
public:
	CDevProperty() :m_bIsEncoder(false), m_bIsDecoder(false)
	{
	}
	~CDevProperty()
	{
	}
	void SetDeviceID(const char *pszDeviceID)
	{
		m_strDeviceID = pszDeviceID;
	}

	const char * GetDeviceID()
	{
		return m_strDeviceID;
	}

	void SetDeviceType(const char *pszDevType)
	{
		m_strDeviceType = pszDevType;
		m_strDeviceType.MakeUpper();
		if (m_strDeviceType.Find("ENCODER") >= 0)
			m_bIsEncoder = true;
		if (m_strDeviceType.Find("DECODER") >= 0)
			m_bIsDecoder = true;

		m_strDeviceType = pszDevType;
	}

	void SetManufacturer(const char *pszManufacturer)
	{
		m_strManufacturer = pszManufacturer;
	}

	void SetModel(const char *pszModel)
	{
		m_strModel = pszModel;
	}

	void SetFirmware(const char *pszFirmware)
	{
		m_strFirmware = pszFirmware;
	}

	void SetMaxCamera(const char *pszMaxCamera)
	{
		m_strMaxCamera = pszMaxCamera;
	}
	void SetMaxAlarm(const char *pszMaxAlarm)
	{
		m_strMaxAlarm = pszMaxAlarm;
	}

	CString &GetDeviceType()
	{
		return m_strDeviceType;
	}

	CString &GetManufacturer()
	{
		return m_strManufacturer;
	}

	CString &GetModel()
	{
		return m_strModel;
	}

	CString &GetFirmware()
	{
		return m_strFirmware;
	}

	bool IsDecoder() const
	{
		return m_bIsDecoder;
	}
	bool IsEncoder() const
	{
		return m_bIsEncoder;
	}

private:
	CString m_strDeviceType;
	CString m_strManufacturer;
	CString m_strModel;
	CString m_strFirmware;
	CString m_strMaxCamera;
	CString m_strMaxAlarm;
	CString m_strDeviceID;
	bool m_bIsEncoder;
	bool m_bIsDecoder;
};
// SDP信息
class CDevSDP : public DeviceBasicObject
{
public:
	CDevSDP()
		: m_strStartTime("0"),
		m_strEndTime("0"),
		m_transType(0),
		m_mediaType(0),
		m_eDevType(HUSDevice_T::IPC_DVR)
	{
	}

	~CDevSDP() {}

	// 设置媒体源IP
	void SetMediaIP(const char *pszMediaSrcIP)
	{
		m_strMediaSrcIP = pszMediaSrcIP;
	}
	// 设置媒体源端口
	void SetMediaPort(const char *pszMediaSrcPort)
	{
		m_strMediaSrcPort = pszMediaSrcPort;
	}

	// 设置会话名
	void SetSessionName(const char *pszSessionName)
	{
		m_strSessionName = pszSessionName;
	}

	// 设置媒体开始-结束时间
	void SetMediaTime(const char *pszStartTime, const char *pszEndTime)
	{
		m_strStartTime = pszStartTime;
		m_strEndTime = pszEndTime;
	}

	// 设置密码
	void SetPassword(const char *pszPassword)
	{
		m_strPassword = pszPassword;
	}

	//  设置SSRC值
	void SetSSRC(const char *pszSSRC)
	{
		m_strSSRC = pszSSRC;
	}

	void SetTransType(int transType)
	{
		this->m_transType = transType;
	}

	void SetMediaType(int mediaType)
	{
		this->m_mediaType = mediaType;
	}

	void SetFileSize(char *fileSize)
	{
		this->m_strFileSize = fileSize;
	}

	int GetCatalogBodyContent(char *pszInfo, int nBufLen, const char * /*pstrSN*/, InfoContext_t * /*pContext*/)
	{
		char szActiveType[][20] = { "sendonly", "recvonly" };
		m_oBodyBuilder.CreateInviteAnswerBody(pszInfo, nBufLen, m_strMediaSrcIP,
			m_strMediaSrcPort, m_strSessionName, szActiveType[m_transType],
			m_strStartTime, m_strEndTime, m_strDeviceID, m_strPassword, m_strSSRC, m_transType, m_mediaType, m_strFileSize);

		return 0;
	}
protected:
	CString			m_strMediaSrcIP;
	CString			m_strMediaSrcPort;
	CString			m_strSessionName;
	CString			m_strStartTime;
	CString			m_strEndTime;
	CString			m_strPassword;
	CString			m_strSSRC;
	int				m_transType;
	int			    m_mediaType;
	CString		    m_strFileSize;
	HUSDevice_T     m_eDevType;
};


class GBID_Creator {
	typedef struct _id_gen_describe_t
	{
		string type_str;
		unsigned long created_cout;
	}id_gen_describe_t;

	bool b_re_create = false;
	bool b_number_only = true;  //只生成数字
	bool b_letter_only = false;  //只生成字母，只有小写字母

	string   m_str_platformid;
	string   m_str_civil_divisions;
	using json = nlohmann::json;
	using id_gen_info_t = DEVICE_INFO_SETTING_T::id_gen_info_t;
	std::string m_strSerializeFilePath;
	bool b_new_id_created = false;

	unordered_map<GBDevice_T, id_gen_describe_t> m_id_gen_info;  //GB设备类型及其描述
	unordered_map<string, GBDevice_T> m_devicekey_info;

public:

	void InitialGenInfoFromFile(id_gen_info_t* p_id_info = nullptr)
	{
		if (!p_id_info) return;

		if (p_id_info->str_local_civil_mask != m_str_civil_divisions)
		{
			//not equel and will recreated
			b_re_create = true;
			return;
		}

		m_id_gen_info[GBDevice_T::ALARM].created_cout = std::stoul(p_id_info->str_last_alarm_id);
		m_id_gen_info[GBDevice_T::CAMERA].created_cout = std::stoul(p_id_info->str_last_camera_id);
		m_id_gen_info[GBDevice_T::DISPLAY].created_cout = std::stoul(p_id_info->str_last_decord_channel_id);
		m_id_gen_info[GBDevice_T::DECODER].created_cout = std::stoul(p_id_info->str_last_decord_id);
		m_id_gen_info[GBDevice_T::DVR].created_cout = std::stoul(p_id_info->str_last_dvr_id);
		m_id_gen_info[GBDevice_T::ENCODER].created_cout = std::stoul(p_id_info->str_last_encord_id);
		m_id_gen_info[GBDevice_T::IPC].created_cout = std::stoul(p_id_info->str_last_ipc_id);
		m_id_gen_info[GBDevice_T::NVR].created_cout = std::stoul(p_id_info->str_last_nvr_id);
	}

	void SerializeToFile()
	{
		if (m_strSerializeFilePath.empty() || !b_new_id_created) return;

		json json_info;
		std::ifstream(m_strSerializeFilePath) >> json_info;

		//check empty
		assert(!json_info.empty());

		auto id_generator = json_info["id_generator"];

		id_generator["civil_mask"] = m_str_civil_divisions;
		id_generator["last_ipc_id"] = std::to_string(m_id_gen_info[GBDevice_T::IPC].created_cout);
		id_generator["last_dvr_id"] = std::to_string(m_id_gen_info[GBDevice_T::DVR].created_cout);
		id_generator["last_camera_id"] = std::to_string(m_id_gen_info[GBDevice_T::CAMERA].created_cout);
		id_generator["last_nvr_id"] = std::to_string(m_id_gen_info[GBDevice_T::NVR].created_cout);
		id_generator["last_alarm_id"] = std::to_string(m_id_gen_info[GBDevice_T::ALARM].created_cout);
		id_generator["last_encord_id"] = std::to_string(m_id_gen_info[GBDevice_T::ENCODER].created_cout);
		id_generator["last_decord_id"] = std::to_string(m_id_gen_info[GBDevice_T::DECODER].created_cout);
		id_generator["last_decord_channel_id"] = std::to_string(m_id_gen_info[GBDevice_T::DISPLAY].created_cout);

		std::ofstream o(m_strSerializeFilePath.c_str());
		o << std::setw(4) << json_info << std::endl;
	}

	string CreateSubDeviceid(GBDevice_T device_type, CString deviceid, int index)
	{
		string _deviceid(deviceid);
		string new_sub_id = {};
		switch (device_type)
		{
		case	GBDevice_T::DVR:
			new_sub_id = Create_DvrSubID(_deviceid, index);
			break;

		case GBDevice_T::ENCODER:
			new_sub_id = Create_encoderSubID(_deviceid, index);
			break;
		case GBDevice_T::DECODER:
			new_sub_id = Create_DecorderSubID(_deviceid, index);
			break;
		case GBDevice_T::CAMERA:
			new_sub_id = Create_CameraSubID(_deviceid,index);
			break;
		default:
			break;
		}
		b_new_id_created = true;
		return new_sub_id;
	}

	string Create_IPCID()
	{
		return getnewdeviceid(GBDevice_T::IPC);
	}

	string Create_EncoderID()
	{
		return getnewdeviceid(GBDevice_T::ENCODER);
	}

	string Create_NvrID()
	{
		return getnewdeviceid(GBDevice_T::NVR);
	}

	string Create_DecoderID()
	{
		return getnewdeviceid(GBDevice_T::DECODER);
	}

	string Create_CameraID()
	{
		return getnewdeviceid(GBDevice_T::CAMERA);
	}

	string Create_DvrID()
	{
		return getnewdeviceid(GBDevice_T::DVR);
	}

	string Create_DisplayID()
	{
		return getnewdeviceid(GBDevice_T::DISPLAY);
	}

	string Create_AlarmDevID()
	{
		return getnewdeviceid(GBDevice_T::ALARM);
	}

	string Create_DevIDByType(GBDevice_T gb_dev_type)
	{
		return getnewdeviceid(gb_dev_type);
	}

	string Create_encoderSubID(string encoderid, unsigned int channelindex)
	{
		if (20 == encoderid.length())
		{
			string encoderid_dilisions = encoderid.substr(19, 1);
			string channelTem = to_string(channelindex);
			auto devicekind = "131";
			auto str_id = m_str_civil_divisions + devicekind;
			auto order_str = getChildDeviceOrderStr(encoderid_dilisions, channelTem);
			return (m_str_civil_divisions + devicekind + order_str);
		}
		return{};
	}

	   string Create_CameraSubID(string CameraId, unsigned int channelindex) const
	{
		if (20 == strlen(CameraId.c_str()))
		{
			string cameraid_dilisions = CameraId.substr(19, 1);
			string channelTem = to_string(channelindex);
			auto devicekind = "131";
			auto str_id = m_str_civil_divisions + devicekind;
			auto order_str = getChildDeviceOrderStr(cameraid_dilisions, channelTem);
			return (m_str_civil_divisions + devicekind + order_str);
		}
	}	
	string Create_DecorderSubID(string decorderId, unsigned int channelindex)
	{
		if (20 == decorderId.length())
		{
			string encoderid_dilisions = decorderId.substr(19, 1);
			string channelTem = to_string(channelindex);
			auto devicekind = "133";
			auto str_id = m_str_civil_divisions + devicekind;
			auto order_str = getChildDeviceOrderStr(encoderid_dilisions, channelTem);
			return (m_str_civil_divisions + devicekind + order_str);
		}
		return{};
	}

	string Create_DvrSubID(string dvrId, unsigned int channelindex)
	{
		if (20 == dvrId.length())
		{
			string encoderid_dilisions = dvrId.substr(19, 1);
			string channelTem = to_string(channelindex);
			auto devicekind = "131";
			auto str_id = m_str_civil_divisions + devicekind;
			auto order_str = getChildDeviceOrderStr(encoderid_dilisions, channelTem);
			return (m_str_civil_divisions + devicekind + order_str);
		}
		return{};
	}

	static string getChildDeviceOrderStr(string encoderid_dilisions, string channelTem)
	{
		auto ncount = 6 - channelTem.size();
		auto nzerostr = string(ncount, '0');
		return (nzerostr + channelTem + encoderid_dilisions);
	}

	bool  initial(string platformid, const char* config_path = nullptr, id_gen_info_t* p_id_info = nullptr)
	{
		m_str_platformid = platformid;

		paser_platform_id(m_str_platformid);

		initialDeviceTypeList();

		m_strSerializeFilePath = config_path;
		b_re_create = p_id_info->b_re_create;
		b_number_only = p_id_info->b_number_only;
		b_letter_only = p_id_info->b_letter_only;

		//如果不是重新生成就读取配置信息
		if (b_re_create) return true;
		//从json文件，反序列化到类型
		InitialGenInfoFromFile(p_id_info);

		return true;
	}

private:

	string getnewdeviceid(GBDevice_T dt) {
		if (dt >= GBDevice_T::MAXCOUNT) throw logic_error("devicetype not exist");

		auto str_devicetype = m_id_gen_info[dt].type_str;

		auto order_seq = ++m_id_gen_info[dt].created_cout;

		auto order_seq_str = getdeviceOrderStr(order_seq);
		auto str_id = m_str_civil_divisions + str_devicetype + move(order_seq_str);
		m_devicekey_info[str_id] = dt;
		b_new_id_created = true;
		return str_id;
	}

	string getdeviceOrderStr(unsigned long order_seq) const
	{
		//if (order_seq % 10 == 0) order_seq++;
		auto str_order_seq = to_string(order_seq);
		auto 	ncount = 7 - str_order_seq.size();
		auto nzerostr = string(ncount, '0');
		str_order_seq = nzerostr + str_order_seq;
		return str_order_seq;
	}

	void initialDeviceTypeList()
	{
		GBDevice_T dtlist[] = {
			GBDevice_T::DEVICE,
			GBDevice_T::DVR,     //111
			GBDevice_T::ENCODER, //113
			GBDevice_T::DECODER, //114
			GBDevice_T::NVR,	    //118
			GBDevice_T::CAMERA,  //131 模拟摄像机
			GBDevice_T::IPC,     //132 网络摄像机
			GBDevice_T::ALARM,   //134
			GBDevice_T::DISPLAY, //133
			GBDevice_T::MAXCOUNT
		};

		for (auto item : dtlist)
		{
			id_gen_describe_t item_dscribe;

			item_dscribe.created_cout = 0;
			item_dscribe.type_str = gettypestring(item);
			m_id_gen_info[item] = item_dscribe;
		}
	}

	string gettypestring(GBDevice_T dt)
	{
		string str_ret;
		switch (dt)
		{
		case	GBDevice_T::DVR:
			str_ret = "111";
			break;   //111
		case	GBDevice_T::ENCODER:
			str_ret = "113";
			break; //113
		case	GBDevice_T::NVR:
			str_ret = "118";
			break;	 //118
		case	GBDevice_T::DECODER:
			str_ret = "114";
			break; //114
		case	GBDevice_T::CAMERA:
			str_ret = "131";
			break;  //131 模拟摄像机
		case	GBDevice_T::IPC:
			str_ret = "132";
			break;     //132 网络摄像机
		case	GBDevice_T::DISPLAY:
			str_ret = "133";
			break; //133
		case	GBDevice_T::ALARM:
			str_ret = "134";
			break; //134
		default:
			break;
		}
		return str_ret;
	}
	
	GBDevice_T gettypebyname(CString _device_type)
	{
		auto _type = GBDevice_T::NONE;
		if (_device_type.Find(_T("dvr")) >= 0)
		{
			_type = GBDevice_T::DVR;
		}
		else if (_device_type.Find(_T("encoder")) >= 0)
		{
			_type = GBDevice_T::ENCODER;
		}
		else if (_device_type.Find(_T("decoder")) >= 0)
		{
			_type = GBDevice_T::DECODER;
		}
		if (_device_type.Find(_T("ipc")) >= 0)
		{
			_type = GBDevice_T::IPC;
		}

		if (_device_type.Find(_T("alarm")) >= 0 || _device_type.Find(_T("channel")) >= 0)
		{
			_type = GBDevice_T::ALARM;
		}

		return _type;
	}

	void	paser_platform_id(string platformid)
	{
		m_str_civil_divisions = platformid.substr(0, 10);
	}
};
