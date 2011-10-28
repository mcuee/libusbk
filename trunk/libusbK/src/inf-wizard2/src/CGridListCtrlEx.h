#pragma once

#define GCSF_FIXED (0x0001)

//------------------------------------------------------------------------
// Author:  Rolf Kristensen
// Source:  http://www.codeproject.com/KB/list/CGridListCtrlEx.aspx
// License: Free to use for all (New BSD License)
// Version: 1.9
//
// Change History:
// 	1.9 - Added new CGridColumnTrait::OnSortRows() to take LVITEM as parameter (2011-05-30)
//		  Renamed CGridColumnConfig to CViewConfigSection (Now general purpose settings manager)
//		  Removed CGridColumnManager and moved LoadState/SaveState into CGridListCtrlEx (Breaking change)
//		  Fixed breaking change in 1.8 where OnEditBegin overrides stopped working
//	1.8 - Added checkbox support for all column editor types (2010-10-01)
//		  Added checkbox toggle for all selected rows
//		  Added min and max width for columns
//	1.7 - Added CGridColumnTraitImage, that provides checkbox editing (2009-12-12)
//		  Renamed OnTraitCustomDraw() to OnCustomDrawCell()
//		  Renamed OnTraitEditBegin() to OnEditBegin()
//		  Renamed OnTraitEditComplete() to OnEditComplete()
//	1.6 - Added OLE drag and drop support (2009-08-10)
//		  Added support for LVS_EX_CHECKBOXES with LVS_OWNERDATA
//		  Added better support for keyboard search with LVS_OWNERDATA
//	1.5 - Added column manager CGridColumnManager (2009-03-29)
//		  Added support for groups on VC6 with platform SDK
//		  Added sample projects for the different versions of Visual Studio
//		  Improved documentation through Doxygen comments
//	1.4 - Added clipboard support (2008-11-07)
//		  Renamed the "Callback"-functions to "OnDisplay"
//	1.3 - Added support for compiling on VC6 (2008-10-09)
//	1.2 - Added row traits and CGridRowTraitXP (2008-09-24)
//	1.1 - Added support for groups with CGridListCtrlGroups (2008-09-18)
//		  Added support for CDateTimeCtrl as cell editor
//	1.0 - Initial Release (2008-09-04)
//------------------------------------------------------------------------

class COleDataSource;
class CViewConfigSection;
class CViewConfigSectionProfiles;
class CGridColumnTrait;
class CGridRowTrait;
template<class T> class COleDropTargetWnd;

//------------------------------------------------------------------------
//! \mainpage Introduction
//! CGridListCtrlEx extends the CListCtrl from MFC. CGridListCtrlGroups
//! extends CGridListCtrlEx even further to support categorization of rows in groups.
//!
//! CGridListCtrlEx makes use of different helper classes that makes it easy
//! to customize and extend it even further.
//!
//! - CGridColumnTrait provides special behavior to the cells in a column
//!		- CGridColumnTraitEdit Implements cell editing using CEdit
//!		- CGridColumnTraitCombo Implements cell editing using CComboBox
//!		- CGridColumnTraitDateTime Implements cell editing using CDateTimeCtrl
//!		- CGridColumnTraitImage Implements cell editing using cell-image (can mimic checkbox)
//! - CGridRowTrait provides an interface to perform custom drawing at row level
//!		- CGridRowTraitText implements alternate row coloring
//!			- CGridRowTraitXP removes the white background for cell images on WinXP
//! - CViewConfigSection provides column state persistence
//!		- CViewConfigSectionProfiles provides the ability switch between different column setups
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//! CGridListCtrlEx extends the CListCtrl with several features.
//! - Cell editing and customization through use of CGridColumnTrait
//! - Sortable
//! - Simple column picker
//! - Cell navigation
//! - Keyboard search navigation
//! - Cell tooltip
//! - Clipboard (copy only)
//! - OLE Drag and drop
//! - Column state persistence (width, order, etc.)
//------------------------------------------------------------------------
class CGridListCtrlEx : public CListCtrl
{
public:
	CGridListCtrlEx();
	~CGridListCtrlEx();

	// CListCtrl
	LRESULT EnableVisualStyles(BOOL bValue);
	inline BOOL UsingVisualStyle() const
	{
		return m_UsingVisualStyle;
	}
	virtual CFont* GetCellFont();
	virtual void SetCellMargin(double margin);
	void SetEmptyMarkupText(const CString& strText);
	static BOOL CheckOSVersion(WORD requestOS);

	// Row
	int GetFocusRow() const;
	void SetFocusRow(int nRow);
	BOOL IsRowSelected(int nRow) const;
	BOOL SelectRow(int nRow, BOOL bSelect);
	virtual CGridRowTrait* GetRowTrait(int nRow);
	virtual void SetDefaultRowTrait(CGridRowTrait* pRowTrait);

	// Column
	const CHeaderCtrl* GetHeaderCtrl() const;
	CHeaderCtrl* GetHeaderCtrl()
	{
		return CListCtrl::GetHeaderCtrl();
	}
	int GetColumnCount() const;
	int GetColumnData(int nCol) const;
	int GetColumnOrder(int nCol) const;
	CString GetColumnHeading(int nCol) const;
	virtual BOOL EnsureColumnVisible(int nCol, BOOL bPartialOK);
	virtual BOOL SetColumnWidthAuto(int nCol = -1, BOOL bIncludeHeader = false);
	virtual void SetSortArrow(int nCol, BOOL bAscending);
	virtual BOOL ShowColumn(int nCol, BOOL bShow);
	virtual BOOL IsColumnVisible(int nCol);
	virtual BOOL IsColumnResizable(int nCol);
	virtual BOOL IsColumnAlwaysVisible(int nCol);
	virtual BOOL IsColumnAlwaysHidden(int nCol);
	virtual int GetFirstVisibleColumn();
	virtual int InsertHiddenLabelColumn();
	virtual int InsertColumnTrait(int nCol, const CString& strColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1, CGridColumnTrait* pTrait = NULL);
	virtual CGridColumnTrait* GetColumnTrait(int nCol);
	virtual int GetColumnTraitSize() const;

	// Cell / Subitem
	void CellHitTest(const CPoint& pt, int& nRow, int& nCol) const;
	BOOL GetCellRect(int nRow, int nCol, UINT nCode, CRect& rect);
	inline int GetFocusCell() const
	{
		return m_FocusCell;
	}
	virtual void SetFocusCell(int nCol, BOOL bRedraw = false);
	virtual CWnd* EditCell(int nRow, int nCol)
	{
		return EditCell(nRow, nCol, CPoint(-1, -1));
	}
	virtual CWnd* EditCell(int nRow, int nCol, CPoint pt);
	BOOL IsCellEditorOpen() const;
	BOOL IsCellCallback(int nRow, int nCol) const;
	int GetCellImage(int nRow, int nCol) const;
	BOOL SetCellImage(int nRow, int nCol, int nImageId);
	virtual CGridColumnTrait* GetCellColumnTrait(int nRow, int nCol);

	virtual BOOL ResizeColumns();

	// Column Editor
	virtual void SetupColumnConfig(CViewConfigSectionProfiles* pColumnConfig);
	virtual void LoadState(CViewConfigSection& config);
	virtual void SaveState(CViewConfigSection& config);
	virtual void LoadColumnState(int nConfigCol, int nOwnerCol, CViewConfigSection& config);
	virtual void SaveColumnState(int nConfigCol, int nOwnerCol, CViewConfigSection& config);
	virtual BOOL HasColumnEditor(int nCol, CString& strTitle);
	virtual void OpenColumnEditor(int nCol);
	virtual BOOL HasColumnPicker(CString& strTitle);
	virtual void OpenColumnPicker();
	virtual BOOL HasColumnDefaultState(CString& strTitle);
	virtual void ResetColumnDefaultState();
	virtual CString HasColumnProfiles(CSimpleArray<CString>& profiles, CString& strTitle);
	virtual void SwichColumnProfile(const CString& strProfile);
	virtual void OnSaveStateColumnPick();
	virtual void OnSaveStateColumnResize();
	virtual void OnSaveStateKillFocus();

	// DataModel callbacks
	virtual BOOL OnDisplayCellText(int nRow, int nCol, CString& strResult);
	virtual BOOL OnDisplayCellImage(int nRow, int nCol, int& nImageId);
	virtual BOOL OnDisplayCellTooltip(const CPoint& point) const;
	virtual BOOL OnDisplayCellTooltip(int nRow, int nCol, CString& strResult);
	virtual BOOL OnDisplayCellColor(int nRow, int nCol, COLORREF& textColor, COLORREF& backColor);
	virtual BOOL OnDisplayCellFont(int nRow, int nCol, LOGFONT& font);
	virtual BOOL OnDisplayRowColor(int nRow, COLORREF& textColor, COLORREF& backColor);
	virtual BOOL OnDisplayRowFont(int nRow, LOGFONT& font);
	virtual void OnDisplayDragOverRow(int nRow);
	virtual BOOL OnDisplayToClipboard(CString& strResult, BOOL includeHeader = true);
	virtual BOOL OnDisplayToClipboard(int nRow, CString& strResult);
	virtual BOOL OnDisplayToClipboard(int nRow, int nCol, CString& strResult);
	virtual BOOL OnDisplayToDragDrop(CString& strResult);
	virtual BOOL OnOwnerDataDisplayCheckbox(int nRow);
	virtual void OnOwnerDataToggleCheckBox(int nRow);
	virtual int  OnKeyboardSearch(int nCol, int nStartRow, const CString& strSearch);

protected:
	// Maintaining column traits (and visible state)
	CSimpleArray<CGridColumnTrait*> m_ColumnTraits;	//!< Column traits registered (One for each column)
	virtual void InsertColumnTrait(int nCol, CGridColumnTrait* pTrait);
	virtual void DeleteColumnTrait(int nCol);
	CViewConfigSectionProfiles* m_pColumnConfig;	//!< Column state persistence
	int InternalColumnPicker(CMenu& menu, int offset);
	int InternalColumnProfileSwitcher(CMenu& menu, int offset, CSimpleArray<CString>& profiles);

	// Maintaining row traits
	CGridRowTrait* m_pDefaultRowTrait;	//!< Default row trait used for special row drawing

	// Maintaining cell/subitem focus
	int m_FocusCell;			//!< Column currently having focus (-1 means entire row)
	virtual void MoveFocusCell(BOOL bMoveRight);

	// Maintaining Keyboard search
	CString m_LastSearchString;	//!< Last search criteria for keyboard search
	CTime	m_LastSearchTime;	//!< Time of last search attempt for keyboard search
	int		m_LastSearchCell;	//!< Last column used in keyboard search
	int		m_LastSearchRow;	//!< Last row matched in keyboard search
	int		m_RepeatSearchCount;//!< How many times the same search have been repeated (same key pressed)

	// Maintaining row sorting
	int m_SortCol;				//!< Rows are sorted according to this column
	BOOL m_Ascending;			//!< Rows are sorted ascending / descending
	virtual BOOL SortColumn(int nCol, BOOL bAscending);

	// Maintaining cell editing
	CWnd* m_pEditor;			//!< Cell value editor currently in use

	BOOL m_UsingVisualStyle;	//!< Vista Style has been enabled (alpha blend)

	// Maintaining margin
	CFont m_GridFont;			//!< Original font of the the list control
	CFont m_CellFont;			//!< Current font to draw rows
	double m_Margin;			//!< Current margin between original font and cell font

	CString m_EmptyMarkupText;	//!< Text to display when list control is empty
	BOOL m_InvalidateMarkupText;//!< Ensure that the empty markup text is cleared properly
	BOOL m_bUseVisualStyles;

	// Maintaining drag drop
	COleDropTargetWnd<CGridListCtrlEx>* m_pOleDropTarget;	//!< Maintains OLE drag drop target
	friend class COleDropTargetWnd<CGridListCtrlEx>;
	virtual BOOL RegisterDropTarget();
	virtual DROPEFFECT DoDragDrop(COleDataSource& oleDataSource);
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual void OnDragLeave();
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual BOOL OnDropSelf(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual BOOL OnDropExternal(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual BOOL MoveSelectedRows(int nDropRow);

	// CustomDraw handlers
	virtual void OnCustomDrawRow(int nRow, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult);
	virtual void OnCustomDrawCell(int nRow, int nCol, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult);

	// Cell editing handlers
	virtual int OnClickEditStart(int nRow, int nCol, CPoint pt, BOOL bDblClick);
	virtual CWnd* OnEditBegin(int nRow, int nCol)
	{
		return OnEditBegin(nRow, nCol, CPoint(-1, -1));
	}
	virtual CWnd* OnEditBegin(int nRow, int nCol, CPoint pt);
	virtual BOOL OnEditComplete(int nRow, int nCol, CWnd* pEditor, LV_DISPINFO* pLVDI);

	// Context Menu Handlers
	virtual void OnContextMenuGrid(CWnd* pWnd, CPoint point);
	virtual void OnContextMenuHeader(CWnd* pWnd, CPoint point, int nCol);
	virtual void OnContextMenuKeyboard(CWnd* pWnd, CPoint point);
	virtual void OnContextMenuCell(CWnd* pWnd, CPoint point, int nRow, int nCol);

	virtual void OnCreateStyle();

	virtual void OnCopyToClipboard();

	//{{AFX_VIRTUAL(CGridListCtrlEx)
	virtual void PreSubclassWindow();
#if defined(_WIN64)
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
#else
	virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
#endif
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CGridListCtrlEx)
	virtual afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual afx_msg LRESULT OnDeleteColumn(WPARAM wParam, LPARAM lParam);
	virtual afx_msg LRESULT OnInsertColumn(WPARAM wParam, LPARAM lParam);
	virtual afx_msg BOOL OnItemClick(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnItemDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	virtual afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg LRESULT OnSetColumnWidth(WPARAM wParam, LPARAM lParam);
	virtual afx_msg BOOL OnHeaderDividerDblClick(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderBeginResize(UINT id, NMHDR* pNmhdr, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderBeginDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderEndResize(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderItemChanging(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderEndDrag(UINT id, NMHDR* pNmhdr, LRESULT* pResult);
	virtual afx_msg BOOL OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnToolNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg BOOL OnOwnerDataFindItem(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual afx_msg void OnContextMenu(CWnd*, CPoint point);
	virtual afx_msg void OnDestroy();
	virtual afx_msg void OnPaint();
	virtual afx_msg void OnKillFocus(CWnd* pNewWnd);
	virtual afx_msg LRESULT OnCopy(WPARAM wParam, LPARAM lParam);
	virtual afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	virtual afx_msg BOOL OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP();
};

//{{AFX_INSERT_LOCATION}}
