#ifndef __USER_DEBUG_H__
#define __USER_DEBUG_H__

#include <windows.h>
#include "doscmd_version.h"

static unsigned char UserDebugLevel = 3;

//#if defined(_DEBUG) ||  defined(DBG)
#if 1
#ifdef RC_FILENAME_STR
#define DOSOUT(Prefix,format,__VA_ARGS__) output_debug_string("[%s-%s] " format, RC_FILENAME_STR, Prefix, __VA_ARGS__)
#else
#define DOSOUT(Prefix,format,__VA_ARGS__) output_debug_string("[%s] " format, Prefix, __VA_ARGS__)
#endif	// RC_FILENAME_STR
#else
#define DOSOUT(Prefix,format,__VA_ARGS__) __noop
#endif	// _DEBUG

#define DOSERR(format,...) DOSOUT("error", format, __VA_ARGS__)
#define DOSMSG(format,...) if (UserDebugLevel > 0) DOSOUT("", format, __VA_ARGS__)
#define DOSWRN(format,...) if (UserDebugLevel > 1) DOSOUT("warning", format, __VA_ARGS__)
#define DOSDBG(format,...) if (UserDebugLevel > 2) DOSOUT("dbg", format, __VA_ARGS__)
#define DOSOFF(format,...) __noop

#define DOSERR0(message) DOSERR("%s", message)
#define DOSMSG0(message) DOSMSG("%s", message)
#define DOSWRN0(message) DOSWRN("%s", message)
#define DOSDBG0(message) DOSDBG("%s", message)
#define DOSOFF0(message) __noop

#define DOS_E_RET(Win32ErrorCode) { DOSERR0(##Win32ErrorCode); return -Win32ErrorCode; }

#ifdef DRIVER
#define output_debug_string DbgPrint
#else

extern VOID output_debug_string(LPCSTR format, ...);

#endif	// DRIVER

#endif	// __USER_DEBUG_H__