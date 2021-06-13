/*!********************************************************************
libusbK - [DPSCAT] Inf catalog and signing tool.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

NOTE: The following source files of dpscat cannot be placed under a BSD
license without also adhering to the LGPL license:
  - pki.c

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "dpscat.h"
#include "inf_parser.h"
#include "difx_dyn.h"
#include <Shellapi.h>
#include <Shlwapi.h>

#define HANDLE_IS_VALID(mHandle) (mHandle && mHandle!=INVALID_HANDLE_VALUE)

unsigned long DebugLevel = 4;

SYSTEM_INFO g_SystemInfo;
BOOL g_ConsoleAttached = FALSE;
HANDLE g_hConsoleOutput = NULL;
COORD g_CursorPos;
CONSOLE_SCREEN_BUFFER_INFO g_ScreenBufferInfo;

void ShowCopyright(void);
void ShowHelp(void);
int RunProgram(int argc, LPWSTR* argv);

static BOOL WINAPI WriteConA(
    CONST VOID* lpBuffer,
    DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten)
{
	BOOL bSuccess = FALSE;

	if (!HANDLE_IS_VALID(g_hConsoleOutput))
		return FALSE;

	bSuccess = WriteConsoleA(g_hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, NULL);

	return bSuccess;
}

static BOOL WINAPI WriteConW(
    CONST VOID* lpBuffer,
    DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten)
{
	BOOL bSuccess = FALSE;

	if (!HANDLE_IS_VALID(g_hConsoleOutput))
		return FALSE;

	bSuccess = WriteConsoleW(g_hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, NULL);

	return bSuccess;
}

VOID WriteInfStatus(CONST WCHAR* fmt, ...)
{
	static WCHAR buf[4096];
	INT len;
	va_list args;

	va_start(args, fmt);
	len = _vsnwprintf(buf, _countof(buf), fmt, args);
	va_end(args);

	if (len < 0 || len >= _countof(buf)) len = _countof(buf) - 1;
	buf[len] = '\0';

	if (g_ConsoleAttached && HANDLE_IS_VALID(g_hConsoleOutput))
	{
		if (WriteConW(buf, len, (LPDWORD)&len))
			return;

	} 
	else 
	{
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdOut != INVALID_HANDLE_VALUE) {
			DWORD transferred = 0;
			if (WriteFile(hStdOut, buf, len, &transferred, NULL)) {
				return;
			}
		}
	}
#ifdef _DEBUG
	OutputDebugStringW(buf);
#endif
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	int argCount = 0;
	int ret = 0;
	static WCHAR _lineBuffer[1024];
	static WCHAR _spaceBuffer[1024];
	DWORD numChars;

	LPWSTR lpCmdLineW		= GetCommandLineW();
	LPWSTR* lpCmdLineArgsW	= CommandLineToArgvW(lpCmdLineW, &argCount);

	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	memset(_lineBuffer, 0, sizeof(_lineBuffer));
	_wcsnset(_spaceBuffer, ' ', _countof(_spaceBuffer) - 1);
	_spaceBuffer[_countof(_spaceBuffer) - 1] = '\0';

	if ((g_ConsoleAttached = AttachConsole(ATTACH_PARENT_PROCESS)) != FALSE)
	{
		g_hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		if (HANDLE_IS_VALID(g_hConsoleOutput) && GetConsoleScreenBufferInfo(g_hConsoleOutput, &g_ScreenBufferInfo))
		{
			COORD posCursorOrig	= g_ScreenBufferInfo.dwCursorPosition;
			g_CursorPos		= posCursorOrig;

			numChars = posCursorOrig.X;
			if (numChars > _countof(_lineBuffer) - 1)
				numChars =  _countof(_lineBuffer) - 1;

			if (numChars)
			{
				posCursorOrig.X = 0;
				if (!ReadConsoleOutputCharacterW(g_hConsoleOutput, _lineBuffer, numChars, posCursorOrig, &numChars))
					numChars = 0;
				else
				{
					g_CursorPos.X = 0;
					SetConsoleCursorPosition(g_hConsoleOutput, g_CursorPos);
					WriteConW(_spaceBuffer, numChars, &numChars);
					if (--g_CursorPos.Y & 0x80000000)
						g_CursorPos.Y = 0;

					SetConsoleCursorPosition(g_hConsoleOutput, g_CursorPos);
				}
			}
			_lineBuffer[numChars] = (WCHAR)0;

		}
		else
		{
			FreeConsole();
			g_hConsoleOutput = INVALID_HANDLE_VALUE;
		}
	}

	ret = RunProgram(argCount, lpCmdLineArgsW);

	if (lpCmdLineArgsW)
		LocalFree(lpCmdLineArgsW);

	if (g_ConsoleAttached)
	{
		if (HANDLE_IS_VALID(g_hConsoleOutput) && _lineBuffer[0] != (WCHAR)0)
		{
			numChars = (DWORD)_tcslen(_lineBuffer);
			WriteConW(_lineBuffer, numChars, &numChars);
		}

		FreeConsole();
	}
	return ret;
}

int RunProgram(int argc, LPWSTR* argv)
{
	PKINF_LIST infList = NULL;
	PKINF_EL infEL = NULL;
	DWORD length;
	WCHAR searchPath[MAX_PATH_LENGTH];
	WCHAR infPathName[MAX_PATH_LENGTH];
	WCHAR certSubject[MAX_PATH_LENGTH];
	DWORD returnCode = ERROR_SUCCESS;
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	int iArg;
	BOOL bGetVerInfo = FALSE;
	WCHAR* pSearchPath = NULL;

	// startup initialization
	memset(searchPath, 0, sizeof(searchPath));
	ShowCopyright();

#if _WIN32_WINNT >= 0x0501
	GetNativeSystemInfo(&g_SystemInfo);
#else
	GetSystemInfo(&g_SystemInfo);
#endif

	// arg parsing

	for (iArg = 1; iArg < argc; iArg++)
	{
		if (_wcsicmp(argv[iArg], L"/path") == 0)
		{
			if (iArg+1 < argc)
			{
				iArg++;
				wcscpy_s(searchPath, _countof(searchPath), argv[iArg]);
				pSearchPath = searchPath;

				while(pSearchPath[0]==(WCHAR)'\"') pSearchPath++;
				while(pSearchPath[wcslen(pSearchPath)-1]==(WCHAR)'\"') pSearchPath[wcslen(pSearchPath)-1]=0;

				if (PathFileExistsW(pSearchPath))
				{
					PathAppendW(pSearchPath, L"*.inf");
				}
				else
				{
					returnCode = ERROR_PATH_NOT_FOUND;
					SetLastError(returnCode);
					USBERRN(L"searchPath does not exists.");
					goto Error;
				}
			}
			else
			{
				returnCode = ERROR_INVALID_PARAMETER;
				SetLastError(returnCode);
				USBERRN(L"Invalid parameter: %s",argv[iArg]);
				goto Error;
			}
		}
		else if (_wcsicmp(argv[iArg], L"/?") == 0)
		{
			ShowHelp();
			returnCode = 0;
			goto Error;
		}
		else if (_wcsicmp(argv[iArg], L"/getverinfo") == 0)
		{
			bGetVerInfo = TRUE;
		}
		else
		{
			returnCode = ERROR_INVALID_PARAMETER;
			SetLastError(returnCode);
			USBERRN(L"Invalid parameter: %s",argv[iArg]);
			ShowHelp();
			goto Error;
		}
	}

	USBMSGN(L"");
	if (pSearchPath == NULL)
	{
		// Set the .inf search path to the current directory.
		length = GetCurrentDirectoryW(_countof(searchPath), searchPath);
		if (!length)
		{
			returnCode = GetLastError();
			USBERRN(L"GetCurrentDirectory Failed. ErrorCode=%08Xh", returnCode);
			goto Error;
		}

		PathAppendW(searchPath, L"*.inf");
		pSearchPath = searchPath;
	}

	hFind = FindFirstFileW(pSearchPath, &findFileData);
	if (!hFind || hFind == INVALID_HANDLE_VALUE)
	{
		returnCode = ERROR_FILE_NOT_FOUND;
		SetLastError(returnCode);
		USBERRN(L"%s does not exist.", pSearchPath);
		goto Error;
	}

	InfK_Init(&infList);

	do
	{
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			memset(infPathName, 0, sizeof(infPathName));
			wcscpy_s(infPathName, _countof(infPathName), pSearchPath);
			PathRemoveFileSpecW(infPathName);
			PathAppendW(infPathName, findFileData.cFileName);

			InfK_AddInfFile(infList, infPathName);
		}
	}
	while(FindNextFileW(hFind, &findFileData));

	if (bGetVerInfo)
	{
		FILE* pFileVerInfo = _wfopen(L"infinfo.txt",L"ab+");
		if (!pFileVerInfo)
		{
			returnCode = ERROR_ACCESS_DENIED;
			SetLastError(returnCode);
			USBERRN(L"%s could not be opened.", L"infinfo.txt");
			goto Error;
		}

		DL_FOREACH(infList->Files, infEL)
		{
			length = _snwprintf(certSubject, _countof(certSubject), L"%u,%u,", infEL->DriverVerVersionBinaryH, infEL->DriverVerVersionBinaryL);
			fwrite(WcsToTempMbs(certSubject), 1, length, pFileVerInfo);
			fwrite(WcsToTempMbs(infEL->InfFullPath), 1, wcslen(infEL->InfFullPath), pFileVerInfo);
			fwrite("\n",1,1,pFileVerInfo);
		}

		fflush(pFileVerInfo);
		fclose(pFileVerInfo);
	}
	else
	{
		DL_FOREACH(infList->Files, infEL)
		{
			int lenSubject = _snwprintf(certSubject, _countof(certSubject), L"CN=\"%s (%s) [Self]\"", infEL->Provider, infEL->InfTitle);			if (lenSubject < 0 || lenSubject >= _countof(certSubject)) lenSubject = _countof(certSubject) - 1;
			certSubject[lenSubject] = '\0';

			if (CreateCatEx(infEL))
				SelfSignFile(infEL->CatFullPath, certSubject);
		}
	}

Error:
	USBMSG(L"\n");
	InfK_Free(infList);
	return (int)returnCode;

}

//////////////////////////////////////////////////////////////////////////////
/* END OF PROGRAM                                                           */
//////////////////////////////////////////////////////////////////////////////
void ShowCopyright(void)
{
	USBMSGN(L"Copyright(c) 2012 Travis Lee Robinson. (DUAL BSD/GPL)");
	USBMSGN(L"Portions Copyright(c) Pete Batard. (LGPL)");
}

void ShowHelp(void)
{
	CONST WCHAR* src;
	DWORD src_count, charsWritten;
	HGLOBAL res_data;
	HRSRC hSrc;

	USBMSGN(L"");

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_HELP_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return;

	src_count = SizeofResource(NULL, hSrc);
	if (!src_count)	return;

	src_count /= sizeof(WCHAR);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return;

	src = (WCHAR*) LockResource(res_data);
	if (!src) return;

	WriteConW(src, src_count, &charsWritten);
}
