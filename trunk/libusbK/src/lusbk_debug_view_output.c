/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/


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

#include "lusbk_debug_view_output.h"
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
