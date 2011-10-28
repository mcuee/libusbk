#pragma once

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "CGridColumnTraitImage.h"

//------------------------------------------------------------------------
//! CGridColumnTraitEdit implements a CEdit as cell-editor
//------------------------------------------------------------------------
class CGridColumnTraitEdit : public CGridColumnTraitImage
{
public:
	CGridColumnTraitEdit();

	void SetStyle(DWORD dwStyle);
	DWORD GetStyle() const;

	void SetLimitText(UINT nMax);
	UINT GetLimitText() const;

	virtual CWnd* OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol);

protected:
	virtual void Accept(CGridColumnTraitVisitor& visitor);
	virtual CEdit* CreateEdit(CGridListCtrlEx& owner, int nRow, int nCol, const CRect& rect);

	DWORD m_EditStyle;				//!< Style to use when creating CEdit
	UINT m_EditLimitText;			//!< Max number of characters the CEdit will accept
};

//------------------------------------------------------------------------
//! CEdit for inplace edit. For internal use by CGridColumnTraitEdit
//------------------------------------------------------------------------
class CGridEditorText : public CEdit
{
public:
	CGridEditorText(int nRow, int nCol);
	virtual void EndEdit(BOOL bSuccess);

protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual	BOOL PreTranslateMessage(MSG* pMsg);

	int		m_Row;					//!< The index of the row being edited
	int		m_Col;					//!< The index of the column being edited
	BOOL	m_Completed;			//!< Ensure the editor only reacts to a single close event
	BOOL	m_Modified;				//!< Register if text was modified while the editor was open

	DECLARE_MESSAGE_MAP();
};