#ifndef __DPSCAT_H_
#define __DPSCAT_H_


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <setupapi.h>
#include <tchar.h>
#include "resources.h"
#include "lusbk_version.h"

#define KUSB_DEBUG_APPMODE 1
#define KUSB_DEBUG_UNICODE 1

#define DebugOutputFunction WriteInfStatus
VOID WriteInfStatus(CONST WCHAR* fmt, ...);
#include "lusbk_debug.h"

extern SYSTEM_INFO g_SystemInfo;

#endif