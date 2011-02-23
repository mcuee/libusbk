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
			USBERR("DeviceIoControl failed. Error = %d", _ret);
			CloseHandle(ol.hEvent);
			SetLastError(_ret);
			return FALSE;
		}
	}
	if (!GetOverlappedResult(dev, &ol, &_ret, TRUE))
	{
		_ret =  GetLastError();
		USBERR("GetOverlappedResult failed. Error = %d", _ret);
		CloseHandle(ol.hEvent);
		SetLastError(_ret);
		return FALSE;
	}

	if (ret)
		*ret = _ret;

	return CloseHandle(ol.hEvent);
}
