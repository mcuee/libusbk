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

#if !defined(__PAGE_POWER_MANAGEMENT_H)
#define __PAGE_POWER_MANAGEMENT_H

#include "ResizablePageEx.h"
#include "afxcmn.h"
#include "afxwin.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPagePowerManagement dialog

class CPagePowerManagement : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPagePowerManagement)

	// Construction
public:
	CPagePowerManagement();
	~CPagePowerManagement();

	// Dialog Data
	//{{AFX_DATA(CPagePowerManagement)
	enum { IDD = IDD_PAGE_POWER_MANAGEMENT };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPagePowerManagement)
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPagePowerManagement)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CToolTipCtrl m_ToolTip;

	BOOL FillForm();
	BOOL SaveForm();

	void EnableWindowGroup(WORD nID, BOOL bEnabled);

public:
	CButton m_ChkDeviceIdleEnabled;
	CButton m_ChkDefaultIdleState;
	CComboBox m_CboIdleTimeout;
	CButton m_ChkUserSetDeviceIdleEnabled;
	CButton m_ChkDeviceIdleIgnoreWakeEnable;
	CButton m_ChkSystemWakeEnabled;
	afx_msg void OnBnClickedChkEnablePower();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__PAGE_POWER_MANAGEMENT_H)
