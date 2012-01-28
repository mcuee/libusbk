// PageIntro.cpp : implementation file
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
#include "CGridColumnTraitImage.h"
#include "GridDrvListCtrl.h"
#include "PageIntro.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// CPageIntro property page

IMPLEMENT_DYNCREATE(CPageIntro, CResizablePageEx)

CPageIntro::CPageIntro() : CResizablePageEx(CPageIntro::IDD, IDS_INFWIZARD, 0, 0)
{
	//{{AFX_DATA_INIT(CPageIntro)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_psp.dwFlags |= PSP_HIDEHEADER;
	m_psp.dwFlags &= ~(PSP_HASHELP);
}

CPageIntro::~CPageIntro()
{
	if (static_cast<HFONT>(m_FontBig))
		VERIFY( m_FontBig.DeleteObject() );
}

void CPageIntro::DoDataExchange(CDataExchange* pDX)
{
	CResizablePageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageIntro)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DRIVER_LIST, m_DriverList);
	DDX_Control(pDX, IDC_LINK_SELECT_DRIVER, m_LnkSelectDriver);
}


BEGIN_MESSAGE_MAP(CPageIntro, CResizablePageEx)
	//{{AFX_MSG_MAP(CPageIntro)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CPageIntro::OnToolTipNotify)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DRIVER_LIST, &CPageIntro::OnLvnItemchangedDriverList)
	ON_NOTIFY(NM_CLICK, IDC_LINK_OPEN_SESSION, &CPageIntro::OnNMClickLinkOpenSession)
	ON_NOTIFY(NM_CLICK, IDC_LINK_SAVE_SESSION, &CPageIntro::OnNMClickLinkSaveSession)
	ON_NOTIFY(NM_CLICK, IDC_LINK_SELECT_DRIVER, &CPageIntro::OnNMClickLinkSelectDriver)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageIntro message handlers

BOOL CPageIntro::OnSetActive()
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	if (g_App->Wdi.Session()->GetDriverType() >= WDI_NB_DRIVERS)
		pSheet->SetWizardButtons(0);
	else
		pSheet->SetWizardButtons(PSWIZB_NEXT);

	return CResizablePageEx::OnSetActive();
}

BOOL CPageIntro::OnInitDialog()
{
	CString sTitle;

	CResizablePageEx::OnInitDialog();

	AddAnchor(IDC_BIGBOLDTITLE, TOP_LEFT);
	AddAnchor(IDC_INTRO_TEXT1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LINK_SELECT_DRIVER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DRIVER_LIST, TOP_LEFT, BOTTOM_RIGHT);

	AddAnchor(IDC_LINK_OPEN_SESSION, BOTTOM_LEFT);
	AddAnchor(IDC_LINK_SAVE_SESSION, BOTTOM_LEFT);

	CWnd* wDlgItem;
	wDlgItem = GetDlgItem(IDC_BIGBOLDTITLE);
	wDlgItem->GetFont()->GetLogFont(&m_LogFontBig);

	m_LogFontBig.lfWeight	= FW_BOLD;
	m_LogFontBig.lfHeight	*= 2;
	m_LogFontBig.lfWidth	= 0;
	m_LogFontBig.lfQuality	= CLEARTYPE_QUALITY;
	_tcscpy(m_LogFontBig.lfFaceName, _T("Tahoma"));
	m_FontBig.CreateFontIndirect(&m_LogFontBig);

	wDlgItem->SetFont(&m_FontBig);

	m_DriverList.InitDriverList(m_DriverListImages);
	if(!m_ToolTip.Create(this))
	{
		TRACE("Unable To create ToolTip\n");
		return FALSE;
	}

	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_OPEN_SESSION, IDS_TIP_OPEN_SESSION);
	CInfWizardDisplay::AddCallbackTool(&m_ToolTip, this, IDC_LINK_SAVE_SESSION, IDS_TIP_SAVE_SESSION);

	m_ToolTip.Activate(TRUE);

	m_ToolTip.SetMaxTipWidth(250);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPageIntro::OnSize(UINT nType, int cx, int cy)
{
	if (m_DriverList.m_hWnd)
	{
		CResizablePageEx::OnSize(nType, cx, cy);
		m_DriverList.ResizeColumns();

		return;
	}
	CResizablePageEx::OnSize(nType, cx, cy);
}

void CPageIntro::OnLvnItemchangedDriverList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVNI_SELECTED))
	{
		if (pNMListView->iItem >= 0)
		{
			g_App->Wdi.Session()->SetDriverType((int)m_DriverList.GetItemData(pNMListView->iItem));

			CPropertySheet* pSheet = (CPropertySheet*)GetParent();
			pSheet->SetWizardButtons(PSWIZB_NEXT);
			return;
		}
	}
}

BOOL CPageIntro::SelectSessionDriverType(void)
{
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();

	int nItems = m_DriverList.GetItemCount();
	for (int iItem = 0; iItem < nItems; iItem++)
	{
		int driverType = (int)m_DriverList.GetItemData(iItem);
		if (driverType == g_App->Wdi.Session()->GetDriverType())
		{
			m_DriverList.SelectRow(iItem, TRUE);
			pSheet->SetWizardButtons(PSWIZB_NEXT);
			return TRUE;
		}
		m_DriverList.SelectRow(iItem, FALSE);
	}

	pSheet->SetWizardButtons(0);
	return FALSE;
}

void CPageIntro::OnNMClickLinkOpenSession(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();

	if (g_App->Wdi.OpenSession(GetParent()))
	{
		if (SelectSessionDriverType())
		{
			pSheet->SetActivePage(3);
		}
	}
}

void CPageIntro::OnNMClickLinkSaveSession(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	g_App->Wdi.SaveSession(GetParent());
	*pResult = 0;
}

BOOL CPageIntro::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CResizablePageEx::PreTranslateMessage(pMsg);
}

BOOL CPageIntro::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CInfWizardDisplay::HandleToolTipNotify(id, pNMHDR, pResult);
}
void CPageIntro::OnNMClickLinkSelectDriver(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	PNMLINK pnmLink = (PNMLINK) pNMHDR;
	switch(pnmLink->item.iLink)
	{
	case 0:
		if (g_App->Wdi.LoadDll(this))
		{
			m_DriverList.UpdateDriverList();
			SelectSessionDriverType();
		}
		break;
	case 1:
		MessageBox(_T("Package repositories are not yet available."), _T("InfWizard v2.0"), MB_OK | MB_ICONINFORMATION);
		break;
	}
	*pResult = 0;
}
