// CompletionNew.h : header file
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

#if !defined(AFX_COMPLETION_H__19E7B195_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
#define AFX_COMPLETION_H__19E7B195_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_

#include "ResizablePageEx.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPageFinished dialog

class CPageFinished : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPageFinished)

	CFont m_fontTitle;
	CFont m_fontBig;

	// Construction
public:
	CPageFinished();
	~CPageFinished();

	// Dialog Data
	//{{AFX_DATA(CPageFinished)
	enum { IDD = IDD_PAGE_FINISHED };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageFinished)
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	// Generated message map functions
	//{{AFX_MSG(CPageFinished)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMClickLinkOpenSession(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickLinkSaveSession(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnExplorePackageFolder();
	afx_msg void OnBnClickedBtnExploreBaseFolder();

private:
	CToolTipCtrl m_ToolTip;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPLETION_H__19E7B195_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
