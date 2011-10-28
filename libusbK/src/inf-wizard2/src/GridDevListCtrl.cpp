#include "stdafx.h"
#include "CGridRowTraitXP.h"
#include "CGridColumnTraitImage.h"
#include "CGridColumnTraitEdit.h"
#include "GridDevListCtrl.h"
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

SERVICE_NAME_MAP g_ServiceMap[]=
{
	{_T("bthusb"), SERVICE_DISPLAY_DANGER},
	{_T("usbscan"), SERVICE_DISPLAY_DANGER},
	{_T("usbhub"), SERVICE_DISPLAY_DANGER},
	{_T("hidclass"), SERVICE_DISPLAY_WARNING},
	{_T("hidusb"), SERVICE_DISPLAY_WARNING},
	{_T("usbstor"), SERVICE_DISPLAY_DANGER},
	{_T("wpdusb"), SERVICE_DISPLAY_DANGER},
	{_T("usbprint"), SERVICE_DISPLAY_WARNING},
	{_T("usbccid"), SERVICE_DISPLAY_DANGER},
	{_T("usbvideo"), SERVICE_DISPLAY_DANGER},
	{_T("usbccgp"), SERVICE_DISPLAY_WARNING},
	{_T("usb8023"), SERVICE_DISPLAY_WARNING},
	{_T("winusb"), SERVICE_DISPLAY_OK},
	{_T("libusbk"), SERVICE_DISPLAY_OK},
	{_T("libusb0"), SERVICE_DISPLAY_OK},
};

/*
Audio Class								usbaudio,SERVICE_DISPLAY_DANGER
Bluetooth Class							bthusb, SERVICE_DISPLAY_DANGER
Imaging Class							usbscan, SERVICE_DISPLAY_DANGER
Hub Device Class						usbhub, SERVICE_DISPLAY_DANGER
Human Interface Device (HID) Class		hidclass (Win7), SERVICE_DISPLAY_DANGER
Human Interface Device (HID) Class		hidusb, SERVICE_DISPLAY_DANGER
Mass Storage Class						usbstor, SERVICE_DISPLAY_DANGER
Media Transfer Class					wpdusb, SERVICE_DISPLAY_DANGER
Printer Class							usbprint, SERVICE_DISPLAY_DANGER
Smart Card Class						usbccid , SERVICE_DISPLAY_DANGER
Video Class								usbvideo, SERVICE_DISPLAY_DANGER
USB Generic Parent Composite Driver		usbccgp, SERVICE_DISPLAY_WARNING
Communications Device Class (CDC)		usb8023, SERVICE_DISPLAY_WARNING
WinUSB Driver							winusb, SERVICE_DISPLAY_OK
libusbK Driver							libusbk, SERVICE_DISPLAY_OK
libusb-win32 Driver						libusb0, SERVICE_DISPLAY_OK
*/

CGridDevListCtrl::CGridDevListCtrl(void)
{
	CreateGridFont(TRUE,TRUE);
}

CGridDevListCtrl::~CGridDevListCtrl(void)
{
}

DWORD CGridDevListCtrl::GetServiceDisplay(CString serviceName)
{
	serviceName.Trim();
	serviceName.MakeLower();

	if (serviceName.IsEmpty() || serviceName == _T("(none)"))
		return SERVICE_DISPLAY_NODRIVER;

	int serviceCount = sizeof(g_ServiceMap)/sizeof(g_ServiceMap[0]);

	for (int serviceIndex=0; serviceIndex < serviceCount; serviceIndex++)
	{
		if (serviceName==g_ServiceMap[serviceIndex].ServiceName)
			return g_ServiceMap[serviceIndex].DisplayType;
	}

	return SERVICE_DISPLAY_WARNING;
}

BEGIN_MESSAGE_MAP(CGridDevListCtrl, CGridListCtrlEx)
	//{{AFX_MSG_MAP(CGridDevListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CGridDevListCtrl::GetDriverTip(CString serviceName, CString& serviceTip)
{
	serviceName.MakeLower();
	return FALSE;
}

BOOL CGridDevListCtrl::OnDisplayCellTooltip(int nRow, int nCol, CString& strResult)
{
	CString cellText;
	CString slDeviceID, slHardwareID,sDeviceID, sHardwareID;

	if (nRow!=-1 && nCol!=-1)
	{
		switch (nCol)
		{
		case DEVLST_COLID_VID:
			strResult = this->GetItemText(nRow,0);
			return TRUE;
		case DEVLST_COLID_PID:
			PWDI_DEVICE_INFO pDevInfo =(PWDI_DEVICE_INFO) this->GetItemData(nRow);
			ASSERT(pDevInfo!=NULL);

			slDeviceID.LoadString(IDS_DEVICE_ID);
			slHardwareID.LoadString(IDS_HARDWARE_ID);
			sDeviceID=pDevInfo->device_id;
			sHardwareID=pDevInfo->hardware_id;

			strResult.Format(_T("%s (%s)"), sDeviceID,sHardwareID);

			return TRUE;
		}

		strResult = GetItemText(nRow, nCol);	// use cell text for ToolTip
		return TRUE;
	}
	return FALSE;
}

BOOL CGridDevListCtrl::OnDisplayCellColor(int nRow, int nCol, COLORREF& textColor, COLORREF& backColor)
{
	if (nRow >=0 && nCol == DEVLST_COLID_DRIVER)
	{
		CString cellText=this->GetItemText(nRow,nCol);
		DWORD dwDispayType=GetServiceDisplay(cellText);
		switch(dwDispayType)
		{
		case SERVICE_DISPLAY_DANGER:
			textColor = CInfWizardDisplay::ColorError;
			return TRUE;
		case SERVICE_DISPLAY_WARNING:
			textColor = CInfWizardDisplay::ColorWarning;
			return TRUE;
		case SERVICE_DISPLAY_OK:
			textColor = CInfWizardDisplay::ColorGood;
			return TRUE;
		case SERVICE_DISPLAY_NODRIVER:
			textColor = CInfWizardDisplay::ColorSuccess;
			return TRUE;

		}

	}

	return FALSE;
}

BOOL CGridDevListCtrl::OnDisplayCellFont(int nRow, int nCol, LOGFONT& font)
{
	if (nRow >= 0)
	{
		font = m_LogFontGrid;
		return true;
	}
	return CGridListCtrlEx::OnDisplayCellFont(nRow,nCol,font);
}

BOOL CGridDevListCtrl::UpdateDevList(BOOL newDevicesOnly)
{
	if (newDevicesOnly)
		g_App->Wdi.CreateList(FALSE,FALSE);
	else
		g_App->Wdi.CreateList(TRUE,TRUE);

	DeleteAllItems();

	// Insert data into list-control by copying from datamodel
	for (int iRow=0; iRow < g_App->Wdi.DeviceItemArray.GetCount(); iRow++)
	{
		PWDI_DEVICE_INFO devInfo = g_App->Wdi.DeviceItemArray.GetAt(iRow);
		CString sDesc(devInfo->desc);
		CString sDriver(devInfo->driver);
		CString sVendorName(g_App->Wdi.GetVendorName(devInfo->vid));
		CString sVid,sPid;

		sVid.Format(_T("0x%04x"),devInfo->vid);
		sPid.Format(_T("0x%04x"),devInfo->pid);

		InsertItem(iRow, sVendorName);
		SetItemData(iRow, (DWORD_PTR)devInfo);

		for(int iCol = 1; iCol < GetColumnCount(); iCol++)
		{
			switch(iCol)
			{
			case 1:		// Vid
				SetItemText(iRow, iCol, sVid);
				break;
			case 2:		// Pid
				SetItemText(iRow, iCol, sPid);
				break;
			case 3:		// Description
				SetItemText(iRow, iCol, sDesc);
				break;
			case 4:		// Driver
				if (sDriver.IsEmpty())
					SetItemText(iRow, iCol, _T("(None)"));
				else
					SetItemText(iRow, iCol, sDriver);
				break;
			default:
				ASSERT(FALSE);
			}
		}

	}
	return TRUE;
}

BOOL CGridDevListCtrl::InitDevList(void)
{
	// Create and attach image list
	m_ImgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 0);

	RemoveAllGroups();

	// TODO: Add List Images
	// m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_FLGDEN));
	SetImageList(&m_ImgList, LVSIL_SMALL);

	// Give better margin to editors
	SetCellMargin(1.2);
	CGridRowTraitText* pRowTrait = new CGridRowTraitXP;
	SetDefaultRowTrait(pRowTrait);

	InsertHiddenLabelColumn();	// Requires one never uses column 0

	CString sColTitle;
	CGridColumnTraitText* gridColText;
	int colIndex = 0;
	int colWidth;

	gridColText = new CGridColumnTraitText;
	sColTitle.LoadString(IDS_VENDOR_ID);
	colWidth=100;
	colIndex++;
	gridColText->SetMetaFlag(GCSF_FIXED,true);
	InsertColumnTrait(colIndex, sColTitle, LVCFMT_LEFT, colWidth, colIndex, gridColText);

	gridColText = new CGridColumnTraitText;

	sColTitle.LoadString(IDS_PRODUCT_ID);
	colWidth=100;
	colIndex++;
	gridColText->SetMetaFlag(GCSF_FIXED,true);
	InsertColumnTrait(colIndex, sColTitle, LVCFMT_LEFT, colWidth, colIndex, gridColText);

	gridColText = new CGridColumnTraitText;

	sColTitle.LoadString(IDS_DESCRIPTION);
	colIndex++;
	gridColText->SetMetaFlag(GCSF_FIXED,false);
	InsertColumnTrait(colIndex, sColTitle, LVCFMT_LEFT, colWidth, colIndex, gridColText);

	gridColText = new CGridColumnTraitText;

	sColTitle.LoadString(IDS_INSTALLED_DRIVER);
	colWidth=150;
	colIndex++;
	gridColText->SetMetaFlag(GCSF_FIXED,true);
	InsertColumnTrait(colIndex, sColTitle, LVCFMT_LEFT, colWidth, colIndex, gridColText);

	CViewConfigSectionWinApp* pColumnProfile = new CViewConfigSectionWinApp(_T("Device List"));
	pColumnProfile->AddProfile(_T("Default"));
	SetupColumnConfig(pColumnProfile);

	UpdateDevList(TRUE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}