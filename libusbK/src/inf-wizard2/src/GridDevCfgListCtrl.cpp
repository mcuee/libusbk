#include "stdafx.h"
#include "CGridRowTraitXP.h"
#include "CGridColumnTraitImage.h"
#include "CGridColumnTraitEdit.h"
#include "GridDevCfgListCtrl.h"
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

#define BAR_IMAGE_WIDTH (7)

CGridDevCfgListCtrl::CGridDevCfgListCtrl(void)
{
	m_bUseVisualStyles=FALSE;
	CreateGridFont(TRUE,TRUE);
}

CGridDevCfgListCtrl::~CGridDevCfgListCtrl(void)
{
}

BEGIN_MESSAGE_MAP(CGridDevCfgListCtrl, CGridListCtrlEx)
	//{{AFX_MSG_MAP(CGridDevCfgListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CGridDevCfgListCtrl::AddColumnTraits(void)
{
	int colIndex = InsertHiddenLabelColumn();	// Requires one never uses column 0
	colIndex++;

	CGridColumnTraitImage*	colImage	= new CGridColumnTraitImage;
	CGridColumnTraitText*	colLabelText = new CGridColumnTraitText;
	CGridColumnTraitEdit*	colValueText = new CGridColumnTraitEdit;

	colImage->SetMetaFlag(GCSF_FIXED,true);
	colLabelText->SetMetaFlag(GCSF_FIXED,true);
	colValueText->SetMetaFlag(GCSF_FIXED,false);

	for (colIndex; colIndex < DEVCFG_COLID_MAX; colIndex++)
	{
		switch(colIndex)
		{
		case DEVCFG_COLID_IMAGE:
			InsertColumnTrait(colIndex, _T("Image"), LVCFMT_CENTER, BAR_IMAGE_WIDTH, colIndex, colImage);
			break;
		case DEVCFG_COLID_FIELD:
			InsertColumnTrait(colIndex, _T("Field"), LVCFMT_RIGHT, 100, colIndex, colLabelText);
			break;
		case DEVCFG_COLID_VALUE:
			InsertColumnTrait(colIndex, _T("Value"), LVCFMT_LEFT, 250, colIndex, colValueText);
			break;
		default:
			ASSERT(FALSE);
		}
	}

	return TRUE;
}

BOOL CGridDevCfgListCtrl::AddRow(int rowId, BOOL isMandatory, CString fieldName, CString fieldValue)
{
	// Row
	InsertItem(rowId,_T(""),0);
	SetItemData(rowId,rowId);

	if (fieldName.GetAt(fieldName.GetLength()-1)!=(TCHAR)'#')
		fieldName.AppendChar((TCHAR)':');

	for (int colIndex=1; colIndex < DEVCFG_COLID_MAX; colIndex++)
	{
		switch(colIndex)
		{
		case DEVCFG_COLID_IMAGE:
			SetItemText(rowId,colIndex,fieldName);
			SetCellImage(rowId,colIndex,isMandatory);
			break;
		case DEVCFG_COLID_FIELD:
			SetItemText(rowId,colIndex,fieldName);
			break;
		case DEVCFG_COLID_VALUE:
			SetItemText(rowId,colIndex,fieldValue);
			break;
		default:
			ASSERT(FALSE);
		}
	}
	return TRUE;
}

BOOL CGridDevCfgListCtrl::InitDevCfgList(CImageList& devCfgListImages)
{
	// Create and attach image list
	RemoveAllGroups();

	m_BmpBarMandatory.LoadBitmap(IDB_BAR_ORANGE);
	m_BmpBarOptional.LoadBitmap(IDB_BAR_BLUE);

	devCfgListImages.Create(BAR_IMAGE_WIDTH,25,ILC_COLOR24, 1, 0);

	devCfgListImages.Add(&m_BmpBarOptional,(COLORREF)RGB(255,0,255));
	devCfgListImages.Add(&m_BmpBarMandatory,(COLORREF)RGB(255,0,255));

	SetImageList(&devCfgListImages, LVSIL_SMALL);

	// Give better margin to editors
	SetCellMargin(1.2);

	CGridRowTraitXP* pRowTrait = new CGridRowTraitXP;
	SetDefaultRowTrait(pRowTrait);
	AddColumnTraits();



	CViewConfigSectionWinApp* pColumnProfile = new CViewConfigSectionWinApp(_T("Device Config List"));
	pColumnProfile->AddProfile(_T("Default"));
	SetupColumnConfig(pColumnProfile);

	return TRUE;  // return TRUE  unless you set the focus to a control
}


BOOL CGridDevCfgListCtrl::OnDisplayCellFont(int nRow, int nCol, LOGFONT& font)
{
	if (nRow >= 0)
	{
		font = m_LogFontGrid;
		return true;
	}
	return CGridListCtrlEx::OnDisplayCellFont(nRow,nCol,font);
}

BOOL CGridDevCfgListCtrl::OnDisplayCellColor(int nRow, int nCol, COLORREF& textColor, COLORREF& backColor)
{
	if (nRow >= 0)
	{
		switch(nCol)
		{
		case DEVCFG_COLID_IMAGE:
			goto Done;
		case DEVCFG_COLID_FIELD:
			//backColor=(COLORREF)RGB(230,242,255);
			//textColor=(COLORREF)RGB(255,255,255);
			backColor = (COLORREF)GetSysColor(COLOR_3DLIGHT);
			switch(nRow)
			{
			case DEVCFG_FIELD_VID:
			case DEVCFG_FIELD_PID:
				textColor = CInfWizardDisplay::ColorError;
				break;
			case DEVCFG_FIELD_MFG:
			case DEVCFG_FIELD_DESC:
				return TRUE;
			default:
				textColor = (COLORREF)GetSysColor(COLOR_3DDKSHADOW);

			}
			break;
		case DEVCFG_COLID_VALUE:
			backColor=(COLORREF)RGB(255,255,255);
			break;
		default:
			ASSERT(FALSE);
		}
		return TRUE;
	}

Done:
	return CGridListCtrlEx::OnDisplayCellColor(nRow,nCol,textColor,backColor);
}

BOOL CGridDevCfgListCtrl::OnDisplayCellTooltip(int nRow, int nCol, CString& strResult)
{
	if (nRow!=-1 && nCol!=-1)
	{
		if (nCol==DEVCFG_COLID_FIELD)
		{
			switch(nRow)
			{
			case DEVCFG_FIELD_VID:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_VENDOR_ID)->GetBuffer(0);
				return TRUE;

			case DEVCFG_FIELD_PID:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_PRODUCT_ID)->GetBuffer(0);
				return TRUE;

			case DEVCFG_FIELD_MFG:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_MANUFACTURER)->GetBuffer(0);
				return TRUE;

			case DEVCFG_FIELD_DESC:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_DEVICE_DESCRIPTION)->GetBuffer(0);
				return TRUE;

			case DEVCFG_FIELD_MI:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_INTERFACE_NUMBER)->GetBuffer(0);
				return TRUE;

			case DEVCFG_FIELD_GUID:
				strResult = CInfWizardDisplay::GetTipString(IDS_TIP_INTERFACE_GUID)->GetBuffer(0);
				return TRUE;

				//case DEVCFG_FIELD_DEVID:
				//	strResult = CInfWizardDisplay::GetTipString(IDS_TIP_DEVICE_ID)->GetBuffer(0);
				//return TRUE;

			}
		}
		strResult = GetItemText(nRow, nCol);	// Cell-ToolTip
		return true;
	}
	return false;
}

BOOL CGridDevCfgListCtrl::Load(CLibWdiSession* wdi)
{

	m_WdiSession = wdi;
	CString fieldName,fieldValue;

	for (int iRow=0; iRow < DEVCFG_FIELD_MAX; iRow++)
	{
		BOOL isMandatory = FALSE;

		switch(iRow)
		{
		case DEVCFG_FIELD_VID:
			fieldName.LoadString(IDS_VENDOR_ID);
			fieldValue.Format(_T("0x%04X"),wdi->vid);
			isMandatory = TRUE;
			break;
		case DEVCFG_FIELD_PID:
			fieldName.LoadString(IDS_PRODUCT_ID);
			fieldValue.Format(_T("0x%04X"),wdi->pid);
			isMandatory = TRUE;
			break;
		case DEVCFG_FIELD_MFG:
			fieldName.LoadString(IDS_MANUFACTURER);
			fieldValue = wdi->m_VendorName;
			break;
		case DEVCFG_FIELD_DESC:
			fieldName.LoadString(IDS_DESCRIPTION);
			fieldValue = wdi->desc;
			break;
		case DEVCFG_FIELD_MI:
			fieldName.LoadString(IDS_MI);
			if (wdi->is_composite)
				fieldValue.Format(_T("0x%02X"),wdi->mi);
			else
				fieldValue.Empty();
			break;
			//		case DEVCFG_FIELD_DEVID:
			//			fieldName.LoadString(IDS_DEVICE_ID);
			//			fieldValue = wdi->device_id;
			//			break;
		case DEVCFG_FIELD_GUID:
			fieldName.LoadString(IDS_GUID);
			fieldValue = wdi->GetGuid();
			break;
		default:
			ASSERT(FALSE);
		}

		AddRow(iRow, isMandatory, fieldName, fieldValue);
	}

	return TRUE;
}

BOOL CGridDevCfgListCtrl::OnEditComplete(int nRow, int nCol, CWnd* pEditor, LV_DISPINFO* pLVDI)
{
	return  CGridListCtrlEx::OnEditComplete(nRow,nCol,pEditor,pLVDI);
}

