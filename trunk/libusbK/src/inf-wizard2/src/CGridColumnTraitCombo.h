#pragma once

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "CGridColumnTraitImage.h"

//------------------------------------------------------------------------
//! CGridColumnTraitCombo implements a CComboBox as cell-editor
//------------------------------------------------------------------------
class CGridColumnTraitCombo : public CGridColumnTraitImage
{
public:
	CGridColumnTraitCombo();

	void SetMaxItems(int nMaxItems);
	int  GetMaxItems() const;

	void SetStyle(DWORD dwStyle);
	DWORD GetStyle() const;

	void SetMaxWidth(int nMaxWidth);
	int  GetMaxWidth() const;

	void LoadList(const CSimpleMap<int, CString>& comboList, int nCurSel);
	void AddItem(int nItemData, const CString& strItemText);

	virtual CWnd* OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol);
	virtual void  OnEditEnd();

protected:
	virtual void Accept(CGridColumnTraitVisitor& visitor);
	virtual CComboBox* CreateComboBox(CGridListCtrlEx& owner, int nRow, int nCol, const CRect& rect);

	CSimpleMap<int, CString> m_ComboList;	//!< Fixed list of items in the combo-box
	CComboBox* m_pComboBox;					//!< CComboBox currently open
	DWORD m_ComboBoxStyle;					//!< Style to use when creating CComboBox
	int m_MaxItems;							//!< Max height (in items) of the CComboBox when doing dropdown
	int m_MaxWidth;							//!< Max width (in pixels) of the CComboBox when doing dropdown

private:
	// Private because they doesn't handle CSimpleMap
	CGridColumnTraitCombo(const CGridColumnTraitCombo&);
	CGridColumnTraitCombo& operator=(const CGridColumnTraitCombo& other);
};

//------------------------------------------------------------------------
//! CEdit inside CComboBox for inplace edit. For internal use by CGridColumnTraitCombo
//
// Taken from "MFC Grid control" credits Chris Maunder
// http://www.codeproject.com/KB/miscctrl/gridctrl.aspx
//------------------------------------------------------------------------
class CGridEditorComboBoxEdit : public CEdit
{
protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	DECLARE_MESSAGE_MAP();
};

//------------------------------------------------------------------------
//! CComboBox for inplace edit. For internal use by CGridColumnTraitCombo
//
// Taken from "MFC Grid control" credits Chris Maunder
// http://www.codeproject.com/KB/miscctrl/gridctrl.aspx
//------------------------------------------------------------------------
class CGridEditorComboBox : public CComboBox
{
public:
	CGridEditorComboBox(int nRow, int nCol, int nMaxWidthPixels, int nMaxHeightItems);

	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	virtual void EndEdit(BOOL bSuccess);

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnDestroy();
	afx_msg void OnDropDown();
	afx_msg void OnCloseUp();
	afx_msg void OnChangeSelection();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP();

	CGridEditorComboBoxEdit m_Edit;	//!< Subclassed edit control inside the CComboBox
	BOOL	m_Completed;			//!< Ensure the editor only reacts to a single close event
	BOOL	m_Modified;				//!< Register if selection was modified while the editor was open
	int		m_Row;					//!< The index of the row being edited
	int		m_Col;					//!< The index of the column being edited
	int		m_MaxWidthPixels;		//!< Max width (in pixels) of the CComboBox when doing dropdown
	int		m_MaxHeightItems;		//!< Max height (in items) of the CComboBox when doing dropdown
};
