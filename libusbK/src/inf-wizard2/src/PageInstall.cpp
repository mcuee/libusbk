// PageInstall.cpp : implementation file
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
#include "PageInstall.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPageInstall property page

IMPLEMENT_DYNCREATE(CPageInstall, CResizablePageEx)

CPageInstall::CPageInstall() : CResizablePageEx(CPageInstall::IDD, IDS_INFWIZARD, IDS_PAGE_INSTALL_TITLE, IDS_PAGE_INSTALL_SUBTITLE)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);

	//{{AFX_DATA_INIT(CPageInstall)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	//m_strHeaderTitle = "Setup Page (Test)";
	//m_psp.dwFlags |= PSP_HIDEHEADER;
}

CPageInstall::~CPageInstall()
{
}

void CPageInstall::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageInstall)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BTN_SAVE_PACKAGE, m_BtnSavePackage);
	DDX_Control(pDX, IDC_TXT_STATUS, m_TxtStatus);
	DDX_Control(pDX, IDC_BTN_SAVE_BASE_FOLDER, m_BtnSaveLocation);
	DDX_Control(pDX, IDC_TXT_SAVE_BASE_FOLDER, m_TxtSaveBaseFolder);
	DDX_Control(pDX, IDC_TXT_SAVE_NAME, m_TxtSaveName);
}


BEGIN_MESSAGE_MAP(CPageInstall, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageInstall)
	ON_WM_SIZE()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)

	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_SAVE_PACKAGE, OnBnClickedBtnSavePackage)
	ON_BN_CLICKED(IDC_BTN_INSTALL_PACKAGE, OnBnClickedBtnInstallPackage)
	ON_BN_CLICKED(IDC_BTN_SAVE_AND_INSTALL_PACKAGE, OnBnClickedBtnSaveAndInstallPackage)
	ON_BN_CLICKED(IDC_BTN_SAVE_BASE_FOLDER, OnBnClickedBtnSaveBaseFolder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageInstall message handlers

BOOL CPageInstall::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	if (!m_TxtSaveBaseFolder.GetWindowTextLength())
		m_TxtSaveBaseFolder.SetWindowText(g_App->Wdi.Session()->GetPackageBaseDir());

	m_TxtSaveName.SetWindowText(g_App->Wdi.Session()->GetPackageName());
	m_TxtStatus.SetSel(0, 0);

	return CResizablePageEx::OnSetActive();
}

LRESULT CPageInstall::OnWizardNext()
{
	BOOL bSave = ((CButton*)GetDlgItem(IDC_BTN_SAVE_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bInstallOnly = ((CButton*)GetDlgItem(IDC_BTN_INSTALL_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bSaveInstall = ((CButton*)GetDlgItem(IDC_BTN_SAVE_AND_INSTALL_PACKAGE))->GetCheck() == BST_CHECKED;
	CString fmtRtf;
	CStringA infPathA;
	CStringA infNameA;

	int errorCode = ERROR_SUCCESS;

	if (!bSave && !bInstallOnly && !bSaveInstall)
		return CResizablePageEx::OnWizardNext();

	CLibWdiDynamicAPI& API = g_App->Wdi;

	m_TxtStatus.SetWindowText(_T(""));
	m_TxtStatus.SetSel(0, -1);
	SetStatusFont(TRUE, RGB(60, 133, 201), _T("Tahoma"), TwipsPerPixelY() * 9);
	SetStatusFormat(PFA_LEFT, TRUE);

	fmtRtf.Format(_T("%s..\n"), CInfWizardDisplay::GetTipString(IDS_STATUS_START_PACKAGER)->GetBuffer(0));
	this->AppendStatus(fmtRtf);

	WDI_DEVICE_INFO deviceInfo;
	g_App->Wdi.Session()->CopyTo(&deviceInfo);
	g_App->Wdi.Session()->RefreshSession();

	wdi_options_prepare_driver prepareOptions;
	memset(&prepareOptions, 0, sizeof(prepareOptions));
	prepareOptions.driver_type = g_App->Wdi.Session()->GetDriverType();
	if (g_App->Wdi.Session()->m_VendorName.GetLength() > 0)
		prepareOptions.vendor_name = g_App->Wdi.Session()->chVendorName;
	prepareOptions.device_guid = g_App->Wdi.Session()->chDeviceGuid;

	wdi_options_install_driver installOptions;
	memset(&installOptions, 0, sizeof(installOptions));
	installOptions.hWnd = GetSafeHwnd();


	CString infPath, infName;
	m_TxtSaveName.GetWindowText(infName);
	m_TxtSaveBaseFolder.GetWindowText(infPath);

	if (bInstallOnly)
	{
		// Using a temp directory.
		CString tempPath;
		GetTempPath(256, tempPath.GetBufferSetLength(4096));
		tempPath.ReleaseBuffer();

		PathAppend(tempPath.GetBufferSetLength(4096), _T("InfWizard_Driver"));
		tempPath.ReleaseBuffer();
		infPath = tempPath;
		CreateDirectory(tempPath, NULL);

		CString infFullPath;
		GetTempFileName(tempPath, _T("Drv"), 0, infFullPath.GetBufferSetLength(4096));
		infFullPath.ReleaseBuffer();
		infName = PathFindFileName(infFullPath);
		PathRemoveExtension(infName.GetBuffer(0));
		infName.ReleaseBuffer();
	}
	else
	{
		PathAppend(infPath.GetBufferSetLength(4096), infName);
		infPath.ReleaseBuffer();
	}
	if (!PathIsDirectory(infPath))
	{
		if (SHCreateDirectoryEx(this->GetSafeHwnd(), infPath, NULL) != ERROR_SUCCESS)
		{
			errorCode = -ERROR_BAD_PATHNAME;
			CString* pTxtInvalidPath = CInfWizardDisplay::GetTipString(IDS_STATUS_INVALID_PATH);
			WriteStatusError(pTxtInvalidPath->GetBuffer(0), infPath);
			goto Done;
		}
	}


	CString* pTxtPackageFolder = CInfWizardDisplay::GetTipString(IDS_PACKAGE_DIR);
	fmtRtf.Format(_T("%s %s..\n"), pTxtPackageFolder->GetBuffer(0), infPath);
	this->AppendStatus(fmtRtf);

	CString* pTxtPackageName = CInfWizardDisplay::GetTipString(IDS_PACKAGE_NAME);
	fmtRtf.Format(_T("%s %s..\n"), pTxtPackageName->GetBuffer(0), infName);
	this->AppendStatus(fmtRtf);

	infName += _T(".inf");
	infPathA = infPath;
	infNameA = infName;

	fmtRtf.Format(_T("%s..\n"), CInfWizardDisplay::GetTipString(IDS_STATUS_PREPARE_DRIVER)->GetBuffer(0));
	this->AppendStatus(fmtRtf);

	errorCode = g_App->Wdi.PrepareDriver(&deviceInfo, infPathA.GetBuffer(0), infNameA.GetBuffer(0), &prepareOptions);
	if (errorCode != ERROR_SUCCESS)
	{
		CString* pTxtErrorPrepareDriver = CInfWizardDisplay::GetTipString(IDS_ERROR_PREPARE_DRIVER);
		CString wdiError;
		LPCSTR chWdiError = g_App->Wdi.StrError(errorCode);
		wdiError = chWdiError;
		WriteStatusError(pTxtErrorPrepareDriver->GetBuffer(0), wdiError);
		goto Done;
	}

	if (bInstallOnly || bSaveInstall)
	{
		fmtRtf.Format(_T("%s..\n"), CInfWizardDisplay::GetTipString(IDS_STATUS_INSTALL_DRIVER)->GetBuffer(0));
		this->AppendStatus(fmtRtf);

		errorCode = g_App->Wdi.InstallDriver(&deviceInfo, infPathA.GetBuffer(0), infNameA.GetBuffer(0), &installOptions);
		if (errorCode != ERROR_SUCCESS)
		{
			CString* pTxtErrorInstallDriver = CInfWizardDisplay::GetTipString(IDS_ERROR_INSTALL_DRIVER);
			CString wdiError;
			LPCSTR chWdiError = g_App->Wdi.StrError(errorCode);
			wdiError = chWdiError;
			WriteStatusError(pTxtErrorInstallDriver->GetBuffer(0), wdiError);
			goto Done;
		}
	}

Done:
	SetStatusFormat(PFA_LEFT, FALSE);
	g_App->Wdi.Session()->Destroy(&deviceInfo);
	if (errorCode == ERROR_SUCCESS)
	{
		if (bInstallOnly)
		{
			RemoveDirectory(infPath);
		}
		else
		{
			m_TxtSaveName.GetWindowText(infName);
			m_TxtSaveBaseFolder.GetWindowText(infPath);

			g_App->Wdi.Session()->m_PackageName		= infName;
			g_App->Wdi.Session()->m_PackageBaseDir	= infPath;

		}

		if (bSave)
			((CButton*)GetDlgItem(IDC_BTN_SAVE_PACKAGE))->SetCheck(BST_UNCHECKED);
		else if (bInstallOnly)
			((CButton*)GetDlgItem(IDC_BTN_INSTALL_PACKAGE))->SetCheck(BST_UNCHECKED);
		else if (bSaveInstall)
			((CButton*)GetDlgItem(IDC_BTN_SAVE_AND_INSTALL_PACKAGE))->SetCheck(BST_UNCHECKED);

		CString* pTxtPackageSuccess = CInfWizardDisplay::GetTipString(IDS_STATUS_PACKAGE_SUCCESS);
		fmtRtf.Format(_T("%s\n"), pTxtPackageSuccess->GetBuffer(0));
		SetStatusFont(TRUE, CInfWizardDisplay::ColorSuccess, NULL, TwipsPerPixelY() * 11);
		this->AppendStatus(fmtRtf);

		return CResizablePageEx::OnWizardNext();
	}
	return -1;

}

BOOL CPageInstall::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();

	HICON hImgSaveLocation = (HICON)LoadImageW(g_App->m_hInstance, MAKEINTRESOURCE(IDI_ICON_OPEN_FOLDER), IMAGE_ICON, 16, 16, LR_SHARED);
	m_BtnSaveLocation.SetIcon(hImgSaveLocation);

	AddAnchor(IDC_BTN_SAVE_PACKAGE, TOP_LEFT);
	AddAnchor(IDC_BTN_INSTALL_PACKAGE, TOP_LEFT);
	AddAnchor(IDC_BTN_SAVE_AND_INSTALL_PACKAGE, TOP_LEFT);

	AddAnchor(IDC_GRP_SAVE_INFORMATION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_LBL_SAVE_BASE_FOLDER, TOP_LEFT, TOP_LEFT);
	AddAnchor(IDC_TXT_SAVE_BASE_FOLDER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BTN_SAVE_BASE_FOLDER, TOP_RIGHT);

	AddAnchor(IDC_LBL_SAVE_NAME, TOP_LEFT, TOP_LEFT);
	AddAnchor(IDC_TXT_SAVE_NAME, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GRP_STATUS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TXT_STATUS, TOP_LEFT, BOTTOM_RIGHT);

	COLORREF clrStatusBack = GetSysColor(COLOR_BTNFACE);
	m_TxtStatus.SetBackgroundColor(FALSE, clrStatusBack);

	if (g_App->Wdi.Session()->m_PackageBaseDir.IsEmpty())
	{
		CButton* wndInstallPackage = (CButton*)GetDlgItem(IDC_BTN_SAVE_AND_INSTALL_PACKAGE);
		wndInstallPackage->SetCheck(BST_CHECKED);
		OnBnClickedBtnSaveAndInstallPackage();
	}

	if(!m_ToolTip.Create(this))
	{
		TRACE("Unable To create ToolTip\n");
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_SAVE_PACKAGE, IDS_TIP_SAVE_PACKAGE);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_INSTALL_PACKAGE, IDS_TIP_INSTALL_PACKAGE);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_SAVE_AND_INSTALL_PACKAGE, IDS_TIP_SAVE_AND_INSTALL_PACKAGE);

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_TXT_SAVE_BASE_FOLDER, IDS_TIP_SAVE_BASE_FOLDER);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_TXT_SAVE_NAME, IDS_TIP_PACKAGE_NAME);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(250);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPageInstall::OnSize(UINT nType, int cx, int cy)
{
	CResizablePageEx::OnSize(nType, cx, cy);
}

void CPageInstall::SetStatusFont(BOOL isBold, COLORREF textColor, LPCTSTR pszFontName, LONG fontSize)
{
	DWORD dwMask = 0;
	CHARFORMAT charFmt;

	memset(&charFmt, 0, sizeof(charFmt));
	charFmt.cbSize = sizeof(CHARFORMAT);

	charFmt.dwMask |= CFM_BOLD;
	if (isBold) charFmt.dwEffects |= CFE_BOLD;

	if ((int)textColor != -1)
	{
		charFmt.dwMask |= CFM_COLOR;
		charFmt.crTextColor = textColor;
	}
	if (pszFontName)
	{
		charFmt.dwMask |= CFM_FACE;
		_tcscpy_s(charFmt.szFaceName, 32, pszFontName);
	}
	if (fontSize > 0)
	{
		charFmt.dwMask |= CFM_SIZE;
		charFmt.yHeight = fontSize;
	}

	m_TxtStatus.SetSelectionCharFormat(charFmt);
}

void CPageInstall::AppendStatus(CString statusText)
{
	m_TxtStatus.ReplaceSel(statusText, FALSE);
}

int CPageInstall::TwipsPerPixelY()
{
	CClientDC dc = CClientDC(this);
	int logPixY = dc.GetDeviceCaps(LOGPIXELSY);
	return 1440 / logPixY;
}
void CPageInstall::WriteStatusError(LPCTSTR szCaption, LPCTSTR szText)
{
	CString fmtRtf;

	SetStatusFont(TRUE, CInfWizardDisplay::ColorError, NULL, TwipsPerPixelY() * 10);

	fmtRtf.Format(_T("%s:\n"), szCaption);
	AppendStatus(fmtRtf);

	SetStatusFont(TRUE, RGB(0, 0, 0), NULL, TwipsPerPixelY() * 10);
	fmtRtf.Format(_T("%s\n"), szText);
	AppendStatus(fmtRtf);
}

void CPageInstall::WritePackageStatus()
{
	BOOL bSave = ((CButton*)GetDlgItem(IDC_BTN_SAVE_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bInstallOnly = ((CButton*)GetDlgItem(IDC_BTN_INSTALL_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bSaveInstall = ((CButton*)GetDlgItem(IDC_BTN_SAVE_AND_INSTALL_PACKAGE))->GetCheck() == BST_CHECKED;

	CString* headerText;
	if (bSave)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_PACKAGE_SAVE_HEADER);
	else if (bInstallOnly)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_PACKAGE_INSTALL_HEADER);
	else if (bSaveInstall)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_PACKAGE_SAVE_AND_INSTALL_HEADER);
	else
		return;

	//m_TxtStatus.SetSel(0,-1);
	m_TxtStatus.SetWindowText(_T(""));

	SetStatus(PFA_CENTER, *headerText, _T("Tahoma"), TwipsPerPixelY() * 16, TRUE, CInfWizardDisplay::ColorGood);
	SetStatusFormat(PFA_CENTER, FALSE);

}

void CPageInstall::SetStatusFormat(DWORD pfaAlignment, BOOL bulletMode)
{
	PARAFORMAT rtfParaFmt;
	memset(&rtfParaFmt, 0, sizeof(rtfParaFmt));
	rtfParaFmt.cbSize = sizeof(rtfParaFmt);

	rtfParaFmt.dwMask		= PFM_ALIGNMENT | PFM_NUMBERING;
	rtfParaFmt.wAlignment	= (WORD)pfaAlignment;
	rtfParaFmt.wNumbering	= bulletMode ? PFN_BULLET : 0;

	m_TxtStatus.SetParaFormat(rtfParaFmt);
}

void CPageInstall::SetStatus(DWORD pfaAlignment, CString statusText, LPCTSTR fontName, LONG fontSize, BOOL fontBold, COLORREF textColor)
{
	SetStatusFont(fontBold, textColor, fontName, fontSize);
	m_TxtStatus.ReplaceSel(statusText, FALSE);
	if (pfaAlignment)
	{
		PARAFORMAT rtfParaFmt;
		rtfParaFmt.cbSize		= sizeof(rtfParaFmt);
		rtfParaFmt.dwMask		= PFM_ALIGNMENT;
		rtfParaFmt.wAlignment	= (WORD)pfaAlignment;
		m_TxtStatus.SetParaFormat(rtfParaFmt);
	}
}

void CPageInstall::EnableWindowGroup(WORD nID, BOOL bEnabled)
{
	CWnd* wndGrp = GetDlgItem(nID);

	ASSERT(wndGrp);
	wndGrp->EnableWindow(bEnabled);

	wndGrp = wndGrp->GetNextWindow();

	while (wndGrp)
	{
		if (wndGrp->GetStyle() & WS_GROUP)
			break;
		wndGrp->EnableWindow(bEnabled);
		wndGrp = wndGrp->GetNextWindow();
	}
}

void CPageInstall::OnBnClickedBtnSavePackage()
{
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, TRUE);
}

void CPageInstall::OnBnClickedBtnInstallPackage()
{
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, FALSE);
}

void CPageInstall::OnBnClickedBtnSaveAndInstallPackage()
{
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, TRUE);
}

BOOL CPageInstall::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CResizablePageEx::PreTranslateMessage(pMsg);
}

BOOL CPageInstall::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CInfWizardDisplay::HandleToolTipNotify(id, pNMHDR, pResult);
}

void CPageInstall::OnBnClickedBtnSaveBaseFolder()
{
	CString baseDir, name;

	m_TxtSaveBaseFolder.GetWindowText(baseDir);
	m_TxtSaveName.GetWindowText(name);

	if (g_App->Wdi.Session()->ShowSavePackageDialog(this->GetParent(), baseDir, name))
	{
		m_TxtSaveBaseFolder.SetWindowText(g_App->Wdi.Session()->GetPackageBaseDir());
		m_TxtSaveName.SetWindowText(g_App->Wdi.Session()->GetPackageName());
	}
}
