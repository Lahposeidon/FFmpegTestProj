
// AudioSampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AudioSample.h"
#include "AudioSampleDlg.h"
#include "afxdialogex.h"
#include "../GlobalUtils/GlobalFunc.h"
#include <vector>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ENCODING_END	WM_USER + 100

static void av_log_callback( void *ptr, int level, const char *fmt, va_list argptr)
{
#if 1
	if (level > av_log_get_level())
		;
	else {
		char szBuffer[BUF_SIZE];
		ZeroMemory(szBuffer, BUF_SIZE);

		try {
			_locale_t locale = _create_locale(LC_ALL, "C");
			va_start(argptr, fmt);
			_vsnprintf_s_l(szBuffer, BUF_SIZE, _TRUNCATE, fmt, locale, argptr);
			va_end(argptr);

			CStringA DebugMsg = "";
			DebugMsg.Format("%d - %s", level, szBuffer);
			OutputDebugStringA(DebugMsg);		
		}catch(...){
			OutputDebugString(ShowErrorMessage(GetLastError()));
		}
	}
#endif
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


// CAudioSampleDlg dialog




CAudioSampleDlg::CAudioSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAudioSampleDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAudioSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAudioSampleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_MP3_1, &CAudioSampleDlg::OnBnClickedBtnMp31)
	ON_BN_CLICKED(IDC_BTN_MP3_2, &CAudioSampleDlg::OnBnClickedBtnMp32)
	ON_BN_CLICKED(IDOK, &CAudioSampleDlg::OnBnClickedOk)
	ON_MESSAGE(ENCODING_END, &CAudioSampleDlg::EncodingEnd)
	ON_BN_CLICKED(IDC_BTN_MP3_3, &CAudioSampleDlg::OnBnClickedBtnMp33)
	ON_BN_CLICKED(IDC_BTN_MP3_4, &CAudioSampleDlg::OnBnClickedBtnMp34)
	ON_BN_CLICKED(IDC_BTN_CONCAT, &CAudioSampleDlg::OnBnClickedBtnConcat)
END_MESSAGE_MAP()


// CAudioSampleDlg message handlers

BOOL CAudioSampleDlg::OnInitDialog()
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
	av_register_all();

	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback( av_log_callback);
	
	SetDlgItemText(IDC_EDIT_MP3_1, _T("C:\\Users\\wckjk\\Documents\\DoubleU\\sampledata\\140501_omnivue.mp3"));
	SetDlgItemText(IDC_EDIT_MP3_2, _T("C:\\Users\\wckjk\\Desktop\\WinSCP\\newsaod\\news\\2015\\02\\10\\201502102204327156.mp3"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAudioSampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CAudioSampleDlg::OnPaint()
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
HCURSOR CAudioSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAudioSampleDlg::OnBnClickedBtnMp31()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetAudioFile(IDC_EDIT_MP3_1);
}


void CAudioSampleDlg::OnBnClickedBtnMp32()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetAudioFile(IDC_EDIT_MP3_2);
}

void CAudioSampleDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	EnableButton(FALSE);
	AfxBeginThread(EncodingThreadFunc, this);
}

void CAudioSampleDlg::RunEncoding(CAVEncode &encode, char *filename, AVAudioFifo *fifo )
{
	CAVDecode decode(filename, encode.getCodecContext());
	if(decode.getCodecContext() == NULL)
		goto end;

	while(1)
	{
		const int output_frame_size = encode.getCodecContext()->frame_size;
		int finished                = 0;

		while (av_audio_fifo_size(fifo) < output_frame_size) {
			if (decode.decode_convert_and_store(fifo, &finished))
				goto end;

			if (finished)
				break;
		}

		while (av_audio_fifo_size(fifo) >= output_frame_size ||
			(finished && av_audio_fifo_size(fifo) > 0))
		{
			if (encode.load_encode_and_write(fifo) < 0)
				break;
		}

		if (finished) {
			int data_written;
			do {
				if (encode.encode_audio_frame(NULL, &data_written) < 0)
					break;
			} while (data_written);

			break;
		}
	}
end:
	fprintf(stderr, "skip %s.", filename);
}

UINT CAudioSampleDlg::EncodingThreadFunc( LPVOID pParam )
{
	CAudioSampleDlg* pParent = (CAudioSampleDlg*)pParam;

	char szSpecailPath[MAX_PATH] = {0};
	SHGetSpecialFolderPathA(NULL, szSpecailPath, CSIDL_DESKTOP, FALSE);

	CString strPath = _T("");
	strPath.Format(_T("%s\\result.mp3"), GlobalFunc::Char2String(szSpecailPath));

	vector<CString> vecFileList;

	CString strMP3 = _T("");

	for(int i = IDC_EDIT_MP3_1; i < IDC_EDIT_MP3_1 + 4; i++)
	{
		pParent->GetDlgItemText(i, strMP3);

		if(!strMP3.IsEmpty())
			vecFileList.emplace_back(strMP3);
	}

	DebugAndLogPrint(_T("Encode Start. Target file is %s\n"), strPath);
	AVAudioFifo *fifo = NULL;
	char *szPath = GlobalFunc::String2Char(strPath);
	CAVEncode encode(szPath, &fifo);
	if(fifo == NULL)
		goto cleanup;
	
	for each(auto const &iter in vecFileList)
	{
		LPSTR szFilepath = GlobalFunc::String2Char(iter);
		pParent->RunEncoding(encode, szFilepath, fifo);
		delete szFilepath;
	}

cleanup:
	encode.end();
	delete szPath;
	DebugAndLogPrint(_T("Encode End. Target file is %s\n"), strPath);
	pParent->PostMessage(ENCODING_END);
	return 0;
}

LRESULT CAudioSampleDlg::EncodingEnd( WPARAM wParam, LPARAM lParam )
{
	EnableButton();
	return 0;
}

BOOL CAudioSampleDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if(pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN || pMsg->wParam == VK_SPACE) 
	{
		// ESCAPE,Return,Space 버튼 무시, 
		return 1;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CAudioSampleDlg::OnBnClickedBtnMp33()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetAudioFile(IDC_EDIT_MP3_3);
}


void CAudioSampleDlg::OnBnClickedBtnMp34()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetAudioFile(IDC_EDIT_MP3_4);
}

void CAudioSampleDlg::SetAudioFile(int nItemID)
{
	CString FileName, data;
	TCHAR Filter[] = _T("MP3 File(*.mp3) |*.mp3|모든파일(*.*)|*.*|");
	CFileDialog dlg(TRUE, _T("mp3 file(*.mp3)"), _T("*.mp3"), OFN_HIDEREADONLY | 
		OFN_FILEMUSTEXIST, Filter, NULL);
	dlg.m_ofn.lStructSize = sizeof(OPENFILENAME) + 12;
	if(dlg.DoModal() == IDOK)
	{
		FileName = dlg.GetPathName();
		SetDlgItemText(nItemID, FileName);
		char *szFileName = GlobalFunc::String2Char(FileName);
		av_log(NULL, AV_LOG_INFO, "File(%s)", szFileName);
		delete [] szFileName;
	}
}


void CAudioSampleDlg::OnBnClickedBtnConcat()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	EnableButton(FALSE);
	AfxBeginThread(ConcatThreadFunc, this);
}

void CAudioSampleDlg::EnableButton(BOOL bEnable)
{
	GetDlgItem(IDC_BTN_MP3_1)->EnableWindow(bEnable);
	GetDlgItem(IDC_BTN_MP3_2)->EnableWindow(bEnable);
	GetDlgItem(IDC_BTN_MP3_3)->EnableWindow(bEnable);
	GetDlgItem(IDC_BTN_MP3_4)->EnableWindow(bEnable);
	GetDlgItem(IDOK)->EnableWindow(bEnable);
	GetDlgItem(IDOK)->EnableWindow(bEnable);
	GetDlgItem(IDCANCEL)->EnableWindow(bEnable);
	GetDlgItem(IDC_BTN_CONCAT)->EnableWindow(bEnable);
}

UINT CAudioSampleDlg::ConcatThreadFunc( LPVOID pParam )
{
	CAudioSampleDlg* pParent = (CAudioSampleDlg*)pParam;

	TCHAR szSpecailPath[MAX_PATH] = {0};
	SHGetSpecialFolderPath(NULL, szSpecailPath, CSIDL_DESKTOP, FALSE);

	CString strPath = _T("");
	strPath.Format(_T("%s\\result.mp3"), szSpecailPath);

	DebugAndLogPrint(_T("Merge Start. Target file is %s\n"), szSpecailPath);

	vector<CString> vecFileList;
	CString strFiles = _T("");

	CString strMP3 = _T("");

	for(int i = IDC_EDIT_MP3_1; i < IDC_EDIT_MP3_1 + 4; i++)
	{
		pParent->GetDlgItemText(i, strMP3);

		if(!strMP3.IsEmpty())
		{
			vecFileList.emplace_back(strMP3);
			strFiles += strMP3 + _T("|");
		}
	}

	CW2A szPath(strPath);

	int err = 0;

	//CW2A szFile((*vecFileList.begin()));
	strFiles.Format(_T("concat:%s"), strFiles.Mid(0, strFiles.GetLength()-1));
	CW2A szFile(strFiles);
	CAVCopy concat(szFile, szPath, err);
#if 0
	concat.copy();
#else
	for each(auto const &iter in vecFileList)
	{
		CW2A szFilepath(iter);		
		concat.merge(szFilepath);
	}
#endif	
	concat.end();

	DebugAndLogPrint(_T("Concat End"));
	pParent->PostMessage(ENCODING_END);
	return 0;
}