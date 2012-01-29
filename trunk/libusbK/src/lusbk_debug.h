/*! \file lusbk_debug.h
*/

#ifndef __KUSB_DEBUG_H__
#define __KUSB_DEBUG_H__

#ifndef DEFINE_TO_STR
#define _DEFINE_TO_STR(x) #x
#define  DEFINE_TO_STR(x) _DEFINE_TO_STR(x)
#endif

//
// Globals
//
extern ULONG DebugLevel;

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

#ifndef DebugOutputFunction
#define DebugOutputFunction DbgPrint
#endif

#define IFDBGLVL(level) if (DebugLevel > level)

#ifndef USB_LN
#define USB_LN "\n"
#endif

#define USBLOG_PRINTLN(format,...) DebugOutputFunction(format USB_LN,__VA_ARGS__)
#define USBLOG_PRINT(format,...) DebugOutputFunction(format,__VA_ARGS__)

#define USBERRN(format,...) USBERR(format USB_LN,__VA_ARGS__)
#define USBWRNN(format,...) USBWRN(format USB_LN,__VA_ARGS__)
#define USBMSGN(format,...) USBMSG(format USB_LN,__VA_ARGS__)
#define USBDBGN(format,...) USBDBG(format USB_LN,__VA_ARGS__)
#define USBDEVN(format,...) USBDEV(format USB_LN,__VA_ARGS__)

#if defined(DBG) || defined (_DEBUG) || defined(KUSB_DEBUG_APPMODE)
#define DEBUG_LOGGING_ENABLED

#define USBLOG(MinDebugLevel,LogAppNameString,CategoryText,FunctionText,format,...) \
	IFDBGLVL(MinDebugLevel) DebugOutputFunction("%s%s%s " format, CategoryText, LogAppNameString, FunctionText,__VA_ARGS__)

#if (KUSB_DEBUG_APPMODE > 0)
#define USBERR(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#define USBDEV(format,...) USBLOG_PRINT(format,__VA_ARGS__)
#else
#define USBERR(format,...) USBLOG(0, LOG_APPNAME, "[ERR]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG(1, LOG_APPNAME, "[WRN]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG(2, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG(3, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBDEV(format,...) USBLOG(255, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#endif

#define USBE_OK(format,...) USBMSG("[Ok!] " format,__VA_ARGS__)
#define USBE_PARAM(ParameterName) USBERRN("invalid parameter: %s",DEFINE_TO_STR(ParameterName))

#else
// Logging Off //

#define USBERR(format,...) USB_LOG_NOP()
#define USBMSG(format,...) USB_LOG_NOP()
#define USBWRN(format,...) USB_LOG_NOP()
#define USBDBG(format,...) USB_LOG_NOP()
#define USBDEV(format,...) USB_LOG_NOP()

#define USBE_OK(format,...) USB_LOG_NOP()
#define USBE_PARAM(ParameterName) USB_LOG_NOP()

#endif // defined(DBG) || defined (_DEBUG)

#define USB_LOG_NOP() NOP_FUNCTION

#endif // __KUSB_DEBUG_H__
