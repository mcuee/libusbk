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

static void KUSB_API Cleanup_OvlPoolK(PKOVL_POOL_HANDLE_INTERNAL handle)
{
	PKOVL_LINK_EL linkEL;

	PoolHandle_Dead_OvlPoolK(handle);
	DL_FOREACH(handle->MasterList, linkEL)
	{
		PoolHandle_Dec_OvlK(linkEL->OverlappedK);
	}
}

static void KUSB_API Cleanup_OvlK(PKOVL_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_OvlK(handle);
	SafeCloseEvent(handle->Overlapped.hEvent);
	Mem_Free(&handle->Private);
}

static void o_Reuse(PKOVL_HANDLE_INTERNAL overlapped)
{
	if (!overlapped->Overlapped.hEvent)
		overlapped->Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	else
	{
		SetEvent(overlapped->Overlapped.hEvent);
		ResetEvent(overlapped->Overlapped.hEvent);
	}
}

KUSB_EXP BOOL KUSB_API OvlK_Acquire(
    __out KOVL_HANDLE* OverlappedK,
    __in KOVL_POOL_HANDLE Pool)
{
	PKOVL_HANDLE_INTERNAL overlapped = NULL;
	PKOVL_POOL_HANDLE_INTERNAL pool;
	PKOVL_LINK_EL linkEL;
	BOOL isNewFromPool = FALSE;

	Pub_To_Priv_OvlPoolK(Pool, pool, return FALSE);
	ErrorParamAction(!IsHandleValid(OverlappedK), "OverlappedK", return FALSE);
	*OverlappedK = NULL;

	//// L O C K ////
	ErrorParamAction(!PoolHandle_Acquire_Spin_OvlPoolK(pool, TRUE), "Pool", return FALSE);
	/////////////////

	if ((linkEL = pool->ReleasedList) == NULL)
	{
		if (pool->MaxCount && pool->Count >= pool->MaxCount)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			goto Done;
		}
		// Get a new OverlappedK handle.
		overlapped = PoolHandle_Acquire_OvlK(Cleanup_OvlK);
		ErrorNoSet(!overlapped, Done, "->PoolHandle_Acquire_OvlK");

		overlapped->Pool = pool;
		overlapped->MasterLink.OverlappedK = overlapped;
		overlapped->Link.OverlappedK = overlapped;
		overlapped->Private = Mem_Alloc(sizeof(*overlapped->Private));

		DL_APPEND(pool->MasterList, &overlapped->MasterLink);
		DL_APPEND(pool->AcquiredList, &overlapped->Link);

		pool->Count++;
		isNewFromPool = TRUE;
	}
	else
	{
		// 1 or more overlappedKs available in the refurbished release list.
		overlapped = linkEL->OverlappedK;

		DL_DELETE(pool->ReleasedList, linkEL);
		DL_APPEND(pool->AcquiredList, linkEL);
	}

	overlapped->IsAcquired = 1;
	o_Reuse(overlapped);

Done:
	// U N L O C K //
	PoolHandle_Release_Spin_OvlPoolK(pool);
	/////////////////
	*OverlappedK = (KOVL_HANDLE)overlapped;
	if (overlapped)
	{
		if (isNewFromPool)
		{
			PoolHandle_Live_OvlK(overlapped);
		}
		return TRUE;
	}
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_Release(
    __in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped = NULL;
	PKOVL_LINK_EL linkEL;
	BOOL success = FALSE;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);

	ErrorParamAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

	//// L O C K - I N C ////
	ErrorParamAction(!PoolHandle_Acquire_Spin_OvlPoolK(overlapped->Pool, TRUE), "Pool", PoolHandle_Dec_OvlK(overlapped); return FALSE);
	/////////////////

	success = (InterlockedDecrement(&overlapped->IsAcquired) == 0);
	ErrorSet(!success, Done, ERROR_ACCESS_DENIED, "OverlappedK is not acquired.");

	SetEvent(overlapped->Overlapped.hEvent);

	linkEL = &overlapped->Link;
	DL_DELETE(overlapped->Pool->AcquiredList, linkEL);
	DL_APPEND(overlapped->Pool->ReleasedList, linkEL);

Done:
	// U N L O C K - DEC //
	PoolHandle_Release_Spin_OvlPoolK(overlapped->Pool);
	PoolHandle_Dec_OvlK(overlapped);
	/////////////////

	return success;
}

KUSB_EXP BOOL KUSB_API OvlK_InitPool(
    __out KOVL_POOL_HANDLE* PoolHandle,
    __in USHORT MaxOverlappedCount)
{
	PKOVL_POOL_HANDLE_INTERNAL pool = NULL;
	ErrorParamAction(!IsHandleValid(PoolHandle), "PoolHandle", return FALSE);
	*PoolHandle = NULL;

	CheckLibInit();

	pool = PoolHandle_Acquire_OvlPoolK(Cleanup_OvlPoolK);
	ErrorNoSetAction(!IsHandleValid(pool), return FALSE, "->PoolHandle_Acquire_OvlPoolK");

	pool->MaxCount = MaxOverlappedCount;
	*PoolHandle = (KOVL_POOL_HANDLE)pool;
	PoolHandle_Live_OvlPoolK(pool);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API OvlK_FreePool(
    __in KOVL_POOL_HANDLE Pool)
{
	PKOVL_POOL_HANDLE_INTERNAL pool;

	Pub_To_Priv_OvlPoolK(Pool, pool, return FALSE);
	PoolHandle_Dec_OvlPoolK(pool);

	return TRUE;
}

KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
    __in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped;
	Pub_To_Priv_OvlK(OverlappedK, overlapped, return NULL);
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return overlapped->Overlapped.hEvent;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_Wait(
    __in KOVL_HANDLE OverlappedK,
    __in_opt DWORD TimeoutMS,
    __in_opt KOVL_WAIT_FLAGS WaitFlags,
    __out PULONG TransferredLength)
{
	PKOVL_HANDLE_INTERNAL overlapped;
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;
	DWORD wait;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);
	ErrorParamAction(!TransferredLength, "TransferredLength", return FALSE);

	// INC REF //
	ErrorParamAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

	// wait the specified timeout interval
	wait	= WaitForSingleObject(overlapped->Overlapped.hEvent, TimeoutMS);

	// check for an overlapped result regardless of the WaitForSingleObject return value
	success = GetOverlappedResult(overlapped->Private->DeviceHandle, &overlapped->Overlapped, TransferredLength, FALSE);
	if (success)
	{
		// the overlapped I/O completed successfully
		if (WaitFlags & WAIT_FLAGS_RELEASE_ON_SUCCESS) OvlK_Release(OverlappedK);
	}
	else if (wait == WAIT_TIMEOUT)
	{
		// overlapped I/O has not completed yet but is still pending.
		errorCode = ERROR_IO_INCOMPLETE;
		if (WaitFlags & WAIT_FLAGS_CANCEL_ON_TIMEOUT)
		{
			if (!overlapped->Private->Cancel(OverlappedK))
			{
				errorCode = GetLastError();
				USBERRN("Failed cancelling OverlappedK. errorCode=%08Xh", errorCode);
				SetEvent(overlapped->Overlapped.hEvent);
			}
			else
			{
				errorCode = ERROR_OPERATION_ABORTED;
				if (WaitForSingleObject(overlapped->Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
				{
					errorCode = GetLastError();
					USBERRN("Failed waiting for OverlappedK cancel I/O operation. errorCode=%08Xh", errorCode);
					SetEvent(overlapped->Overlapped.hEvent);
				}
			}
		}

		if (WaitFlags & WAIT_FLAGS_RELEASE_ON_TIMEOUT) OvlK_Release(OverlappedK);
	}
	else
	{
		// overlapped I/O failed and will never complete.
		errorCode = GetLastError();
		if (WaitFlags & WAIT_FLAGS_RELEASE_ON_FAIL) OvlK_Release(OverlappedK);
		USBERRN("Unknown OverlappedK failure. errorCode=%08Xh", errorCode);
	}

	// DEC REF //
	PoolHandle_Dec_OvlK(overlapped);
	return LusbwError(errorCode);
}

KUSB_EXP BOOL KUSB_API OvlK_WaitOrCancel(
    __in KOVL_HANDLE OverlappedK,
    __in_opt DWORD TimeoutMS,
    __out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, WAIT_FLAGS_CANCEL_ON_TIMEOUT, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_WaitAndRelease(
    __in KOVL_HANDLE OverlappedK,
    __in_opt DWORD TimeoutMS,
    __out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, WAIT_FLAGS_RELEASE_ALWAYS, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
    __in KOVL_HANDLE OverlappedK)
{
	ErrorHandleAction(!ALLK_VALID_HANDLE(OverlappedK, OvlK), "OverlappedK", return FALSE);
	return mOvlK_IsComplete(OverlappedK);
}

KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
    __in KOVL_HANDLE OverlappedK)
{
	PKOVL_HANDLE_INTERNAL overlapped;

	Pub_To_Priv_OvlK(OverlappedK, overlapped, return FALSE);

	// INC REF //
	ErrorHandleAction(!PoolHandle_Inc_OvlK(overlapped), "OverlappedK", return FALSE);

	o_Reuse(overlapped);

	// DEC REF //
	PoolHandle_Dec_OvlK(overlapped);

	return TRUE;
}
