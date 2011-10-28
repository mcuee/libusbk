// CompleteNew.cpp : implementation file
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
#include "Shlwapi.h"
#include "InfWizardApp.h"
#include "PageFinished.h"
#include "InfWizardDisplay.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPageFinished property page

IMPLEMENT_DYNCREATE(CPageFinished, CResizablePageEx)
CPageFinished::CPageFinished() : CResizablePageEx(CPageFinished::IDD, IDS_INFWIZARD)
{
	//{{AFX_DATA_INIT(CPageFinished)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_psp.dwFlags |= PSP_HIDEHEADER;
	m_psp.dwFlags &= ~(PSP_HASHELP);

}

CPageFinished::~CPageFinished()
{
}

void CPageFinished::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageFinished)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageFinished, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageFinished)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
	ON_NOTIFY(NM_CLICK, IDC_LINK_OPEN_SESSION, OnNMClickLinkOpenSession)
	ON_NOTIFY(NM_CLICK, IDC_LINK_SAVE_SESSION, OnNMClickLinkSaveSession)
	ON_BN_CLICKED(IDC_BTN_EXPLORE_PACKAGE_FOLDER, OnBnClickedBtnExplorePackageFolder)
	ON_BN_CLICKED(IDC_BTN_EXPLORE_BASE_FOLDER, OnBnClickedBtnExploreBaseFolder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageFinished message handlers

BOOL CPageFinished::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);
	pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH);
	return CResizablePageEx::OnSetActive();
}

BOOL CPageFinished::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();

	AddAnchor(IDC_LBL_FINISHED_CAPTION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BIGBOLDTITLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_PIC_FINISHED_TEXT, TOP_LEFT);
	AddAnchor(IDC_LBL_FINISHED_TEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GRP_ADDITIONAL_TASKS, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_BTN_EXPLORE_PACKAGE_FOLDER, TOP_LEFT);
	AddAnchor(IDC_BTN_EXPLORE_BASE_FOLDER, TOP_LEFT);
	AddAnchor(IDC_LBL_EXPLORE_PACKAGE_FOLDER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LBL_EXPLORE_BASE_FOLDER, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_LINK_OPEN_SESSION, BOTTOM_LEFT);
	AddAnchor(IDC_LINK_SAVE_SESSION, BOTTOM_LEFT);

	LOGFONT lfTitle, lfBig;
	GetDlgItem(IDC_LBL_FINISHED_CAPTION)->GetFont()->GetLogFont(&lfTitle);
	GetDlgItem(IDC_LBL_FINISHED_CAPTION)->GetFont()->GetLogFont(&lfBig);

	lfTitle.lfWeight = FW_BOLD;
	lfTitle.lfHeight *= 2;
	m_FontTitle.CreateFontIndirect(&lfTitle);

	lfBig.lfWeight = FW_BOLD;
	m_FontBig.CreateFontIndirect(&lfBig);

	GetDlgItem(IDC_BIGBOLDTITLE)->SetFont(&m_FontTitle);
	GetDlgItem(IDC_LBL_FINISHED_CAPTION)->SetFont(&m_FontBig);

	HICON hImgInfo = (HICON)LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_ICON_EXPLORER), IMAGE_ICON, 16, 16, LR_SHARED);
	((CButton*)GetDlgItem(IDC_BTN_EXPLORE_PACKAGE_FOLDER))->SetIcon(hImgInfo);
	((CButton*)GetDlgItem(IDC_BTN_EXPLORE_BASE_FOLDER))->SetIcon(hImgInfo);

	if(!m_ToolTip.Create(this))
	{
		TRACE(_T("Unable To create ToolTip\n"));
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_EXPLORE_PACKAGE_FOLDER, IDS_TIP_EXPLORE_PACKAGE_FOLDER);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_EXPLORE_BASE_FOLDER, IDS_TIP_EXPLORE_BASE_FOLDER);

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_OPEN_SESSION, IDS_TIP_OPEN_SESSION);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_SAVE_SESSION, IDS_TIP_SAVE_SESSION);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(250);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPageFinished::OnNMClickLinkOpenSession(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	if (g_App->Wdi.OpenSession(GetParent()))
	{
		pSheet->SetActivePage(3);
		return;
	}
}

void CPageFinished::OnNMClickLinkSaveSession(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	g_App->Wdi.SaveSession(GetParent());
}

void CPageFinished::OnBnClickedBtnExplorePackageFolder()
{
	CString packagePath = g_App->Wdi.Session()->GetPackageBaseDir();
	CString packageName = g_App->Wdi.Session()->GetPackageName();

	PathAppend(packagePath.GetBufferSetLength(4096), packageName.GetBuffer(0));
	packagePath.ReleaseBuffer();

	if (!PathIsDirectory(packagePath.GetBuffer(4096)))
		packagePath =  g_App->Wdi.Session()->GetPackageBaseDir();

	ShellExecute(NULL, _T("explore"), packagePath.GetBuffer(4096), NULL, NULL, SW_SHOW);
}

void CPageFinished::OnBnClickedBtnExploreBaseFolder()
{
	CString packagePath = g_App->Wdi.Session()->GetPackageBaseDir();
	ShellExecute(NULL, _T("explore"), packagePath.GetBuffer(0), NULL, NULL, SW_SHOW);
}

BOOL CPageFinished::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CResizablePageEx::PreTranslateMessage(pMsg);
}

BOOL CPageFinished::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CInfWizardDisplay::HandleToolTipNotify(id, pNMHDR, pResult);
}
