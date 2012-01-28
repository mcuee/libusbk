#include "dpscat.h"
#include "inf_parser.h"
#include <Shlwapi.h>

// warning C4127: conditional expression is constant
#pragma warning(disable: 4127)

BOOL KINF_API InfK_Init(PKINF_LIST* List)
{
	HANDLE heap;
	PKINF_LIST list;

	if ((heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0)) == NULL)
	{
		USBERRN("HeapCreate Failed. ErrorCode=%08Xh", GetLastError());
		return FALSE;
	}

	list = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KINF_LIST));
	if (list == NULL)
	{
		USBERRN("HeapAlloc Failed. ErrorCode=%08Xh", GetLastError());
		return FALSE;
	}

	list->HeapHandle = heap;
	*List = list;
	return TRUE;
}

BOOL KINF_API InfK_Free(PKINF_LIST List)
{
	if (List && List->HeapHandle)
	{
		HANDLE heap = List->HeapHandle;
		List->HeapHandle = NULL;
		HeapDestroy(heap);
	}

	return TRUE;
}

#define MAX_TEMP_BUFFERS 4
CHAR* WcsToTempMbs(WCHAR* str)
{
	static CHAR _strTempA[MAX_TEMP_BUFFERS][1024];
	static unsigned long _bufIndexA = 0;
	CHAR* pStrTempA;
	size_t len;

	pStrTempA = _strTempA[_bufIndexA++ & (MAX_TEMP_BUFFERS - 1)];

	len = wcstombs(pStrTempA, str, 1023);
	if (len == (size_t) - 1)
		*pStrTempA = '\0';
	else
		pStrTempA[len] = '\0';

	return pStrTempA;
}

WCHAR* MbsToTempWcs(CHAR* str)
{
	static WCHAR _strTempW[MAX_TEMP_BUFFERS][1024];
	static unsigned long _bufIndexW = 0;
	WCHAR* pStrTempW;
	size_t len;

	pStrTempW = _strTempW[_bufIndexW++ & (MAX_TEMP_BUFFERS - 1)];

	len = mbstowcs(pStrTempW, str, 1023);
	if (len == (size_t) - 1)
		*pStrTempW = '\0';
	else
		pStrTempW[len] = '\0';

	return pStrTempW;
}

CHAR* TcsToTempMbs(TCHAR* str)
{
#ifdef UNICODE
	return WcsToTempMbs(str);
#else
	return str;
#endif
}

WCHAR* TcsToTempWcs(TCHAR* str)
{
#ifdef UNICODE
	return str;
#else
	return MbsToTempWcs(str);
#endif
}

BOOL inf_AddSourceFiles(PKINF_LIST List, PKINF_EL infFileEL, HINF infHandle, INFCONTEXT* infContext, LPWSTR extension)
{
	BOOL success;
	DWORD length;
	static WCHAR sectionName[MAX_PATH_LENGTH];
	static WCHAR sourceFile[MAX_PATH];

	wcscpy_s(sectionName, sizeof(sectionName), L"SourceDisksFiles");
	if (extension)
	{
		if (_wcsnicmp(extension, L"NT", 2) == 0)
			extension = &extension[2];

		wcscat_s(sectionName, _countof(sectionName), L".");
		wcscat_s(sectionName, _countof(sectionName), extension);
	}

	success = SetupFindFirstLineW(infHandle, sectionName, NULL, infContext);
	while (success)
	{
		if (SetupGetStringFieldW(infContext, 0, sourceFile, sizeof(sourceFile), &length))
		{
			PKINF_FILE_EL fileCheckEL;
			PKINF_FILE_EL fileEL;

			fileEL = HeapAlloc(List->HeapHandle, HEAP_ZERO_MEMORY, sizeof(KINF_FILE_EL));

			wcscpy(fileEL->Filename, sourceFile);
			_wcslwr(fileEL->Filename);
			SetupGetStringFieldW(infContext, 2, fileEL->SubDir, sizeof(fileEL->SubDir), &length);

			DL_FOREACH(infFileEL->Files, fileCheckEL)
			{
				if (_wcsicmp(fileCheckEL->Filename, fileEL->Filename) == 0 &&
				        _wcsicmp(fileCheckEL->SubDir, fileEL->SubDir) == 0)
					break;
			}
			if (fileCheckEL == NULL)
			{
				DL_APPEND(infFileEL->Files, fileEL);
			}
			else
			{
				HeapFree(List->HeapHandle, 0, fileEL);
			}
		}
		success = SetupFindNextLine(infContext, infContext);
	}

	return TRUE;
}


BOOL KINF_API InfK_AddInfFile(PKINF_LIST List, LPCWSTR InfFilename)
{
	DWORD errorCode;
	PKINF_EL infFileEL = NULL;
	INFCONTEXT infContext, infContextProperty;
	HINF infHandle = NULL;
	BOOL success = TRUE;
	WCHAR sectionName[MAX_PATH * 4];
	DWORD sectionSize;
	UINT sectionIndex = (UINT) - 1;
	PKINF_FILE_EL fileEL;
	OSVERSIONINFOW osVersion;
	SP_ALTPLATFORM_INFO platFormInfo;

	memset(&infContext, 0, sizeof(infContext));
	if (!InfFilename)
	{
		USBERRN("Inf filename is NULL.");
		errorCode = ERROR_EMPTY;
		goto Error;
	}

	memset(&osVersion, 0, sizeof(osVersion));
	osVersion.dwOSVersionInfoSize = sizeof(osVersion);
	GetVersionExW(&osVersion);

	memset(&platFormInfo, 0, sizeof(platFormInfo));
	platFormInfo.cbSize = sizeof(platFormInfo);
	platFormInfo.Platform = osVersion.dwPlatformId;
	platFormInfo.ProcessorArchitecture = g_SystemInfo.wProcessorArchitecture;

	infHandle = SetupOpenInfFileW(InfFilename, NULL, INF_STYLE_WIN4, NULL);
	infFileEL = HeapAlloc(List->HeapHandle, HEAP_ZERO_MEMORY, sizeof(KINF_EL));
	if (!infFileEL)
	{
		USBERRN("Memory allocation failure.");
		goto Error;
	}

	// Get InfFullPath
	wcscpy_s(infFileEL->InfFullPath, MAX_PATH_LENGTH, InfFilename);

	// Get InfTitle
	wcscpy_s(infFileEL->InfTitle, MAX_PATH_LENGTH, InfFilename);
	PathStripPathW(infFileEL->InfTitle);

	// Get InfFullPath
	wcscpy_s(infFileEL->BaseDir, MAX_PATH_LENGTH, InfFilename);
	PathRemoveFileSpecW(infFileEL->BaseDir);

	// Move to the version section
	success = SetupFindFirstLineW(infHandle, L"Version", NULL, &infContext);
	if (!success)
	{
		USBERRN("SetupFindFirstLineW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	// Move to Version.CatalogFile
	success = SetupFindNextMatchLineW(&infContext, L"CatalogFile", &infContextProperty);
	if (!success)
	{
		USBERRN("SetupFindNextMatchLineW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	// Get CatalogFile name
	success = SetupGetStringFieldW(&infContextProperty, 1, infFileEL->CatTitle, sizeof(infFileEL->CatTitle), &sectionSize);
	if (!success)
	{
		USBERRN("SetupGetStringFieldW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	// Move to Version.VendorName
	success = SetupFindNextMatchLineW(&infContext, L"Provider", &infContextProperty);
	if (!success)
	{
		USBERRN("SetupFindNextMatchLineW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	// Get Provider
	success = SetupGetStringFieldW(&infContextProperty, 1, infFileEL->Provider, sizeof(infFileEL->Provider), &sectionSize);
	if (!success)
	{
		USBERRN("SetupGetStringFieldW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	// Get CatFullPath
	wcscpy_s(infFileEL->CatFullPath, _countof(infFileEL->CatFullPath), infFileEL->BaseDir);
	PathAppendW(infFileEL->CatFullPath, infFileEL->CatTitle);

	// Move to Manufacturer section
	success = SetupFindFirstLineW(infHandle, L"Manufacturer", NULL, &infContext);
	if (!success)
	{
		USBERRN("SetupFindFirstLineW Failed. ErrorCode=%08Xh", GetLastError());
		goto Error;
	}

	while(success)
	{
		// Get the models-section-name
		success = SetupGetStringFieldW(&infContext, 1, sectionName, sizeof(sectionName), &sectionSize);
		if (success)
		{
			WCHAR sectionNameWithExt[MAX_PATH];
			LPWSTR sectionNameExt = NULL;
			// Get the full models-section-name for the current PC
			success = SetupDiGetActualSectionToInstallExW(infHandle, sectionName, &platFormInfo, sectionNameWithExt, sizeof(sectionNameWithExt), &sectionSize, &sectionNameExt, NULL);
			if (success)
			{
				USBDBGN("ActualSectionToInstall=%s", WcsToTempMbs(sectionNameWithExt));
				success = SetupFindFirstLineW(infHandle, sectionNameWithExt, NULL, &infContextProperty);
				while (success)
				{
					sectionIndex = 1;

					// Get all the hardware ids for this section line
					while(SetupGetStringFieldW(&infContextProperty, ++sectionIndex, sectionName, sizeof(sectionName), &sectionSize))
					{
						PKINF_DEVICE_EL deviceEL = HeapAlloc(List->HeapHandle, HEAP_ZERO_MEMORY, sizeof(KINF_DEVICE_EL));
						wcscpy_s(deviceEL->HardwareID, sizeof(deviceEL->HardwareID), sectionName);
						DL_APPEND(infFileEL->Devices, deviceEL);
					}

					// Get the next section line
					success = SetupFindNextLine(&infContextProperty, &infContextProperty);
				}
			}
		}

		// Get the next models-section-name name
		success = SetupFindNextLine(&infContext, &infContext);
	}

	switch(g_SystemInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		inf_AddSourceFiles(List, infFileEL, infHandle, &infContext, L"amd64");
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		inf_AddSourceFiles(List, infFileEL, infHandle, &infContext, L"ia64");
		break;
	default:
		inf_AddSourceFiles(List, infFileEL, infHandle, &infContext, L"x86");
		break;
	}

	// Add this .inf file to the source files list
	fileEL = HeapAlloc(List->HeapHandle, HEAP_ZERO_MEMORY, sizeof(KINF_FILE_EL));
	wcscpy_s(fileEL->Filename, _countof(fileEL->Filename), infFileEL->InfTitle);
	DL_APPEND(infFileEL->Files, fileEL);

	// Append the inf file element
	DL_APPEND(List->Files, infFileEL);

	SetupCloseInfFile(infHandle);
	return TRUE;

Error:
	if (infHandle)
		SetupCloseInfFile(infHandle);
	if (infFileEL)
		HeapFree(List->HeapHandle, 0, infFileEL);

	return FALSE;
}

