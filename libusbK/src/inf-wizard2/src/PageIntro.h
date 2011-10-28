// PageIntro.h : header file
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

#if !defined(AFX_INTROPAGE_H__19E7B192_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
#define AFX_INTROPAGE_H__19E7B192_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_

#include "ResizablePageEx.h"
#include "afxcmn.h"
#include "CGridColumnTraitText.h"
#include "GridDrvListCtrl.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPageIntro dialog

class CPageIntro : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPageIntro)

private:
	CFont	m_FontBig;
	LOGFONT m_LogFontBig;

	// Construction
public:
	CPageIntro();
	~CPageIntro();

	BOOL SelectSessionDriverType(void);
	// Dialog Data
	//{{AFX_DATA(CPageIntro)
	enum { IDD = IDD_PAGE_INTRO };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageIntro)
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPageIntro)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CGridDrvListCtrl m_DriverList;
	CImageList m_DriverListImages;

	afx_msg void OnLvnItemchangedDriverList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickLinkOpenSession(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickLinkSaveSession(NMHDR* pNMHDR, LRESULT* pResult);
private:
	CToolTipCtrl m_ToolTip;

public:
	afx_msg void OnNMClickLinkSelectDriver(NMHDR* pNMHDR, LRESULT* pResult);
	CLinkCtrl m_LnkSelectDriver;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTROPAGE_H__19E7B192_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
