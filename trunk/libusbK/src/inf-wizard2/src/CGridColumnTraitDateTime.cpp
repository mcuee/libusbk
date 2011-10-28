//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:4100)	// unreferenced formal parameter

#include "CGridColumnTraitDateTime.h"

#include "CGridColumnTraitVisitor.h"
#include "CGridListCtrlEx.h"

//------------------------------------------------------------------------
//! CGridColumnTraitDateTime - Constructor
//------------------------------------------------------------------------
CGridColumnTraitDateTime::CGridColumnTraitDateTime()
	: m_ParseDateTimeFlags(0)
	, m_ParseDateTimeLCID(LOCALE_USER_DEFAULT)
	, m_DateTimeCtrlStyle(0)
{}

//------------------------------------------------------------------------
//! Accept Visitor Pattern
//------------------------------------------------------------------------
void CGridColumnTraitDateTime::Accept(CGridColumnTraitVisitor& visitor)
{
	visitor.Visit(*this);
}

//------------------------------------------------------------------------
//! Set the DateTime format used to display the date
//!
//! @param strFormat Format string that defines the desired display
//------------------------------------------------------------------------
void CGridColumnTraitDateTime::SetFormat(const CString& strFormat)
{
	m_Format = strFormat;
}

//------------------------------------------------------------------------
//! Get the DateTime format used to display the date
//------------------------------------------------------------------------
CString CGridColumnTraitDateTime::GetFormat() const
{
	return m_Format;
}

//------------------------------------------------------------------------
//! Set the flags for converting the datetime in text format to actual date.
//!
//! @param dwFlags Flags for locale settings and parsing
//! @param lcid Locale ID to use for the conversion
//------------------------------------------------------------------------
void CGridColumnTraitDateTime::SetParseDateTime(DWORD dwFlags, LCID lcid)
{
	m_ParseDateTimeFlags = dwFlags;
	m_ParseDateTimeLCID = lcid;
}

//------------------------------------------------------------------------
//! Set style used when creating CDataTimeCtrl for cell value editing
//!
//! @param dwStyle Style flags
//------------------------------------------------------------------------
void CGridColumnTraitDateTime::SetStyle(DWORD dwStyle)
{
	m_DateTimeCtrlStyle = dwStyle;
}

//------------------------------------------------------------------------
//! Get style used when creating CDataTimeCtrl for cell value editing
//------------------------------------------------------------------------
DWORD CGridColumnTraitDateTime::GetStyle() const
{
	return m_DateTimeCtrlStyle;
}

//------------------------------------------------------------------------
//! Create a CDateTimeCtrl as cell value editor
//!
//! @param owner The list control starting a cell edit
//! @param nRow The index of the row
//! @param nCol The index of the column
//! @param rect The rectangle where the inplace cell value editor should be placed
//! @return Pointer to the cell editor to use
//------------------------------------------------------------------------
CDateTimeCtrl* CGridColumnTraitDateTime::CreateDateTimeCtrl(CGridListCtrlEx& owner, int nRow, int nCol, const CRect& rect)
{
	// Get the text-style of the cell to edit
	DWORD dwStyle = m_DateTimeCtrlStyle;
	HDITEM hd = {0};
	hd.mask = HDI_FORMAT;
	VERIFY( owner.GetHeaderCtrl()->GetItem(nCol, &hd) );
	if (hd.fmt & HDF_RIGHT)
		dwStyle = DTS_RIGHTALIGN;

	// Create control to edit the cell
	CDateTimeCtrl* pDateTimeCtrl = new CGridEditorDateTimeCtrl(nRow, nCol);
	VERIFY( pDateTimeCtrl->Create(WS_CHILD | dwStyle, rect, &owner, 0) );
	if (!owner.UsingVisualStyle())
		pDateTimeCtrl->ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE, SWP_FRAMECHANGED);	// Remove sunken edge

	// Configure font
	pDateTimeCtrl->SetFont(owner.GetCellFont());

	// Configure datetime format
	if (!m_Format.IsEmpty())
		pDateTimeCtrl->SetFormat(m_Format);

	return pDateTimeCtrl;
}

//------------------------------------------------------------------------
//! Overrides OnEditBegin() to provide a CDateTimeCtrl cell value editor
//!
//! @param owner The list control starting edit
//! @param nRow The index of the row for the cell to edit
//! @param nCol The index of the column for the cell to edit
//! @return Pointer to the cell editor to use (NULL if cell edit is not possible)
//------------------------------------------------------------------------
CWnd* CGridColumnTraitDateTime::OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol)
{
	// Convert cell-text to date/time format
	CString cellText = owner.GetItemText(nRow, nCol);

	COleDateTime dt;
	if(dt.ParseDateTime(cellText, m_ParseDateTimeFlags, m_ParseDateTimeLCID) == FALSE)
		dt.SetDateTime(1970, 1, 1, 0, 0, 0);

	// Get position of the cell to edit
	CRect rectCell = GetCellEditRect(owner, nRow, nCol);

	// Create control to edit the cell
	CDateTimeCtrl* pDateTimeCtrl = CreateDateTimeCtrl(owner, nRow, nCol, rectCell);
	VERIFY(pDateTimeCtrl != NULL);

	pDateTimeCtrl->SetTime(dt);

	// Check with the original string
	CString timeText;
	pDateTimeCtrl->GetWindowText(timeText);
	if (cellText != timeText)
	{
		dt.SetDateTime(1970, 1, 1, 0, 0, 0);
		pDateTimeCtrl->SetTime(dt);
	}

	return pDateTimeCtrl;
}

//------------------------------------------------------------------------
//! Compares two cell values according to specified sort order
//!
//! @param pszLeftValue Left cell value
//! @param pszRightValue Right cell value
//! @param bAscending Perform sorting in ascending or descending order
//! @return Is left value less than right value (-1) or equal (0) or larger (1)
//------------------------------------------------------------------------
int CGridColumnTraitDateTime::OnSortRows(LPCTSTR pszLeftValue, LPCTSTR pszRightValue, BOOL bAscending)
{
	COleDateTime leftDate;
	if(leftDate.ParseDateTime(pszLeftValue, m_ParseDateTimeFlags, m_ParseDateTimeLCID) == FALSE)
		leftDate.SetDateTime(1970, 1, 1, 0, 0, 0);

	COleDateTime rightDate;
	if(rightDate.ParseDateTime(pszRightValue, m_ParseDateTimeFlags, m_ParseDateTimeLCID) == FALSE)
		rightDate.SetDateTime(1970, 1, 1, 0, 0, 0);

	if (bAscending)
		return (int)(leftDate - rightDate);
	else
		return (int)(rightDate - leftDate);
}

//------------------------------------------------------------------------
// CGridEditorDateTimeCtrl (For internal use)
//------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CGridEditorDateTimeCtrl, CDateTimeCtrl)
	//{{AFX_MSG_MAP(CGridEditorDateTimeCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
	ON_NOTIFY_REFLECT(DTN_DATETIMECHANGE, OnDateTimeChange)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------------
//! CGridEditorDateTimeCtrl - Constructor
//!
//! @param nRow The index of the row
//! @param nCol The index of the column
//------------------------------------------------------------------------
CGridEditorDateTimeCtrl::CGridEditorDateTimeCtrl(int nRow, int nCol)
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
void CGridEditorDateTimeCtrl::EndEdit(BOOL bSuccess)
{
	// Avoid two messages if key-press is followed by kill-focus
	if (m_Completed)
		return;

	m_Completed = true;

	// Format time back to string
	CString str;
	GetWindowText(str);

	// Send Notification to parent of ListView ctrl
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
//! WM_KILLFOCUS message handler called when CDateTimeCtrl is loosing focus
//! to other control. Used register that cell value editor should close.
//!
//! @param pNewWnd Pointer to the window that receives the input focus (may be NULL or may be temporary).
//------------------------------------------------------------------------
void CGridEditorDateTimeCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CDateTimeCtrl::OnKillFocus(pNewWnd);
	if (GetMonthCalCtrl() == NULL)
		EndEdit(true);
}

//------------------------------------------------------------------------
//! WM_NCDESTROY message handler called when CDateTimeCtrl window is about to
//! be destroyed. Used to delete the inplace CDateTimeCtrl editor object as well.
//! This is necessary when the CDateTimeCtrl is created dynamically.
//------------------------------------------------------------------------
void CGridEditorDateTimeCtrl::OnNcDestroy()
{
	CDateTimeCtrl::OnNcDestroy();
	delete this;
}

//------------------------------------------------------------------------
//! WM_CHAR message handler to monitor date modifications
//!
//! @param nChar Specifies the virtual key code of the given key.
//! @param nRepCnt Repeat count (the number of times the keystroke is repeated as a result of the user holding down the key).
//! @param nFlags Specifies the scan code, key-transition code, previous key state, and context code
//------------------------------------------------------------------------
void CGridEditorDateTimeCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	m_Modified = true;
	CDateTimeCtrl::OnChar(nChar, nRepCnt, nFlags);
}

//------------------------------------------------------------------------
//! DTN_DATETIMECHANGE message handler to monitor date modifications
//!
//! @param pNMHDR Pointer to NMDATETIMECHANGE structure
//! @param pResult Must be set to zero
//------------------------------------------------------------------------
void CGridEditorDateTimeCtrl::OnDateTimeChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_Modified = true;
	*pResult = 0;
}

//------------------------------------------------------------------------
//! Hook to proces windows messages before they are dispatched.
//! Catch keyboard events that can should cause the cell value editor to close
//!
//! @param pMsg Points to a MSG structure that contains the message to process
//! @return Nonzero if the message was translated and should not be dispatched; 0 if the message was not translated and should be dispatched.
//------------------------------------------------------------------------
BOOL CGridEditorDateTimeCtrl::PreTranslateMessage(MSG* pMsg)
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
	return CDateTimeCtrl::PreTranslateMessage(pMsg);
}
