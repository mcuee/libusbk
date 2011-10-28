#include "stdafx.h"
#include "InfWizardDisplay.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Red
COLORREF CInfWizardDisplay::ColorError		= RGB(177,25,0);
// Orange
COLORREF CInfWizardDisplay::ColorWarning	= RGB(200,83,0);
// Blue
COLORREF CInfWizardDisplay::ColorGood		= RGB(0,83,200);
// Green
COLORREF CInfWizardDisplay::ColorSuccess	= RGB(0,145,72);

CMapWordToPtr CInfWizardDisplay::m_ToolTipStrings;

static volatile long refCount = 0;

CInfWizardDisplay::CInfWizardDisplay(void)
{
	memset(&m_LogFontGrid,0,sizeof(m_LogFontGrid));
	if (InterlockedIncrement(&refCount)==1)
	{
		AllocToolTipStrings();
	}
}

CInfWizardDisplay::~CInfWizardDisplay(void)
{
	if (InterlockedDecrement(&refCount)==0)
	{
		FreeToolTipStrings();
	}

	m_FontGrid.DeleteObject();
}

void CInfWizardDisplay::CreateGridFont(BOOL bSmall, BOOL bBold)
{
	int fontSize	= bSmall ? 12 : 13;
	int fontWeight	= bBold ? FW_HEAVY:FW_NORMAL;
	m_FontGrid.CreateFont(12, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, DEF_FONTNAME);
	m_FontGrid.GetLogFont(&m_LogFontGrid);
}

void CInfWizardDisplay::AddTipString(WORD nID)
{
	if (GetTipString(nID)!=NULL)
		return;

	CString* tipString=new CString();
	tipString->LoadString(nID);
	m_ToolTipStrings.SetAt(nID,tipString);
}

void CInfWizardDisplay::AddCallbackTool(CToolTipCtrl* tipCtrl, CWnd* parent,  WORD nDlgID, LPARAM nStringID)
{
	TOOLINFO tool;
	memset(&tool,0,sizeof(tool));
	tool.cbSize		= sizeof(tool);
	tool.uFlags		= TTF_IDISHWND;
	tool.hwnd		= parent->GetSafeHwnd();
	tool.lpszText	= LPSTR_TEXTCALLBACK;
	tool.lParam		= nStringID;
	tool.uId		=(UINT_PTR)parent->GetDlgItem(nDlgID)->GetSafeHwnd();

	tipCtrl->SendMessage(TTM_ADDTOOL,(WPARAM)0,(LPARAM)&tool);
}

void CInfWizardDisplay::AllocToolTipStrings(void)
{
	WORD nID = 0;

	while (++nID)
	{
		CString* tipString=new CString();
		if (!tipString->LoadString(nID))
		{
			delete tipString;
			return;
		}
		m_ToolTipStrings.SetAt(nID,tipString);
	}
}

void CInfWizardDisplay::FreeToolTipStrings(void)
{
	POSITION pos = m_ToolTipStrings.GetStartPosition();
	CString* tipString;
	WORD tipKey;
	while (pos != NULL)
	{
		m_ToolTipStrings.GetNextAssoc(pos, tipKey, (void*&)tipString);
		if (tipString)
			delete tipString;
		m_ToolTipStrings.RemoveKey( tipKey );
	}
}

CString* CInfWizardDisplay::GetTipString(WORD nID)
{
	CString* tipString;

	if (m_ToolTipStrings.Lookup(nID,(void*&)tipString))
		return tipString;

	return NULL;
}

BOOL CInfWizardDisplay::HandleToolTipNotify(UINT id, NMHDR* pNMHDR,LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
	UINT nID =pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		CString* tipString = CInfWizardDisplay::GetTipString((WORD)pTTT->lParam);
		if (tipString)
		{
			pTTT->lpszText = tipString->GetBuffer(0);
			return(TRUE);
		}
	}
	return(FALSE);
}
