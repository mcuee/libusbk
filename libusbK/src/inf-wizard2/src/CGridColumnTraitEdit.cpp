//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:4100)	// unreferenced formal parameter

#include "CGridColumnTraitEdit.h"

#include "CGridColumnTraitVisitor.h"
#include "CGridListCtrlEx.h"

//------------------------------------------------------------------------
//! CGridColumnTraitEdit - Constructor
//------------------------------------------------------------------------
CGridColumnTraitEdit::CGridColumnTraitEdit()
	: m_EditStyle(ES_AUTOHSCROLL | ES_NOHIDESEL)
	, m_EditLimitText(UINT_MAX)
{
}

//------------------------------------------------------------------------
//! Accept Visitor Pattern
//------------------------------------------------------------------------
void CGridColumnTraitEdit::Accept(CGridColumnTraitVisitor& visitor)
{
	visitor.Visit(*this);
}

//------------------------------------------------------------------------
//! Set style used when creating CEdit for cell value editing
//!
//! @param dwStyle Style flags
//------------------------------------------------------------------------
void CGridColumnTraitEdit::SetStyle(DWORD dwStyle)
{
	m_EditStyle = dwStyle;
}

//------------------------------------------------------------------------
//! Get style used when creating CEdit for cell value editing
//------------------------------------------------------------------------
DWORD CGridColumnTraitEdit::GetStyle() const
{
	return m_EditStyle;
}

//------------------------------------------------------------------------
//! Set max number of characters the CEdit will accept
//!
//! @param nMax The text limit, in characters.
//------------------------------------------------------------------------
void CGridColumnTraitEdit::SetLimitText(UINT nMax)
{
	m_EditLimitText = nMax;
}

//------------------------------------------------------------------------
//! Get max number of characters the CEdit will accept
//------------------------------------------------------------------------
UINT CGridColumnTraitEdit::GetLimitText() const
{
	return m_EditLimitText;
}

//------------------------------------------------------------------------
//! Create a CEdit as cell value editor
//!
//! @param owner The list control starting a cell edit
//! @param nRow The index of the row
//! @param nCol The index of the column
//! @param rect The rectangle where the inplace cell value editor should be placed
//! @return Pointer to the cell editor to use
//------------------------------------------------------------------------
CEdit* CGridColumnTraitEdit::CreateEdit(CGridListCtrlEx& owner, int nRow, int nCol, const CRect& rect)
{
	// Get the text-style of the cell to edit
	DWORD dwStyle = m_EditStyle;
	HDITEM hd = {0};
	hd.mask = HDI_FORMAT;
	VERIFY( owner.GetHeaderCtrl()->GetItem(nCol, &hd) );
	if (hd.fmt & HDF_CENTER)
		dwStyle |= ES_CENTER;
	else if (hd.fmt & HDF_RIGHT)
		dwStyle |= ES_RIGHT;
	else
		dwStyle |= ES_LEFT;

	CEdit* pEdit = new CGridEditorText(nRow, nCol);
	VERIFY( pEdit->Create( WS_CHILD | dwStyle, rect, &owner, 0) );

	// Configure font
	pEdit->SetFont(owner.GetCellFont());

	// First item (Label) doesn't have a margin (Subitems does)
	if (nCol == 0)
		pEdit->SetMargins(0, 0);
	else
		pEdit->SetMargins(4, 0);

	if (m_EditLimitText != UINT_MAX)
		pEdit->SetLimitText(m_EditLimitText);

	return pEdit;
}

//------------------------------------------------------------------------
//! Overrides OnEditBegin() to provide a CEdit cell value editor
//!
//! @param owner The list control starting edit
//! @param nRow The index of the row for the cell to edit
//! @param nCol The index of the column for the cell to edit
//! @return Pointer to the cell editor to use (NULL if cell edit is not possible)
//------------------------------------------------------------------------
CWnd* CGridColumnTraitEdit::OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol)
{
	// Get position of the cell to edit
	CRect rectCell = GetCellEditRect(owner, nRow, nCol);

	// Create edit control to edit the cell
	CEdit* pEdit = CreateEdit(owner, nRow, nCol, rectCell);
	VERIFY(pEdit != NULL);

	pEdit->SetWindowText(owner.GetItemText(nRow, nCol));
	pEdit->SetSel(0, -1, 0);

	return pEdit;
}

//------------------------------------------------------------------------
// CGridEditorText (For internal use)
//------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CGridEditorText, CEdit)
	//{{AFX_MSG_MAP(CGridEditorText)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
	ON_WM_NCCALCSIZE()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------------
//! CGridEditorText - Constructor
//------------------------------------------------------------------------
CGridEditorText::CGridEditorText(int nRow, int nCol)
	: m_Row(nRow)
	, m_Col(nCol)
	, m_Completed(false)
	, m_Modified(false)
{}

//------------------------------------------------------------------------
//! The cell value editor was closed and the entered should be saved.
//!
//! @param bSuccess Should the entered cell value be saved
//------------------------------------------------------------------------
void CGridEditorText::EndEdit(BOOL bSuccess)
{
	// Avoid two messages if key-press is followed by kill-focus
	if (m_Completed)
		return;

	m_Completed = true;

	// Send Notification to parent of ListView ctrl
	CString str;
	GetWindowText(str);

	LV_DISPINFO dispinfo = {0};
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;

	dispinfo.item.iItem = m_Row;
	dispinfo.item.iSubItem = m_Col;
	if (bSuccess && m_Modified)
	{
		dispinfo.item.mask = LVIF_TEXT;
		dispinfo.item.pszText = str.GetBuffer(0);
		dispinfo.item.cchTextMax = str.GetLength();
	}
	ShowWindow(SW_HIDE);
	GetParent()->GetParent()->SendMessage( WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo );
	PostMessage(WM_CLOSE);
}

//------------------------------------------------------------------------
//! WM_KILLFOCUS message handler called when CEdit is loosing focus
//! to other control. Used register that cell value editor should close.
//!
//! @param pNewWnd Pointer to the window that receives the input focus (may be NULL or may be temporary).
//------------------------------------------------------------------------
void CGridEditorText::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	EndEdit(true);
}

//------------------------------------------------------------------------
//! WM_NCDESTROY message handler called when CEdit window is about to
//! be destroyed. Used to delete the inplace CEdit editor object as well.
//! This is necessary when the CDateTimeCtrl is created dynamically.
//------------------------------------------------------------------------
void CGridEditorText::OnNcDestroy()
{
	CEdit::OnNcDestroy();
	delete this;
}

void CGridEditorText::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);

	CRect editRect(lpncsp->rgrc[0]);
	CRect editRectOrig(lpncsp->rgrc[0]);
	CClientDC dc(this);

	dc.DrawText(_T("KHz"), &editRect, DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);
	int iAddCY = ((editRectOrig.Height() - editRect.Height()) / 2);
	editRectOrig.top	+= iAddCY + 2;
	editRectOrig.DeflateRect(2, 2);

	lpncsp->rgrc[0] = editRectOrig;

}


//------------------------------------------------------------------------
//! WM_CHAR message handler to monitor text modifications
//!
//! @param nChar Specifies the virtual key code of the given key.
//! @param nRepCnt Repeat count (the number of times the keystroke is repeated as a result of the user holding down the key).
//! @param nFlags Specifies the scan code, key-transition code, previous key state, and context code
//------------------------------------------------------------------------
void CGridEditorText::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	m_Modified = true;
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

//------------------------------------------------------------------------
//! Hook to proces windows messages before they are dispatched.
//! Catch keyboard events that can should cause the cell value editor to close
//!
//! @param pMsg Points to a MSG structure that contains the message to process
//! @return Nonzero if the message was translated and should not be dispatched; 0 if the message was not translated and should be dispatched.
//------------------------------------------------------------------------
BOOL CGridEditorText::PreTranslateMessage(MSG* pMsg)
{
	switch(pMsg->message)
	{
	case WM_KEYDOWN:
	{
		switch(pMsg->wParam)
		{
		case VK_RETURN:
			EndEdit(true);
			return TRUE;
		case VK_TAB:
			EndEdit(true);
			return FALSE;
		case VK_ESCAPE:
			EndEdit(false);
			return TRUE;
		}
		break;
	};
	case WM_MOUSEWHEEL:
		EndEdit(true);
		return FALSE;	// Don't steal event
	}
	return CEdit::PreTranslateMessage(pMsg);
}
