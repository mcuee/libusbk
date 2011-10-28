#pragma once

#define DEF_FONTNAME _T("Tahoma")

class CInfWizardDisplay
{
public:
	CInfWizardDisplay(void);
	~CInfWizardDisplay(void);

protected:
	CFont	m_FontGrid;
	LOGFONT	m_LogFontGrid;
	void CreateGridFont(BOOL bSmall, BOOL bBold);

public:
	static COLORREF ColorError;
	static COLORREF ColorWarning;
	static COLORREF ColorGood;
	static COLORREF ColorSuccess;

	static void AddTipString(WORD nID);
	static CString* GetTipString(WORD nID);
	static BOOL CreateWizardFont(CFont& font, LOGFONT& logFont, BOOL bSmall = TRUE, BOOL bBold = TRUE);

	static void AddCallbackTool(CToolTipCtrl* tipCtrl, CWnd* parent,  WORD nDlgID, LPARAM nStringID);
	static BOOL HandleToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

private:
	static void AllocToolTipStrings(void);
	static void FreeToolTipStrings(void);
	static CMapWordToPtr m_ToolTipStrings;

};
