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
#include "lusbk_common.h"
#include "lusbk_overlapped.h"
#include "lusbk_linked_list.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define IncLock(LockField) InterlockedIncrement(&LockField)
#define DecLock(LockField) InterlockedDecrement(&LockField)

#define IncUsageLock(LockParentPtr) IncLock(LockParentPtr->UsageCount)
#define DecUsageLock(LockParentPtr) DecLock(LockParentPtr->UsageCount)

#define SafeCloseEvent(EventHandle) if (EventHandle){ SetEvent(EventHandle); WaitForSingleObject(EventHandle,0); CloseHandle(EventHandle); }

#define ReturnOnFreeOverlapped(OverlappedKPtr,ReturnValue)							\
{																					\
	if (IncUsageLock(((PKOVL_OVERLAPPED_INTERNAL)(OverlappedKPtr))) < 2)				\
	{																				\
		DecUsageLock(((PKOVL_OVERLAPPED_INTERNAL)(OverlappedKPtr)));					\
		LusbwError(ERROR_ACCESS_DENIED);											\
		return ReturnValue;															\
	}																				\
	DecUsageLock(((PKOVL_OVERLAPPED_INTERNAL)(OverlappedKPtr)));						\
}

#define LockOnInUseOverlapped(OverlappedKPtr,ReturnValue)							\
	if (IncUsageLock(((PKOVL_OVERLAPPED_INTERNAL)(OverlappedKPtr))) < 2)				\
	{																				\
		DecUsageLock(((PKOVL_OVERLAPPED_INTERNAL)(OverlappedKPtr)));					\
		LusbwError(ERROR_ACCESS_DENIED);											\
		return ReturnValue;															\
	}

#define CALC_POOL_SIZE(OverlappedCount) sizeof(KOVL_OVERLAPPED_POOL_USER)+(sizeof(KOVL_OVERLAPPED_INTERNAL)*OverlappedCount)

typedef struct _KOVL_OVERLAPPED_EL
{
	PKOVL_OVERLAPPED Overlapped;
	struct _KOVL_OVERLAPPED_EL* next;
	struct _KOVL_OVERLAPPED_EL* prev;

} KOVL_OVERLAPPED_EL, *PKOVL_OVERLAPPED_EL;

typedef struct _KOVL_OVERLAPPED_INTERNAL
{
	OVERLAPPED Overlapped;

	KUSB_USER_CONTEXT UserContext;

	KOVL_OVERLAPPED_INFO Private;

	KOVL_OVERLAPPED_EL ReUseLink;

	volatile long UsageCount;
	volatile long IsReUsableLock;

	PKOVL_OVERLAPPED_POOL Pool;

} KOVL_OVERLAPPED_INTERNAL, *PKOVL_OVERLAPPED_INTERNAL;

typedef struct _KOVL_OVERLAPPED_POOL_PRIVATE
{
	ULONG MaxCount;
	ULONG PoolSize;
	PKOVL_OVERLAPPED_EL ReUseList;
	volatile long NextPosition;
	volatile long BusyCount;
} KOVL_OVERLAPPED_POOL_PRIVATE, *PKOVL_OVERLAPPED_POOL_PRIVATE;

#pragma warning(disable:4200)
typedef struct _KOVL_OVERLAPPED_POOL_USER
{
	KUSB_USER_CONTEXT UserContext;
	KOVL_OVERLAPPED_POOL_PRIVATE Private;
	KOVL_OVERLAPPED_INTERNAL OverlappedKs[0];
} KOVL_OVERLAPPED_POOL_USER, *PKOVL_OVERLAPPED_POOL_USER;
#pragma warning(default:4200)

typedef struct _KOVL_OVERLAPPED_POOL_DEFAULT
{
	KUSB_USER_CONTEXT UserContext;
	KOVL_OVERLAPPED_POOL_PRIVATE Private;
	KOVL_OVERLAPPED_INTERNAL OverlappedKs[KOVL_MAX_DEFAULT_POOL_COUNT];
} KOVL_OVERLAPPED_POOL_DEFAULT, *PKOVL_OVERLAPPED_POOL_DEFAULT;
C_ASSERT(sizeof(KOVL_OVERLAPPED_POOL_DEFAULT) == CALC_POOL_SIZE(KOVL_MAX_DEFAULT_POOL_COUNT));

static volatile BOOL IsDefaultPoolInitialized = FALSE;
static volatile long DefaultPoolInitCount = 0;
static KOVL_OVERLAPPED_POOL_DEFAULT DefaultPool;

BOOL k_InitPool(
    __in PKOVL_OVERLAPPED_POOL_USER Pool,
    __in ULONG PoolSize,
    __in USHORT MaxOverlappedCount)
{
	int overlappedPos;
	Pool->Private.PoolSize = PoolSize;
	Pool->Private.MaxCount = MaxOverlappedCount;
	Pool->Private.NextPosition = -1;
	Pool->Private.ReUseList = NULL;

	for (overlappedPos = 0; overlappedPos < MaxOverlappedCount; overlappedPos++)
	{
		Pool->OverlappedKs[overlappedPos].Pool = Pool;
		Pool->OverlappedKs[overlappedPos].ReUseLink.Overlapped = (PKOVL_OVERLAPPED)&Pool->OverlappedKs[overlappedPos];
		Pool->OverlappedKs[overlappedPos].IsReUsableLock = 0;
	}

	// RELEASE the POOL lock
	SpinLock_Release(&Pool->Private.BusyCount);
	return TRUE;
}



KUSB_EXP PKOVL_OVERLAPPED KUSB_API OvlK_Acquire(
    __in_opt PKOVL_OVERLAPPED_POOL Pool)
{
	ULONG count = 0;
	PKOVL_OVERLAPPED_INTERNAL next = NULL;
	PKOVL_OVERLAPPED_POOL_USER pool = (PKOVL_OVERLAPPED_POOL_USER)Pool;

	CheckLibInitialized();

	if (!pool)
		pool = (PKOVL_OVERLAPPED_POOL_USER)&DefaultPool;

Retry:
	if (pool->Private.ReUseList)
	{
		PKOVL_OVERLAPPED_EL oldest;

		// ACQUIRE the POOL lock
		SpinLock_Acquire(&pool->Private.BusyCount, TRUE);

		if ((oldest = pool->Private.ReUseList) != NULL)
		{
			// We can use a refurbished one.
			next = (PKOVL_OVERLAPPED_INTERNAL)oldest->Overlapped;
			if (next)
			{
				DL_DELETE(pool->Private.ReUseList, oldest);

				// RELEASE the POOL lock
				SpinLock_Release(&pool->Private.BusyCount);

				// MARK overlapped as NOT re-usable
				SpinLock_Release(&next->IsReUsableLock);

				goto Success;
			}
		}

		// RELEASE the POOL lock
		SpinLock_Release(&pool->Private.BusyCount);
	}

	// No refurbished overlappedKs; get a new one.
	while(count++ < pool->Private.MaxCount)
	{
		// NextPosition rolls around OVERLAPPED_POOLEDHANDLE_MAX
		long nextpos = IncLock(pool->Private.NextPosition) % pool->Private.MaxCount;
		next = &pool->OverlappedKs[nextpos];

		if (next->IsReUsableLock || pool->Private.ReUseList)
		{
			// There is one refurbishing; go try and get it.
			count = 0;
			goto Retry;
		}

		if (!next->UsageCount)
		{
			// make sure it's not already in-use.
			if (IncUsageLock(next) == 1)
				goto Success;
			// it was..
			DecUsageLock(next);
		}
	}

	// this queue is all used up
	SetLastError(ERROR_NO_MORE_ITEMS);
	return NULL;


Success:
	OvlK_ReUse((PKOVL_OVERLAPPED)next);
	return (PKOVL_OVERLAPPED)next;
}

KUSB_EXP BOOL KUSB_API OvlK_Release(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	PKOVL_OVERLAPPED_POOL_USER pool;

	ErrorHandle(!overlapped, Error, "OverlappedK");

	pool = (PKOVL_OVERLAPPED_POOL_USER)overlapped->Pool;

	// increment the usgae lock, but decrement, fail, and return if it's new count < 2
	LockOnInUseOverlapped(overlapped, FALSE);

	// ACQUIRE the POOL lock
	SpinLock_Acquire(&pool->Private.BusyCount, TRUE);

	// MARK overlapped as RE-USABLE and add to ReUseList (unless it is already in the list)
	if (SpinLock_Acquire(&overlapped->IsReUsableLock, FALSE))
	{
		overlapped->ReUseLink.Overlapped = OverlappedK;
		DL_APPEND(pool->Private.ReUseList, &overlapped->ReUseLink);
	}

	// RELEASE the POOL lock
	SpinLock_Release(&pool->Private.BusyCount);

	// release this functions reference.
	// if ref count is now 1; it will stay this way while in the refurbished list.
	DecUsageLock(overlapped);

	return TRUE;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_DestroyPool(
    __in PKOVL_OVERLAPPED_POOL Pool)
{
	PKOVL_OVERLAPPED_POOL_USER pool = (PKOVL_OVERLAPPED_POOL_USER)Pool;
	ULONG pos;
	ErrorHandle(!Pool, Error, "Pool");

	if (pool == (PKOVL_OVERLAPPED_POOL_USER)&DefaultPool)
		IsDefaultPoolInitialized = FALSE;

	// ACQUIRE the POOL lock
	SpinLock_Acquire(&pool->Private.BusyCount, TRUE);

	for (pos = 0; pos < pool->Private.MaxCount; pos++)
	{
		HANDLE hEvent = pool->OverlappedKs[pos].Overlapped.hEvent;
		if (hEvent)
		{
			SetEvent(hEvent);
			WaitForSingleObject(hEvent, 0);
			CloseHandle(hEvent);
		}
	}

	if (pool != (PKOVL_OVERLAPPED_POOL_USER)&DefaultPool)
	{
		Mem_Free(&pool);
	}
	else
	{
		IsDefaultPoolInitialized = FALSE;
		DecLock(DefaultPoolInitCount);
		OvlK_CreateDefaultPool();
	}
	return TRUE;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_DestroyDefaultPool()
{
	return OvlK_DestroyPool(&DefaultPool);
}

KUSB_EXP PKOVL_OVERLAPPED_POOL KUSB_API OvlK_CreatePool(
    __in USHORT MaxOverlappedCount)
{
	PKOVL_OVERLAPPED_POOL_USER pool = NULL;
	ULONG poolSize;
	BOOL success = TRUE;

	if (MaxOverlappedCount > 512)
	{
		LusbwError(ERROR_RANGE_NOT_FOUND);
		return FALSE;
	}

	poolSize = CALC_POOL_SIZE(MaxOverlappedCount);
	USBDBG("MaxOverlappedCount=%u poolSize=%u\n", MaxOverlappedCount, poolSize);

	pool = Mem_Alloc(poolSize);
	ErrorMemory(!pool, Error);

	success = k_InitPool(pool, poolSize, MaxOverlappedCount);
	ErrorNoSet(!success, Error, "->k_InitPool");

	return pool;
Error:
	return NULL;
}


KUSB_EXP PKOVL_OVERLAPPED_POOL KUSB_API OvlK_CreateDefaultPool()
{
	long lockCount;
	while(!IsDefaultPoolInitialized)
	{
		if ((lockCount = IncLock(DefaultPoolInitCount)) == 1)
		{
			// init default pool.
			k_InitPool((PKOVL_OVERLAPPED_POOL_USER)&DefaultPool, sizeof(DefaultPool), KOVL_MAX_DEFAULT_POOL_COUNT);
			IsDefaultPoolInitialized = TRUE;
			USBDBG(
			    "Memory Usage:\r\n"
			    "\tDefaultPool  : %u bytes (%u each)\r\n",
			    sizeof(DefaultPool), sizeof(DefaultPool.OverlappedKs[0])
			);
		}
		else
		{
			DecLock(DefaultPoolInitCount);
			while (lockCount-- > 0)
				Sleep(0);

			USBDBG("lock collision; retrying..\n");
		}
	}
	return &DefaultPool;
}

KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return overlapped->Overlapped.hEvent;
Error:
	return FALSE;
}

KUSB_EXP PKOVL_OVERLAPPED_INFO KUSB_API OvlK_GetInfo(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return &overlapped->Private;
Error:
	return NULL;
}

KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetContext(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return &overlapped->UserContext;
Error:
	return NULL;
}

KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetPoolContext(
    __in_opt PKOVL_OVERLAPPED_POOL Pool)
{
	PKOVL_OVERLAPPED_POOL_USER pool = (PKOVL_OVERLAPPED_POOL_USER)Pool;
	CheckLibInitialized();
	if (!pool)
		pool = (PKOVL_OVERLAPPED_POOL_USER)&DefaultPool;

	return &pool->UserContext;
}

KUSB_EXP BOOL KUSB_API OvlK_Wait(
    __in PKOVL_OVERLAPPED OverlappedK,
    __in_opt DWORD TimeoutMS,
    __in_opt KOVL_WAIT_FLAGS WaitFlags,
    __out PULONG TransferredLength)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	DWORD wait;
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	ErrorHandle(!overlapped, Error, "OverlappedK");
	ErrorParam(!TransferredLength, Error, "TransferredLength");

	// make sure this this overlapped is in-use.
	ReturnOnFreeOverlapped(overlapped, FALSE);

	wait = WaitForSingleObject(overlapped->Overlapped.hEvent, TimeoutMS);
	switch(wait)
	{
	case WAIT_OBJECT_0:
		success = GetOverlappedResult(overlapped->Private.DeviceHandle, &overlapped->Overlapped, TransferredLength, FALSE);
		if (!success)
		{
			errorCode = GetLastError();
			if (WaitFlags & WAIT_FLAGS_RELEASE_ON_FAIL)
				OvlK_Release(OverlappedK);
		}
		else
		{
			if (WaitFlags & WAIT_FLAGS_RELEASE_ON_SUCCESS)
				OvlK_Release(OverlappedK);
		}
		break;
	case WAIT_TIMEOUT:
		errorCode = ERROR_IO_INCOMPLETE;
		if (WaitFlags & WAIT_FLAGS_CANCEL_ON_TIMEOUT)
		{
			if (!overlapped->Private.Cancel(OverlappedK))
			{
				errorCode = GetLastError();
				USBERR("failed cancelling OverlappedK.\n");
				SetEvent(overlapped->Overlapped.hEvent);
			}
			else
			{
				errorCode = ERROR_OPERATION_ABORTED;
				if (WaitForSingleObject(overlapped->Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
					SetEvent(overlapped->Overlapped.hEvent);
			}
		}
		if (WaitFlags & WAIT_FLAGS_RELEASE_ON_TIMEOUT)
			OvlK_Release(OverlappedK);
		break;
	default:
		errorCode = GetLastError();
		if (WaitFlags & WAIT_FLAGS_RELEASE_ON_FAIL)
			OvlK_Release(OverlappedK);
		break;
	}

	return LusbwError(errorCode);
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_WaitOrCancel(
    __in PKOVL_OVERLAPPED OverlappedK,
    __in_opt DWORD TimeoutMS,
    __out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, WAIT_FLAGS_CANCEL_ON_TIMEOUT, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_WaitAndRelease(
    __in PKOVL_OVERLAPPED OverlappedK,
    __in_opt DWORD TimeoutMS,
    __out PULONG TransferredLength)
{
	return OvlK_Wait(OverlappedK, TimeoutMS, WAIT_FLAGS_RELEASE_ALWAYS, TransferredLength);
}

KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	return WaitForSingleObject(((PKOVL_OVERLAPPED_INTERNAL)OverlappedK)->Overlapped.hEvent, 0) != WAIT_TIMEOUT;
}

KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
    __in PKOVL_OVERLAPPED OverlappedK)
{
	PKOVL_OVERLAPPED_INTERNAL overlapped = (PKOVL_OVERLAPPED_INTERNAL)OverlappedK;
	PKOVL_OVERLAPPED_POOL_USER pool;

	ErrorHandle(!overlapped, Error, "OverlappedK");

	// make sure this this overlapped is in-use.
	LockOnInUseOverlapped(overlapped, FALSE);

	pool = (PKOVL_OVERLAPPED_POOL_USER)overlapped->Pool;

	// create/reuse the event
	if (!IsHandleValid(overlapped->Overlapped.hEvent))
		overlapped->Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset
	else
		ResetEvent(overlapped->Overlapped.hEvent);

	// clear the overlapped
	overlapped->Overlapped.Internal = 0;
	overlapped->Overlapped.InternalHigh = 0;
	overlapped->Overlapped.Pointer = OverlappedK;

	DecUsageLock(overlapped);
	return TRUE;

Error:
	return TRUE;
}
