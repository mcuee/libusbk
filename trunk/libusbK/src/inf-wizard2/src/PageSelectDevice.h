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

#if !defined(__PAGE_SELECT_DEVICE_H)
#define __PAGE_SELECT_DEVICE_H

#include "ResizablePageEx.h"
#include "afxcmn.h"
#include "CGridColumnTraitText.h"
#include "GridDevListCtrl.h"
#include "afxwin.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPageSelectDevice dialog

class CPageSelectDevice : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPageSelectDevice)

	// Construction
public:
	CPageSelectDevice();
	~CPageSelectDevice();

	// Dialog Data
	//{{AFX_DATA(CPageSelectDevice)
	enum { IDD = IDD_PAGE_SELECT_DEVICE };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

protected:
	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageSelectDevice)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageSelectDevice)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CGridDevListCtrl m_DevList;

	afx_msg void OnLvnItemchangedDevList(NMHDR* pNMHDR, LRESULT* pResult);
	CButton m_BtnShowNew;
	CButton m_BtnShowAll;
	afx_msg void OnBnClickedShowAllDevices();
	afx_msg void OnBnClickedShowNewDevices();
	LRESULT OnWmDeviceChange(WPARAM wParam, LPARAM lParam);

private:
	CToolTipCtrl m_ToolTip;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__PAGE_SELECT_DEVICE_H)
