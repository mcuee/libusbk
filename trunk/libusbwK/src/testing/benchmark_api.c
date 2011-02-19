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
#include "benchmark_api.h"
#include <setupapi.h>

#ifndef DEFINE_TO_STR
#define _DEFINE_TO_STR(x) #x
#define  DEFINE_TO_STR(x) _DEFINE_TO_STR(x)
#endif

#ifndef DEFINE_TO_STRW
#define _DEFINE_TO_STRW(x) L#x
#define  DEFINE_TO_STRW(x) _DEFINE_TO_STRW(x)
#endif

CONST LPSTR TestTypeStrings[4] = {"None", "Read", "Write", "Loop"};
#define DeviceInterfacePathMaxLength (1024)
#define DeviceInterfaceDetailMaxSize (sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A)+DeviceInterfacePathMaxLength)

LONG ShowSystemErrorMessage(__in_opt DWORD errorCode);

LONG WinError(__in_opt DWORD errorCode)
{
	LPSTR buffer = NULL;

	errorCode = errorCode ? labs(errorCode) : GetLastError();
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	                   NULL, errorCode, 0, (LPSTR)&buffer, 0, NULL) > 0)
	{
		USBMSG("%s\n", buffer);
	}
	else
	{
		USBERR("FormatMessage error!\n");
	}

	if (buffer)
		LocalFree(buffer);

	return -labs(errorCode);
}

LONG Bm_Open(__in DWORD instanceIndex, __in_opt LPCSTR pipeSuffix, __inout HANDLE* fileHandle)
{
	LONG ret;
	HDEVINFO hDevInfo;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	UCHAR DeviceInterfaceDetailBuffer[DeviceInterfaceDetailMaxSize + 1];
	GUID guidDevIntf = {0};
	DWORD currentInstanceIndex = 0;
	PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDeviceInterfaceDetail;


	if (!fileHandle)
	{
		USBE_PARAM(fileHandle);
		ret = WinError(ERROR_INVALID_HANDLE);
		return ret;
	}
	*fileHandle = NULL;

	if ((ret = CLSIDFromString(DEFINE_TO_STRW(Benchmark_DeviceInterfaceGUID), &guidDevIntf)) != NOERROR)
	{
		USBERR("invalid DeviceInterfaceGUID define\n");
		ret = WinError(ERROR_BAD_FORMAT);
		return ret;
	}

	hDevInfo = SetupDiGetClassDevsA(&guidDevIntf, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(!hDevInfo || hDevInfo == INVALID_HANDLE_VALUE)
	{
		USBERR("device is not (and has never been) installed\n");
		ret = WinError(ERROR_CLASS_DOES_NOT_EXIST);
		return ret;
	}

	// init vars
	memset(&deviceInterfaceData, 0, sizeof(deviceInterfaceData));
	deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

	pDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)&DeviceInterfaceDetailBuffer;

	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guidDevIntf, currentInstanceIndex++, &deviceInterfaceData))
	{
		memset(pDeviceInterfaceDetail, 0, sizeof(DeviceInterfaceDetailBuffer));
		pDeviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		if (SetupDiGetDeviceInterfaceDetailA(hDevInfo,
		                                     &deviceInterfaceData,
		                                     pDeviceInterfaceDetail,
		                                     DeviceInterfaceDetailMaxSize,
		                                     NULL, NULL))
		{
			PCHAR devicePath = &pDeviceInterfaceDetail->DevicePath[0];
			if (instanceIndex + 1 == currentInstanceIndex)
			{
				//USBDBG("using index=%u, devicePath=%s\n", currentInstanceIndex - 1, devicePath);
				*fileHandle = CreateFileA(devicePath,
				                          GENERIC_READ | GENERIC_WRITE,
				                          FILE_SHARE_READ | FILE_SHARE_WRITE,
				                          NULL,
				                          OPEN_EXISTING,
				                          FILE_FLAG_OVERLAPPED,
				                          NULL);
				if (!*fileHandle || *fileHandle == INVALID_HANDLE_VALUE)
				{
					*fileHandle = NULL;
					USBWRN("failed creating device handle for existing device at index=%u\n",
					       currentInstanceIndex - 1);
					ret = WinError(0);
				}
				else
				{
					// success
					ret = ERROR_SUCCESS;
				}
				break;

			}
			else
			{
				USBDBG("found index=%u, devicePath=%s\n", currentInstanceIndex - 1, devicePath);
			}
		}
	}

	if(hDevInfo && hDevInfo != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hDevInfo);

	// success.
	if (*fileHandle && ret == ERROR_SUCCESS)
		return ret;

	// not found.
	if (!*fileHandle && ret == ERROR_SUCCESS)
		return WinError(ERROR_DEVICE_NOT_CONNECTED);

	// failed to open (in-use).
	Bm_Close(fileHandle);
	return ret;
}

VOID Bm_Close(__inout HANDLE* fileHandle)
{
	if (fileHandle && *fileHandle && *fileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(*fileHandle);
		*fileHandle = NULL;
	}
}

LONG Bm_TestSelect(__in WINUSB_INTERFACE_HANDLE interfaceHandle,
                   __in BM_COMMAND command,
                   __inout PBM_TEST_TYPE testType)
{
	LONG ret = ERROR_SUCCESS;
	BOOL success;
	UCHAR buffer[1];
	ULONG transferred = 0;

	WINUSB_SETUP_PACKET setupPacket =
	{
		(BMREQUEST_VENDOR << 5) | USB_ENDPOINT_DIRECTION_MASK,	// RequestType
		0,			// Request	(BmCommand)
		0,			// Value	(BmSetTest)
		0,			// Index	(Interface#)
		1			// Length	(1)
	};

	if (!interfaceHandle || interfaceHandle == INVALID_HANDLE_VALUE)
		return WinError(ERROR_INVALID_HANDLE);

	setupPacket.Request = (UCHAR)command;
	setupPacket.Value = (USHORT) * testType;

	success = LUsbW_ControlTransfer(interfaceHandle,
	                                setupPacket,
	                                buffer,
	                                1,
	                                &transferred,
	                                NULL);

	if (success && transferred == 1)
	{
		*testType = (BM_TEST_TYPE)buffer[0];
	}
	else
	{
		if (!success)
			ret = WinError(0);
		else
			ret = WinError(ERROR_NO_DATA);

		USBERR("[Fail] success=%u, transferred=%u\n", success, transferred);
	}

	return (ret) ? ret : transferred;
}