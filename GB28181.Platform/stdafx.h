// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
#endif

#include <afx.h>
#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include   <atlbase.h>
#include   <atlcom.h>
#include   <atlsafe.h>
//#include   <atlwin.h>
#include "Log/Log.h"
#include "Main/MainThread.h"

// Import .NET framework mscorlib. //rename_namespace("NameSpace_new"), rename("GetName","GetNameNEW")
#import "mscorlib.tlb" raw_interfaces_only rename("ReportEvent","ReportEvent_Sys")
//using namespace mscorlib;
typedef mscorlib::_TypePtr _TypePtr;
typedef mscorlib::Guid Guid;

#ifdef SDK_VERSION_5_0
// Import adapter related libs.
#import "./../Dependancy/Dlls/5.0/DeviceConfig.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/AdaptorFactory.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/VideoAdaptorWrapper.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/ECConnectionManager.tlb" named_guids

using namespace DeviceConfig;
using namespace AdaptorFactory;
using namespace VideoAdaptorWrapper;
using namespace ECConnectionManager;

// Import web-service synchronize related libs.
#import "./../Dependancy/Dlls/5.0/HUSCommunicationMsgBase.tlb" named_guids no_namespace
#import "./../Dependancy/Dlls/5.0/SiteImage.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/SiteImageAdaptor.tlb" named_guids  rename("GetUserName","GetUserName_hus")
#import "./../Dependancy/Dlls/5.0/SynchronizeClient.tlb" named_guids

using namespace SiteImage;
using namespace SiteImageAdaptor;
using namespace SynchronizeClient;

#import "./../Dependancy/Dlls/5.0/DataManager.Search.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/DataManager.Client.Contract.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/DataManager.Search.Contract.tlb" named_guids
#import "./../Dependancy/Dlls/5.0/DataManager.Client.tlb" named_guids

using namespace HUS_DataManager_Search;
using namespace HUS_DataManager_Client;
using namespace HUS_DataManager_Client_Contract;
using namespace HUS_DataManager_Search_Contract;

#endif

#ifdef SDK_VERSION_4_3
// Import adapter related libs.
#import "./../Dependancy/Dlls/4.3/DeviceConfig.tlb" named_guids
#import "./../Dependancy/Dlls/4.3/AdaptorFactory.tlb" named_guids
#import "./../Dependancy/Dlls/4.3/VideoAdaptorWrapper.tlb" named_guids
#import "./../Dependancy/Dlls/4.3/ECConnectionManager.tlb" named_guids
using namespace DeviceConfig;
using namespace AdaptorFactory;
using namespace VideoAdaptorWrapper;
using namespace ECConnectionManager;

// Import web-service synchronize related libs.
#import "./../Dependancy/Dlls/4.3/HUSCommunicationMsgBase.tlb" named_guids no_namespace
#import "./../Dependancy/Dlls/4.3/SiteImage.tlb" named_guids
#import "./../Dependancy/Dlls/4.3/SiteImageAdaptor.tlb" named_guids rename("GetUserName","GetUserName_hus")
#import "./../Dependancy/Dlls/4.3/SynchronizeClient.tlb" named_guids
#import "./../Dependancy/Dlls/4.3/GB28181.SiteImageAdaptorEx.tlb" named_guids
using namespace SiteImage;
using namespace SiteImageAdaptor;
using namespace SynchronizeClient;
using namespace HUS_GB28181_SiteImageAdaptorEx;

#endif

