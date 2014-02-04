
// LauncherDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CLauncherDlg dialog
class CLauncherDlg : public CDialogEx
{
// Construction
public:
	CLauncherDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_LAUNCHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBnClickedBrowse();
	CComboBox ExeFile;
	afx_msg void OnBnClickedRun();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTrayRun();
	afx_msg void OnTrayExit();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
};
