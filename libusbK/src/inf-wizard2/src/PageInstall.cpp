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
#include "InfWizardDlg.h"
#include "PageInstall.h"
#include <shlwapi.h>
#include "libwdimanager.h"
#include "res\\packed_resources.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct _PACKED_ITEM
{
	WORD	nID;
	LPCTSTR Filename;
} PACKED_ITEM, *PPACKED_ITEM;

PACKED_ITEM PackedItems[] =
{
	{ ID_DPINST_XML,		_T("dpinst.xml") },
	{ ID_7ZDP_SFX,			_T("7ZDP_LZMA.sfx") },
	{ ID_7ZDP_CFG,			_T("7zDP_LZMA.cfg") },
	{ ID_INSTRUCTIONS_TXT,	_T("Instructions.txt") },
	{ ID_REPACK_CMD,		_T("re-pack-files.cmd") },
	{ ID_7ZA_EXE,			_T("7za.exe") },
};
const DWORD PackedItemsCount = sizeof(PackedItems) / sizeof(PackedItems[0]);

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
	ON_BN_CLICKED(IDC_BTN_CLIENT_INSTALLER, OnBnClickedBtnClientInstaller)
	ON_BN_CLICKED(IDC_BTN_LEGACY_PACKAGE, OnBnClickedBtnLegacyPackage)
	ON_BN_CLICKED(IDC_BTN_INSTALL_ONLY, OnBnClickedBtnInstallOnly)
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
	BOOL bClientInstaller = ((CButton*)GetDlgItem(IDC_BTN_CLIENT_INSTALLER))->GetCheck() == BST_CHECKED;
	BOOL bLegacyPackage = ((CButton*)GetDlgItem(IDC_BTN_LEGACY_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bInstallOnly = ((CButton*)GetDlgItem(IDC_BTN_INSTALL_ONLY))->GetCheck() == BST_CHECKED;
	CString fmtRtf;
	CStringA infPathA;
	CStringA infNameA;

	int errorCode = ERROR_SUCCESS;

	if (!bClientInstaller && !bLegacyPackage && !bInstallOnly)
	{
		return CResizablePageEx::OnWizardNext();
	}
	CLibWdiDynamicAPI& API = g_App->Wdi;

	m_TxtStatus.SetWindowText(_T(""));
	m_TxtStatus.SetSel(0, -1);
	SetStatusFont(TRUE, RGB(0, 0, 0), _T("Tahoma"), TwipsPerPixelY() * 9);
	SetStatusFormat(PFA_LEFT, TRUE);

	HCURSOR hWaitCursor = AfxGetApp()->LoadStandardCursor(IDC_WAIT);
	HCURSOR hPrevCursor = SetCursor(hWaitCursor);
	Sleep(0);

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

	g_App->Wdi.Session()->m_InfClassName.Trim();
	g_App->Wdi.Session()->m_InfClassGuid.Trim();
	g_App->Wdi.Session()->m_InfProvider.Trim();

	if (g_App->Wdi.Session()->m_InfClassName.GetLength() > 0)
		prepareOptions.inf_class = g_App->Wdi.Session()->chInfClassName;

	if (g_App->Wdi.Session()->m_InfClassGuid.GetLength() > 0)
		prepareOptions.inf_class_guid = g_App->Wdi.Session()->chInfClassGuid;

	if (g_App->Wdi.Session()->m_InfProvider.GetLength() > 0)
		prepareOptions.inf_provider = g_App->Wdi.Session()->chInfProvider;

	prepareOptions.pwr_default_idle_state = g_App->Wdi.Session()->pwr_default_idle_state;
	prepareOptions.pwr_default_idle_timeout = g_App->Wdi.Session()->pwr_default_idle_timeout;
	prepareOptions.pwr_device_idle_enabled = g_App->Wdi.Session()->pwr_device_idle_enabled;
	prepareOptions.pwr_device_idle_ignore_wake_enable = g_App->Wdi.Session()->pwr_device_idle_ignore_wake_enable;
	prepareOptions.pwr_system_wake_enabled = g_App->Wdi.Session()->pwr_system_wake_enabled;
	prepareOptions.pwr_user_set_device_idle_enabled = g_App->Wdi.Session()->pwr_user_set_device_idle_enabled;

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

	if (bClientInstaller)
	{
		prepareOptions.driver_type |= WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER;
	}
	else if (bLegacyPackage)
	{
		prepareOptions.driver_type |= WDI_PACKAGE_FLAG_CREATE_NORMAL;
	}

	prepareOptions.disable_signing = 1;
	prepareOptions.disable_cat = 1;

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

	if (!FinalizePrepareDriver(&deviceInfo, infPathA.GetBuffer(0), infNameA.GetBuffer(0), &prepareOptions))
	{
		errorCode = GetLastError();
		goto Done;
	}

	if (bInstallOnly)
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

	SetCursor(hPrevCursor);

	SetStatusFormat(PFA_LEFT, FALSE);
	g_App->Wdi.Session()->Destroy(&deviceInfo);
	if (errorCode == ERROR_SUCCESS)
	{
		g_App->Wdi.Session()->m_PackageStatus |= CLibWdiSession::PACKAGE_STATUS_SUCCESS;

		if (bLegacyPackage)
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

		if (bClientInstaller)
			((CButton*)GetDlgItem(IDC_BTN_CLIENT_INSTALLER))->SetCheck(BST_UNCHECKED);
		else if (bLegacyPackage)
			((CButton*)GetDlgItem(IDC_BTN_LEGACY_PACKAGE))->SetCheck(BST_UNCHECKED);
		else if (bInstallOnly)
			((CButton*)GetDlgItem(IDC_BTN_INSTALL_ONLY))->SetCheck(BST_UNCHECKED);

		CString* pTxtPackageSuccess = CInfWizardDisplay::GetTipString(IDS_STATUS_PACKAGE_SUCCESS);
		fmtRtf.Format(_T("%s\n"), pTxtPackageSuccess->GetBuffer(0));
		SetStatusFont(TRUE, CInfWizardDisplay::ColorGood, NULL, TwipsPerPixelY() * 11);
		this->AppendStatus(fmtRtf);

		InfWizardDlg* pDlg = (InfWizardDlg*)GetParent();
		ASSERT_KINDOF(InfWizardDlg, pDlg);

		m_TxtStatus.GetTextRange(0, m_TxtStatus.GetTextLength(), pDlg->m_InstallResults);
		return CResizablePageEx::OnWizardNext();
	}
	else
	{
		g_App->Wdi.Session()->m_PackageStatus |= CLibWdiSession::PACKAGE_STATUS_FAIL;
		CString* pTxtPackageError = CInfWizardDisplay::GetTipString(IDSE_STATUS_PACKAGE);
		fmtRtf.Format(_T("%s\n"), pTxtPackageError->GetBuffer(0));
		SetStatusFont(TRUE, CInfWizardDisplay::ColorError, NULL, TwipsPerPixelY() * 11);
		this->AppendStatus(fmtRtf);

	}
	return -1;

}

BOOL CPageInstall::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();

	m_BtnSaveLocation.SetIcon(IDI_ICON_OPEN_FOLDER);

	AddAnchor(IDC_BTN_CLIENT_INSTALLER, TOP_LEFT);
	AddAnchor(IDC_BTN_LEGACY_PACKAGE, TOP_LEFT);
	AddAnchor(IDC_BTN_INSTALL_ONLY, TOP_LEFT);

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
		CButton* wndInstallPackage = (CButton*)GetDlgItem(IDC_BTN_CLIENT_INSTALLER);
		wndInstallPackage->SetCheck(BST_CHECKED);
		OnBnClickedBtnClientInstaller();
	}

	if(!m_ToolTip.Create(this))
	{
		TRACE("Unable To create ToolTip\n");
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_CLIENT_INSTALLER, IDS_TIP_CLIENT_INSTALLER);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_LEGACY_PACKAGE, IDS_TIP_LEGACY_PACKAGE);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_BTN_INSTALL_ONLY, IDS_TIP_INSTALL_ONLY);

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_TXT_SAVE_BASE_FOLDER, IDS_TIP_SAVE_BASE_FOLDER);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_TXT_SAVE_NAME, IDS_TIP_PACKAGE_NAME);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(400);
	m_ToolTip.SetDelayTime(TTDT_AUTOPOP, 10000);

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
	m_TxtStatus.UpdateWindow();
	Sleep(0);
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
	BOOL bClientInstaller = ((CButton*)GetDlgItem(IDC_BTN_CLIENT_INSTALLER))->GetCheck() == BST_CHECKED;
	BOOL bLegacyPackage = ((CButton*)GetDlgItem(IDC_BTN_LEGACY_PACKAGE))->GetCheck() == BST_CHECKED;
	BOOL bInstallOnly = ((CButton*)GetDlgItem(IDC_BTN_INSTALL_ONLY))->GetCheck() == BST_CHECKED;

	CString* headerText;
	if (bClientInstaller)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_CLIENT_INSTALLER);
	else if (bLegacyPackage)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_LEGACY_PACKAGE);
	else if (bInstallOnly)
		headerText = CInfWizardDisplay::GetTipString(IDS_TXT_INSTALL_ONLY);
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

void CPageInstall::OnBnClickedBtnClientInstaller()
{
	g_App->Wdi.Session()->m_PackageStatus = CLibWdiSession::PACKAGE_TYPE_CLIENT_INSTALLER;
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, TRUE);
}

void CPageInstall::OnBnClickedBtnLegacyPackage()
{
	g_App->Wdi.Session()->m_PackageStatus = CLibWdiSession::PACKAGE_TYPE_LEGACY;
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, TRUE);
}

void CPageInstall::OnBnClickedBtnInstallOnly()
{
	g_App->Wdi.Session()->m_PackageStatus = CLibWdiSession::PACKAGE_TYPE_INSTALL_ONLY;
	WritePackageStatus();
	EnableWindowGroup(IDC_GRP_SAVE_INFORMATION, FALSE);
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

VOID CPageInstall::OnBnClickedBtnSaveBaseFolder()
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

BOOL CPageInstall::FinalizePrepareDriver(
    PWDI_DEVICE_INFO DeviceInfo,
    LPCSTR InfPathA,
    LPCSTR InfNameA,
    PWDI_OPTIONS_PREPARE_DRIVER Options)
{
	CString infPath(InfPathA);
	CString infName(InfNameA);
	CString fileName;
	CFile extractFile;
	CString errorMessage;
	DWORD errorCode;
	BYTE* resourceData;
	DWORD resourceSize;
	BOOL success;
	PPACKED_ITEM packedItem = NULL;
	CString fmtRtf;


	if (Options->driver_type & WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER)
	{
		fmtRtf.Format(_T("%s..\n"), CInfWizardDisplay::GetTipString(IDS_STATUS_EXPORT_INSTALLER_RESOURCES)->GetBuffer(0));
		this->AppendStatus(fmtRtf);

		WORD idPacked = ID_PACKED_BEGIN - 1;
		while (g_App->Wdi.GetDriverResource(++idPacked, ID_PACKED_RES, (LPVOID*)&resourceData, &resourceSize))
		{
			BOOL bTag = FALSE;
			BOOL bOverwrite = TRUE;
			LPCTSTR outFileName = NULL;
			int i;

			for(i = 0; i < ::PackedItemsCount; i++)
			{
				if (idPacked == PackedItems[i].nID)
				{
					packedItem = &PackedItems[i];
					break;
				}
			}
			if (!packedItem)
			{
				MessageBox(_T("You must compile from source to add resources."), _T("Missing file definitions"), MB_OK | MB_ICONEXCLAMATION);
				continue;
			}


			switch(idPacked)
			{
			case ID_7ZA_EXE:
				// Extract the 7za.exe tool.
				GetTempPath(MAX_PATH, fileName.GetBufferSetLength(MAX_PATH));
				PathAppend(fileName.GetBuffer(), _T("7za.exe"));
				fileName.ReleaseBuffer();
				outFileName = NULL;
				SetEnvironmentVariable(_T("7ZA_EXE"), fileName);
				SetEnvironmentVariable(_T("NO_REPACK_ERROR_WAIT"), _T("1"));

				bOverwrite = FALSE;
				break;
			default:
				fileName = infPath;
				outFileName = packedItem->Filename;
				break;
			}

			if (outFileName)
			{
				if (!PathAppend(fileName.GetBufferSetLength(MAX_PATH), outFileName))
				{
					errorCode = GetLastError();
					errorMessage.Format(IDSF_PATH_TO_LONG, MAX_PATH, fileName, errorCode);
					MessageBox(errorMessage, _T("Path is to long"), MB_OK | MB_ICONEXCLAMATION);

					SetLastError(errorCode);
					return FALSE;
				}
				fileName.ReleaseBuffer();
			}

			if (!bOverwrite && PathFileExists(fileName))
				continue;

			success = extractFile.Open(fileName, CFile::modeCreate | CFile::shareDenyNone | CFile::modeWrite);

			if (!success)
			{
				errorCode = GetLastError();
				errorMessage.Format(IDSF_CREATE_FILE_FAILED, fileName, errorCode);
				MessageBox(errorMessage, _T("Create file failed"), MB_OK | MB_ICONEXCLAMATION);

				SetLastError(errorCode);
				return FALSE;
			}

			extractFile.Write(resourceData, resourceSize);
			extractFile.Flush();
			extractFile.Close();

		}

		packedItem = NULL;
		for(int i = 0; i < PackedItemsCount; i++)
		{
			if (PackedItems[i].nID == ID_REPACK_CMD)
			{
				packedItem = &PackedItems[i];
				break;
			}
		}
		if (packedItem)
		{
			SHELLEXECUTEINFO execInfo;
			DWORD exitCode = 0;

			fmtRtf.Format(_T("%s..\n"), CInfWizardDisplay::GetTipString(IDS_STATUS_CREATE_USER_INSTALLER)->GetBuffer(0));
			this->AppendStatus(fmtRtf);

			memset(&execInfo, 0, sizeof(execInfo));
			execInfo.cbSize = sizeof(execInfo);
			execInfo.lpDirectory = infPath;
			execInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
			execInfo.nShow = SW_HIDE;
			execInfo.lpFile =  packedItem->Filename;

			if(ShellExecuteEx(&execInfo) != FALSE)
			{
				WaitForSingleObject(execInfo.hProcess, INFINITE);
				GetExitCodeProcess(execInfo.hProcess, &exitCode);
				if (exitCode != 0)
				{
					LPCTSTR caption = CInfWizardDisplay::GetTipString(IDSE_CREATE_USER_INSTALLER)->GetBuffer(0);
					WriteStatusError(caption, execInfo.lpFile);
					MessageBox(execInfo.lpFile, caption, MB_OK | MB_ICONEXCLAMATION);

					SetLastError(exitCode);
					return FALSE;
				}
			}
			else
			{
				errorCode = GetLastError();
				LPCTSTR caption = CInfWizardDisplay::GetTipString(IDSE_UNKNOWN)->GetBuffer(0);
				WriteStatusError(caption, execInfo.lpFile);
				MessageBox(execInfo.lpFile, caption, MB_OK | MB_ICONEXCLAMATION);

				SetLastError(errorCode);
				return FALSE;
			}
		}
	}
	return TRUE;
}
