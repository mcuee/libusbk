#pragma once

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "CGridColumnTraitImage.h"

//------------------------------------------------------------------------
//! CGridColumnTraitDateTime implements a CDateTimeCtrl as cell-editor
//------------------------------------------------------------------------
class CGridColumnTraitDateTime : public CGridColumnTraitImage
{
public:
	CGridColumnTraitDateTime();

	void SetFormat(const CString& strFormat);
	CString GetFormat() const;

	void SetStyle(DWORD dwStyle);
	DWORD GetStyle() const;

	void SetParseDateTime(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT);

	virtual CWnd* OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol);
	virtual int OnSortRows(LPCTSTR pszLeftValue, LPCTSTR pszRightValue, BOOL bAscending);

protected:
	virtual void Accept(CGridColumnTraitVisitor& visitor);
	virtual CDateTimeCtrl* CreateDateTimeCtrl(CGridListCtrlEx& owner, int nRow, int nCol, const CRect& rect);

	CString m_Format;				//!< DateTime format used to display the date
	DWORD	m_ParseDateTimeFlags;	//!< Flags for locale settings and parsing (COleDateTime::ParseDateTime)
	LCID	m_ParseDateTimeLCID;	//!< Locale ID to use for the conversion. (COleDateTime::ParseDateTime)
	DWORD	m_DateTimeCtrlStyle;	//!< Style to use when creating CDateTimeCtrl
};

//------------------------------------------------------------------------
//! CDateTimeCtrl for inplace edit. For internal use by CGridColumnTraitDateTime
//------------------------------------------------------------------------
class CGridEditorDateTimeCtrl : public CDateTimeCtrl
{
public:
	CGridEditorDateTimeCtrl(int nRow, int nCol);

protected:
	virtual void EndEdit(BOOL bSuccess);

	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnDateTimeChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	BOOL	m_Completed;			//!< Ensure the editor only reacts to a single close event
	BOOL	m_Modified;				//!< Register if date was modified while the editor was open
	int		m_Row;					//!< The index of the row being edited
	int		m_Col;					//!< The index of the column being edited

	DECLARE_MESSAGE_MAP();
};