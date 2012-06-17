/*! \file lusbk_debug.h
*/

#ifndef __KUSB_DEBUG_H__
#define __KUSB_DEBUG_H__

#ifndef DEFINE_TO_STR
#define _DEFINE_TO_STR(x) #x
#define  DEFINE_TO_STR(x) _DEFINE_TO_STR(x)
#endif

#ifndef KUSB_DEBUG_UNICODE
#define KUSB_DEBUG_UNICODE 0
#endif

//
// Globals
//
extern ULONG DebugLevel;


#ifndef DebugOutputFunction
#define DebugOutputFunction DbgPrint
#endif

#if KUSB_DEBUG_UNICODE==0
// Ascii

#ifndef LOG_APPNAME

#ifdef RC_FILENAME_STR
#define LOG_APPNAME "(" RC_FILENAME_STR ")"
#else
#define LOG_APPNAME "(lusbk)"
#endif

#endif

#ifdef KUSB_DEBUG_NO_APPNAME

#ifdef LOG_APPNAME
#undef LOG_APPNAME
#endif

#define LOG_APPNAME ""

#endif // KUSB_DEBUG_NO_APPNAME

#ifndef USB_LN
#define USB_LN "\n"
#endif

#define KUSBD_BASE_FMT "%s%s%s "
#define KUSBD_ERR_FMT  "[ERR]"
#define KUSBD_WRN_FMT  "[WRN]"
#define KUSBD_MSG_FMT  ""
#define KUSBD_DBG_FMT  ""
#define KUSBD_DEV_FMT  ""
#define KUSBD_FUNCTION  "["__FUNCTION__"]"

#define USBLOG(MinDebugLevel,LogAppNameString,CategoryText,FunctionText,format,...) \
	IFDBGLVL(MinDebugLevel) DebugOutputFunction(KUSBD_BASE_FMT format, CategoryText, LogAppNameString, FunctionText,__VA_ARGS__)

#else
// Unicode

#ifndef LOG_APPNAME

#ifdef RC_FILENAME_STR
#define LOG_APPNAME L"(" L##RC_FILENAME_STR L")"
#else
#define LOG_APPNAME L"(lusbk)"
#endif

#endif

#ifdef KUSB_DEBUG_NO_APPNAME

#ifdef LOG_APPNAME
#undef LOG_APPNAME
#endif

#define LOG_APPNAME L""

#endif // KUSB_DEBUG_NO_APPNAME
#ifndef USB_LN
#define USB_LN L"\n"
#endif

#define KUSBD_BASE_FMT L"%s%s%s "
#define KUSBD_ERR_FMT  L"[ERR]"
#define KUSBD_WRN_FMT  L"[WRN]"
#define KUSBD_MSG_FMT  L""
#define KUSBD_DBG_FMT  L""
#define KUSBD_DEV_FMT  L""
#define KUSBD_FUNCTION  L"[" L##__FUNCTION__ L"]"

#define USBLOG(MinDebugLevel,LogAppNameString,CategoryText,FunctionText,format,...) \
	IFDBGLVL(MinDebugLevel) DebugOutputFunction(KUSBD_BASE_FMT format, CategoryText, LogAppNameString, FunctionText,__VA_ARGS__)

#endif

#define USBLOG_PRINTLN(format,...) DebugOutputFunction(format USB_LN,__VA_ARGS__)
#define USBLOG_PRINT(format,...) DebugOutputFunction(format,__VA_ARGS__)

#define USBERRN(format,...) USBERR(format USB_LN,__VA_ARGS__)
#define USBWRNN(format,...) USBWRN(format USB_LN,__VA_ARGS__)
#define USBMSGN(format,...) USBMSG(format USB_LN,__VA_ARGS__)
#define USBDBGN(format,...) USBDBG(format USB_LN,__VA_ARGS__)
#define USBDEVN(format,...) USBDEV(format USB_LN,__VA_ARGS__)

#define IFDBGLVL(level) if (DebugLevel > level)

#if defined(DBG) || defined (_DEBUG) || defined(KUSB_DEBUG_APPMODE)
#define DEBUG_LOGGING_ENABLED

#if (KUSB_DEBUG_APPMODE > 0)
#define USBERR(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBDEV(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#else
#define USBERR(format,...) USBLOG(0, LOG_APPNAME, KUSBD_ERR_FMT, KUSBD_FUNCTION, format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG(1, LOG_APPNAME, KUSBD_WRN_FMT, KUSBD_FUNCTION, format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG(2, LOG_APPNAME, KUSBD_MSG_FMT, KUSBD_FUNCTION, format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG(3, LOG_APPNAME, KUSBD_DBG_FMT, KUSBD_FUNCTION, format,__VA_ARGS__)
#define USBDEV(format,...) USBLOG(255, LOG_APPNAME, KUSBD_DEV_FMT, KUSBD_FUNCTION, format,__VA_ARGS__)
#endif

#else
// Logging Off //

#define USBERR(format,...) USB_LOG_NOP()
#define USBMSG(format,...) USB_LOG_NOP()
#define USBWRN(format,...) USB_LOG_NOP()
#define USBDBG(format,...) USB_LOG_NOP()
#define USBDEV(format,...) USB_LOG_NOP()

#endif // defined(DBG) || defined (_DEBUG)

#define USB_LOG_NOP() NOP_FUNCTION

#endif // __KUSB_DEBUG_H__
