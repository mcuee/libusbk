// InfWizardDlg.h : header file
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

#if !defined(AFX_SAMPWIZP_H__99E7B18F_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
#define AFX_SAMPWIZP_H__99E7B18F_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_

#include "ResizableSheetEx.h"
#include "DeviceNotifier.h"

#include "PageIntro.h"
#include "PageSelectDevice.h"
#include "PageConfigDevice.h"
#include "PageInstall.h"
#include "PageFinished.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// InfWizardDlg

class InfWizardDlg : public CResizableSheetEx, public CDeviceNotifier
{
	DECLARE_DYNAMIC(InfWizardDlg)

	// Construction
public:
	InfWizardDlg();

	// Attributes
public:
	CPageIntro m_PageIntro;
	CPageSelectDevice m_PageSelectDevice;
	CPageConfigDevice m_PageConfigDevice;
	CPageInstall m_PageInstall;
	CPageFinished m_PageFinished;

	CBitmap m_bmpWatermark;
	CBitmap m_bmpHeader;

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(InfWizardDlg)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~InfWizardDlg();

protected:
	HICON m_hIcon;

	// Generated message map functions
protected:
	//{{AFX_MSG(InfWizardDlg)
	virtual BOOL OnInitDialog();
	virtual afx_msg void OnDestroy();
	virtual afx_msg LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAMPWIZP_H__99E7B18F_4A5C_11D1_BF2C_00C04FC99F83__INCLUDED_)
