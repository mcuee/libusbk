// Interior1.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#if !defined(__PAGE_INSTALL_H)
#define __PAGE_INSTALL_H

#include "ResizablePageEx.h"

#include "afxcmn.h"
#include "CGridColumnTraitText.h"
#include "GridDevCfgListCtrl.h"
#include "afxwin.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CPageInstall dialog

class CPageInstall : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPageInstall)

	// Construction
public:
	CPageInstall();
	~CPageInstall();

	// Dialog Data
	//{{AFX_DATA(CPageInstall)
	enum { IDD = IDD_PAGE_INSTALL };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageInstall)
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();

private:
	CHAR m_chLogBuffer[1024];
	CString m_LastLogMessage;

	CImageList m_ImageList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	int TwipsPerPixelY();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	// Generated message map functions
	//{{AFX_MSG(CPageInstall)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void EnableWindowGroup(WORD nID, BOOL bEnabled = 1);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CToolTipCtrl m_ToolTip;

	void SetStatus(DWORD pfaAlignment, CString statusText, LPCTSTR fontName, LONG fontSize, BOOL fontBold, COLORREF textColor);
	void SetStatusFont(BOOL isBold, COLORREF textColor, LPCTSTR pszFontName, LONG fontSize);
	void SetStatusFormat(DWORD pfaAlignment, BOOL bulletMode);
	void AppendStatus(CString statusText);
	void WritePackageStatus(void);
	void WriteStatusError(LPCTSTR szCaption, LPCTSTR szText);
	BOOL FinalizePrepareDriver(
	    PWDI_DEVICE_INFO DeviceInfo,
	    LPCSTR InfPath,
	    LPCSTR InfName,
	    PWDI_OPTIONS_PREPARE_DRIVER Options);


public:
	CImageList m_InstallImages;
	afx_msg void OnBnClickedBtnClientInstaller();
	afx_msg void OnBnClickedBtnLegacyPackage();
	afx_msg void OnBnClickedBtnInstallOnly();
	CRichEditCtrl m_TxtStatus;
	CButtonST m_BtnSaveLocation;
	afx_msg void OnBnClickedBtnSaveBaseFolder();
	CEdit m_TxtSaveBaseFolder;
	CEdit m_TxtSaveName;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__PAGE_INSTALL_H)
