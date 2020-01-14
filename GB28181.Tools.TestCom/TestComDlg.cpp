
// TestComDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestCom.h"
#include "TestComDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestComDlg 对话框



CTestComDlg::CTestComDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TESTCOM_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	CoInitialize(NULL);
}

void CTestComDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON, m_btnDVRVideoRecordSearch);
	DDX_Control(pDX, IDC_BUTTON2, m_btnHUSVideoRecordSearch);
}

BEGIN_MESSAGE_MAP(CTestComDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON, &CTestComDlg::OnBnClickedButton)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestComDlg::OnClickedButton2)
END_MESSAGE_MAP()


// CTestComDlg 消息处理程序

BOOL CTestComDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//ShowWindow(SW_MAXIMIZE);

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestComDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTestComDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTestComDlg::OnBnClickedButton()
{
	//HUS_DataManager_Search::_DVRVideoRecordSearchContext hus_record_search_contextptr

	CComPtr<IDVRVideoRecordSearchContext> dvr_record_search_context = NULL;
	HRESULT hr = dvr_record_search_context.CoCreateInstance(HUS_DataManager_Search::CLSID_DVRVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && dvr_record_search_context != NULL);
	if (FAILED(hr))
	{
		MessageBox(_T("DVRVideoRecordSearch注册失败!"));
	}
	else
	{
		MessageBox(_T("DVRVideoRecordSearch注册成功!"));
	}
}


void CTestComDlg::OnClickedButton2()
{
	CComPtr<IHUSVideoRecordSearchContext> hus_record_search_context = NULL;
	HRESULT hr = hus_record_search_context.CoCreateInstance(HUS_DataManager_Search::CLSID_HUSVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_search_context != NULL);
	if (FAILED(hr))
	{
		MessageBox(_T("HUSVideoRecordSearch注册失败!"));
	}
	else
	{
		MessageBox(_T("HUSVideoRecordSearch注册成功!"));
	}
}
