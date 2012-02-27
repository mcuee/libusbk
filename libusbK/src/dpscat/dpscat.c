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

VOID WriteInfStatus(CONST CHAR* fmt, ...)
{
	static CHAR buf[256];
	INT len;
	va_list args;

	va_start(args, fmt);
#if __STDC_WANT_SECURE_LIB__
	len = _vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, args);
#else
	len = _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
#endif
	va_end(args);

	if (len < 0)
		len = sizeof(buf) - 1;
	buf[len] = '\0';

	if (g_ConsoleAttached && HANDLE_IS_VALID(g_hConsoleOutput))
	{
		if (WriteConA(buf, len, (LPDWORD)&len))
			return;

	}
	OutputDebugStringA(buf);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	int argCount = 0;
	int ret = 0;
	static TCHAR _lineBuffer[1024];
	static TCHAR _spaceBuffer[1024];
	DWORD numChars;

	LPWSTR lpCmdLineW		= GetCommandLineW();
	LPWSTR* lpCmdLineArgsW	= CommandLineToArgvW(lpCmdLineW, &argCount);

	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	memset(_lineBuffer, 0, sizeof(_lineBuffer));
	memset(_spaceBuffer, ' ', sizeof(_lineBuffer));
	_spaceBuffer[sizeof(_lineBuffer) - 1] = '\0';

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
				if (!ReadConsoleOutputCharacter(g_hConsoleOutput, _lineBuffer, numChars, posCursorOrig, &numChars))
					numChars = 0;
				else
				{
					g_CursorPos.X = 0;
					SetConsoleCursorPosition(g_hConsoleOutput, g_CursorPos);
					WriteConA(_spaceBuffer, numChars, &numChars);
					if (--g_CursorPos.Y & 0x80000000)
						g_CursorPos.Y = 0;

					SetConsoleCursorPosition(g_hConsoleOutput, g_CursorPos);
				}
			}
			_lineBuffer[numChars] = (TCHAR)0;

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
		if (HANDLE_IS_VALID(g_hConsoleOutput) && _lineBuffer[0] != (TCHAR)0)
		{
			numChars = (DWORD)_tcslen(_lineBuffer);
			WriteConW(_lineBuffer, numChars, &numChars);
		}

		FreeConsole();
	}
}

int RunProgram(int argc, LPWSTR* argv)
{
	PKINF_LIST infList = NULL;
	PKINF_EL infEL = NULL;
	DWORD length;
	WCHAR searchPath[MAX_PATH_LENGTH];
	WCHAR infPathName[MAX_PATH_LENGTH];
	CHAR certSubject[MAX_PATH_LENGTH];
	DWORD returnCode = ERROR_SUCCESS;
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;

	// startup initialization
	memset(searchPath, 0, sizeof(searchPath));
	ShowCopyright();

#if _WIN32_WINNT >= 0x0501
	GetNativeSystemInfo(&g_SystemInfo);
#else
	GetSystemInfo(&g_SystemInfo);
#endif

	// arg parsing
	if (argc == 3)
	{
		USBMSGN("");
		if (_wcsicmp(argv[1], L"/path") != 0)
		{
			returnCode = ERROR_INVALID_PARAMETER;
			SetLastError(returnCode);
			USBERRN("Invalid parameter: %s", WcsToTempMbs(argv[1]));
			goto Error;
		}
		if (PathFileExistsW(argv[2]))
		{
			wcscpy_s(searchPath, _countof(searchPath), argv[2]);
			PathAppendW(searchPath, L"*.inf");

		}
		else
		{
			returnCode = ERROR_PATH_NOT_FOUND;
			SetLastError(returnCode);
			USBERRN("searchPath does not exists.");
			goto Error;
		}
	}
	else if (argc == 2)
	{
		ShowHelp();
		returnCode = 0;
		goto Error;
	}
	else
	{
		USBMSGN("");
		// Set the .inf search path to the current directory.
		length = GetCurrentDirectoryW(sizeof(searchPath), searchPath);
		if (!length)
		{
			returnCode = GetLastError();
			USBERRN("GetCurrentDirectory Failed. ErrorCode=%08Xh", returnCode);
			goto Error;
		}

		PathAppendW(searchPath, L"*.inf");

	}


	hFind = FindFirstFileW(searchPath, &findFileData);
	if (!hFind || hFind == INVALID_HANDLE_VALUE)
	{
		returnCode = ERROR_FILE_NOT_FOUND;
		SetLastError(returnCode);
		USBERRN("%s does not exist.", WcsToTempMbs(searchPath));
		goto Error;
	}

	InfK_Init(&infList);

	do
	{
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			memset(infPathName, 0, sizeof(infPathName));
			wcscpy_s(infPathName, _countof(infPathName), searchPath);
			PathRemoveFileSpecW(infPathName);
			PathAppendW(infPathName, findFileData.cFileName);

			InfK_AddInfFile(infList, infPathName);
		}
	}
	while(FindNextFileW(hFind, &findFileData));


	DL_FOREACH(infList->Files, infEL)
	{
		sprintf(certSubject, "CN=%s (SelfSigned)", WcsToTempMbs(infEL->Provider));

		if (CreateCatEx(infEL))
			SelfSignFile(WcsToTempMbs(infEL->CatFullPath), certSubject);
	}

Error:
	USBMSG("\n");
	InfK_Free(infList);
	return (int)returnCode;

}

//////////////////////////////////////////////////////////////////////////////
/* END OF PROGRAM                                                           */
//////////////////////////////////////////////////////////////////////////////
void ShowCopyright(void)
{
	USBMSGN("Copyright(c) 2012 Travis Lee Robinson. (DUAL BSD/GPL)");
	USBMSGN("Portions Copyright(c) Pete Batard. (LGPL)");
}

void ShowHelp(void)
{
	CONST CHAR* src;
	DWORD src_count, charsWritten;
	HGLOBAL res_data;
	HRSRC hSrc;

	USBMSGN("");

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_HELP_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return;

	src_count = SizeofResource(NULL, hSrc);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return;

	src = (char*) LockResource(res_data);
	if (!src) return;

	WriteConA(src, src_count, &charsWritten);
}

