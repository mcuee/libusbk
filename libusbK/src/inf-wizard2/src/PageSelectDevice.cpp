// PageSelectDevice.cpp : implementation file
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
#include "GridDevListCtrl.h"
#include "PageSelectDevice.h"
#include "DeviceNotifier.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPageSelectDevice property page

IMPLEMENT_DYNCREATE(CPageSelectDevice, CResizablePageEx)

CPageSelectDevice::CPageSelectDevice() : CResizablePageEx(CPageSelectDevice::IDD, IDS_INFWIZARD, IDS_PAGE_SELECT_DEVICE_TITLE, IDS_PAGE_SELECT_DEVICE_SUBTITLE)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);

	//{{AFX_DATA_INIT(CPageSelectDevice)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	//m_strHeaderTitle = "Setup Page (Test)";
	//m_psp.dwFlags |= PSP_HIDEHEADER;
}

CPageSelectDevice::~CPageSelectDevice()
{
}

void CPageSelectDevice::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageSelectDevice)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DEV_LIST, m_DevList);
	DDX_Control(pDX, IDC_SHOW_NEW_DEVICES, m_BtnShowNew);
	DDX_Control(pDX, IDC_SHOW_ALL_DEVICES, m_BtnShowAll);
}


BEGIN_MESSAGE_MAP(CPageSelectDevice, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageSelectDevice)
	ON_WM_SIZE()
	ON_MESSAGE(WM_DEVICECHANGE, OnWmDeviceChange)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CPageSelectDevice::OnToolTipNotify)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DEV_LIST, &CPageSelectDevice::OnLvnItemchangedDevList)
	ON_BN_CLICKED(IDC_SHOW_ALL_DEVICES, &CPageSelectDevice::OnBnClickedShowAllDevices)
	ON_BN_CLICKED(IDC_SHOW_NEW_DEVICES, &CPageSelectDevice::OnBnClickedShowNewDevices)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageSelectDevice message handlers

BOOL CPageSelectDevice::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);
	pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT);

	if (CResizablePageEx::OnSetActive())
	{
		CDeviceNotifier* pNotifier = (CDeviceNotifier*)GetParent();

		if (pNotifier->SetDirty(0))
		{
			this->OnBnClickedShowAllDevices();
		}
		pNotifier->RegisterWnd(this);
		return TRUE;
	}

	return FALSE;
}

BOOL CPageSelectDevice::OnKillActive()
{
	CDeviceNotifier* pNotifier = (CDeviceNotifier*)GetParent();
	pNotifier->RegisterWnd(NULL);

	return CResizablePageEx::OnKillActive();
}

BOOL CPageSelectDevice::OnInitDialog()
{
	CResizablePageEx::OnInitDialog();
	AddAnchor(IDC_SHOW_NEW_DEVICES, TOP_LEFT);
	AddAnchor(IDC_SHOW_ALL_DEVICES, TOP_LEFT);
	AddAnchor(IDC_DEV_LIST, TOP_LEFT, BOTTOM_RIGHT);

	m_BtnShowNew.SetCheck(BST_CHECKED);

	m_DevList.InitDevList();
	if(!m_ToolTip.Create(this))
	{
		TRACE("Unable To create ToolTip\n");
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_SHOW_NEW_DEVICES, IDS_TIP_SHOW_NEW_DEVICES);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_SHOW_ALL_DEVICES, IDS_TIP_SHOW_ALL_DEVICES);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(250);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPageSelectDevice::OnSize(UINT nType, int cx, int cy)
{
	if (m_DevList.m_hWnd)
	{
		m_DevList.SetRedraw(FALSE);

		CResizablePageEx::OnSize(nType, cx, cy);
		m_DevList.ResizeColumns();

		m_DevList.SetRedraw(TRUE);

		CRect rectList;
		m_DevList.GetWindowRect(&rectList);
		m_DevList.SendMessage(WM_SIZE, (WPARAM)0, (LPARAM)MAKELONG(rectList.Width(), rectList.Height()));

		m_DevList.ResizeColumns();


		return;
	}
	CResizablePageEx::OnSize(nType, cx, cy);
}

void CPageSelectDevice::OnLvnItemchangedDevList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVNI_SELECTED))
	{
		if (pNMListView->iItem >= 0)
		{
			g_App->Wdi.Session()->CopyFrom((PWDI_DEVICE_INFO)m_DevList.GetItemData(pNMListView->iItem));
			g_App->Wdi.Session()->m_VendorName = m_DevList.GetItemText(pNMListView->iItem, 0);
			g_App->Wdi.Session()->SetPackageName(g_App->Wdi.Session()->desc);
		}
	}

	*pResult = 0;
}

void CPageSelectDevice::OnBnClickedShowAllDevices()
{
	// TODO: Add your control notification handler code here
	int iAll = this->m_BtnShowAll.GetCheck();
	int iNew = this->m_BtnShowNew.GetCheck();

	TRACE2("[Show] All:%d New:%d\n", iAll, iNew);

	if (this->m_BtnShowNew.GetCheck() != BST_CHECKED)
	{
		m_DevList.UpdateDevList(FALSE);
		return;
	}

	m_DevList.UpdateDevList(TRUE);

}

void CPageSelectDevice::OnBnClickedShowNewDevices()
{
	OnBnClickedShowAllDevices();
}

LRESULT CPageSelectDevice::OnWmDeviceChange(WPARAM wParam, LPARAM lParam)
{
	OnBnClickedShowAllDevices();
	return 1;
}

BOOL CPageSelectDevice::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CResizablePageEx::PreTranslateMessage(pMsg);
}

BOOL CPageSelectDevice::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CInfWizardDisplay::HandleToolTipNotify(id, pNMHDR, pResult);
}
