// PagePowerManagement.cpp : implementation file
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
#include "PagePowerManagement.h"
#include "InfWizardDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPagePowerManagement property page

IMPLEMENT_DYNCREATE(CPagePowerManagement, CResizablePageEx)

CPagePowerManagement::CPagePowerManagement() : CResizablePageEx(CPagePowerManagement::IDD, IDS_INFWIZARD, IDS_PAGE_POWER_MANAGEMENT_TITLE, IDS_PAGE_POWER_MANAGEMENT_SUBTITLE)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);

	//{{AFX_DATA_INIT(CPagePowerManagement)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	//m_strHeaderTitle = "Setup Page (Test)";
	//m_psp.dwFlags |= PSP_HIDEHEADER;
}

CPagePowerManagement::~CPagePowerManagement()
{
}

void CPagePowerManagement::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPagePowerManagement)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHK_DEVICE_IDLE_ENABLED, m_ChkDeviceIdleEnabled);
	DDX_Control(pDX, IDC_CHK_DEFAULT_IDLE_STATE, m_ChkDefaultIdleState);
	DDX_Control(pDX, IDC_CBO_DEFAULT_IDLE_TIMEOUT, m_CboIdleTimeout);
	DDX_Control(pDX, IDC_CHK_USER_SET_DEVICE_IDLE_ENABLED, m_ChkUserSetDeviceIdleEnabled);
	DDX_Control(pDX, IDC_CHK_DEVICE_IDLE_IGNORE_WAKE_ENABLE, m_ChkDeviceIdleIgnoreWakeEnable);
	DDX_Control(pDX, IDC_CHK_SYSTEM_WAKE_ENABLED, m_ChkSystemWakeEnabled);
}


BEGIN_MESSAGE_MAP(CPagePowerManagement, CResizablePageEx)
	//{{AFX_MSG_MAP(CPagePowerManagement)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
	ON_BN_CLICKED(IDC_CHK_DEVICE_IDLE_ENABLED, &CPagePowerManagement::OnBnClickedChkEnablePower)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPagePowerManagement message handlers

BOOL CPagePowerManagement::OnSetActive()
{
	InfWizardDlg* pSheet = (InfWizardDlg*)GetParent();
	ASSERT_KINDOF(InfWizardDlg, pSheet);

	FillForm();

	DWORD dwBtns = 0;
	dwBtns |= pSheet->m_BackDlgID != -1 ? PSWIZB_BACK : 0;
	dwBtns |= pSheet->m_NextDlgID != -1 ? PSWIZB_NEXT : 0;
	pSheet->SetWizardButtons(dwBtns);

	return CResizablePageEx::OnSetActive();
}

BOOL CPagePowerManagement::FillForm()
{
	m_ChkDeviceIdleEnabled.SetCheck(g_App->Wdi.Session()->pwr_device_idle_enabled != WDI_POWER_FLAG_OFF ? BST_CHECKED : BST_UNCHECKED);
	m_ChkDefaultIdleState.SetCheck(g_App->Wdi.Session()->pwr_default_idle_state == WDI_POWER_FLAG_ON ? BST_CHECKED : BST_UNCHECKED);
	m_ChkUserSetDeviceIdleEnabled.SetCheck(g_App->Wdi.Session()->pwr_user_set_device_idle_enabled == WDI_POWER_FLAG_ON ? BST_CHECKED : BST_UNCHECKED);
	m_ChkDeviceIdleIgnoreWakeEnable.SetCheck(g_App->Wdi.Session()->pwr_device_idle_ignore_wake_enable == WDI_POWER_FLAG_ON ? BST_CHECKED : BST_UNCHECKED);
	m_ChkSystemWakeEnabled.SetCheck(g_App->Wdi.Session()->pwr_system_wake_enabled == WDI_POWER_FLAG_ON ? BST_CHECKED : BST_UNCHECKED);

	CString sTimeout;
	sTimeout.Format(_T("%d"), g_App->Wdi.Session()->pwr_default_idle_timeout);
	if (m_CboIdleTimeout.SelectString(-1, sTimeout) == -1)
		m_CboIdleTimeout.SetWindowText(sTimeout);

	if (m_ChkDeviceIdleEnabled.GetCheck() == BST_CHECKED)
		EnableWindowGroup(IDC_CHK_DEFAULT_IDLE_STATE, TRUE);
	else
		EnableWindowGroup(IDC_CHK_DEFAULT_IDLE_STATE, FALSE);

	return TRUE;
}

BOOL CPagePowerManagement::SaveForm()
{
	if (m_ChkDeviceIdleEnabled.GetCheck() == BST_CHECKED)
		g_App->Wdi.Session()->pwr_device_idle_enabled = WDI_POWER_FLAG_ON;
	else
		g_App->Wdi.Session()->pwr_device_idle_enabled = WDI_POWER_FLAG_OFF;

	if (m_ChkDefaultIdleState.GetCheck() == BST_CHECKED)
		g_App->Wdi.Session()->pwr_default_idle_state = WDI_POWER_FLAG_ON;
	else
		g_App->Wdi.Session()->pwr_default_idle_state = WDI_POWER_FLAG_OFF;

	if (m_ChkUserSetDeviceIdleEnabled.GetCheck() == BST_CHECKED)
		g_App->Wdi.Session()->pwr_user_set_device_idle_enabled = WDI_POWER_FLAG_ON;
	else
		g_App->Wdi.Session()->pwr_user_set_device_idle_enabled = WDI_POWER_FLAG_OFF;

	if (m_ChkDeviceIdleIgnoreWakeEnable.GetCheck() == BST_CHECKED)
		g_App->Wdi.Session()->pwr_device_idle_ignore_wake_enable = WDI_POWER_FLAG_ON;
	else
		g_App->Wdi.Session()->pwr_device_idle_ignore_wake_enable = WDI_POWER_FLAG_OFF;

	if (m_ChkSystemWakeEnabled.GetCheck() == BST_CHECKED)
		g_App->Wdi.Session()->pwr_system_wake_enabled = WDI_POWER_FLAG_ON;
	else
		g_App->Wdi.Session()->pwr_system_wake_enabled = WDI_POWER_FLAG_OFF;

	CString sTimeout;
	m_CboIdleTimeout.GetWindowText(sTimeout);
	g_App->Wdi.Session()->pwr_default_idle_timeout = _tcstoul(sTimeout, NULL, 0);
	if (g_App->Wdi.Session()->pwr_default_idle_timeout == 0 || g_App->Wdi.Session()->pwr_default_idle_timeout & 0x80000000)
		g_App->Wdi.Session()->pwr_default_idle_timeout = 5000;

	return TRUE;

}
LRESULT CPagePowerManagement::OnWizardNext()
{
	InfWizardDlg* pSheet = (InfWizardDlg*)GetParent();
	ASSERT_KINDOF(InfWizardDlg, pSheet);

	SaveForm();

	return pSheet->m_NextDlgID;
}

LRESULT CPagePowerManagement::OnWizardBack()
{
	InfWizardDlg* pSheet = (InfWizardDlg*)GetParent();
	ASSERT_KINDOF(InfWizardDlg, pSheet);

	SaveForm();

	return pSheet->m_BackDlgID;
}

BOOL CPagePowerManagement::OnInitDialog()
{

	AddAnchor(IDC_GRP_POWER, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_CHK_DEVICE_IDLE_ENABLED, TOP_LEFT);
	AddAnchor(IDC_CHK_DEFAULT_IDLE_STATE, TOP_LEFT);
	AddAnchor(IDC_LBL_DEFAULT_IDLE_TIMEOUT, TOP_LEFT);
	AddAnchor(IDC_CBO_DEFAULT_IDLE_TIMEOUT, TOP_LEFT);
	AddAnchor(IDC_CHK_USER_SET_DEVICE_IDLE_ENABLED, TOP_LEFT);
	AddAnchor(IDC_CHK_DEVICE_IDLE_IGNORE_WAKE_ENABLE, TOP_LEFT);
	AddAnchor(IDC_CHK_SYSTEM_WAKE_ENABLED, BOTTOM_LEFT);

	//////////////////////////////////////////////////////////////////
	// Populate idle timeout CBO
	//////////////////////////////////////////////////////////////////

	if(!m_ToolTip.Create(this))
	{
		TRACE("Unable To create ToolTip\n");
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CHK_DEVICE_IDLE_ENABLED, IDS_TIP_DEVICE_IDLE_ENABLED);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CHK_DEFAULT_IDLE_STATE, IDS_TIP_DEFAULT_IDLE_STATE);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CBO_DEFAULT_IDLE_TIMEOUT, IDS_TIP_DEFAULT_IDLE_TIMEOUT);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LBL_DEFAULT_IDLE_TIMEOUT, IDS_TIP_DEFAULT_IDLE_TIMEOUT);

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CHK_USER_SET_DEVICE_IDLE_ENABLED, IDS_TIP_USER_SET_DEVICE_IDLE_ENABLED);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CHK_DEVICE_IDLE_IGNORE_WAKE_ENABLE, IDS_TIP_DEVICE_IDLE_IGNORE_WAKE_ENABLE);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_CHK_SYSTEM_WAKE_ENABLED, IDS_TIP_SYSTEM_WAKE_ENABLED);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(400);
	m_ToolTip.SetDelayTime(TTDT_AUTOPOP, 10000);

	if (CResizablePageEx::OnInitDialog())
	{
		m_CboIdleTimeout.AddString(_T("100"));
		m_CboIdleTimeout.AddString(_T("500"));
		m_CboIdleTimeout.AddString(_T("1000"));
		m_CboIdleTimeout.AddString(_T("2000"));
		m_CboIdleTimeout.AddString(_T("5000"));
		m_CboIdleTimeout.AddString(_T("10000"));
		m_CboIdleTimeout.AddString(_T("20000"));
		m_CboIdleTimeout.AddString(_T("30000"));

		m_CboIdleTimeout.SelectString(-1, _T("5000"));
		m_ChkDeviceIdleEnabled.SetCheck(BST_CHECKED);
		return TRUE;  // return TRUE unless you set the focus to a control
	}

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPagePowerManagement::OnSize(UINT nType, int cx, int cy)
{
	CResizablePageEx::OnSize(nType, cx, cy);

}

void CPagePowerManagement::OnBnClickedChkEnablePower()
{
	// TODO: Add your control notification handler code here
	if (m_ChkDeviceIdleEnabled.GetCheck() == BST_CHECKED)
		EnableWindowGroup(IDC_CHK_DEFAULT_IDLE_STATE, TRUE);
	else
		EnableWindowGroup(IDC_CHK_DEFAULT_IDLE_STATE, FALSE);
}

void CPagePowerManagement::EnableWindowGroup(WORD nID, BOOL bEnabled)
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

BOOL CPagePowerManagement::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CInfWizardDisplay::HandleToolTipNotify(id, pNMHDR, pResult);
}

BOOL CPagePowerManagement::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CResizablePageEx::PreTranslateMessage(pMsg);
}