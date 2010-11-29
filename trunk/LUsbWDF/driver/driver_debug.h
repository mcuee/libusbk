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

#ifndef _DRIVER_DEBUG_H
#define _DRIVER_DEBUG_H

#include "private.h"

#ifdef DBG
#define IFDBGLVL(level) if (DebugLevel > level)
#define LOG_APPNAME "LUSBW"
#define USBERR(format,...) IFDBGLVL(0) DbgPrint("[" LOG_APPNAME " Error] : [" __FUNCTION__ "] " format, __VA_ARGS__)
#define USBWRN(format,...) IFDBGLVL(1) DbgPrint("[" LOG_APPNAME " Warn] : [" __FUNCTION__ "] " format,__VA_ARGS__)
#define USBMSG(format,...) IFDBGLVL(2) DbgPrint("[" LOG_APPNAME "] [" __FUNCTION__ "] " format,__VA_ARGS__)
#define USBDBG(format,...) IFDBGLVL(3) DbgPrint("[" LOG_APPNAME " Debug] : [" __FUNCTION__ "] " format,__VA_ARGS__)
#else
#define USBERR(format,...)
#define USBMSG(format,...)
#define USBWRN(format,...)
#define USBDBG(format,...)
#endif

#define USBERR0(message) USBERR("%s", message)
#define USBMSG0(message) USBMSG("%s", message)
#define USBWRN0(message) USBWRN("%s", message)
#define USBDBG0(message) USBDBG("%s", message)

#endif
