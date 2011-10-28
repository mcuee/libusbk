//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
//------------------------------------------------------------------------

#include "stdafx.h"
#include "CGridListCtrlGroups.h"

#include <shlwapi.h>	// IsCommonControlsEnabled

#pragma warning(disable:4100)	// unreferenced formal parameter

#include "CGridColumnTrait.h"

// WIN32 defines for group-support is only available from 2003 PSDK
#if _WIN32_WINNT >= 0x0501

// If using VS2008 then the MFC libary will complain that group mode is only available for Unicode builds
//	- By default CGridListCtrlGroups will be disabled in non-Unicode mode
//  - Define CGRIDLISTCTRLEX_GROUPMODE in stdafx.h to force group mode in non-unicode builds
#if _MSC_VER < 1500 || defined UNICODE || defined CGRIDLISTCTRLEX_GROUPMODE

#if _MSC_VER >= 1500 && defined CGRIDLISTCTRLEX_GROUPMODE
#pragma warning(disable:4996)
#endif

BEGIN_MESSAGE_MAP(CGridListCtrlGroups, CGridListCtrlEx)
#if _WIN32_WINNT >= 0x0600
	ON_NOTIFY_REFLECT_EX(LVN_LINKCLICK, OnGroupTaskClick)	// Task-Link Click
	ON_NOTIFY_REFLECT_EX(LVN_GETEMPTYMARKUP, OnGetEmptyMarkup)	// Request text to display when empty
#endif
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

//------------------------------------------------------------------------
//! Constructor
//------------------------------------------------------------------------
CGridListCtrlGroups::CGridListCtrlGroups()
	: m_GroupHeight(-1)
{}

//------------------------------------------------------------------------
//! Inserts a group into the list view control.
//!
//! @param nIndex The insert position of the group
//! @param nGroupId ID of the new group
//! @param strHeader The group header title
//! @param dwState Specifies the state of the group when inserted
//! @param dwAlign Indicates the alignment of the header or footer text for the group
//! @return Returns the index of the item that the group was added to, or -1 if the operation failed.
//------------------------------------------------------------------------
LRESULT CGridListCtrlGroups::InsertGroupHeader(int nIndex, int nGroupId, const CString& strHeader, DWORD dwState /* = LVGS_NORMAL */, DWORD dwAlign /*= LVGA_HEADER_LEFT*/)
{
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.iGroupId = nGroupId;
	lg.state = dwState;
	lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN;
	lg.uAlign = dwAlign;

	// Header-title must be unicode (Convert if necessary)
#ifdef UNICODE
	lg.pszHeader = (LPWSTR)(LPCTSTR)strHeader;
	lg.cchHeader = strHeader.GetLength();
#else
	CComBSTR header = strHeader;
	lg.pszHeader = header;
	lg.cchHeader = header.Length();
#endif

	return InsertGroup(nIndex, (PLVGROUP)&lg );
}

//------------------------------------------------------------------------
//! Moves a row into a group
//!
//! @param nRow The index of the row
//! @param nGroupId ID of the group
//! @return Nonzero if successful; otherwise zero
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetRowGroupId(int nRow, int nGroupId)
{
	//OBS! Rows not assigned to a group will not show in group-view
	LVITEM lvItem = {0};
	lvItem.mask = LVIF_GROUPID;
	lvItem.iItem = nRow;
	lvItem.iSubItem = 0;
	lvItem.iGroupId = nGroupId;
	return SetItem( &lvItem );
}

//------------------------------------------------------------------------
//! Retrieves the group id of a row
//!
//! @param nRow The index of the row
//! @return ID of the group
//------------------------------------------------------------------------
int CGridListCtrlGroups::GetRowGroupId(int nRow)
{
	LVITEM lvi = {0};
	lvi.mask = LVIF_GROUPID;
	lvi.iItem = nRow;
	VERIFY( GetItem(&lvi) );
	return lvi.iGroupId;
}

//------------------------------------------------------------------------
//! Retrieves the group header title of a group
//!
//! @param nGroupId ID of the group
//! @return Group header title
//------------------------------------------------------------------------
CString CGridListCtrlGroups::GetGroupHeader(int nGroupId)
{
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.iGroupId = nGroupId;
	lg.mask = LVGF_HEADER | LVGF_GROUPID;
	VERIFY( GetGroupInfo(nGroupId, (PLVGROUP)&lg) != -1 );

#ifdef UNICODE
	return lg.pszHeader;
#elif  _MSC_VER >= 1300
	CComBSTR header( lg.pszHeader );
	return (LPCTSTR)COLE2T(header);
#else
	USES_CONVERSION;
	return W2A(lg.pszHeader);
#endif
}

//------------------------------------------------------------------------
//! Checks if it is possible to modify the collapse state of a group.
//! This is only possible in Windows Vista.
//!
//! @return Groups can be collapsed (true / false)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::IsGroupStateEnabled()
{
	if (!IsGroupViewEnabled())
		return FALSE;

	return CheckOSVersion(0x0600);
}

//------------------------------------------------------------------------
//! Checks whether a group has a certain state
//!
//! @param nGroupId ID of the group
//! @param dwState Specifies the state to check
//! @return The group has the state (true / false)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::HasGroupState(int nGroupId, DWORD dwState)
{
	// Vista SDK - ListView_GetGroupState / LVM_GETGROUPSTATE
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_STATE;
	lg.stateMask = dwState;
	if ( GetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return lg.state == dwState;
}

//------------------------------------------------------------------------
//! Updates the state of a group
//!
//! @param nGroupId ID of the group
//! @param dwState Specifies the new state of the group
//! @return The group state was updated (true / false)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetGroupState(int nGroupId, DWORD dwState)
{
	// Vista SDK - ListView_SetGroupState / LVM_SETGROUPINFO
	if (!IsGroupStateEnabled())
		return FALSE;

	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_STATE;
	lg.state = dwState;
	lg.stateMask = dwState;

#ifdef LVGS_COLLAPSIBLE
	// Maintain LVGS_COLLAPSIBLE state
	if (HasGroupState(nGroupId, LVGS_COLLAPSIBLE))
		lg.state |= LVGS_COLLAPSIBLE;
#endif

	if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return TRUE;
}

//------------------------------------------------------------------------
//! Find the group-id below the given point
//!
//! @param point Mouse position
//! @return ID of the group
//------------------------------------------------------------------------
int CGridListCtrlGroups::GroupHitTest(const CPoint& point)
{
	if (!IsGroupViewEnabled())
		return -1;

	if (HitTest(point) != -1)
		return -1;

	if (IsGroupStateEnabled())
	{
#if _WIN32_WINNT >= 0x0600
#ifdef ListView_HitTestEx
#ifdef LVHT_EX_GROUP
#ifdef ListView_GetGroupInfoByIndex
		LVHITTESTINFO lvhitinfo = {0};
		lvhitinfo.pt = point;
		ListView_HitTestEx(m_hWnd, &lvhitinfo);
		if ((lvhitinfo.flags & LVHT_EX_GROUP) == 0)
			return -1;

		LVGROUP lg = {0};
		lg.cbSize = sizeof(lg);
		lg.mask = LVGF_GROUPID;
		VERIFY( ListView_GetGroupInfoByIndex(m_hWnd, lvhitinfo.iGroup, &lg) );
		return lg.iGroupId;
#endif
#endif
#endif
#endif
	}

	if (IsGroupStateEnabled())
	{
		// Running on Vista or newer, but compiled without _WIN32_WINNT >= 0x0600
#ifndef LVM_GETGROUPINFOBYINDEX
#define LVM_GETGROUPINFOBYINDEX   (LVM_FIRST + 153)
#endif
#ifndef LVM_GETGROUPCOUNT
#define LVM_GETGROUPCOUNT         (LVM_FIRST + 152)
#endif
#ifndef LVM_GETGROUPRECT
#define LVM_GETGROUPRECT          (LVM_FIRST + 98)
#endif
#ifndef LVGGR_HEADER
#define LVGGR_HEADER		      (1)
#endif

		LRESULT groupCount = SNDMSG((m_hWnd), LVM_GETGROUPCOUNT, (WPARAM)0, (LPARAM)0);
		if (groupCount <= 0)
			return -1;
		for(int i = 0 ; i < groupCount; ++i)
		{
			LVGROUP lg = {0};
			lg.cbSize = sizeof(lg);
			lg.mask = LVGF_GROUPID;

			VERIFY( SNDMSG((m_hWnd), LVM_GETGROUPINFOBYINDEX, (WPARAM)(i), (LPARAM)(&lg)) );

			CRect rect(0, LVGGR_HEADER, 0, 0);
			VERIFY( SNDMSG((m_hWnd), LVM_GETGROUPRECT, (WPARAM)(lg.iGroupId), (LPARAM)(RECT*)(&rect)) );

			if (rect.PtInRect(point))
				return lg.iGroupId;
		}
		// Don't try other ways to find the group
		return -1;
	}

	// We require that each group contains atleast one item
	if (GetItemCount() == 0)
		return -1;

	// This logic doesn't support collapsible groups
	int nFirstRow = -1;
	CRect gridRect;
	GetWindowRect(&gridRect);
	for(CPoint pt = point ; pt.y < gridRect.bottom ; pt.y += 2)
	{
		nFirstRow = HitTest(pt);
		if (nFirstRow != -1)
			break;
	}

	if (nFirstRow == -1)
		return -1;

	int nGroupId = GetRowGroupId(nFirstRow);

	// Extra validation that the above row belongs to a different group
	int nAboveRow = GetNextItem(nFirstRow, LVNI_ABOVE);
	if (nAboveRow != -1 && nGroupId == GetRowGroupId(nAboveRow))
		return -1;

	return nGroupId;
}

//------------------------------------------------------------------------
//! Update the checkbox of the label column (first column)
//!
//! @param nGroupId ID of the group
//! @param bChecked The new check box state
//------------------------------------------------------------------------
void CGridListCtrlGroups::CheckEntireGroup(int nGroupId, BOOL bChecked)
{
	if (!(GetExtendedStyle() & LVS_EX_CHECKBOXES))
		return;

	for (int nRow = 0; nRow < GetItemCount(); ++nRow)
	{
		if (GetRowGroupId(nRow) == nGroupId)
		{
			SetCheck(nRow, bChecked ? TRUE : FALSE);
		}
	}
}

//------------------------------------------------------------------------
//! Removes the group and all the rows part of the group
//!
//! @param nGroupId ID of the group
//------------------------------------------------------------------------
void CGridListCtrlGroups::DeleteEntireGroup(int nGroupId)
{
	for (int nRow = 0; nRow < GetItemCount(); ++nRow)
	{
		if (GetRowGroupId(nRow) == nGroupId)
		{
			DeleteItem(nRow);
			nRow--;
		}
	}
	RemoveGroup(nGroupId);
}

//------------------------------------------------------------------------
//! Create a group for each unique values within a column
//!
//! @param nCol The index of the column
//! @return Succeeded in creating the group
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::GroupByColumn(int nCol)
{
	CWaitCursor waitCursor;

	SetSortArrow(-1, false);

	SetRedraw(FALSE);

	RemoveAllGroups();

	EnableGroupView( GetItemCount() > 0 );

	if (IsGroupViewEnabled())
	{
		CSimpleMap<CString, CSimpleArray<int> > groups;

		// Loop through all rows and find possible groups
		for(int nRow = 0; nRow < GetItemCount(); ++nRow)
		{
			CString cellText = GetItemText(nRow, nCol);

			int nGroupId = groups.FindKey(cellText);
			if (nGroupId == -1)
			{
				CSimpleArray<int> rows;
				groups.Add(cellText, rows);
				nGroupId = groups.FindKey(cellText);
			}
			groups.GetValueAt(nGroupId).Add(nRow);
		}

		// Look through all groups and assign rows to group
		for(int nGroupId = 0; nGroupId < groups.GetSize(); ++nGroupId)
		{
			const CSimpleArray<int>& groupRows = groups.GetValueAt(nGroupId);
			DWORD dwState = LVGS_NORMAL;
#ifdef LVGS_COLLAPSIBLE
			if (IsGroupStateEnabled())
				dwState = LVGS_COLLAPSIBLE;
#endif
			VERIFY( InsertGroupHeader(nGroupId, nGroupId, groups.GetKeyAt(nGroupId), dwState) != -1);

			for(int groupRow = 0; groupRow < groupRows.GetSize(); ++groupRow)
			{
				VERIFY( SetRowGroupId(groupRows[groupRow], nGroupId) );
			}
		}

		SetRedraw(TRUE);
		Invalidate(FALSE);
		return TRUE;
	}

	SetRedraw(TRUE);
	Invalidate(FALSE);
	return FALSE;
}

//------------------------------------------------------------------------
//! Collapse all groups
//------------------------------------------------------------------------
void CGridListCtrlGroups::CollapseAllGroups()
{
	if (!IsGroupStateEnabled())
		return;

	// Loop through all rows and find possible groups
	for(int nRow = 0; nRow < GetItemCount(); ++nRow)
	{
		int nGroupId = GetRowGroupId(nRow);
		if (nGroupId != -1)
		{
			if (!HasGroupState(nGroupId, LVGS_COLLAPSED))
			{
				SetGroupState(nGroupId, LVGS_COLLAPSED);
			}
		}
	}
}

//------------------------------------------------------------------------
//! Expand all groups
//------------------------------------------------------------------------
void CGridListCtrlGroups::ExpandAllGroups()
{
	if (!IsGroupStateEnabled())
		return;

	// Loop through all rows and find possible groups
	for(int nRow = 0; nRow < GetItemCount(); ++nRow)
	{
		int nGroupId = GetRowGroupId(nRow);
		if (nGroupId != -1)
		{
			if (HasGroupState(nGroupId, LVGS_COLLAPSED))
			{
				SetGroupState(nGroupId, LVGS_NORMAL);
			}
		}
	}
}

//------------------------------------------------------------------------
//! Called by the framework when a drop operation is to occur, where the
//! origin is the CGridListCtrlEx itself
//!
//! @param pDataObject Points to the data object containing the data that can be dropped
//! @param dropEffect The effect that the user chose for the drop operation (DROPEFFECT_COPY, DROPEFFECT_MOVE, DROPEFFECT_LINK)
//! @param point Contains the current location of the cursor in client coordinates.
//! @return Nonzero if the drop is successful; otherwise 0
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::OnDropSelf(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	// Internal drag (Move rows to other group)
	int nRow, nCol;
	CellHitTest(point, nRow, nCol);
	if (!IsGroupViewEnabled())
		return CGridListCtrlEx::MoveSelectedRows(nRow);

	if (GetStyle() & LVS_OWNERDATA)
		return false;

	int nGroupId = nRow != -1 ? GetRowGroupId(nRow) : GroupHitTest(point);
	if (nGroupId == -1)
		return FALSE;

	if (MoveSelectedRows(nGroupId))
	{
		if (nRow != -1)
		{
			EnsureVisible(nRow, FALSE);
			SetFocusRow(nRow);
		}
	}
	return TRUE;
}

//------------------------------------------------------------------------
//! Moves the selected rows to the specified group
//!
//! @param nDropGroupId Moved the selected rows to this group
//! @return Was rows rearranged ? (true / false)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::MoveSelectedRows(int nDropGroupId)
{
	if (GetStyle() & LVS_OWNERDATA)
		return false;

	if (nDropGroupId == -1)
		return false;

	POSITION pos = GetFirstSelectedItemPosition();
	if (pos == NULL)
		return false;

	while(pos != NULL)
	{
		int nRow = GetNextSelectedItem(pos);
		int nGroupId = GetRowGroupId(nRow);
		if (nGroupId != nDropGroupId)
			SetRowGroupId(nRow, nDropGroupId);
	}

	return true;
}

//------------------------------------------------------------------------
//! WM_CONTEXTMENU message handler to show popup menu when mouse right
//! click is used (or SHIFT+F10 on the keyboard)
//!
//! @param pWnd Handle to the window in which the user right clicked the mouse
//! @param point Position of the cursor, in screen coordinates, at the time of the mouse click.
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if ( IsGroupViewEnabled() )
	{
		if (pWnd != GetHeaderCtrl())
		{
			if (point.x != -1 && point.y != -1)
			{
				CPoint pt = point;
				ScreenToClient(&pt);

				int nGroupId = GroupHitTest(pt);
				if (nGroupId != -1)
				{
					OnContextMenuGroup(pWnd, point, nGroupId);
					return;
				}
			}
		}
	}
	CGridListCtrlEx::OnContextMenu(pWnd, point);
}

namespace
{
BOOL IsCommonControlsEnabled()
{
	// Test if application has access to common controls (Required for grouping)
	HMODULE hinstDll = ::LoadLibrary(_T("comctl32.dll"));
	if (hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hinstDll, "DllGetVersion");
		::FreeLibrary(hinstDll);
		if (pDllGetVersion != NULL)
		{
			DLLVERSIONINFO dvi = {0};
			dvi.cbSize = sizeof(dvi);
			HRESULT hRes = pDllGetVersion ((DLLVERSIONINFO*) &dvi);
			if (SUCCEEDED(hRes))
				return dvi.dwMajorVersion >= 6;
		}
	}
	return false;
}
}

//------------------------------------------------------------------------
//! Override this method to change the context menu when activating context
//! menu for the column headers
//!
//! @param pWnd Handle to the window in which the user right clicked the mouse
//! @param point Position of the cursor, in screen coordinates, at the time of the mouse click.
//! @param nCol The index of the column
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnContextMenuHeader(CWnd* pWnd, CPoint point, int nCol)
{
	// Only Windows XP and above supports groups
	if (!IsCommonControlsEnabled())
	{
		CGridListCtrlEx::OnContextMenuHeader(pWnd, point, nCol);
		return;
	}

	// Show context-menu with the option to show hide columns
	CMenu menu;
	VERIFY( menu.CreatePopupMenu() );

	if (nCol != -1)
	{
		// Retrieve column-title
		const CString& columnTitle = GetColumnHeading(nCol);
		menu.AppendMenu(MF_STRING, 3, CString(_T("Group by: ")) + columnTitle);
	}

	if (IsGroupViewEnabled())
	{
		menu.AppendMenu(MF_STRING, 4, _T("Disable grouping"));
	}

	CString title_editor;
	if (HasColumnEditor(nCol, title_editor))
	{
		menu.AppendMenu(MF_STRING, 1, static_cast<LPCTSTR>(title_editor));
	}

	CString title_picker;
	if (HasColumnPicker(title_picker))
	{
		menu.AppendMenu(MF_STRING, 2, static_cast<LPCTSTR>(title_picker));
	}
	else
	{
		if (menu.GetMenuItemCount() > 0)
			menu.AppendMenu(MF_SEPARATOR, 0, _T(""));

		InternalColumnPicker(menu, 6);
	}

	CSimpleArray<CString> profiles;
	InternalColumnProfileSwitcher(menu, GetColumnCount() + 7, profiles);

	CString title_resetdefault;
	if (HasColumnDefaultState(title_resetdefault))
	{
		if (profiles.GetSize() == 0)
			menu.AppendMenu(MF_SEPARATOR, 0, _T(""));
		menu.AppendMenu(MF_STRING, 5, title_resetdefault);
	}

	// Will return zero if no selection was made (TPM_RETURNCMD)
	int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(nResult)
	{
	case 0:
		break;
	case 1:
		OpenColumnEditor(nCol);
		break;
	case 2:
		OpenColumnPicker();
		break;
	case 3:
		GroupByColumn(nCol);
		break;
	case 4:
	{
		// Very strange problem when disabling group mode, then scrollbars are not updated
		// If placed in the bottom and disables group mode, then suddenly there is a strange offset
		//	- Quick fix scroll to top, and then fix scroll bars afterwards
		int pos = GetScrollPos(SB_VERT);
		EnsureVisible(0, FALSE);
		RemoveAllGroups();
		EnableGroupView(FALSE);
		Scroll(CSize(0, pos));
	}
	break;
	case 5:
		ResetColumnDefaultState();
		break;
	default:
	{
		int nCol = nResult - 6;
		if (nCol < GetColumnCount())
		{
			ShowColumn(nCol, !IsColumnVisible(nCol));
		}
		else
		{
			int nProfile = nResult - GetColumnCount() - 7;
			SwichColumnProfile(profiles[nProfile]);
		}
	}
	break;
	}
}

//------------------------------------------------------------------------
//! Override this method to change the context menu when activating context
//! menu for the group headers
//!
//! @param pWnd Handle to the window in which the user right clicked the mouse
//! @param point Position of the cursor, in screen coordinates, at the time of the mouse click.
//! @param nGroupId ID of the group
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnContextMenuGroup(CWnd* pWnd, CPoint point, int nGroupId)
{
	CMenu menu;
	UINT uFlags = MF_BYPOSITION | MF_STRING;
	VERIFY( menu.CreatePopupMenu() );

	const CString& groupHeader = GetGroupHeader(nGroupId);

	// Provide menu-options for collapsing groups, if the collapsible state is not available
#ifndef LVGS_COLLAPSIBLE
	if (IsGroupStateEnabled())
	{
		if (HasGroupState(nGroupId, LVGS_COLLAPSED))
		{
			CString menuText = CString(_T("Expand group: ")) + groupHeader;
			menu.InsertMenu(0, uFlags, 1, menuText);
		}
		else
		{
			CString menuText = CString(_T("Collapse group: ")) + groupHeader;
			menu.InsertMenu(0, uFlags, 2, menuText);
		}
	}
#endif

	if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
	{
		CString menuText = CString(_T("Check group: ")) + groupHeader;
		menu.InsertMenu(1, uFlags, 3, menuText);
		menuText = CString(_T("Uncheck group: ")) + groupHeader;
		menu.InsertMenu(2, uFlags, 4, menuText);
	}

	int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
	switch(nResult)
	{
	case 1:
		SetGroupState(nGroupId, LVGS_NORMAL);
		break;
	case 2:
		SetGroupState(nGroupId, LVGS_COLLAPSED);
		break;
	case 3:
		CheckEntireGroup(nGroupId, true);
		break;
	case 4:
		CheckEntireGroup(nGroupId, false);
		break;
	}
}

//------------------------------------------------------------------------
//! Override this method to change the context menu when activating context
//! menu in client area with no rows
//!
//! @param pWnd Handle to the window in which the user right clicked the mouse
//! @param point Position of the cursor, in screen coordinates, at the time of the mouse click.
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnContextMenuGrid(CWnd* pWnd, CPoint point)
{
	if (IsGroupStateEnabled())
	{
		CMenu menu;
		UINT uFlags = MF_BYPOSITION | MF_STRING;
		VERIFY( menu.CreatePopupMenu() );

		menu.InsertMenu(0, uFlags, 1, _T("Expand all groups"));
		menu.InsertMenu(1, uFlags, 2, _T("Collapse all groups"));
		menu.InsertMenu(2, uFlags, 3, _T("Disable grouping"));

		int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, 0);
		switch(nResult)
		{
		case 1:
			ExpandAllGroups();
			break;
		case 2:
			CollapseAllGroups();
			break;
		case 3:
			RemoveAllGroups();
			EnableGroupView(FALSE);
			break;
		}
	}
}

//------------------------------------------------------------------------
//! Update the description text of the group footer
//!
//! @param nGroupId ID of the group
//! @param strFooter The footer description text
//! @param dwAlign Indicates the alignment of the footer text for the group
//! @return Succeeded in updating the group footer
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetGroupFooter(int nGroupId, const CString& strFooter, DWORD dwAlign)
{
	if (!IsGroupStateEnabled())
		return FALSE;

#if _WIN32_WINNT >= 0x0600
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_FOOTER | LVGF_ALIGN;
	lg.uAlign = dwAlign;
#ifdef UNICODE
	lg.pszFooter = (LPWSTR)(LPCTSTR)strFooter;
	lg.cchFooter = strFooter.GetLength();
#else
	CComBSTR bstrFooter = strFooter;
	lg.pszFooter = bstrFooter;
	lg.cchFooter = bstrFooter.Length();
#endif

	if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

//------------------------------------------------------------------------
//! Update the task link of the group header
//!
//! @param nGroupId ID of the group
//! @param strTask The task description text
//! @return Succeeded in updating the group task
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetGroupTask(int nGroupId, const CString& strTask)
{
	if (!IsGroupStateEnabled())
		return FALSE;

#if _WIN32_WINNT >= 0x0600
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_TASK;
#ifdef UNICODE
	lg.pszTask = (LPWSTR)(LPCTSTR)strTask;
	lg.cchTask = strTask.GetLength();
#else
	CComBSTR bstrTask = strTask;
	lg.pszTask = bstrTask;
	lg.cchTask = bstrTask.Length();
#endif

	if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

//------------------------------------------------------------------------
//! Update the subtitle in the group header
//!
//! @param nGroupId ID of the group
//! @param strSubtitle The subtitle description text
//! @return Succeeded in updating the group subtitle
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetGroupSubtitle(int nGroupId, const CString& strSubtitle)
{
	if (!IsGroupStateEnabled())
		return FALSE;

#if _WIN32_WINNT >= 0x0600
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_SUBTITLE;
#ifdef UNICODE
	lg.pszSubtitle = (LPWSTR)(LPCTSTR)strSubtitle;
	lg.cchSubtitle = strSubtitle.GetLength();
#else
	CComBSTR bstrSubtitle = strSubtitle;
	lg.pszSubtitle = bstrSubtitle;
	lg.cchSubtitle = bstrSubtitle.Length();
#endif

	if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

//------------------------------------------------------------------------
//! Update the image icon in the group header together with top and bottom
//! description. Microsoft encourage people not to use this functionality.
//!
//! @param nGroupId ID of the group
//! @param nImage Index of the title image in the control imagelist.
//! @param strTopDesc Description text placed oppposite of the image
//! @param strBottomDesc Description text placed below the top description
//! @return Succeeded in updating the group image
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SetGroupTitleImage(int nGroupId, int nImage, const CString& strTopDesc, const CString& strBottomDesc)
{
	if (!IsGroupStateEnabled())
		return FALSE;

#if _WIN32_WINNT >= 0x0600
	LVGROUP lg = {0};
	lg.cbSize = sizeof(lg);
	lg.mask = LVGF_TITLEIMAGE;
	lg.iTitleImage = nImage;	// Index of the title image in the control imagelist.

#ifdef UNICODE
	if (!strTopDesc.IsEmpty())
	{
		// Top description is drawn opposite the title image when there is
		// a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
		lg.mask |= LVGF_DESCRIPTIONTOP;
		lg.pszDescriptionTop = (LPWSTR)(LPCTSTR)strTopDesc;
		lg.cchDescriptionTop = strTopDesc.GetLength();
	}
	if (!strBottomDesc.IsEmpty())
	{
		// Bottom description is drawn under the top description text when there is
		// a title image, no extended image, and uAlign==LVGA_HEADER_CENTER.
		lg.mask |= LVGF_DESCRIPTIONBOTTOM;
		lg.pszDescriptionBottom = (LPWSTR)(LPCTSTR)strBottomDesc;
		lg.cchDescriptionBottom = strBottomDesc.GetLength();
	}
#else
	CComBSTR bstrTopDesc = strTopDesc;
	CComBSTR bstrBottomDesc = strBottomDesc;
	if (!strTopDesc.IsEmpty())
	{
		lg.mask |= LVGF_DESCRIPTIONTOP;
		lg.pszDescriptionTop = bstrTopDesc;
		lg.cchDescriptionTop = bstrTopDesc.Length();
	}
	if (!strBottomDesc.IsEmpty())
	{
		lg.mask |= LVGF_DESCRIPTIONBOTTOM;
		lg.pszDescriptionBottom = bstrBottomDesc;
		lg.cchDescriptionBottom = bstrBottomDesc.Length();
	}
#endif

	if (SetGroupInfo(nGroupId, (PLVGROUP)&lg) == -1)
		return FALSE;

	return TRUE;
#else
	return FALSE;
#endif
}

//------------------------------------------------------------------------
//! LVN_GETEMPTYMARKUP message handler to show markup text when the list
//! control is empty.
//!
//! @param pNMHDR Pointer to NMLVEMPTYMARKUP structure
//! @param pResult Not used
//! @return Is final message handler (Return FALSE to continue routing the message)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::OnGetEmptyMarkup(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (m_EmptyMarkupText.IsEmpty())
		return FALSE;

#if _WIN32_WINNT >= 0x0600
	NMLVEMPTYMARKUP* pEmptyMarkup = reinterpret_cast<NMLVEMPTYMARKUP*>(pNMHDR);
	pEmptyMarkup->dwFlags = EMF_CENTERED;

#ifdef UNICODE
	lstrcpyn(pEmptyMarkup->szMarkup, (LPCTSTR)m_EmptyMarkupText, sizeof(pEmptyMarkup->szMarkup) / sizeof(WCHAR));
#else
#if __STDC_WANT_SECURE_LIB__
	mbstowcs_s(NULL, pEmptyMarkup->szMarkup, static_cast<LPCTSTR>(m_EmptyMarkupText), sizeof(pEmptyMarkup->szMarkup) / sizeof(WCHAR));
#else
	mbstowcs(pEmptyMarkup->szMarkup, static_cast<LPCTSTR>(m_EmptyMarkupText), sizeof(pEmptyMarkup->szMarkup) / sizeof(WCHAR));
#endif
#endif
	*pResult = TRUE;
#endif

	return TRUE;
}

//------------------------------------------------------------------------
//! LVN_LINKCLICK message handler called when a group task link is clicked
//!
//! @param pNMHDR Pointer to NMLVLINK structure
//! @param pResult Not used
//! @return Is final message handler (Return FALSE to continue routing the message)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::OnGroupTaskClick(NMHDR* pNMHDR, LRESULT* pResult)
{
#if _WIN32_WINNT >= 0x0600
	NMLVLINK* pLinkInfo = reinterpret_cast<NMLVLINK*>(pNMHDR);
	int nGroupId = pLinkInfo->iSubItem;
	nGroupId;	// Avoid unreferenced variable warning
#endif
	return FALSE;
}

//------------------------------------------------------------------------
//! The framework calls this member function when the user double-clicks
//! the left mouse button. Used to expand and collapse groups when group
//! header is clicked.
//!
//! @param nFlags Indicates whether various virtual keys are down (MK_CONTROL, MK_SHIFT, etc.)
//! @param point Specifies the x- and y-coordinate of the cursor relative to the upper-left corner of the window.
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CGridListCtrlEx::OnLButtonDblClk(nFlags, point);

	if (!IsGroupStateEnabled())
		return;

	int nGroupId = GroupHitTest(point);
	if (nGroupId != -1)
	{
		if (HasGroupState(nGroupId, LVGS_COLLAPSED))
			SetGroupState(nGroupId, LVGS_NORMAL);
		else
			SetGroupState(nGroupId, LVGS_COLLAPSED);
	}
}

//------------------------------------------------------------------------
//! Override this method to provide the group a cell belongs to.
//!
//! @param nRow The index of the row
//! @param nCol The index of the column
//! @param nGroupId Text string to display in the cell
//! @return True if the cell belongs to a group
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::OnDisplayCellGroup(int nRow, int nCol, int& nGroupId)
{
	return false;
}

//------------------------------------------------------------------------
//! LVN_GETDISPINFO message handler, which is called when details are
//! needed for an item that specifies callback.
//!		- Cell-Group, when item is using I_GROUPIDCALLBACK
//!
//! @param pNMHDR Pointer to an NMLVDISPINFO structure
//! @param pResult Not used
//! @return Is final message handler (Return FALSE to continue routing the message)
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pNMW = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	int nRow = pNMW->item.iItem;
	int nCol = pNMW->item.iSubItem;

	if (nRow < 0 || nRow >= GetItemCount())
		return FALSE;	// requesting invalid item

	if (nCol < 0 || nCol >= GetHeaderCtrl()->GetItemCount())
		return FALSE;	// requesting invalid item

	if (pNMW->item.mask & LVIF_GROUPID)
	{
		// Request group-id of the column (Virtual-list/LVS_OWNERDATA)
		int result = -1;
		if (OnDisplayCellGroup(nRow, nCol, result))
			pNMW->item.iGroupId = result;
		else
			pNMW->item.iGroupId = I_GROUPIDNONE;
	}
	return CGridListCtrlEx::OnGetDispInfo(pNMHDR, pResult);
}

namespace
{
struct PARAMSORT
{
	PARAMSORT(HWND hWnd, int nCol, BOOL bAscending, CGridColumnTrait* pTrait)
		: m_hWnd(hWnd)
		, m_pTrait(pTrait)
		, m_ColumnIndex(nCol)
		, m_Ascending(bAscending)
	{}

	HWND m_hWnd;
	int  m_ColumnIndex;
	BOOL m_Ascending;
	CGridColumnTrait* m_pTrait;
	CSimpleMap<int, CString> m_GroupNames;

	const CString& LookupGroupName(int nGroupId)
	{
		int groupIdx = m_GroupNames.FindKey(nGroupId);
		if (groupIdx == -1)
		{
			static const CString emptyStr;
			return emptyStr;
		}
		return m_GroupNames.GetValueAt(groupIdx);
	}
};

int CALLBACK SortFuncGroup(int leftId, int rightId, void* lParamSort)
{
	PARAMSORT& ps = *(PARAMSORT*)lParamSort;

	const CString& leftText = ps.LookupGroupName(leftId);
	const CString& rightText = ps.LookupGroupName(rightId);

	LVITEM leftItem = {0};
	leftItem.pszText = const_cast<LPTSTR>((LPCTSTR)leftText);
	leftItem.cchTextMax = leftText.GetLength();
	LVITEM rightItem = {0};
	rightItem.pszText = const_cast<LPTSTR>((LPCTSTR)rightText);
	rightItem.cchTextMax = leftText.GetLength();
	return ps.m_pTrait->OnSortRows(leftItem, rightItem, ps.m_Ascending);
}
}

//------------------------------------------------------------------------
//! Changes the row sorting in regard to the specified column
//!
//! @param nCol The index of the column
//! @param bAscending Should the arrow be up or down
//! @return True / false depending on whether sort is possible
//------------------------------------------------------------------------
BOOL CGridListCtrlGroups::SortColumn(int nCol, BOOL bAscending)
{
	CWaitCursor waitCursor;

	if (IsGroupViewEnabled())
	{
		SetRedraw(FALSE);

		GroupByColumn(nCol);

		// Cannot use GetGroupInfo during sort
		PARAMSORT paramsort(m_hWnd, nCol, bAscending, GetColumnTrait(nCol));
		for(int nRow = 0 ; nRow < GetItemCount() ; ++nRow)
		{
			int nGroupId = GetRowGroupId(nRow);
			if (nGroupId != -1 && paramsort.m_GroupNames.FindKey(nGroupId) == -1)
				paramsort.m_GroupNames.Add(nGroupId, GetGroupHeader(nGroupId));
		}

		SetRedraw(TRUE);
		Invalidate(FALSE);

		// Avoid bug in CListCtrl::SortGroups() which differs from ListView_SortGroups
		if (!ListView_SortGroups(m_hWnd, SortFuncGroup, &paramsort))
			return false;
	}
	else
	{
		if (!CGridListCtrlEx::SortColumn(nCol, bAscending))
			return false;
	}

	return true;
}

//------------------------------------------------------------------------
//! WM_PAINT message handler called when needing to redraw list control.
//! Used to display text when the list control is empty
//------------------------------------------------------------------------
void CGridListCtrlGroups::OnPaint()
{
#if _WIN32_WINNT >= 0x0600
	if (UsingVisualStyle())
	{
		// Use LVN_GETEMPTYMARKUP if available
		CListCtrl::OnPaint();	// default
		return;
	}
#endif

	CGridListCtrlEx::OnPaint();
}

// MFC headers with group-support is only availabe from VS.NET
#if _MSC_VER < 1300

AFX_INLINE LRESULT CGridListCtrlGroups::InsertGroup(int index, PLVGROUP pgrp)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_InsertGroup(m_hWnd, index, pgrp);
}
AFX_INLINE int CGridListCtrlGroups::SetGroupInfo(int iGroupId, PLVGROUP pgrp)
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)ListView_SetGroupInfo(m_hWnd, iGroupId, pgrp);
}
AFX_INLINE int CGridListCtrlGroups::GetGroupInfo(int iGroupId, PLVGROUP pgrp) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)ListView_GetGroupInfo(m_hWnd, iGroupId, pgrp);
}
AFX_INLINE LRESULT CGridListCtrlGroups::RemoveGroup(int iGroupId)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_RemoveGroup(m_hWnd, iGroupId);
}
AFX_INLINE LRESULT CGridListCtrlGroups::MoveGroup(int iGroupId, int toIndex)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_MoveGroup(m_hWnd, iGroupId, toIndex);
}
AFX_INLINE LRESULT CGridListCtrlGroups::MoveItemToGroup(int idItemFrom, int idGroupTo)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_MoveItemToGroup(m_hWnd, idItemFrom, idGroupTo);
}
AFX_INLINE void CGridListCtrlGroups::SetGroupMetrics(PLVGROUPMETRICS pGroupMetrics)
{
	ASSERT(::IsWindow(m_hWnd));
	ListView_SetGroupMetrics(m_hWnd, pGroupMetrics);
}
AFX_INLINE void CGridListCtrlGroups::GetGroupMetrics(PLVGROUPMETRICS pGroupMetrics) const
{
	ASSERT(::IsWindow(m_hWnd));
	ListView_GetGroupMetrics(m_hWnd, pGroupMetrics);
}
AFX_INLINE LRESULT CGridListCtrlGroups::EnableGroupView(BOOL fEnable)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_EnableGroupView(m_hWnd, fEnable);
}
AFX_INLINE BOOL CGridListCtrlGroups::SortGroups(PFNLVGROUPCOMPARE _pfnGroupCompare, LPVOID _plv)
{
	ASSERT(::IsWindow(m_hWnd));
	return (BOOL)::SendMessage(m_hWnd, LVM_SORTGROUPS, (WPARAM)(LPARAM)_plv, (LPARAM)_pfnGroupCompare );
}
AFX_INLINE LRESULT CGridListCtrlGroups::InsertGroupSorted(PLVINSERTGROUPSORTED pStructInsert)
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_InsertGroupSorted(m_hWnd, pStructInsert);
}
AFX_INLINE void CGridListCtrlGroups::RemoveAllGroups()
{
	ASSERT(::IsWindow(m_hWnd));
	ListView_RemoveAllGroups(m_hWnd);
}
AFX_INLINE BOOL CGridListCtrlGroups::HasGroup(int iGroupId) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (BOOL)ListView_HasGroup(m_hWnd, iGroupId);
}
AFX_INLINE BOOL CGridListCtrlGroups::IsGroupViewEnabled() const
{
	ASSERT(::IsWindow(m_hWnd));
	return ListView_IsGroupViewEnabled(m_hWnd);
}
#endif // _MSC_VER < 1300

#endif // _MSC_VER < 1500 || defined UNICODE || defined CGRIDLISTCTRLEX_GROUPMODE

#endif // _WIN32_WINNT >= 0x0501
