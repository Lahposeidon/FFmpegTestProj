
// VideoSampleDlg.h : header file
//

#pragma once

// CVideoSampleDlg dialog
class CVideoSampleDlg : public CDialogEx
{
// Construction
public:
	CVideoSampleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_VIDEOSAMPLE_DIALOG };

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
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void SetVideoFile(int nItemID);
	afx_msg void OnBnClickedBtnPath1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedBtnPath2();
	afx_msg void OnBnClickedBtnPath3();
};
