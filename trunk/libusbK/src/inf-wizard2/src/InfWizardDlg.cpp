// SampWizP.cpp : implementation file
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

#include "stdafx.h"
#include "InfWizardApp.h"
#include "PageFinished.h"
#include "PageIntro.h"
#include "PageSelectDevice.h"
#include "InfWizardDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// InfWizardDlg

IMPLEMENT_DYNAMIC(InfWizardDlg, CResizableSheetEx)

InfWizardDlg::InfWizardDlg()
{
	VERIFY(m_bmpWatermark.LoadBitmap(IDB_WATERMARK));
	VERIFY(m_bmpHeader.LoadBitmap(IDB_BANNER));

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	Construct(IDS_INFWIZARD, NULL, 0, m_bmpWatermark, NULL, m_bmpHeader);

	// TODO: Add Pages
	AddPage(&m_PageIntro);
	AddPage(&m_PageSelectDevice);
	AddPage(&m_PageConfigDevice);
	AddPage(&m_PageInstall);
	AddPage(&m_PageFinished);
	AddPage(&m_PagePowerManagment);

	// use the right flag for InfWizardApp style
	//	m_psh.dwFlags |= bOldStyle ? PSH_IE4WIZARD97|PSH_STRETCHWATERMARK : PSH_IE5WIZARD97;
	m_psh.dwFlags |= PSH_IE5WIZARD97 | PSH_NOCONTEXTHELP;
	m_psh.dwFlags &= ~(PSH_HASHELP);
}

InfWizardDlg::~InfWizardDlg()
{
}


BEGIN_MESSAGE_MAP(InfWizardDlg, CResizableSheetEx)
	//{{AFX_MSG_MAP(InfWizardDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// InfWizardDlg message handlers

BOOL InfWizardDlg::OnInitDialog()
{
	CResizableSheetEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CRect rect;
	GetWindowRect(&rect);
	SetMinTrackSize(CSize(GetMinWidth(), rect.Height()));

	//EnableSaveRestore("InfWizardApp");

	CString sTitle;
	sTitle.LoadString(IDS_INFWIZARD);
	this->SetTitle(sTitle, 0);

	CDeviceNotifier::RegisterNotifier();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void InfWizardDlg::OnDestroy()
{
	CDeviceNotifier::UnRegisterNotifier();
	CResizableSheetEx::OnDestroy();
}

LRESULT InfWizardDlg::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DEVICECHANGE)
	{
		OnWmDeviceChange(wParam, lParam);
		return TRUE;
	}
	return CResizableSheetEx::WindowProc(uMsg, wParam, lParam);
}
