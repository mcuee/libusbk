#pragma once

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "CGridColumnTraitText.h"

//------------------------------------------------------------------------
//! CGridColumnTraitImage implements an image switcher (can mimic a checkbox)
//!
//! By adding checkbox state-images to the official imagelist using
//! AppendStateImages(), then one can use this column trait as checkbox
//! editor. To get/set the checkbox value of a cell use the methods
//! GetCellImage()/SetCellImage() on CGridListCtrlEx
//------------------------------------------------------------------------
class CGridColumnTraitImage : public CGridColumnTraitText
{
public:
	CGridColumnTraitImage();
	CGridColumnTraitImage(int nImageIndex, int nImageCount);

	void AddImageIndex(int nImageIdx);
	void AddImageIndex(int nImageIdx, const CString& strImageText, BOOL bEditable = true);

	void SetImageText(int nImageIdx, const CString& strImageText, BOOL bEditable = true);

	void SetSortImageIndex(BOOL bValue);
	void SetToggleSelection(BOOL bValue);

	static int AppendStateImages(CGridListCtrlEx& owner, CImageList& imagelist);

protected:
	virtual int OnSortRows(LPCTSTR pszLeftValue, LPCTSTR pszRightValue, BOOL bAscending)
	{
		return CGridColumnTraitText::OnSortRows(pszLeftValue, pszRightValue, bAscending);
	}
	virtual int OnSortRows(const LVITEM& leftItem, const LVITEM& rightItem, BOOL bAscending);
	virtual BOOL IsCellReadOnly(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt) const;
	virtual int OnClickEditStart(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt, BOOL bDblClick);
	virtual CWnd* OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol)
	{
		return CGridColumnTraitText::OnEditBegin(owner, nRow, nCol);
	}
	virtual CWnd* OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt);
	virtual void Accept(CGridColumnTraitVisitor& visitor);
	virtual int FlipImageIndex(CGridListCtrlEx& owner, int nRow, int nCol);

	//! @cond INTERNAL
	struct ImageCell
	{
		CString m_CellText;
		BOOL m_Editable;

		ImageCell()
			: m_Editable(true) {}
		ImageCell(const CString& cellText, BOOL editable)
			: m_CellText(cellText), m_Editable(editable) {}
	};
	//! @endcond INTERNAL

	CSimpleMap<int, ImageCell> m_ImageIndexes;	//!< Fixed list of image items to switch between

	BOOL m_SortImageIndex;	//!< Should image be used as primary sort index ?
	BOOL m_ToggleSelection;	//!< Should the image of all selected rows be flipped, when clicked ?
};