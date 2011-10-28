//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:4100)	// unreferenced formal parameter

#include "CGridColumnTraitText.h"

#include "CGridColumnTraitVisitor.h"
#include "CGridListCtrlEx.h"

//------------------------------------------------------------------------
//! CGridColumnTraitText - Constructor
//------------------------------------------------------------------------
CGridColumnTraitText::CGridColumnTraitText()
	:m_pOldFont(NULL)
	,m_TextColor(COLORREF(-1))
	,m_BackColor(COLORREF(-1))
	,m_SortFormatNumber(false)
	,m_OldTextColor(COLORREF(-1))
	,m_OldBackColor(COLORREF(-1))
{}

//------------------------------------------------------------------------
//! Accept Visitor Pattern
//------------------------------------------------------------------------
void CGridColumnTraitText::Accept(CGridColumnTraitVisitor& visitor)
{
	visitor.Visit(*this);
}

//------------------------------------------------------------------------
//! Changes the text color if one is specified
//!
//! @param textColor Current text color
//! @return New text color was specified (true / false)
//------------------------------------------------------------------------
BOOL CGridColumnTraitText::UpdateTextColor(COLORREF& textColor)
{
	if (m_TextColor!=COLORREF(-1))
	{
		textColor = m_TextColor;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
//! Changes the background color if one is specified
//!
//! @param backColor Current background color
//! @return New background color was specified (true / false)
//------------------------------------------------------------------------
BOOL CGridColumnTraitText::UpdateBackColor(COLORREF& backColor)
{
	if (m_BackColor!=COLORREF(-1))
	{
		backColor = m_BackColor;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
//! Overrides the custom draw handler, to allow custom coloring of cells
//! with this column trait.
//!
//! @param owner The list control drawing
//! @param pLVCD Pointer to NMLVCUSTOMDRAW structure
//! @param pResult Modification to the drawing stage (CDRF_NEWFONT, etc.)
//------------------------------------------------------------------------
void CGridColumnTraitText::OnCustomDraw(CGridListCtrlEx& owner, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult)
{
	int nRow = (int)pLVCD->nmcd.dwItemSpec;

	switch (pLVCD->nmcd.dwDrawStage)
	{
		// Before painting a cell
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
	{
		int nCol = pLVCD->iSubItem;

		m_OldTextColor = pLVCD->clrText;
		m_OldBackColor = pLVCD->clrTextBk;

		// Only change cell colors when not selected
		if (UpdateTextColor(pLVCD->clrText))
			*pResult |= CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;

		if (UpdateBackColor(pLVCD->clrTextBk))
			*pResult |= CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;

		if (owner.OnDisplayCellColor(nRow, nCol, pLVCD->clrText, pLVCD->clrTextBk))
			*pResult |= CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;

		LOGFONT newFont = {0};
		if (owner.OnDisplayCellFont(nRow, nCol, newFont))
		{
			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			CFont* pNewFont = new CFont;
			VERIFY( pNewFont->CreateFontIndirect(&newFont) );
			m_pOldFont = pDC->SelectObject(pNewFont);
			*pResult |= CDRF_NOTIFYPOSTPAINT;	// We need to restore the original font
			*pResult |= CDRF_NEWFONT;
		}
	}
	break;

	// After painting a cell
	case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
	{
		if (m_pOldFont!=NULL)
		{
			// Restore the original font
			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			CFont* pNewFont = pDC->SelectObject(m_pOldFont);
			delete pNewFont;
		}

		pLVCD->clrText = m_OldTextColor;
		pLVCD->clrTextBk = m_OldBackColor;
		*pResult |= CDRF_NEWFONT;
	}
	break;
	}
}

//------------------------------------------------------------------------
//! Compares two cell values according to specified sort order
//!
//! @param pszLeftValue Left cell value
//! @param pszRightValue Right cell value
//! @param bAscending Perform sorting in ascending or descending order
//! @return Is left value less than right value (-1) or equal (0) or larger (1)
//------------------------------------------------------------------------
int CGridColumnTraitText::OnSortRows(LPCTSTR pszLeftValue, LPCTSTR pszRightValue, BOOL bAscending)
{
	if (m_SortFormatNumber)
	{
		int nLeftValue = _ttoi(pszLeftValue);
		int nRightValue = _ttoi(pszRightValue);
		if (bAscending)
			return nLeftValue - nRightValue;
		else
			return nRightValue - nLeftValue;
	}
	else
	{
		if (bAscending)
			return _tcscmp( pszLeftValue, pszRightValue );
		else
			return _tcscmp( pszRightValue, pszLeftValue );
	}
}

//------------------------------------------------------------------------
//! Should cell values be compared as numbers when sorting
//!
//! @param bValue Enabled / Disabled
//------------------------------------------------------------------------
void CGridColumnTraitText::SetSortFormatNumber(BOOL bValue)
{
	m_SortFormatNumber = bValue;
}

//------------------------------------------------------------------------
//! Calculates the proper row-height according to font, which a cell value
//! editor should fit in.
//!
//! @param owner The list control for the inplace cell value editor
//! @return Height in pixels of the row.
//------------------------------------------------------------------------
int CGridColumnTraitText::GetCellFontHeight(CGridListCtrlEx& owner)
{
	const CString testText = _T("yjpÍÁ");

	CRect rcRequired = CRect(0,0,0,0);

	CClientDC dc(&owner);
	dc.SelectObject(owner.GetCellFont());
	dc.DrawText(testText, &rcRequired, DT_CALCRECT|DT_SINGLELINE);

	return rcRequired.Height();
}

//------------------------------------------------------------------------
//! Returns the proper rectangle, which a cell value editor should fit in
//!
//! @param owner The list control for the inplace cell value editor
//! @param nRow The index of the row
//! @param nCol The index of the column
//! @return Rectangle where the inplace cell value editor should be placed.
//------------------------------------------------------------------------
CRect CGridColumnTraitText::GetCellEditRect(CGridListCtrlEx& owner, int nRow, int nCol)
{
	// Get position of the cell to edit
	CRect rectCell;
	VERIFY( owner.GetCellRect(nRow, nCol, LVIR_LABEL, rectCell) );

	// Adjust cell rectangle according to grid-lines
	if (owner.GetExtendedStyle() & LVS_EX_GRIDLINES)
		rectCell.bottom -= ::GetSystemMetrics(SM_CXBORDER);

	if (owner.GetExtendedStyle() & LVS_EX_SUBITEMIMAGES)
	{
		// Add margin to cell image
		if (owner.GetImageList(LVSIL_SMALL)!=NULL && owner.GetCellImage(nRow,nCol)!=I_IMAGECALLBACK)
			rectCell.left += ::GetSystemMetrics(SM_CXBORDER);
	}

	// Check if there is enough room for normal margin
	int requiredHeight = GetCellFontHeight(owner);
	requiredHeight += 2*::GetSystemMetrics(SM_CXEDGE);
	if (requiredHeight > rectCell.Height())
		rectCell.bottom = rectCell.top + requiredHeight;

	return rectCell;
}