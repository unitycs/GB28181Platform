
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展


#include <afxdisp.h>        // MFC 自动化类



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // 功能区和控件条的 MFC 支持

extern CComModule _Module;
#include   <atlbase.h>
#include   <atlcom.h>
#include   <atlsafe.h>

#import "mscorlib.tlb" raw_interfaces_only rename("ReportEvent","ReportEvent_Sys")
typedef mscorlib::_TypePtr _TypePtr;
typedef mscorlib::Guid Guid;

#import "../GB28181.Platform/tlbs/5.0/DeviceConfig.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/AdaptorFactory.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/VideoAdaptorWrapper.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/ECConnectionManager.tlb" named_guids

using namespace DeviceConfig;
using namespace AdaptorFactory;
using namespace VideoAdaptorWrapper;
using namespace ECConnectionManager;

// Import web-service synchronize related libs.
#import "../GB28181.Platform/tlbs/5.0/HUSCommunicationMsgBase.tlb" named_guids no_namespace
#import "../GB28181.Platform/tlbs/5.0/SiteImage.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/SiteImageAdaptor.tlb" named_guids  rename("GetUserName","GetUserName_hus")
#import "../GB28181.Platform/tlbs/5.0/SynchronizeClient.tlb" named_guids

using namespace SiteImage;
using namespace SiteImageAdaptor;
using namespace SynchronizeClient;

#import "../GB28181.Platform/tlbs/5.0/DataManager.Search.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Client.Contract.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Search.Contract.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Client.tlb" named_guids


#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN           
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      

#define _AFX_ALL_WARNINGS

#include <afxwin.h>        
#include <afxext.h>       


#include <afxdisp.h>       



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>          
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     

extern CComModule _Module;
#include   <atlbase.h>
#include   <atlcom.h>
#include   <atlsafe.h>

#import "mscorlib.tlb" raw_interfaces_only rename("ReportEvent","ReportEvent_Sys")
typedef mscorlib::_TypePtr _TypePtr;
typedef mscorlib::Guid Guid;

#import "../GB28181.Platform/tlbs/5.0/DeviceConfig.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/AdaptorFactory.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/VideoAdaptorWrapper.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/ECConnectionManager.tlb" named_guids

using namespace DeviceConfig;
using namespace AdaptorFactory;
using namespace VideoAdaptorWrapper;
using namespace ECConnectionManager;

// Import web-service synchronize related libs.
#import "../GB28181.Platform/tlbs/5.0/HUSCommunicationMsgBase.tlb" named_guids no_namespace
#import "../GB28181.Platform/tlbs/5.0/SiteImage.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/SiteImageAdaptor.tlb" named_guids  rename("GetUserName","GetUserName_hus")
#import "../GB28181.Platform/tlbs/5.0/SynchronizeClient.tlb" named_guids

using namespace SiteImage;
using namespace SiteImageAdaptor;
using namespace SynchronizeClient;

#import "../GB28181.Platform/tlbs/5.0/DataManager.Search.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Client.Contract.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Search.Contract.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Client.tlb" named_guids
#import "../GB28181.Platform/tlbs/5.0/DataManager.Client.Contract.tlb"

using namespace HUS_DataManager_Search;
using namespace HUS_DataManager_Client;
using namespace HUS_DataManager_Client_Contract;
using namespace HUS_DataManager_Search_Contract;


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


using namespace HUS_DataManager_Search;
using namespace HUS_DataManager_Client;
using namespace HUS_DataManager_Client_Contract;
using namespace HUS_DataManager_Search_Contract;


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


