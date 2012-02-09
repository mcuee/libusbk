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
	DDX_Control(pDX, IDC_PIC_TEXT_BACK, m_PicTextBack);
	DDX_Control(pDX, IDC_LINK_EXPLORE_PACKAGE_FOLDER, m_LinkExplore);
	DDX_Control(pDX, IDC_LINK_INSTALL_DRIVER_NOW, m_LinkInstall);
}


BEGIN_MESSAGE_MAP(CPageFinished, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageFinished)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
	ON_NOTIFY(NM_CLICK, IDC_LINK_SAVE_SESSION, OnNMClickLinkSaveSession)
	ON_NOTIFY(NM_CLICK, IDC_LINK_EXPLORE_PACKAGE_FOLDER, &CPageFinished::OnNMClickLinkExplorePackageFolder)
	ON_NOTIFY(NM_CLICK, IDC_LINK_INSTALL_DRIVER_NOW, &CPageFinished::OnNMClickLinkInstallDriverNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageFinished message handlers

BOOL CPageFinished::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);
	pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH);

	CWnd* pWndCancel = pSheet->GetDlgItem(IDCANCEL);
	ASSERT_KINDOF(CWnd, pWndCancel);
	pWndCancel->ShowWindow(SW_HIDE);

	if (g_App->Wdi.Session()->m_PackageStatus & CLibWdiSession::PACKAGE_TYPE_CLIENT_INSTALLER)
	{
		m_LinkInstall.EnableWindow(TRUE);
		m_LinkInstall.SetItemState(0, LIS_ENABLED);

		m_LinkExplore.EnableWindow(TRUE);
		m_LinkExplore.SetItemState(0, LIS_ENABLED);

	}
	else if (g_App->Wdi.Session()->m_PackageStatus & CLibWdiSession::PACKAGE_TYPE_LEGACY)
	{
		m_LinkInstall.SetItemState(0, 0);
		m_LinkInstall.EnableWindow(FALSE);

		m_LinkExplore.SetItemState(0, LIS_ENABLED);
		m_LinkExplore.EnableWindow(TRUE);

	}
	else
	{
		m_LinkInstall.SetItemState(0, 0);
		m_LinkInstall.EnableWindow(FALSE);

		m_LinkExplore.SetItemState(0, 0);
		m_LinkExplore.EnableWindow(FALSE);
	}

	return CResizablePageEx::OnSetActive();
}

BOOL CPageFinished::OnKillActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	CWnd* pWndCancel = pSheet->GetDlgItem(IDCANCEL);
	ASSERT_KINDOF(CWnd, pWndCancel);
	pWndCancel->ShowWindow(SW_SHOW);

	return CResizablePageEx::OnKillActive();
}

BOOL CPageFinished::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();

	AddAnchor(IDC_BIGBOLDTITLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_PIC_TEXT_BACK, TOP_LEFT);
	AddAnchor(IDC_LBL_TEXT_BACK, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LBL_TEXT_FINISH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LINK_EXPLORE_PACKAGE_FOLDER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LINK_INSTALL_DRIVER_NOW, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LINK_SAVE_SESSION, BOTTOM_RIGHT);

	LOGFONT lfTitle, lfBig;
	GetDlgItem(IDC_BIGBOLDTITLE)->GetFont()->GetLogFont(&lfTitle);
	_tcscpy(lfTitle.lfFaceName, _T("Tahoma"));
	memcpy(&lfBig, &lfTitle, sizeof(lfBig));

	lfTitle.lfWeight	= FW_BOLD;
	lfTitle.lfHeight	*= 2;
	lfTitle.lfWidth		= 0;
	lfTitle.lfQuality	= CLEARTYPE_QUALITY;
	m_FontTitle.CreateFontIndirect(&lfTitle);

	HICON hImgInfo = (HICON)LoadImage(g_App->m_hInstance, MAKEINTRESOURCE(IDI_ICON_INFORMATION), IMAGE_ICON, 16, 16, LR_SHARED);
	m_PicTextBack.SetIcon(hImgInfo);

	lfBig.lfWeight	= FW_BOLD;
	lfBig.lfHeight	= 80;
	lfBig.lfWidth	= 0;
	lfBig.lfQuality	= CLEARTYPE_QUALITY;
	m_FontBig.CreatePointFontIndirect(&lfBig);

	GetDlgItem(IDC_BIGBOLDTITLE)->SetFont(&m_FontTitle);

	if(!m_ToolTip.Create(this))
	{
		TRACE(_T("Unable To create ToolTip\n"));
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_EXPLORE_PACKAGE_FOLDER, IDS_TIP_EXPLORE_PACKAGE_FOLDER);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_SAVE_SESSION, IDS_TIP_SAVE_SESSION);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(250);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPageFinished::OnNMClickLinkSaveSession(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	g_App->Wdi.SaveSession(GetParent());
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

void CPageFinished::OnNMClickLinkExplorePackageFolder(NMHDR* pNMHDR, LRESULT* pResult)
{
	CString packagePath = g_App->Wdi.Session()->GetPackageBaseDir();

	CString packageName = g_App->Wdi.Session()->GetPackageName();

	PathAppend(packagePath.GetBufferSetLength(4096), packageName.GetBuffer(0));
	packagePath.ReleaseBuffer();

	if (!PathIsDirectory(packagePath.GetBuffer(4096)))
		packagePath =  g_App->Wdi.Session()->GetPackageBaseDir();

	HINSTANCE hInst = ShellExecute(NULL, _T("explore"), packagePath.GetBuffer(4096), NULL, NULL, SW_SHOWNORMAL);
	if ((DWORD)hInst > 32)
	{
		*pResult = 0;

		CPropertySheet* pSheet = (CPropertySheet*)GetParent();
		ASSERT_KINDOF(CPropertySheet, pSheet);
		pSheet->EndDialog(ID_WIZFINISH);
		return;
	}
	*pResult = 0;
}


void CPageFinished::OnNMClickLinkInstallDriverNow(NMHDR* pNMHDR, LRESULT* pResult)
{
	CString exeName = _T("InstallDriver.exe");
	CString packagePath = g_App->Wdi.Session()->GetPackageBaseDir();
	CString packageName = g_App->Wdi.Session()->GetPackageName();

	PathAppend(packagePath.GetBufferSetLength(4096), packageName.GetBuffer(0));
	packagePath.ReleaseBuffer();

	if (PathIsDirectory(packagePath.GetBuffer(4096)))
	{
		PathAppend(packagePath.GetBufferSetLength(4096), exeName.GetBuffer(0));
		packagePath.ReleaseBuffer();
		if (PathFileExists(packagePath))
		{
			HINSTANCE hInst = ShellExecute(NULL, _T("open"), packagePath.GetBuffer(0), NULL, NULL, SW_SHOWNORMAL);
			if ((DWORD)hInst > 32)
			{
				*pResult = 0;

				CPropertySheet* pSheet = (CPropertySheet*)GetParent();
				ASSERT_KINDOF(CPropertySheet, pSheet);
				pSheet->EndDialog(ID_WIZFINISH);
				return;
			}
		}
	}
	*pResult = 0;
}
