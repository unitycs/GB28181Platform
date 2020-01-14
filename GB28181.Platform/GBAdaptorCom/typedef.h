/*
*  当前文件定义用于UAS和UAC通信的用的RawData协议的数据字段说明
0                   1                   2                   End
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 ...   8 9 0 1
---------------------------------------------------------------
|HEAD |CMD ID |  SEQ  | Operate OR  Other Fields ...        |#
---------------------------------------------------------------
*/
#pragma once
#include <list>
#include "Common/common.h"
#ifdef  interface 
#  undef  interface
#  define interface  class
#endif

#define SAFE_DELETE(p) { if(p) { delete (p); (p)=NULL; } }


//command id
#define CMD_UNDEFINED		0x00000000
#define CMD_REGISTER		0x00000001	
#define CMD_REGISTERRES		0x80000001	
#define CMD_INVITE			0x00000002	
#define CMD_INVITERES		0x80000002	
#define CMD_MESSAGE			0x00000003	
#define CMD_MESSAGERES		0x80000003	
#define CMD_LOGIN			0x00000004	
#define CMD_LOGINRES		0x80000004	
#define CMD_SUBSCRIBE		0x00000005	
#define CMD_SUBSCRIBERES	0x80000005	
#define CMD_NOTIFY			0x00000006	
#define CMD_NOTIFYRES		0x80000006	
#define CMD_INFO			0x00000007	
#define CMD_INFORES			0x80000007	
#define CMD_BYE				0x00000008	
#define CMD_BYERES			0x80000008	
#define CMD_LOGOUT			0x00000009	
#define CMD_LOGOUTRES		0x80000009	
#define CMD_DECODER_PLAY	0x0000000A	
#define CMD_DECODER_PLAYRES	0x8000000A	
#define CMD_CALL_ACK		0x00000010	
#define CMD_CALLACKRES		0x80000010	
#define CMD_RESET_ALARM		0x00000011	
#define CMD_RESET_ALARMRES	0x80000011	
#define CMD_USEREXIT		0x00001000	
#define CMD_PRIVATE         0x000000FF	


//video control sub type
#define INFO_PLAY	           0
#define INFO_PAUSE             1
#define INFO_SPEED             2
#define INFO_PLAY_RESUME       3      //继续播放

//INVITE cmd type
#define INVITE_REALPLAY	       0
#define INVITE_PLAYBACK        1
#define INVITE_DOWNLOAD        2

//ptz cmd type
#define PTZ_CMD			       0
#define DRAG_ZOOM_NONE	       0
#define DRAG_ZOOM_IN	       1
#define DRAG_ZOOM_OUT	       2
#define DRAG_ZOOM_NULL	       3
#define SWITCH_BEGIN		   0
#define SWITCH_OFF			   0
#define SWITCH_ON			   1
#define SWITCH_END			   2


//message command type
#define MSG_DEVICE_CTRL			    1
#define MSG_DEVICE_CTRL_RES		    2
#define MSG_INFO_INQUIRE			3
#define MSG_INFO_INQUIRE_RES		4
#define MSG_STATUS_INQUIRE			5
#define MSG_STATUS_INQUIRE_RES		6
#define MSG_RECORD_INQUIRE			7
#define MSG_RECORD_INQUIRE_RES		8
#define MSG_CATALOG_INQUIRE		    9
#define MSG_CATALOG_INQUIRE_RES	    10
#define MSG_SET_RECORD				11
#define MSG_SET_RECORD_RES			12
#define MSG_SET_GUARD				13
#define MSG_SET_GUARD_RES			14
#define MSG_ALARM_NOTIFY			15
#define MSG_ALARM_NOTIFY_RES		16
#define MSG_REMOTE_STARTUP			17
#define MSG_REMOTE_STARTUP_RES		18
#define CALL_MSG_FILE_TOEND			19
#define CALL_MSG_FILE_TOEND_RES		20
#define MSG_DEVICE_CONFIG			21
#define MSG_PRESET_QUERY			22
#define MSG_CONFIG_DOWNLOAD	    	23
#define MSG_CATALOG_SUBSCRIBE		24
#define MSG_REAL_PLAY_URL			25  //支持申通
#define MSG_PLAY_BACK_URL			26  //支持申通
#define MSG_DECODER_STATUS			27  //支持申通
#define MSG_SUBSCRIBE_ALARM		    28
#define MSG_ALARM_QUERY		    	29
#define MSG_ALARM_QUERY_RES	        30
#define MSG_SEND_EVENT_SUBSCRIBE    31
#define MSG_STOP_PLAY_URL           32
#define MSG_DECODER_DIVISION        33   //支持申通



//自定义类型
#define SIP_CLIENT_LOGIN_TYPE                1
#define SIP_CLIENT_LOGIN_RES_TYPE            2
#define SIP_CLIENT_LOGOUT_TYPE               3
#define SIP_CLIENT_LOGOUT_RES_TYPE           4
#define SIP_CLIENT_CHANGEPASSWORD_TYPE       5
#define SIP_CLIENT_CHANGEPASSWORD_RES_TYPE   6

// 为了提高效率
// 把digest长度和nonce的位置定义成常量
#define LEN_DIGEST  	                66
#define	LEN_NONCE    	                32
#define LEN_BODY_BUF	                5000
#define LEN_PASSWD                      32
#define LEN_MESSAGENAME                 100
#define LEN_CALLID                      128
#define LEN_PRIORITY                    4
#define LEN_MONTH                       4

#define LEN_RECIVE_BUFFER		    	4096
#define LEN_SEND_RESPONCE		    	10000
#define LEN_SEND_REQUEST		    	512
#define LEN_SEND_LOGIN			    	10000
#define	LEN_RECEIVE		                65536		// 每次接收的数据量


#define LEN_ANSWER_RESPONSE_PKG_SIZE            10000
#define LEN_ANSWER_REGSTER_PKG_SIZE	    	    38
#define LEN_ANSWER_DEVICE_CONTROL_PKG_SIZE	    21
#define LEN_ANSWER_BUFF_EXTEND_PKG_SIZE	        128
#define LEN_ANSWER_MEDIA_STATUS_PKG_SIZE        32
#define LEN_ANSWER_PLAYBACK_CTRL_PKG_SIZE       24
#define LEN_ANSWER_DEVICE_STARTUP_PKG_SIZE	    21

//users status
#define STATUS_EMPTY			-2
#define STATUS_NOT_REGISTERED	-1
#define STATUS_REGISTERING		 0
#define STATUS_REGISTERED		 1
#define STATUS_LOGING_OUT		 2


// Device Status Content
enum enmDeviceStatusContent {
	AC_DEVICE_STATUS_NONE = 0,
	AC_DEVICE_STATUS_ONLINE = 1,
	AC_DEVICE_STATUS_OFFLINE = 2
};


//device type
#define IPC_DEVICE_TYPE		        1
#define CLIENT_DEVICE_TYPE          2       //Adapter SIP平级平台
#define THE_OTHER_TYPE		        2
#define NVR_DEVICE_TYPE		        0
#define EC_DEVICE_TYPE		        1
#define CLIENT_DEVICE_TYPE	        2


//trans type
#define TYPE_TCP_TRANS			        	 0
#define TYPE_UDP_TRANS			         	 1



typedef struct CLIENTGETWAYINFO_T
{
	char strToUserID[MAX_PATH];
	char strToUserIP[MAX_PATH];
	char strToUserPort[MAX_PATH];
	char strPassword[MAX_PATH];
}clientgetwayinfo;

typedef struct USERGETWAYINFO_T {
	bool nOnlineStatus;
	char strToUserID[MAX_PATH];
	char strToUserIP[MAX_PATH];
	char strToUserPort[MAX_PATH];
	char strPassword[MAX_PATH];
	char strUasTcpPort[MAX_PATH];
	bool nCheckAuth;
	int nServiceType;//0服务端 1客户端
	std::vector<CLIENTGETWAYINFO_T> v_ClientGetwayInfo;
} usergetwayinfo;

typedef struct REGPARAM_T {
	int regid;
	int expiry;
	int auth;
} regparam;



typedef struct SUBUSERGB28181 {
	SUBUSERGB28181()
	{
		memset(strDeviceID, 0, ID_LEN + 1);
		nType = 0;
	}
	char strDeviceID[ID_LEN + 1];
	int nType;
}sub_userGB28181_t;
typedef struct USERGB28181_T {
	USERGB28181_T()
	{
		nStatus = STATUS_EMPTY;
		nType = 0;
		memset(strDeviceID, 0, ID_LEN + 1);
		memset(strPassWord, 0, LEN_PASSWD + 1);
		memset(strIP, 0, IP_LEN + 1);
		memset(strPort, 0, PORT_LEN + 1);
		timLastKeepAlive = 0;
		nExpires = -1;
		nKeepaliveTime = 0;
	}

	USERGB28181_T& operator = (USERGB28181_T &temp)
	{
		this->nStatus = temp.nStatus;
		this->nType = temp.nType;
		strncpy_s(this->strDeviceID, temp.strDeviceID, strlen(temp.strDeviceID));
		strncpy_s(this->strPassWord, temp.strPassWord, strlen(temp.strPassWord));
		strncpy_s(this->strIP, temp.strIP, strlen(temp.strIP));
		strncpy_s(this->strPort, temp.strPort, strlen(temp.strPort));
		this->nExpires = temp.nExpires;
		this->timLastKeepAlive = temp.timLastKeepAlive;
		return *this;
	}

	int nStatus;			//-1:not online, 0: registering, 1:registered
	int nType;				//0:IPC 1:CLIENT 2:EC 3:NVR
	int nExpires;
	int nKeepaliveTime;
	char strDeviceID[ID_LEN + 1];
	char strPassWord[LEN_PASSWD + 1];
	char strDeviceType[ID_LEN + 1];
	char strIP[IP_LEN + 1];
	char strPort[PORT_LEN + 1];
	time_t timLastKeepAlive;
	std::vector<sub_userGB28181_t> subDevice;

}usergb28181_t, *lpUserGB28181_t;

typedef struct PLAYBACK_T {
	PLAYBACK_T()
	{
		memset(strCallID, 0, LEN_CALLID + 1);
		//nCid = 0;
		//nDid = 0;
	}
	char strCallID[LEN_CALLID + 1];
	//int nCid;
	//int nDid;
}playBack_t;
typedef struct USERTCP_T {
	USERTCP_T()
	{
		nStatus = STATUS_EMPTY;
		nType = 0;
		memset(strIP, 0, IP_LEN + 1);
		memset(strPort, 0, PORT_LEN + 1);
		socUser = INVALID_SOCKET;
		pParent = nullptr;
	}

	USERTCP_T& operator = (USERTCP_T &temp)
	{
		this->nStatus = temp.nStatus;
		this->nType = temp.nType;
		strncpy_s(this->strIP, temp.strIP, strlen(temp.strIP));
		strncpy_s(this->strPort, temp.strPort, strlen(temp.strPort));
		socUser = temp.socUser;
		return *this;
	}

	int nStatus;			//-1:not online, 0: registering, 1:registered
	int nType;				//0:is not EC 1:is EC
	char strIP[IP_LEN + 1];
	char strPort[PORT_LEN + 1];
	SOCKET socUser;
	void *pParent;
	std::list<playBack_t> streamCallID;

}usertcp_t, *lpUserTcp_t;

