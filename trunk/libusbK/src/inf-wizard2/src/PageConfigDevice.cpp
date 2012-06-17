// PageConfigDevice.cpp : implementation file
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
#include "CGridRowTraitXP.h"
#include "ViewConfigSection.h"
#include "GridDevCfgListCtrl.h"
#include "PageConfigDevice.h"
#include "InfWizardDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPageConfigDevice property page

IMPLEMENT_DYNCREATE(CPageConfigDevice, CResizablePageEx)

CPageConfigDevice::CPageConfigDevice() : CResizablePageEx(CPageConfigDevice::IDD, IDS_INFWIZARD, IDS_PAGE_CONFIG_DEVICE_TITLE, IDS_PAGE_CONFIG_DEVICE_SUBTITLE)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);

	//{{AFX_DATA_INIT(CPageConfigDevice)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	//m_strHeaderTitle = "Setup Page (Test)";
	//m_psp.dwFlags |= PSP_HIDEHEADER;
}

CPageConfigDevice::~CPageConfigDevice()
{
}

void CPageConfigDevice::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageConfigDevice)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DEV_LIST, m_DevCfgList);
	DDX_Control(pDX, IDC_SYSLINK_POWER_MANAGEMENT, m_LinkPowerPage);
}


BEGIN_MESSAGE_MAP(CPageConfigDevice, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageConfigDevice)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_DEV_LIST, OnNMDblclkDevList)
	ON_NOTIFY(NM_CLICK, IDC_DEV_LIST, OnNMClickDevList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_DEV_LIST, OnLvnKeydownDevList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_DEV_LIST, OnLvnEndlabeleditDevList)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_POWER_MANAGEMENT, &CPageConfigDevice::OnNMClickSyslinkPowerManagement)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageConfigDevice message handlers

BOOL CPageConfigDevice::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	m_DevCfgList.DeleteAllItems();
	m_DevCfgList.Load(g_App->Wdi.Session());

	if (g_App->Wdi.Session()->IsValid() == ERROR_SUCCESS)
		pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	else
		pSheet->SetWizardButtons(PSWIZB_BACK);

	if (g_App->Wdi.Session()->GetDriverType() == WDI_LIBUSB0)
	{
		m_LinkPowerPage.SetItemState(0, 0);
		m_LinkPowerPage.EnableWindow(FALSE);
	}
	else
	{
		m_LinkPowerPage.EnableWindow(TRUE);
		m_LinkPowerPage.SetItemState(0, LIS_ENABLED);
	}
	return CResizablePageEx::OnSetActive();
}

BOOL CPageConfigDevice::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();
	AddAnchor(IDC_DEV_LIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SYSLINK_POWER_MANAGEMENT, BOTTOM_LEFT);

	m_DevCfgList.InitDevCfgList(m_DevCfgListImages);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPageConfigDevice::OnSize(UINT nType, int cx, int cy)
{
	if (m_DevCfgList.m_hWnd)
	{
		CResizablePageEx::OnSize(nType, cx, cy);
		m_DevCfgList.ResizeColumns();

		return;
	}
	CResizablePageEx::OnSize(nType, cx, cy);

}

void CPageConfigDevice::OnNMDblclkDevList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem >= 0 && pNMItemActivate->iSubItem == DEVCFG_COLID_FIELD)
	{
		m_DevCfgList.SetFocusCell(DEVCFG_COLID_VALUE);
		m_DevCfgList.EditCell(pNMItemActivate->iItem, DEVCFG_COLID_VALUE);
	}
	TRACE2("iItem:%d iSubItem:%d\n", pNMItemActivate->iItem, pNMItemActivate->iSubItem);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CPageConfigDevice::OnNMClickDevList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem >= 0 && pNMItemActivate->iSubItem == DEVCFG_COLID_VALUE)
	{
		m_DevCfgList.SetFocusCell(DEVCFG_COLID_VALUE);
		m_DevCfgList.EditCell(pNMItemActivate->iItem, DEVCFG_COLID_VALUE);
	}
	TRACE2("iItem:%d iSubItem:%d\n", pNMItemActivate->iItem, pNMItemActivate->iSubItem);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CPageConfigDevice::OnLvnKeydownDevList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	TRACE2("[OnLvnKeydownDevList] flags:%d wVKey:%d\n", pLVKeyDow->flags, pLVKeyDow->wVKey);

	if (pLVKeyDow->wVKey == VK_SPACE)
	{
		int rowId = m_DevCfgList.GetSelectionMark();
		if (rowId >= 0)
		{
			m_DevCfgList.SetFocusCell(DEVCFG_COLID_VALUE);
			m_DevCfgList.EditCell(rowId, DEVCFG_COLID_VALUE);
			*pResult = 1;
			return;
		}
	}

	*pResult = 0;
}

void CPageConfigDevice::OnLvnEndlabeleditDevList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	/*
	If the user cancels editing, the pszText member of the LVITEM structure is NULL; otherwise,
	pszText is the address of the edited text.

	If the pszText member of the LVITEM structure is non-NULL, return TRUE to set the item's
	label to the edited text. Return FALSE to reject the edited text and revert to the original
	label.
	*/

	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	if (pDispInfo->item.pszText)
	{
		CString fieldValue		= pDispInfo->item.pszText;
		CString fieldIntValue	= fieldValue;

		fieldIntValue.MakeLower();

		if (fieldIntValue.Find(_T("0x"), 0) != 0)
			fieldIntValue.Insert(0, _T("0x"));

		fieldIntValue.TrimRight((TCHAR)'H');
		DWORD dwValue = _tcstoul(fieldIntValue, NULL, NULL);


		switch(pDispInfo->item.iItem)
		{
		case DEVCFG_FIELD_VID:
			g_App->Wdi.Session()->vid = (WORD)dwValue;
			break;

		case DEVCFG_FIELD_PID:
			g_App->Wdi.Session()->pid = (WORD)dwValue;
			break;

		case DEVCFG_FIELD_MI:
			if (fieldValue.GetLength() == 0)
				g_App->Wdi.Session()->is_composite = FALSE;
			else
				g_App->Wdi.Session()->is_composite = TRUE;

			g_App->Wdi.Session()->mi = (UCHAR)dwValue;
			break;

		case DEVCFG_FIELD_DESC:
			g_App->Wdi.Session()->desc = fieldValue;
			g_App->Wdi.Session()->SetPackageName(fieldValue);
			break;

		case DEVCFG_FIELD_GUID:
			g_App->Wdi.Session()->SetGuid(fieldValue);
			break;

		case DEVCFG_FIELD_MFG:
			g_App->Wdi.Session()->m_VendorName = fieldValue;
			break;

		case DEVCFG_FIELD_INF_CLASS:
			g_App->Wdi.Session()->SetInfClassName(fieldValue);
			break;

		case DEVCFG_FIELD_INF_CLASS_GUID:
			g_App->Wdi.Session()->SetInfClassGuid(fieldValue);
			break;

		case DEVCFG_FIELD_INF_PROVIDER:
			g_App->Wdi.Session()->SetInfProvider(fieldValue);
			break;

		default:
			TRACE0("[ERROR] Unhandled field.\n");
			ASSERT(FALSE);
		}
	}
	if (g_App->Wdi.Session()->IsValid() == ERROR_SUCCESS)
	{
		pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
		return;
	}

	pSheet->SetWizardButtons(PSWIZB_BACK);

	*pResult = TRUE;
}

void CPageConfigDevice::OnNMClickSyslinkPowerManagement(NMHDR* pNMHDR, LRESULT* pResult)
{
	InfWizardDlg* pSheet = (InfWizardDlg*)GetParent();
	ASSERT_KINDOF(InfWizardDlg, pSheet);


	pSheet->m_BackDlgID = IDD_PAGE_CONFIG_DEVICE;
	pSheet->m_NextDlgID = IDD_PAGE_INSTALL;
	pSheet->SetActivePage(pSheet->GetPageIndex(&pSheet->m_PagePowerManagment));
	*pResult = 0;

}
