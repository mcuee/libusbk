#pragma once

#include "CGridListCtrlEx.h"
#include "InfWizardDisplay.h"

enum DEVLST_COLID_TYPE
{
    DEVLST_COLID_VID = 1,
    DEVLST_COLID_PID,
    DEVLST_COLID_DESC,
    DEVLST_COLID_DRIVER,

    DEVLST_COLID_MAX
};

enum SERVICE_DISPLAY_TYPE
{
    SERVICE_DISPLAY_DANGER = 1,
    SERVICE_DISPLAY_WARNING,
    SERVICE_DISPLAY_OK,
    SERVICE_DISPLAY_NODRIVER,

};

typedef struct _SERVICE_NAME_MAP
{
	LPCTSTR ServiceName;
	DWORD DisplayType;
} SERVICE_NAME_MAP;

extern SERVICE_NAME_MAP g_ServiceMap[];

class CGridDevListCtrl : public CGridListCtrlEx, public CInfWizardDisplay
{

public:
	CGridDevListCtrl(void);
	virtual ~CGridDevListCtrl(void);
	virtual BOOL OnDisplayCellTooltip(int nRow, int nCol, CString& strResult);

	static BOOL GetDriverTip(CString serviceName, CString& serviceTip);
	static DWORD GetServiceDisplay(CString serviceName);
	BOOL InitDevList(void);
	BOOL UpdateDevList(BOOL newDevicesOnly);

private:
	CImageList m_ImgList;

protected:
	virtual BOOL OnDisplayCellFont(int nRow, int nCol, LOGFONT& font);
	virtual BOOL OnDisplayCellColor(int nRow, int nCol, COLORREF& textColor, COLORREF& backColor);

	// Generated message map functions
	//{{AFX_MSG(CGridDevListCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
