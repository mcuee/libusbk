#pragma once

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "CGridColumnTrait.h"

//------------------------------------------------------------------------
//! CGridColumnTraitText provides customization of cell text and background
//------------------------------------------------------------------------
class CGridColumnTraitText : public CGridColumnTrait
{
public:
	CGridColumnTraitText();
	virtual void OnCustomDraw(CGridListCtrlEx& owner, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult);
	virtual int OnSortRows(LPCTSTR pszLeftValue, LPCTSTR pszRightValue, BOOL bAscending);

	void SetSortFormatNumber(BOOL bValue);

protected:
	CFont*	m_pOldFont;		//!< Backup of the original font while drawing with specified font
	COLORREF m_OldTextColor;//!< Backup of the original text color while drawing with specified color
	COLORREF m_OldBackColor;//!< Backup of the original background color while drawing with specified color
	COLORREF m_TextColor;	//!< Text color to use for this column
	COLORREF m_BackColor;	//!< Background color to use for this column
	BOOL m_SortFormatNumber;//!< Column contains integers

	virtual BOOL UpdateTextColor(COLORREF& textColor);
	virtual BOOL UpdateBackColor(COLORREF& backColor);

	virtual void Accept(CGridColumnTraitVisitor& visitor);
	virtual int GetCellFontHeight(CGridListCtrlEx& owner);
	virtual CRect GetCellEditRect(CGridListCtrlEx& owner, int nRow, int nCol);
};
