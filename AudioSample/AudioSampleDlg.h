
// AudioSampleDlg.h : header file
//

#include "AVDecode.h"
#include "AVEncode.h"
#include "AVCopy.h"

// CAudioSampleDlg dialog
class CAudioSampleDlg : public CDialogEx
{
// Construction
public:
	CAudioSampleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_AudioSample_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnMp31();
	afx_msg void OnBnClickedBtnMp32();
	afx_msg void OnBnClickedOk();

	void EnableButton(BOOL bEnable=1);
	void RunEncoding(CAVEncode &encode, char *filename, AVAudioFifo *fifo );
	static UINT EncodingThreadFunc(LPVOID pParam);
	LRESULT EncodingEnd(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtnMp33();
	afx_msg void OnBnClickedBtnMp34();
	void SetAudioFile(int nItemID);

	afx_msg void OnBnClickedBtnConcat();
	static UINT ConcatThreadFunc( LPVOID pParam );
};
