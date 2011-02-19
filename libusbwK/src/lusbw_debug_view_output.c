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

//
// To send log messages to stdout, uncomment the line below.
// #define DebugOutputFunction printf
//

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <objbase.h>
#include <ctype.h>
#include <windows.h>
#include <winioctl.h>

#include "lusbw_debug_view_output.h"
#if defined(DebugOutputFunction)

//
// logging function
// HINT: Use DebugView in combination with debug builds to view log messages.
// http://technet.microsoft.com/en-us/sysinternals/bb896647
//
VOID DebugViewOutput(CONST CHAR* fmt, ...)
{
	CHAR buf[256];
	INT len;
	va_list args;

	va_start(args, fmt);
#if __STDC_WANT_SECURE_LIB__
	len = _vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, args);
#else
	len = _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
#endif
	va_end(args);

	if (len > 0)
		buf[len - 1] = '\0';

	buf[sizeof(buf) - 1] = '\0';

	OutputDebugStringA(buf);
}
#endif
