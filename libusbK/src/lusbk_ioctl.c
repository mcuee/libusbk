/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
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

	ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

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
BOOL Ioctl_SyncWithTimeout(__in HANDLE DeviceHandle,
                           __in INT code,
                           __in libusb_request* request,
                           __inout_opt PVOID out,
                           __in DWORD out_size,
                           __in INT timeout,
                           __in UCHAR PipeID,
                           __out_opt PDWORD ret)
{
	OVERLAPPED ol;
	DWORD _ret;
	BOOL bWait = FALSE;

	memset(&ol, 0, sizeof(ol));

	if (ret)
		*ret = 0;

	ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

	if (!ol.hEvent)
		return FALSE;

	if (!DeviceIoControl(DeviceHandle, code, request, sizeof(*request), out, out_size, NULL, &ol))
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
			if ((_ret = WaitForSingleObject(ol.hEvent, timeout)) != WAIT_OBJECT_0)
			{
				libusb_request abort_request;
				Mem_Zero(&abort_request, sizeof(abort_request));
				abort_request.endpoint.endpoint = PipeID;

				if (!Ioctl_Sync(DeviceHandle,
				                LIBUSB_IOCTL_ABORT_ENDPOINT,
				                &abort_request, sizeof(abort_request),
				                NULL, 0,
				                NULL))
				{
					_ret =  GetLastError();
					USBERRN("A transfer time out occurred and it could not be aborted. Timeout=%u PipeID=%02Xh ErrorCode=%08Xh",
					        timeout, PipeID, _ret);
				}
				else
				{
					_ret = ERROR_SEM_TIMEOUT;
					SetLastError(_ret);
				}

				WaitForSingleObject(ol.hEvent, 0);
				CloseHandle(ol.hEvent);
				return FALSE;
			}
		}
		else
		{
			_ret = WaitForSingleObject(ol.hEvent, INFINITE);
		}
	}
	else
	{
		bWait = TRUE;
	}

	if (!GetOverlappedResult(DeviceHandle, &ol, &_ret, bWait))
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
