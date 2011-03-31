/*! \file lusbk_debug_view_output.h
*/

#ifndef __KUSB_DEBUG_VIEW_OUTPUT_H__
#define __KUSB_DEBUG_VIEW_OUTPUT_H__

//
// To send log messages to stdout, uncomment the line below.
// #define DebugOutputFunction printf
//
#if !defined(DebugOutputFunction) && (defined(_DEBUG) || defined(DBG))
#define DebugOutputFunction DebugViewOutput

//
// logging function
// HINT: Use DebugView in combination with debug builds to view log messages.
// http://technet.microsoft.com/en-us/sysinternals/bb896647
//
VOID DebugViewOutput(CONST CHAR* fmt, ...);

#endif

#endif

#include "lusbk_debug.h"
