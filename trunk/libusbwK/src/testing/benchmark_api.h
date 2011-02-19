/* libusbwK "Benchmark (LibUsbDotNet)" API
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
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

#ifndef __LUSBW_BENCHMARK_API_H__
#define __LUSBW_BENCHMARK_API_H__

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <objbase.h>
#include <ctype.h>
#include <windows.h>
#include <winioctl.h>

#include "lusbw_debug.h"
#include "lusbw_usb.h"

extern CONST LPSTR TestTypeStrings[4];
#define BmGetTestTypeString(TestTypeValue) TestTypeStrings[(TestTypeValue) & 0x3]

typedef enum _BM_COMMAND
{
	BmSetTest = 0x0E,
	BmGetTest = 0x0F,
} BM_COMMAND, *PBM_COMMAND;

// Tests supported by the official benchmark firmware.
//
typedef enum _BM_TEST_TYPE
{
	BmTestTypeNone	= 0x00,
	BmTestTypeRead	= 0x01,
	BmTestTypeWrite	= 0x02,
	BmTestTypeLoop	= 0x03,
} BM_TEST_TYPE, *PBM_TEST_TYPE;

//! The DeviceInterfaceGUID used by the \see Bm_Open function.
/*!
\remark {6C696275-7362-2D77-696E-33322D574446} is the default guid used by libusbwK.sys.
\remark Devices are created under this GUID when the DeviceInterfaceGUIDs registry key is invalid or non-existent.
*/
#define Benchmark_DeviceInterfaceGUID {6C696275-7362-2D77-696E-33322D574446}

//! Opens a file handle to the device.
/*!
\param instanceIndex	The index of found devices to open,
\param pipeSuffix		If not null, the pipe suffix string for direct access to an endpoint; "PIPEx81" for example.
\param fileHandle		On success, An open file handle to the device or pipe.
\retval -ERROR_NO_GUID_TRANSLATION	The benchmark guid define is incorrectly formed.
*/
LONG Bm_Open(__in DWORD instanceIndex, __in_opt LPCSTR pipeSuffix, __inout HANDLE* fileHandle);

//! Opens a file handle to the device.
/*!
\param fileHandle An open file handle to a device or pipe.
\remark If deviceHandle is NULL or de-references to NULL, nothing is done.
*/
VOID Bm_Close(__inout_opt HANDLE* fileHandle);

LONG Bm_TestSelect(__in WINUSB_INTERFACE_HANDLE interfaceHandle,
                   __in BM_COMMAND command,
                   __inout PBM_TEST_TYPE testType);
#endif