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

#if (defined(DBG) || defined (_DEBUG))

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
#define USB_LN "\n     "

#define USBLOG(MinDebugLevel,LogAppNameString,CategoryText,FunctionText,format,...) \
	IFDBGLVL(MinDebugLevel) DebugOutputFunction("%s%s%s " format, CategoryText, LogAppNameString, FunctionText,__VA_ARGS__)

#define USBERR(format,...) USBLOG(0, LOG_APPNAME, "[ERR]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG(1, LOG_APPNAME, "[WRN]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG(2, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG(3, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBDEV(format,...) USBLOG(255, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)

#define USBE_SUCCESS(success,format,...) USBLOG(2,LOG_APPNAME,((success)?"[Success!]":"[Fail!]"),__FUNCTION__,format,__VA_ARGS__)
#define USBE_OK(format,...) USBMSG("[Ok!] " format,__VA_ARGS__)
#define USBE_PARAM(ParameterName) USBERR("invalid parameter: %s\n",DEFINE_TO_STR(ParameterName))

#else
// Logging Off //

FORCEINLINE VOID USB_LOG_NOP() {}

#define USB_LN
#define USBERR(format,...) USB_LOG_NOP()
#define USBMSG(format,...) USB_LOG_NOP()
#define USBWRN(format,...) USB_LOG_NOP()
#define USBDBG(format,...) USB_LOG_NOP()
#define USBDEV(format,...) USB_LOG_NOP()

#define USBE_OK(format,...) USB_LOG_NOP()
#define USBE_PARAM(ParameterName) USB_LOG_NOP()
#define USBE_SUCCESS(success,format,...) USB_LOG_NOP()

#endif // defined(DBG) || defined (_DEBUG)

#endif // __KUSB_DEBUG_H__
