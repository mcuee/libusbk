/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "lusbk_private.h"

//
// Sends an IOCTL code and returns immediately.
//
BOOL Ioctl_Async(__in HANDLE dev,
                 __in INT code,
                 __in_opt PVOID in,
                 __in DWORD in_size,
                 __inout_opt PVOID out,
                 __in DWORD out_size,
                 __in LPOVERLAPPED overlapped)
{
	DWORD ret = ERROR_INVALID_PARAMETER;

	if (overlapped)
		return DeviceIoControl(dev, code, in, in_size, out, out_size, NULL, overlapped);

	USBERR("overlapped cannot be null.");
	return LusbwError(ret);

}

//
// Sends an IOCTL code and waits for it to complete.
//
BOOL Ioctl_Sync(__in HANDLE dev,
                __in INT code,
                __in_opt PVOID in,
                __in DWORD in_size,
                __inout_opt PVOID out,
                __in DWORD out_size,
                __out_opt PDWORD ret)
{
	OVERLAPPED ol;
	DWORD _ret;

	memset(&ol, 0, sizeof(ol));

	if (ret)
		*ret = 0;

	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!ol.hEvent)
		return FALSE;

	if (!DeviceIoControl(dev, code, in, in_size, out, out_size, NULL, &ol))
	{
		_ret = GetLastError();
		if (_ret != ERROR_IO_PENDING)
		{
			CloseHandle(ol.hEvent);
			SetLastError(_ret);
			return FALSE;
		}
	}

	if (!GetOverlappedResult(dev, &ol, &_ret, TRUE))
	{
		_ret =  GetLastError();
		CloseHandle(ol.hEvent);
		SetLastError(_ret);
		return FALSE;
	}

	if (ret)
		*ret = _ret;

	return CloseHandle(ol.hEvent);
}

//
// Sends an IOCTL code and waits for it to complete.
//
BOOL Ioctl_SyncTranfer(__in PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle,
                       __in INT code,
                       __in libusb_request* request,
                       __inout_opt PVOID out,
                       __in DWORD out_size,
                       __in INT timeout,
                       __out_opt PDWORD ret)
{
	OVERLAPPED ol;
	DWORD _ret;

	memset(&ol, 0, sizeof(ol));

	if (ret)
		*ret = 0;

	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!ol.hEvent)
		return FALSE;

	if (!DeviceIoControl(interfaceHandle->DeviceHandle, code, request, sizeof(*request), out, out_size, NULL, &ol))
	{
		_ret = GetLastError();
		if (_ret != ERROR_IO_PENDING)
		{
			CloseHandle(ol.hEvent);
			SetLastError(_ret);
			return FALSE;
		}
		if (timeout)
		{
			if (WaitForSingleObject(ol.hEvent, timeout) != WAIT_OBJECT_0)
			{
				LUsbK_AbortPipe(interfaceHandle, (UCHAR)request->endpoint.endpoint);
			}
		}
	}

	if (!GetOverlappedResult(interfaceHandle->DeviceHandle, &ol, &_ret, TRUE))
	{
		_ret =  GetLastError();
		CloseHandle(ol.hEvent);
		SetLastError(_ret);
		return FALSE;
	}

	if (ret)
		*ret = _ret;

	return CloseHandle(ol.hEvent);
}
