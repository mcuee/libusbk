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

#ifndef __LUSBW_USER_PRIVATE_API_H__
#define __LUSBW_USER_PRIVATE_API_H__

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <objbase.h>
#include <ctype.h>
#include <windows.h>
#include <winioctl.h>

#include "lusbw_version.h"
#include "lusbw_usb.h"
#include "drv_api.h"
#include "lusbw_debug_view_output.h"

//////////////////////////////////////////////////////////////////////////////
// Defines
//

// The maximum number of open interface handles per proccess.
// Internal interface handles come from a fixed array so the memory is
// always valid as long as the dll is loaded.
#define LUSBW_MAX_INTERFACE_HANDLES 128

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Structs
//

// Internal libusbwK interface handle
typedef struct _LUSBW_INTERFACE_HANDLE_INTERNAL
{
	HANDLE DeviceHandle;
	INT InterfaceIndex;
	volatile LONG ValidCount;
} LUSBW_INTERFACE_HANDLE_INTERNAL, *PLUSBW_INTERFACE_HANDLE_INTERNAL;

//////////////////////////////////////////////////////////////////////////////
// lusbw_ioctl.c - FUNCTION PROTOTYPES
//
BOOL Ioctl_Sync(__in HANDLE dev,
                __in INT code,
                __in_opt PVOID in,
                __in DWORD in_size,
                __inout_opt PVOID out,
                __in DWORD out_size,
                __out_opt PDWORD ret);

BOOL Ioctl_Async(__in HANDLE dev,
                 __in INT code,
                 __in_opt PVOID in,
                 __in DWORD in_size,
                 __inout_opt PVOID out,
                 __in DWORD out_size,
                 __in LPOVERLAPPED overlapped);
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Macros
//

// Sets 'RetErrorCode' and jumps to 'JumpStatement' if 'FailIfTrue' is non-zero.
//
#define FailIf(FailIfTrue, RetErrorCode, JumpStatement) \
	if (FailIfTrue) { ret = RetErrorCode; goto JumpStatement; }

// Checks for null and invalid handle value.
#define IsHandleValid(MemoryPtrOrHandle) (((MemoryPtrOrHandle) && (MemoryPtrOrHandle) != INVALID_HANDLE_VALUE)?TRUE:FALSE)

// Validatates the interface handle and cast it to a PLUSBW_INTERFACE_HANDLE_INTERNAL.
//
#define PublicToPrivateHandle(WinusbInterfaceHandle) \
	(IsHandleValid(WinusbInterfaceHandle)?((PLUSBW_INTERFACE_HANDLE_INTERNAL)(WinusbInterfaceHandle)):(NULL))

// Macros for allocating a libusb_request and setting/getting the data that comes after it.
//
#define AllocRequest(RequiredDataLength) Mem_Alloc(sizeof(libusb_request) + RequiredDataLength)

#define GetRequestDataPtr(LibusbRequestPtr) (&(((PUCHAR)LibusbRequestPtr)[sizeof(libusb_request)]))

#define GetRequestData(LibusbRequestPtr,OutputBuffer,OutputBufferLength)	\
	if ( (LibusbRequestPtr) && (OutputBuffer) && (OutputBufferLength) ) memcpy(OutputBuffer, GetRequestDataPtr(LibusbRequestPtr), (size_t)OutputBufferLength)

#define SetRequestData(LibusbRequestPtr,InputBuffer,InputBufferLength)	\
	if ( (LibusbRequestPtr) && (InputBuffer) && (InputBufferLength) ) memcpy(GetRequestDataPtr(LibusbRequestPtr), InputBuffer, (size_t)InputBufferLength)

//////////////////////////////////////////////////////////////////////////////

FORCEINLINE BOOL LusbwError(__in LONG errorCode)
{
	if (errorCode != ERROR_SUCCESS)
	{
		SetLastError(errorCode);
		return FALSE;
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Inline memory functions.
//

FORCEINLINE PVOID Mem_Zero(__in_opt PVOID memory, __in size_t size)
{
	if (IsHandleValid(memory))
	{
		memset(memory, 0, size);
		return memory;
	}
	return NULL;
}

FORCEINLINE PVOID Mem_Alloc(__in size_t size)
{
	PVOID memory = malloc(size);
	return Mem_Zero(memory, size) ? memory : INVALID_HANDLE_VALUE;
}

FORCEINLINE VOID Mem_Free(__deref_inout_opt PVOID* memoryRef)
{
	if (memoryRef)
	{
		if (IsHandleValid(*memoryRef))
			free(*memoryRef);

		*memoryRef = INVALID_HANDLE_VALUE;
	}
}

//
// Interface context locking functions and macros.
#ifdef UNUSED_LOCK_FUNCTIONS_AND_MACROS

// The semaphore lock structure use for LUSBW_INTERFACE_HANDLE_INTERNAL.
typedef struct _SYNC_LOCK
{
	HANDLE Handle;
	LONG MaxCount;
} SYNC_LOCK, *PSYNC_LOCK;

#define MAX_IO_PER_INTERFACE 256

#define InitLock(SynclockPtr) Lock_Init(SynclockPtr)

#define DestroyLock(SynclockPtr) Lock_Destroy(SynclockPtr)

#define AcquireReadLock(SynclockPtr, TimeoutInMs) \
	(WaitForSingleObject((SynclockPtr)->Handle,TimeoutInMs)==WAIT_OBJECT_0)

#define ReleaseReadLock(SynclockPtr) \
	ReleaseSemaphore((SynclockPtr)->Handle,1,NULL)

#define ReleaseWriteLock(SynclockPtr) \
	ReleaseSemaphore((SynclockPtr)->Handle,(SynclockPtr)->MaxCount,NULL)

#define AcquireWriteLock(SynclockPtr, TimeoutInMs) Lock_AcquireWrite(SynclockPtr,TimeoutInMs)

FORCEINLINE BOOL Lock_Init(__in PSYNC_LOCK syncLock)
{
	Mem_Zero(syncLock, sizeof(*syncLock));
	syncLock->MaxCount = MAX_IO_PER_INTERFACE;
	syncLock->Handle = CreateSemaphore(NULL, syncLock->MaxCount, syncLock->MaxCount, NULL);
	if (!IsHandleValid(syncLock->Handle))
		return FALSE;
	return TRUE;
}

FORCEINLINE BOOL Lock_AcquireWrite(__in PSYNC_LOCK syncLock, __in_opt DWORD timeoutMS)
{
	LONG lockCount = 0;
	while (lockCount < syncLock->MaxCount)
	{
		if (!AcquireReadLock(syncLock, timeoutMS))
		{
			ReleaseSemaphore(syncLock->Handle, lockCount, NULL);
			return FALSE;
		}
	}
	return TRUE;
}

FORCEINLINE BOOL Lock_Destroy(__in PSYNC_LOCK syncLock)
{
	if (syncLock->Handle)
	{
		SYNC_LOCK tempLock;
		if (!AcquireWriteLock(syncLock, INFINITE))
			return FALSE;

		memcpy(&tempLock, syncLock, sizeof(tempLock));
		Mem_Zero(syncLock, sizeof(*syncLock));
		CloseHandle(tempLock.Handle);
	}
	return TRUE;
}
#endif

#endif
