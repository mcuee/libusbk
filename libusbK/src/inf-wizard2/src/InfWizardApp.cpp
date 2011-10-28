// InfWizardApp.cpp : Defines the class behaviors for the application.
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "InfWizardApp.h"
#include "InfWizardDlg.h"
#include "InfWizardDisplay.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

InfWizardApp* g_App;

/////////////////////////////////////////////////////////////////////////////
// InfWizardApp

BEGIN_MESSAGE_MAP(InfWizardApp, CWinApp)
	//{{AFX_MSG_MAP(InfWizardApp)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// InfWizardApp construction

InfWizardApp::InfWizardApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only InfWizardApp object

InfWizardApp theApp;

/////////////////////////////////////////////////////////////////////////////
// InfWizardApp initialization

BOOL InfWizardApp::InitInstance()
{
	g_App = &theApp;

	//TODO: call AfxInitRichEdit2() to initialize richedit2 library.
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
	::AfxInitCommonControlsEx(&InitCtrls);

	::AfxInitRichEdit2();

	InfWizardDlg dlg;
	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

