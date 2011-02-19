/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LUSBW_DEBUG_H__
#define __LUSBW_DEBUG_H__

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
#define LOG_APPNAME "(lusbw)"
#endif

#endif

#ifdef LUSBW_DEBUG_NO_APPNAME

#ifdef LOG_APPNAME
#undef LOG_APPNAME
#endif

#define LOG_APPNAME ""
#define _ERR_FMT "%s[ERR] ["
#define _WRN_FMT "%s[WRN] ["
#define _MSG_FMT "%s["
#define _DBG_FMT "%s["

#else

#define _ERR_FMT "%s[ERR] ["
#define _WRN_FMT "%s[WRN] ["
#define _MSG_FMT "%s["
#define _DBG_FMT "%s["

#endif // LUSBW_DEBUG_NO_APPNAME

#ifndef DebugOutputFunction
#define DebugOutputFunction DbgPrint
#endif

#define IFDBGLVL(level) if (DebugLevel > level)
#define USB_LN "\n" LOG_APPNAME

#define USBLOG(MinDebugLevel,LogAppNameString,CategoryText,FunctionText,format,...) \
	IFDBGLVL(MinDebugLevel) DebugOutputFunction("%s%s%s " format, LogAppNameString, CategoryText, FunctionText,__VA_ARGS__)

#define USBERR(format,...) USBLOG(0, LOG_APPNAME, "[ERR]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBWRN(format,...) USBLOG(1, LOG_APPNAME, "[WRN]", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBMSG(format,...) USBLOG(2, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)
#define USBDBG(format,...) USBLOG(3, LOG_APPNAME, "", "["__FUNCTION__"]",format,__VA_ARGS__)

/*
#define USBERR(format,...) IFDBGLVL(0) DebugOutputFunction(_ERR_FMT __FUNCTION__ "] " format,LOG_APPNAME,__VA_ARGS__)
#define USBWRN(format,...) IFDBGLVL(1) DebugOutputFunction(_WRN_FMT __FUNCTION__ "] " format,LOG_APPNAME,__VA_ARGS__)
#define USBMSG(format,...) IFDBGLVL(2) DebugOutputFunction(_MSG_FMT __FUNCTION__ "] " format,LOG_APPNAME,__VA_ARGS__)
#define USBDBG(format,...) IFDBGLVL(3) DebugOutputFunction(_DBG_FMT __FUNCTION__ "] " format,LOG_APPNAME,__VA_ARGS__)
*/
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

#define USBE_OK(format,...) USB_LOG_NOP()
#define USBE_PARAM(ParameterName) USB_LOG_NOP()
#define USBE_SUCCESS(success,format,...) USB_LOG_NOP()

#endif // defined(DBG) || defined (_DEBUG)

#endif // __LUSBW_DEBUG_H__
