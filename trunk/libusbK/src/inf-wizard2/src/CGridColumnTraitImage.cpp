//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:4100)	// unreferenced formal parameter

#include "CGridColumnTraitImage.h"

#include "CGridColumnTraitVisitor.h"
#include "CGridListCtrlEx.h"

//------------------------------------------------------------------------
//! CGridColumnTraitImage - Constructor
//------------------------------------------------------------------------
CGridColumnTraitImage::CGridColumnTraitImage()
	:m_SortImageIndex(false)
	,m_ToggleSelection(false)
{
}

//------------------------------------------------------------------------
//! CGridColumnTraitImage - Constructor
//!
//! @param nImageIndex The first index in list control imagelist
//! @param nImageCount The number of images to switch between in the imagelist
//------------------------------------------------------------------------
CGridColumnTraitImage::CGridColumnTraitImage(int nImageIndex, int nImageCount)
	:m_SortImageIndex(false)
	,m_ToggleSelection(false)
{
	for(int i = nImageIndex; i < nImageIndex + nImageCount; ++i)
		AddImageIndex(i);
}

//------------------------------------------------------------------------
//! Should primary sorting be based on the image index (checkbox sorting)
//!
//! @param bValue Enabled / Disabled
//------------------------------------------------------------------------
void CGridColumnTraitImage::SetSortImageIndex(BOOL bValue)
{
	m_SortImageIndex = bValue;
}

//------------------------------------------------------------------------
//! Should images (checkboxes) be flipped for all selected rows, when
//! icon is clicked.
//!
//! @param bValue Enabled / Disabled
//------------------------------------------------------------------------
void CGridColumnTraitImage::SetToggleSelection(BOOL bValue)
{
	m_ToggleSelection = bValue;
}

//------------------------------------------------------------------------
//! Adds image index to the list of images to switch between
//!
//! @param nImageIdx The index of the image in the list control imagelist
//------------------------------------------------------------------------
void CGridColumnTraitImage::AddImageIndex(int nImageIdx)
{
	m_ImageIndexes.Add(nImageIdx, ImageCell());
}

//------------------------------------------------------------------------
//! Adds image index to the list of images to switch between
//!
//! @param nImageIdx The index of the image in the list control imagelist
//! @param strImageText The associated cell text to the image
//! @param bEditable Is the cell editable when this image is displayed
//------------------------------------------------------------------------
void CGridColumnTraitImage::AddImageIndex(int nImageIdx, const CString& strImageText, BOOL bEditable)
{
	m_ImageIndexes.Add(nImageIdx, ImageCell(strImageText,bEditable));
}

//------------------------------------------------------------------------
//! Updates the image text for the specified image index
//!
//! @param nImageIdx The index of the image in the list control imagelist
//! @param strImageText The associated cell text to the image
//! @param bEditable Is the cell editable when this image is displayed
//------------------------------------------------------------------------
void CGridColumnTraitImage::SetImageText(int nImageIdx, const CString& strImageText, BOOL bEditable)
{
	int nIndex = m_ImageIndexes.FindKey(nImageIdx);
	if (nIndex==-1)
		AddImageIndex(nImageIdx, strImageText, bEditable);
	else
		m_ImageIndexes.GetValueAt(nIndex) = ImageCell(strImageText,bEditable);
}

//------------------------------------------------------------------------
//! Accept Visitor Pattern
//------------------------------------------------------------------------
void CGridColumnTraitImage::Accept(CGridColumnTraitVisitor& visitor)
{
	visitor.Visit(*this);
}

//------------------------------------------------------------------------
//! Appends the checkbox state images to the list control image list
//!
//! @param owner The list control adding column
//! @param imagelist The image list assigned to the list control
//! @return Image index where the two state images (unchecked/checked) was inserted
//------------------------------------------------------------------------
int CGridColumnTraitImage::AppendStateImages(CGridListCtrlEx& owner, CImageList& imagelist)
{
	if (!(owner.GetExtendedStyle() & LVS_EX_SUBITEMIMAGES))
		owner.SetExtendedStyle(owner.GetExtendedStyle() | LVS_EX_SUBITEMIMAGES);

	if (!imagelist)
		imagelist.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 0);

	if (!owner.GetImageList(LVSIL_SMALL))
		owner.SetImageList(&imagelist, LVSIL_SMALL);

	VERIFY( owner.GetImageList(LVSIL_SMALL)==&imagelist );

	BOOL createdStateImages = false;
	CImageList* pStateList = owner.GetImageList(LVSIL_STATE);
	if (pStateList==NULL)
	{
		if (!(owner.GetExtendedStyle() & LVS_EX_CHECKBOXES))
		{
			createdStateImages = true;
			owner.SetExtendedStyle(owner.GetExtendedStyle() | LVS_EX_CHECKBOXES);
			pStateList = owner.GetImageList(LVSIL_STATE);
		}
	}
	int imageCount = -1;
	ASSERT(pStateList!=NULL);
	if (pStateList!=NULL)
	{
		imageCount = imagelist.GetImageCount();
		HICON uncheckedIcon = pStateList->ExtractIcon(0);
		imagelist.Add(uncheckedIcon);
		DestroyIcon(uncheckedIcon);
		HICON checkedIcon = pStateList->ExtractIcon(1);
		imagelist.Add(checkedIcon);
		DestroyIcon(checkedIcon);
	}
	if (createdStateImages)
		owner.SetExtendedStyle(owner.GetExtendedStyle() & ~LVS_EX_CHECKBOXES);

	return imageCount;
}

//------------------------------------------------------------------------
//! Compares two cell values according to checkbox state
//!
//! @param leftItem Left cell item
//! @param rightItem Right cell item
//! @param bAscending Perform sorting in ascending or descending order
//! @return Is left value less than right value (-1) or equal (0) or larger (1)
//------------------------------------------------------------------------
int CGridColumnTraitImage::OnSortRows(const LVITEM& leftItem, const LVITEM& rightItem, BOOL bAscending)
{
	if (m_SortImageIndex)
	{
		int imageSort = bAscending ? leftItem.iImage - rightItem.iImage : rightItem.iImage - leftItem.iImage;
		if (imageSort != 0)
			return imageSort;
	}
	return OnSortRows(leftItem.pszText, rightItem.pszText, bAscending);
}

//------------------------------------------------------------------------
//! Check if current image index blocks for editing of cell label
//!
//! @param owner The list control starting edit
//! @param nRow The index of the row for the cell
//! @param nCol The index of the column for the cell
//! @param pt The position clicked, in client coordinates.
//! @return Is cell read only ? (true / false)
//------------------------------------------------------------------------
BOOL CGridColumnTraitImage::IsCellReadOnly(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt) const
{
	if (!m_ColumnState.m_Editable)
		return true;

	// Check if current cell image blocks for starting cell editor
	if (m_ImageIndexes.GetSize()!=0)
	{
		int nCurImageIdx = -1;
		for(int i=0; i < m_ImageIndexes.GetSize(); ++i)
		{
			if (!m_ImageIndexes.GetValueAt(i).m_Editable)
			{
				if (nCurImageIdx==-1)
				{
					if (pt!=CPoint(-1,-1))
					{
						CRect rect;
						VERIFY( owner.GetCellRect(nRow, nCol, LVIR_LABEL, rect) );
						if (!rect.PtInRect(pt))
							break;
					}

					nCurImageIdx = owner.GetCellImage(nRow, nCol);
					if (nCurImageIdx==-1)
						break;
				}
				if (nCurImageIdx==m_ImageIndexes.GetKeyAt(i))
					return true;
			}
		}
	}

	return false;	// editable
}

//------------------------------------------------------------------------
//! Check if the cell is editable when clicked
//!
//! @param owner The list control being clicked
//! @param nRow The index of the row
//! @param nCol The index of the column
//! @param pt The position clicked, in client coordinates.
//! @param bDblClick Whether the position was double clicked
//! @return How should the cell editor be started (0 = No editor, 1 = Start Editor, 2 = Start Editor and block click-event)
//------------------------------------------------------------------------
int CGridColumnTraitImage::OnClickEditStart(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt, BOOL bDblClick)
{
	// Begin edit if the cell has focus already
	BOOL startEdit = nRow!=-1 && nCol!=-1 && owner.GetFocusRow()==nRow && owner.GetFocusCell()==nCol && !bDblClick;

	// Check if the cell can be edited without having focus first
	if (m_ToggleSelection)
	{
		if (nCol==0 && owner.GetExtendedStyle() & LVS_EX_CHECKBOXES)
		{
			CRect iconRect;
			if (!owner.GetCellRect(nRow, nCol, LVIR_ICON, iconRect) || !iconRect.PtInRect(pt))
			{
				CRect labelRect;
				if (owner.GetCellRect(nRow, nCol, LVIR_LABEL, labelRect) && !labelRect.PtInRect(pt))
					return 1;	// Clicked the checkbox for the label-column
			}
		}

		if (m_ImageIndexes.GetSize()==0)
			return startEdit ? 1 : 0;	// No images to flip between

		CRect iconRect;
		if (!owner.GetCellRect(nRow, nCol, LVIR_ICON, iconRect) || !iconRect.PtInRect(pt))
			return startEdit ? 1 : 0;	// Didn't click the image icon

		return 2;	// Don't change focus or change selection
	}

	return startEdit ? 1 : 0;
}

//------------------------------------------------------------------------
//! Switch to the next image index
//!
//! @param owner The list control starting edit
//! @param nRow The index of the row for the cell to edit
//! @param nCol The index of the column for the cell to edit
//! @return New image index (-1 if no new image)
//------------------------------------------------------------------------
int CGridColumnTraitImage::FlipImageIndex(CGridListCtrlEx& owner, int nRow, int nCol)
{
	if (m_ImageIndexes.GetSize()==0)
		return -1;

	int nImageIdx = owner.GetCellImage(nRow, nCol);
	int nOldImagePos = -1;
	for(int i=0; i < m_ImageIndexes.GetSize(); ++i)
	{
		if (m_ImageIndexes.GetKeyAt(i)==nImageIdx)
		{
			nOldImagePos = i;
			break;
		}
	}
	if (nOldImagePos==-1)
		return -1;

	int nNewImageIdx = -1;
	if (nOldImagePos+1 == m_ImageIndexes.GetSize())
		nNewImageIdx = m_ImageIndexes.GetKeyAt(0);
	else
		nNewImageIdx = m_ImageIndexes.GetKeyAt(nOldImagePos+1);

	return nNewImageIdx;
}

//------------------------------------------------------------------------
//! Reacts to clicking on the cell image, and switch to the next image index
//!
//! @param owner The list control starting edit
//! @param nRow The index of the row for the cell to edit
//! @param nCol The index of the column for the cell to edit
//! @param pt The position clicked, in client coordinates.
//! @return Pointer to the cell editor to use (NULL if cell edit is not possible)
//------------------------------------------------------------------------
CWnd* CGridColumnTraitImage::OnEditBegin(CGridListCtrlEx& owner, int nRow, int nCol, CPoint pt)
{
	// Check if the user used a shortcut key to edit the label
	if (pt==CPoint(-1,-1))
		return OnEditBegin(owner, nRow, nCol);

	// Check if mouse click was inside the label-part of the cell
	CRect labelRect;
	if (owner.GetCellRect(nRow, nCol, LVIR_LABEL, labelRect) && labelRect.PtInRect(pt))
		return OnEditBegin(owner, nRow, nCol);

	CRect iconRect;
	if (owner.GetCellRect(nRow, nCol, LVIR_ICON, iconRect) && iconRect.PtInRect(pt))
	{
		int nOldImageIdx = owner.GetCellImage(nRow, nCol);
		int nNewImageIdx = FlipImageIndex(owner, nRow, nCol);
		if (nNewImageIdx == -1)
			return NULL;

		CString strOldImageText, strNewImageText;
		for(int i=0; i < m_ImageIndexes.GetSize(); ++i)
		{
			if (m_ImageIndexes.GetKeyAt(i)==nOldImageIdx)
				strOldImageText = m_ImageIndexes.GetValueAt(i).m_CellText;
			if (m_ImageIndexes.GetKeyAt(i)==nNewImageIdx)
				strNewImageText = m_ImageIndexes.GetValueAt(i).m_CellText;
		}

		// Send Notification to parent of ListView ctrl
		LV_DISPINFO dispinfo = {0};
		dispinfo.hdr.hwndFrom = owner.m_hWnd;
		dispinfo.hdr.idFrom = owner.GetDlgCtrlID();
		dispinfo.hdr.code = LVN_ENDLABELEDIT;

		dispinfo.item.iItem = nRow;
		dispinfo.item.iSubItem = nCol;
		dispinfo.item.mask = LVIF_IMAGE;
		dispinfo.item.iImage = nNewImageIdx;

		if (strNewImageText!=strOldImageText)
		{
			dispinfo.item.mask |= LVIF_TEXT;
			dispinfo.item.pszText = strNewImageText.GetBuffer(0);
			dispinfo.item.cchTextMax = strNewImageText.GetLength();
		}

		owner.GetParent()->SendMessage( WM_NOTIFY, owner.GetDlgCtrlID(), (LPARAM)&dispinfo );

		// Toggle all selected rows to the same image index as the one clicked
		if (m_ToggleSelection)
		{
			// The click event for check-boxes doesn't change selection or focus
			if (owner.IsRowSelected(nRow))
			{
				POSITION pos = owner.GetFirstSelectedItemPosition();
				while(pos!=NULL)
				{
					int nSelectedRow = owner.GetNextSelectedItem(pos);
					if (nSelectedRow==nRow)
						continue;	// Don't flip the clicked row

					int nOldImageIdx = owner.GetCellImage(nSelectedRow, nCol);
					if (nOldImageIdx==nNewImageIdx)
						continue;	// Already flipped

					// Send Notification to parent of ListView ctrl
					LV_DISPINFO dispinfo = {0};
					dispinfo.hdr.hwndFrom = owner.m_hWnd;
					dispinfo.hdr.idFrom = owner.GetDlgCtrlID();
					dispinfo.hdr.code = LVN_ENDLABELEDIT;

					dispinfo.item.iItem = nSelectedRow;
					dispinfo.item.iSubItem = nCol;
					dispinfo.item.mask = LVIF_IMAGE;
					dispinfo.item.iImage = nNewImageIdx;

					if (strNewImageText!=strOldImageText)
					{
						dispinfo.item.mask |= LVIF_TEXT;
						dispinfo.item.pszText = strNewImageText.GetBuffer(0);
						dispinfo.item.cchTextMax = strNewImageText.GetLength();
					}

					owner.GetParent()->SendMessage( WM_NOTIFY, owner.GetDlgCtrlID(), (LPARAM)&dispinfo );
				}
			}
		}
	}
	else if (nCol==0 && m_ToggleSelection && owner.GetExtendedStyle() & LVS_EX_CHECKBOXES)
	{
		// Check if we should toggle the label-column checkboxes for all the selected rows
		CRect labelRect;
		if (owner.GetCellRect(nRow, nCol, LVIR_LABEL, labelRect) && !labelRect.PtInRect(pt))
		{
			// The click event for check-boxes doesn't change selection or focus
			if (owner.IsRowSelected(nRow))
			{
				BOOL bChecked = FALSE;
				if (owner.GetStyle() & LVS_OWNERDATA)
					bChecked = owner.OnOwnerDataDisplayCheckbox(nRow) ? TRUE : FALSE;
				else
					bChecked = owner.GetCheck(nRow);	// The clicked row have already been changed by the click-event. We flip the other rows

				POSITION pos = owner.GetFirstSelectedItemPosition();
				while(pos!=NULL)
				{
					int nSelectedRow = owner.GetNextSelectedItem(pos);
					if (nSelectedRow==nRow)
						continue;	// Don't flip the clicked row

					if (owner.GetStyle() & LVS_OWNERDATA)
					{
						BOOL bSelChecked = owner.OnOwnerDataDisplayCheckbox(nSelectedRow) ? TRUE : FALSE;
						if (bChecked==bSelChecked)
							continue;	// Already flipped
					}
					else
					{
						if (owner.GetCheck(nSelectedRow)==bChecked)
							continue;	// Already flipped
					}

					if (owner.GetStyle() & LVS_OWNERDATA)
						owner.OnOwnerDataToggleCheckBox(nSelectedRow);
					else
						owner.SetCheck(nSelectedRow, bChecked);
				}
			}
		}
	}
	return NULL;	// Editor is never really started
}