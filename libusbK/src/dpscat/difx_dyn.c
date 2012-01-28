#include "dpscat.h"
#include "difx_dyn.h"

// warning C4127: conditional expression is constant
#pragma warning(disable: 4127)

HMODULE hDifxDll = NULL;

DriverPackagePreinstallW_t* DriverPackagePreinstallW = NULL;
DriverPackagePreinstallA_t* DriverPackagePreinstallA = NULL;
DriverPackageInstallW_t* DriverPackageInstallW = NULL;
DriverPackageInstallA_t* DriverPackageInstallA = NULL;
DriverPackageUninstallW_t* DriverPackageUninstallW = NULL;
DriverPackageUninstallA_t* DriverPackageUninstallA = NULL;
DriverPackageGetPathW_t* DriverPackageGetPathW = NULL;
DriverPackageGetPathA_t* DriverPackageGetPathA = NULL;
SetDifxLogCallbackW_t* SetDifxLogCallbackW = NULL;
SetDifxLogCallbackA_t* SetDifxLogCallbackA = NULL;
DIFXAPISetLogCallbackW_t* DIFXAPISetLogCallbackW = NULL;
DIFXAPISetLogCallbackA_t* DIFXAPISetLogCallbackA = NULL;

#ifndef DEFINE_TO_STR
#define _DEFINE_TO_STR(x) #x
#define  DEFINE_TO_STR(x) _DEFINE_TO_STR(x)
#endif

#define DIFX_GETPROCADDRESS(mProcField,ErrorJump) do {								\
		mProcField=(mProcField##_t*)GetProcAddress(hDifxDll,DEFINE_TO_STR(mProcField));	\
		if (mProcField == NULL) goto ErrorJump;											\
	}while(0)

BOOL DifK_Init(LPCTSTR LibFullPath)
{
	hDifxDll = LoadLibraryEx(LibFullPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hDifxDll) hDifxDll = LoadLibrary(_T("DIFxAPI.dll"));
	if (!hDifxDll) return FALSE;

	DIFX_GETPROCADDRESS(DriverPackagePreinstallW, Error);
	DIFX_GETPROCADDRESS(DriverPackagePreinstallA, Error);
	DIFX_GETPROCADDRESS(DriverPackageInstallW, Error);
	DIFX_GETPROCADDRESS(DriverPackageInstallA, Error);
	DIFX_GETPROCADDRESS(DriverPackageUninstallW, Error);
	DIFX_GETPROCADDRESS(DriverPackageUninstallA, Error);
	DIFX_GETPROCADDRESS(DriverPackageGetPathW, Error);
	DIFX_GETPROCADDRESS(DriverPackageGetPathA, Error);
	DIFX_GETPROCADDRESS(SetDifxLogCallbackW, Error);
	DIFX_GETPROCADDRESS(SetDifxLogCallbackA, Error);
	DIFX_GETPROCADDRESS(DIFXAPISetLogCallbackW, Error);
	DIFX_GETPROCADDRESS(DIFXAPISetLogCallbackA, Error);

	return hDifxDll != NULL;

Error:
	DifK_Free();
	return FALSE;
}

VOID DifK_Free(void)
{
	if (hDifxDll)
	{
		FreeLibrary(hDifxDll);
		hDifxDll = NULL;
	}
}
