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

#if !defined(__PAGE_CONFIG_DEVICE_H)
#define __PAGE_CONFIG_DEVICE_H

#include "ResizablePageEx.h"
#include "afxcmn.h"
#include "CGridColumnTraitText.h"
#include "GridDevCfgListCtrl.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPageConfigDevice dialog

class CPageConfigDevice : public CResizablePageEx
{
	DECLARE_DYNCREATE(CPageConfigDevice)

	// Construction
public:
	CPageConfigDevice();
	~CPageConfigDevice();

	// Dialog Data
	//{{AFX_DATA(CPageConfigDevice)
	enum { IDD = IDD_PAGE_CONFIG_DEVICE };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageConfigDevice)
public:
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPageConfigDevice)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CGridDevCfgListCtrl m_DevCfgList;
	CImageList m_DevCfgListImages;

	afx_msg void OnNMDblclkDevList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickDevList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnKeydownDevList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnEndlabeleditDevList(NMHDR* pNMHDR, LRESULT* pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__PAGE_CONFIG_DEVICE_H)
