
// VideoSampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VideoSample.h"
#include "VideoSampleDlg.h"
#include "afxdialogex.h"
#include <vector>
#include "../GlobalUtils/GlobalFunc.h"
#include "AVDecode.h"
#include "AVEncode.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static void av_log_callback( void *ptr, int level, const char *fmt, va_list vl)
{
	if (level > av_log_get_level())
		;
	else {
		char szBuffer[BUF_SIZE];
		ZeroMemory(szBuffer, BUF_SIZE);

		try {
			_locale_t locale = _create_locale(LC_ALL, "C");
			va_start(vl, fmt);
			_vsnprintf_s_l(szBuffer, BUF_SIZE, _TRUNCATE, fmt, locale, vl);
			va_end(vl);

			CStringA DebugMsg = "";
			DebugMsg.Format("%d - %s", level, szBuffer);
			OutputDebugStringA(DebugMsg);
		}catch(...){
			OutputDebugString(ShowErrorMessage(GetLastError()));
		}
	}
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CVideoSampleDlg dialog




CVideoSampleDlg::CVideoSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVideoSampleDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVideoSampleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_PATH1, &CVideoSampleDlg::OnBnClickedBtnPath1)
	ON_BN_CLICKED(IDOK, &CVideoSampleDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_PATH2, &CVideoSampleDlg::OnBnClickedBtnPath2)
	ON_BN_CLICKED(IDC_BTN_PATH3, &CVideoSampleDlg::OnBnClickedBtnPath3)
END_MESSAGE_MAP()


// CVideoSampleDlg message handlers

BOOL CVideoSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	avcodec_register_all();
	av_register_all();

	av_log_set_level(AV_LOG_VERBOSE);
	av_log_set_callback( av_log_callback);

	SetDlgItemText(IDC_MEDIA_FILE1, _T("C:\\ShadowLive\\iMBC\\mp4ad\\Double\\20150414\\150414_dettolinfo_30s.mp4"));
	//SetDlgItemText(IDC_MEDIA_FILE2, _T("C:\\Users\\wckjk\\Documents\\DoubleU\\sampledata\\20140709_dalmin.mp4"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVideoSampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVideoSampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVideoSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CVideoSampleDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if(pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN || pMsg->wParam == VK_SPACE) 
	{
		// ESCAPE,Return,Space 버튼 무시, 
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CVideoSampleDlg::SetVideoFile(int nItemID)
{
	CString FileName, data;
	TCHAR Filter[] = _T("MP4 File(*.mp4) |*.mp4|모든파일(*.*)|*.*|");
	CFileDialog dlg(TRUE, _T("mp4 file(*.mp4)"), _T("*.mp4"), OFN_HIDEREADONLY | 
		OFN_FILEMUSTEXIST, Filter, NULL);

	dlg.m_ofn.lStructSize = sizeof(OPENFILENAME);
	if(dlg.DoModal() == IDOK)
	{
		FileName = dlg.GetPathName();
		SetDlgItemText(nItemID, FileName);
		av_log(NULL, AV_LOG_INFO, "File(%s)", FileName);
	}
}

void CVideoSampleDlg::OnBnClickedBtnPath1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetVideoFile(IDC_MEDIA_FILE1);
}


void CVideoSampleDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	char szSpecailPath[MAX_PATH] = {0};
	SHGetSpecialFolderPathA(NULL, szSpecailPath, CSIDL_DESKTOP, FALSE);

	CStringA strResultPath = "";
	strResultPath.Format("%s\\result.mp4", szSpecailPath);

	DebugAndLogPrint(_T("Encode Start.\n\t\tTarget file is %s"), GlobalFunc::Char2String(strResultPath.GetBuffer()));
	vector<CString> vecFileList;

	CString strTempMediaPath = _T("");

	for(int i = IDC_MEDIA_FILE1; i < IDC_MEDIA_FILE1 + 3; i++)
	{
		GetDlgItemText(i, strTempMediaPath);

		if(!strTempMediaPath.IsEmpty())
			vecFileList.emplace_back(strTempMediaPath);
	}

	CAVEncode enc;
	enc.OpenOutputFile(strResultPath);

	for each(auto const &iter in vecFileList)
	{
		int ret = 0;
		CW2A szFilePath(iter);
		CAVDecode dec;
		if((ret = dec.OpenInputFile(szFilePath)) < 0)
			continue;

		int got_frame = 0;
		AVPacket pkt;

		av_init_packet(&pkt);
		pkt.data=NULL;
		pkt.size = 0;

		while(av_read_frame(dec.getFormatContext(), &pkt) >= 0)
			dec.DecodePacket(&got_frame, &pkt, 0);

			/* flush cached frames */
		pkt.data = NULL;
		pkt.size = 0;
		do {
			dec.DecodePacket(&got_frame, &pkt, 1);
		} while (got_frame);
	}



_gEnd:
	AfxMessageBox(_T("End"));
}

void CVideoSampleDlg::OnBnClickedBtnPath2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetVideoFile(IDC_MEDIA_FILE2);
}

void CVideoSampleDlg::OnBnClickedBtnPath3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetVideoFile(IDC_MEDIA_FILE3);
}