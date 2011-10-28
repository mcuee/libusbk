#pragma once

#include "CGridListCtrlEx.h"
#include "InfWizardDisplay.h"

class CGridDrvListCtrl : public CGridListCtrlEx, public CInfWizardDisplay
{

public:
	CGridDrvListCtrl(void);
	virtual ~CGridDrvListCtrl(void);
	virtual BOOL OnDisplayCellTooltip(int nRow, int nCol, CString& strResult);

	BOOL InitDriverList(CImageList& driverListImages);
	BOOL UpdateDriverList(void);
	BOOL TryAppendDriverString(int driverType, LPCTSTR displayName, CString* sSupportedDrivers);
private:
	BOOL AddColumnTraits(void);
	BOOL AddRow(int driverType, LPCTSTR displayName, BOOL hasUsb0Support, BOOL hasUsbKSupport, BOOL hasUsb1Support);

protected:
	virtual BOOL OnDisplayCellFont(int nRow, int nCol, LOGFONT& font);
	// Generated message map functions
	//{{AFX_MSG(CGridDrvListCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
