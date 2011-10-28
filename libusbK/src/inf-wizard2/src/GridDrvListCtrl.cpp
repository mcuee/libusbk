#include "stdafx.h"
#include "CGridRowTraitXP.h"
#include "CGridColumnTraitImage.h"
#include "GridDrvListCtrl.h"
#include "ViewConfigSection.h"
#include "resource.h"       // main symbols
#include "libwdimanager.h"
#include "InfWizardApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern InfWizardApp* g_App;

CGridDrvListCtrl::CGridDrvListCtrl(void)
{
	CreateGridFont(TRUE, TRUE);
}

CGridDrvListCtrl::~CGridDrvListCtrl(void)
{
}

BEGIN_MESSAGE_MAP(CGridDrvListCtrl, CGridListCtrlEx)
	//{{AFX_MSG_MAP(CGridDrvListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CGridDrvListCtrl::AddColumnTraits(void)
{
	int colIndex = InsertHiddenLabelColumn();	// Requires one never uses column 0

	CGridColumnTraitText*	colDriverText = new CGridColumnTraitText;
	CGridColumnTraitImage*	colDriverImage_Usb0 = new CGridColumnTraitImage;
	CGridColumnTraitImage*	colDriverImage_UsbK = new CGridColumnTraitImage;
	CGridColumnTraitImage*	colDriverImage_Usb1 = new CGridColumnTraitImage;


	colDriverText->SetMetaFlag(GCSF_FIXED, true);
	colDriverImage_Usb0->SetMetaFlag(GCSF_FIXED, true);
	colDriverImage_UsbK->SetMetaFlag(GCSF_FIXED, true);
	colDriverImage_Usb1->SetMetaFlag(GCSF_FIXED, true);

	int colWidth;

	CRect rectGrid;
	GetClientRect(&rectGrid);
	colWidth = rectGrid.Width() + 25;

	colIndex++;
	InsertColumnTrait(colIndex, _T("Kernel Driver Package(s)"), LVCFMT_LEFT, colWidth, colIndex, colDriverText);

	colWidth = 60;

	colIndex++;
	InsertColumnTrait(colIndex, _T("libusb0"), LVCFMT_LEFT, colWidth, colIndex, colDriverImage_Usb0);

	colIndex++;
	InsertColumnTrait(colIndex, _T("libusbK"), LVCFMT_LEFT, colWidth, colIndex, colDriverImage_UsbK);
	colIndex++;

	InsertColumnTrait(colIndex, _T("libusb-1.0"), LVCFMT_LEFT, colWidth, colIndex, colDriverImage_Usb1);
	colIndex++;

	return TRUE;
}

BOOL CGridDrvListCtrl::AddRow(int driverType, LPCTSTR displayName, BOOL hasUsb0Support, BOOL hasUsbKSupport, BOOL hasUsb1Support)
{
	CString sSupportedDrivers;
	VS_FIXEDFILEINFO fileInfo;
	CLibWdiDynamicAPI& API = g_App->Wdi;
	int nItem = GetItemCount();

	if (!API.IsApiLoaded())
		return FALSE;

	if (API.IsDriverSupported(driverType, &fileInfo))
	{
		sSupportedDrivers.Format(_T("%s v%d.%d.%d.%d\n"),
		                         displayName,
		                         (int)fileInfo.dwFileVersionMS >> 16,
		                         (int)fileInfo.dwFileVersionMS & 0xFFFF,
		                         (int)fileInfo.dwFileVersionLS >> 16,
		                         (int)fileInfo.dwFileVersionLS & 0xFFFF);

		// Row
		InsertItem(nItem, _T(""), 0);
		SetItemData(nItem, (DWORD_PTR)driverType);

		// Col:DisplayText
		int colIndex = 1;
		SetItemText(nItem, colIndex++, sSupportedDrivers);

		// Col:Usb0
		SetCellImage(nItem, colIndex++, hasUsb0Support ? 0 : 1);
		// Col:UsbK
		SetCellImage(nItem, colIndex++, hasUsbKSupport ? 0 : 1);
		// Col:Usb1
		SetCellImage(nItem, colIndex++, hasUsb1Support ? 0 : 1);

	}

	return TRUE;
}

BOOL CGridDrvListCtrl::InitDriverList(CImageList& driverListImages)
{
	// Create and attach image list
	driverListImages.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 0);


	SetExtendedStyle(GetExtendedStyle() & ~LVS_EX_GRIDLINES);
	RemoveAllGroups();

	// TODO: Add List Images
	driverListImages.Add(AfxGetApp()->LoadIcon(IDI_ICON_OK));
	driverListImages.Add(AfxGetApp()->LoadIcon(IDI_ICON_EXCLAMATION));
	SetImageList(&driverListImages, LVSIL_SMALL);

	// Give better margin to editors
	SetCellMargin(1.2);
	CGridRowTraitText* pRowTrait = new CGridRowTraitXP;
	SetDefaultRowTrait(pRowTrait);

	AddColumnTraits();

	CViewConfigSectionWinApp* pColumnProfile = new CViewConfigSectionWinApp(_T("Driver List"));
	pColumnProfile->AddProfile(_T("Default"));
	SetupColumnConfig(pColumnProfile);

	UpdateDriverList();

	return TRUE;  // return TRUE  unless you set the focus to a control
}
BOOL CGridDrvListCtrl::UpdateDriverList(void)
{
	DeleteAllItems();

	AddRow(WDI_WINUSB, _T("WinUsb"), FALSE, TRUE, TRUE);
	AddRow(WDI_LIBUSBK, _T("libusbK"), TRUE, TRUE, FALSE);
	AddRow(WDI_LIBUSB0, _T("libusb0"), TRUE, TRUE, FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}
BOOL CGridDrvListCtrl::OnDisplayCellTooltip(int nRow, int nCol, CString& strResult)
{
	if (nRow == -1 && nCol != -1)
	{
		switch(nCol)
		{
		case 2:
			strResult = "Support by libusb0.dll";
			return TRUE;
		case 3:
			strResult = "Support by libusbK.dll";
			return TRUE;
		case 4:
			strResult = "Support by libusb-1.0.dll";
			return TRUE;
		}
	}
	if (nRow != -1 && nCol != -1)
	{
		int driverType = this->GetItemData(nRow);

		switch(nCol)
		{
		case 1:		// Display text
			strResult = GetItemText(nRow, nCol);	// Cell-ToolTip
			break;
		case 2:		// Usb0
			switch(driverType)
			{
			case WDI_WINUSB:
				strResult.LoadString(IDS_DLL_USB0_NOT_SUPPORTED);
				break;
			default:
				strResult.LoadString(IDS_DLL_USB0_SUPPORTED);
			}
			break;
		case 3:		// UsbK
			strResult.LoadString(IDS_DLL_USBK_SUPPORTED);
			break;
		case 4:		// Usb1
			switch(driverType)
			{
			case WDI_WINUSB:
				strResult.LoadString(IDS_DLL_USB1_SUPPORTED);
				break;
			default:
				strResult.LoadString(IDS_DLL_USB1_NOT_SUPPORTED);
			}
			break;
		}

		return true;
	}
	return false;
}

BOOL CGridDrvListCtrl::OnDisplayCellFont(int nRow, int nCol, LOGFONT& font)
{
	if (nRow >= 0)
	{
		if (nCol == 1)
		{
			font = m_LogFontGrid;
			return true;
		}
	}
	return CGridListCtrlEx::OnDisplayCellFont(nRow, nCol, font);
}