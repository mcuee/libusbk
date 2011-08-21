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

#include "lusbk_private.h"
#include "lusbk_handles.h"
#include "lusbk_linked_list.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define SafeCloseEvent(EventHandle) if (EventHandle) CloseHandle(EventHandle)
#define mOvlK_IsComplete(mOverlappedK)	\
	(WaitForSingleObject(((PKOVL_HANDLE_INTERNAL)mOverlappedK)->Overlapped.hEvent, 0) != WAIT_TIMEOUT)

#define Ovl_MemAlloc(mStream,mSize)		HeapAlloc((mStream)->Heap,HEAP_ZERO_MEMORY|HEAP_NO_SERIALIZE,mSize)
#define Ovl_SafeMemAlloc(mStream,mSize) HeapAlloc((mStream)->Heap,HEAP_ZERO_MEMORY,mSize)

static void KUSB_API Cleanup_OvlPoolK(PKOVL_POOL_HANDLE_INTERNAL handle)
{
	PKOVL_ITEM ovlItem;

	PoolHandle_Dead_OvlPoolK(handle);

	if (handle->UsbHandle) PoolHandle_Dec_UsbK(handle->UsbHandle);

	while(AL_PopHead_Ovl(handle->MasterList, &ovlItem) == ERROR_SUCCESS)
	{
		if (ovlItem->IsAcquired)
		{
			ULONG dummy;
			OvlK_WaitAndRelease(ovlItem, 0, &dummy);
		}
		PoolHandle_Dec_OvlK(ovlItem);
	}

	AL_Destroy_Ovl(&handle->MasterList, AllK.Heap);
	AL_Destroy_Ovl(&handle->List, AllK.Heap);

}

static void KUSB_API Cleanup_OvlK(PKOVL_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_OvlK(handle);
	SafeCloseEvent(handle->Overlapped.hEvent);
}

static void o_Reuse(PKOVL_HANDLE_INTERNAL overlapped)
{
	if (!overlapped->Overlapped.hEvent)
		overlapped->Overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	else
		ResetEvent(overlapped->Overlapped.hEvent);
}

KUSB_EXP BOOL KUSB_API OvlK_Acquire(
    _out KOVL_HANDLE* OverlappedK,
    _in KOVL_POOL_HANDLE Pool)
{
	PKOVL_HANDLE_INTERNAL overlapped = NULL;
	PKOVL_POOL_HANDLE_INTERNAL pool;
	BOOL isNewFromPool = FALSE;

	ErrorParamAction(!IsHandleValid(OverlappedK), "OverlappedK", return FALSE);
	*OverlappedK = NULL;

	Pub_To_Priv_OvlPoolK(Pool, pool, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_OvlPoolK(pool), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_OvlPoolK");

	if (AL_PopHead_Ovl(pool->List, &overlapped) == ERROR_NO_MORE_ITEMS)
	{
		if (pool->MasterList->Idx.Count >= pool->MasterList->MaxCount)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			goto Error;
		}

		// Get a new OverlappedK handle.
		overlapped = PoolHandle_Acquire_OvlK(Cleanup_OvlK);
		ErrorNoSet(!overlapped, Error, "->PoolHandle_Acquire_OvlK");

		isNewFromPool = TRUE;
		if (AL_PushTail_Ovl(pool->MasterList, overlapped) == ERROR_NO_MORE_ITEMS)
		{
			PoolHandle_Dec_OvlK(overlapped);
			LusbwError(ERROR_NO_MORE_ITEMS);
			goto Error;
		}
	}

	overlapped->Pool = pool;
	overlapped->IsAcquired = 1;
	o_Reuse(overlapped);

	*OverlappedK = (KOVL_HANDLE)overlapped;
	if (isNewFromPool) PoolHandle_Live_OvlK(overlapped);
	PoolHandle_Dec_OvlPoolK(pool);
	return TRUE;
Error:
	PoolHandle_Dec_OvlPoolK(pool);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_Release(
    _in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped = NULL;
	BOOL success = FALSE;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);
	ErrorParamAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

	success = (InterlockedExchange(&overlapped->IsAcquired, 0) != 0);
	ErrorSet(!success, Done, ERROR_ACCESS_DENIED, "OverlappedK is not acquired.");

	AL_PushTail_Ovl(overlapped->Pool->List, overlapped);

Done:
	PoolHandle_Dec_OvlK(overlapped);
	return success;
}

KUSB_EXP BOOL KUSB_API OvlK_InitPool(
    _out KOVL_POOL_HANDLE* PoolHandle,
    _in KUSB_HANDLE UsbHandle,
    _in USHORT MaxOverlappedCount,
    _inopt KOVL_POOL_FLAG Flags)
{
	PKOVL_POOL_HANDLE_INTERNAL handle = NULL;
	PKUSB_HANDLE_INTERNAL usbHandle;

	*PoolHandle = NULL;

	ErrorParamAction(!IsHandleValid(PoolHandle), "PoolHandle", return FALSE);
	ErrorParamAction(MaxOverlappedCount > 64, "MaxOverlappedCount cannot be greater than 64", return FALSE);

	Pub_To_Priv_UsbK(UsbHandle, usbHandle, return FALSE);

	handle = PoolHandle_Acquire_OvlPoolK(Cleanup_OvlPoolK);
	ErrorNoSetAction(!IsHandleValid(handle), return FALSE, "->PoolHandle_Acquire_OvlPoolK");

	handle->Flags		= Flags;

	ErrorSet(!PoolHandle_Inc_UsbK(usbHandle), Error, ERROR_RESOURCE_NOT_AVAILABLE, "->PoolHandle_Inc_UsbK");
	handle->UsbHandle	= usbHandle;

	AL_Create_Ovl(&handle->List, AllK.Heap, MaxOverlappedCount);
	ErrorMemory(!handle->List, Error);

	AL_Create_Ovl(&handle->MasterList, AllK.Heap, MaxOverlappedCount);
	ErrorMemory(!handle->MasterList, Error);

	handle->Flags		= Flags;

	*PoolHandle = (KOVL_POOL_HANDLE)handle;
	PoolHandle_Live_OvlPoolK(handle);
	return TRUE;
Error:
	if (handle) PoolHandle_Dec_OvlPoolK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_FreePool(
    _in KOVL_POOL_HANDLE Pool)
{
	PKOVL_POOL_HANDLE_INTERNAL handle;

	Pub_To_Priv_OvlPoolK(Pool, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_OvlPoolK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_OvlPoolK");

	PoolHandle_Dec_OvlPoolK(handle);
	PoolHandle_Dec_OvlPoolK(handle);
	return TRUE;
}

KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
    _in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped;
	Pub_To_Priv_OvlK(OverlappedK, overlapped, return NULL);
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return overlapped->Overlapped.hEvent;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_Wait(
    _in KOVL_HANDLE OverlappedK,
    _inopt ULONG TimeoutMS,
    _inopt KOVL_WAIT_FLAG WaitFlags,
    _out PULONG TransferredLength)
{
	PKOVL_HANDLE_INTERNAL overlapped;
	DWORD errorCode;
	BOOL success;
	BOOL userCancelled = FALSE;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);
	ErrorParamAction(!TransferredLength, "TransferredLength", return FALSE);

	ErrorParamAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

CancelIoRetry:
	// wait the specified timeout interval
	errorCode = WaitForSingleObject(overlapped->Overlapped.hEvent, TimeoutMS);

	if (errorCode == WAIT_OBJECT_0)
	{
		// check for an overlapped result regardless of the WaitForSingleObject return value
		success = GetOverlappedResult(overlapped->Pool->UsbHandle->Device->MasterDeviceHandle, &overlapped->Overlapped, TransferredLength, FALSE);
		if (!success) errorCode = GetLastError();
	}
	else if (errorCode == WAIT_TIMEOUT)
	{
		success = FALSE;
		errorCode = ERROR_IO_INCOMPLETE;
	}
	else
	{
		success = FALSE;
		errorCode = GetLastError();
	}

	if (errorCode == ERROR_SUCCESS)
	{
		// the overlapped I/O completed successfully
		if (WaitFlags & KOVL_WAIT_FLAG_RELEASE_ON_SUCCESS) OvlK_Release(OverlappedK);
	}
	else if (errorCode == ERROR_IO_INCOMPLETE)
	{
		// overlapped I/O has not completed yet but is still pending.
		if (WaitFlags & KOVL_WAIT_FLAG_CANCEL_ON_TIMEOUT)
		{
			if (AllK.CancelIoEx)
				success = AllK.CancelIoEx(overlapped->Pool->UsbHandle->Device->MasterDeviceHandle, overlapped);
			else
				success = CancelIo(overlapped->Pool->UsbHandle->Device->MasterDeviceHandle);

			// If the IO was cancelled successfully then retry everything again.
			// The operation could still complete successfully.
			if (success)
			{
				WaitFlags ^= KOVL_WAIT_FLAG_CANCEL_ON_TIMEOUT;
				TimeoutMS = INFINITE;
				userCancelled = TRUE;
				goto CancelIoRetry;
			}

			// Some unknown error occured.
			errorCode = GetLastError();
			USBERRN("Failed cancel I/O operation errorCode=%08Xh", errorCode);
			SetEvent(overlapped->Overlapped.hEvent);
		}

		if (WaitFlags & KOVL_WAIT_FLAG_RELEASE_ON_TIMEOUT) OvlK_Release(OverlappedK);
	}
	else if (errorCode == ERROR_OPERATION_ABORTED || errorCode == ERROR_CANCELLED || errorCode == ERROR_SEM_TIMEOUT)
	{
		if (userCancelled)
			errorCode = ERROR_CANCELLED;
		else
			errorCode = ERROR_OPERATION_ABORTED;

		if (WaitFlags & KOVL_WAIT_FLAG_RELEASE_ON_FAIL) OvlK_Release(OverlappedK);
	}
	else
	{
		// overlapped I/O failed and will never complete.
		if (WaitFlags & KOVL_WAIT_FLAG_RELEASE_ON_FAIL) OvlK_Release(OverlappedK);
		USBERRN("Unknown OverlappedK failure. errorCode=%08Xh", errorCode);
	}

	PoolHandle_Dec_OvlK(overlapped);
	return LusbwError(errorCode);
}

KUSB_EXP BOOL KUSB_API OvlK_WaitOrCancel(
    _in KOVL_HANDLE OverlappedK,
    _inopt ULONG TimeoutMS,
    _out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, KOVL_WAIT_FLAG_CANCEL_ON_TIMEOUT, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_WaitAndRelease(
    _in KOVL_HANDLE OverlappedK,
    _inopt ULONG TimeoutMS,
    _out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, KOVL_WAIT_FLAG_RELEASE_ALWAYS, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
    _in KOVL_HANDLE OverlappedK)
{
	ErrorHandleAction(!ALLK_VALID_HANDLE(OverlappedK, OvlK), "OverlappedK", return FALSE);
	return mOvlK_IsComplete(OverlappedK);
}

KUSB_EXP BOOL KUSB_API OvlK_ReUse(
    _in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);

	ErrorHandleAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

	o_Reuse(overlapped);

	PoolHandle_Dec_OvlK(overlapped);
	return TRUE;
}
