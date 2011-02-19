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

#ifndef __LUSBW_DEBUG_VIEW_OUTPUT_H__
#define __LUSBW_DEBUG_VIEW_OUTPUT_H__

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

#include "lusbw_debug.h"
