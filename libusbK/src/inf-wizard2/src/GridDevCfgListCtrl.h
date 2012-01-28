#pragma once

#include "CGridListCtrlEx.h"
#include "LibWdiSession.h"
#include "InfWizardDisplay.h"

enum DEVCFG_FIELD_TYPE
{
    DEVCFG_FIELD_VID,
    DEVCFG_FIELD_PID,
    DEVCFG_FIELD_MFG,
    DEVCFG_FIELD_DESC,
    DEVCFG_FIELD_MI,
    DEVCFG_FIELD_GUID,
    DEVCFG_FIELD_INF_CLASS,
    DEVCFG_FIELD_INF_CLASS_GUID,
    DEVCFG_FIELD_INF_PROVIDER,

    DEVCFG_FIELD_MAX
};

enum DEVCFG_COLID_TYPE
{
    DEVCFG_COLID_IMAGE = 1,
    DEVCFG_COLID_FIELD,
    DEVCFG_COLID_VALUE,

    DEVCFG_COLID_MAX
};

class CGridDevCfgListCtrl : public CGridListCtrlEx, public CInfWizardDisplay
{

public:
	CGridDevCfgListCtrl(void);
	virtual ~CGridDevCfgListCtrl(void);
	virtual BOOL OnDisplayCellTooltip(int nRow, int nCol, CString** strResult, int* maxTipWidth, DWORD* tipDelayMS);
	CLibWdiSession* m_WdiSession;
	BOOL Load(CLibWdiSession* wdi);

	BOOL InitDevCfgList(CImageList& devCfgListImages);
private:
	BOOL AddColumnTraits(void);
	BOOL AddRow(int rowId, BOOL isMandatory, CString fieldName, CString fieldValue);

	CBitmap m_BmpBarOptional;
	CBitmap m_BmpBarMandatory;
public:
	virtual BOOL OnDisplayCellFont(int nRow, int nCol, LOGFONT& font);
	virtual BOOL OnDisplayCellColor(int nRow, int nCol, COLORREF& textColor, COLORREF& backColor);

protected:
	virtual BOOL OnEditComplete(int nRow, int nCol, CWnd* pEditor, LV_DISPINFO* pLVDI);

	// Generated message map functions
	//{{AFX_MSG(CGridDevCfgListCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
