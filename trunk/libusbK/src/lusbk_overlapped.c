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
	if (IncUsageLock(((POVERLAPPED_K_INTERNAL)(OverlappedKPtr))) < 2)				\
	{																				\
		DecUsageLock(((POVERLAPPED_K_INTERNAL)(OverlappedKPtr)));					\
		LusbwError(ERROR_ACCESS_DENIED);											\
		return ReturnValue;															\
	}																				\
	DecUsageLock(((POVERLAPPED_K_INTERNAL)(OverlappedKPtr)));						\
}

#define LockOnInUseOverlapped(OverlappedKPtr,ReturnValue)							\
	if (IncUsageLock(((POVERLAPPED_K_INTERNAL)(OverlappedKPtr))) < 2)				\
	{																				\
		DecUsageLock(((POVERLAPPED_K_INTERNAL)(OverlappedKPtr)));					\
		LusbwError(ERROR_ACCESS_DENIED);											\
		return ReturnValue;															\
	}

#define CALC_POOL_SIZE(OverlappedCount) sizeof(OVERLAPPED_K_POOL_USER)+(sizeof(OVERLAPPED_K_INTERNAL)*OverlappedCount)

typedef struct _OVERLAPPED_K_EL
{
	POVERLAPPED_K Overlapped;
	struct _OVERLAPPED_K_EL* next;
	struct _OVERLAPPED_K_EL* prev;

} OVERLAPPED_K_EL, *POVERLAPPED_K_EL;

typedef struct _OVERLAPPED_K_INTERNAL
{
	OVERLAPPED Overlapped;

	KUSB_USER_CONTEXT UserContext;

	OVERLAPPED_K_INFO Private;

	OVERLAPPED_K_EL ReUseLink;

	volatile long UsageCount;
	volatile long IsReUsableLock;

	POVERLAPPED_K_POOL Pool;

} OVERLAPPED_K_INTERNAL, *POVERLAPPED_K_INTERNAL;

typedef struct _OVERLAPPED_K_POOL_PRIVATE
{
	ULONG MaxCount;
	ULONG PoolSize;
	POVERLAPPED_K_EL ReUseList;
	volatile long NextPosition;
	volatile long BusyCount;
} OVERLAPPED_K_POOL_PRIVATE, *POVERLAPPED_K_POOL_PRIVATE;

#pragma warning(disable:4200)
typedef struct _OVERLAPPED_K_POOL_USER
{
	KUSB_USER_CONTEXT UserContext;
	OVERLAPPED_K_POOL_PRIVATE Private;
	OVERLAPPED_K_INTERNAL OverlappedKs[0];
} OVERLAPPED_K_POOL_USER, *POVERLAPPED_K_POOL_USER;
#pragma warning(default:4200)

typedef struct _OVERLAPPED_K_POOL_DEFAULT
{
	KUSB_USER_CONTEXT UserContext;
	OVERLAPPED_K_POOL_PRIVATE Private;
	OVERLAPPED_K_INTERNAL OverlappedKs[DEFAULT_POOL_MAX_COUNT];
} OVERLAPPED_K_POOL_DEFAULT, *POVERLAPPED_K_POOL_DEFAULT;
C_ASSERT(sizeof(OVERLAPPED_K_POOL_DEFAULT) == CALC_POOL_SIZE(DEFAULT_POOL_MAX_COUNT));

static volatile BOOL IsDefaultPoolInitialized = FALSE;
static volatile long DefaultPoolInitCount = 0;
static OVERLAPPED_K_POOL_DEFAULT DefaultPool;

BOOL k_InitPool(
    __in POVERLAPPED_K_POOL_USER Pool,
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
		Pool->OverlappedKs[overlappedPos].ReUseLink.Overlapped = (POVERLAPPED_K)&Pool->OverlappedKs[overlappedPos];
		Pool->OverlappedKs[overlappedPos].IsReUsableLock = 0;
	}

	SpinLock_Release(&Pool->Private.BusyCount);
	return TRUE;
}



KUSB_EXP POVERLAPPED_K KUSB_API OvlK_Acquire(
    __in_opt POVERLAPPED_K_POOL Pool)
{
	ULONG count = 0;
	POVERLAPPED_K_INTERNAL next = NULL;
	POVERLAPPED_K_POOL_USER pool = (POVERLAPPED_K_POOL_USER)Pool;

	if (!pool)
		pool = (POVERLAPPED_K_POOL_USER)&DefaultPool;

Retry:
	if (pool->Private.ReUseList)
	{
		SpinLock_Acquire(&pool->Private.BusyCount, TRUE);
		if (pool->Private.ReUseList)
		{
			// We can use a refurbished one.
			next = (POVERLAPPED_K_INTERNAL)pool->Private.ReUseList->Overlapped;
			if (next)
			{
				SpinLock_Release(&next->IsReUsableLock);
				DL_DELETE(pool->Private.ReUseList, pool->Private.ReUseList);
				SpinLock_Release(&pool->Private.BusyCount);
				goto Success;
			}
		}

		SpinLock_Release(&pool->Private.BusyCount);
	}

	// No refurbished overlappedKs; get a new one.
	while(count++ < pool->Private.MaxCount)
	{
		// NextPosition rolls around OVERLAPPED_POOLEDHANDLE_MAX
		long nextpos = IncLock(pool->Private.NextPosition) % pool->Private.MaxCount;
		next = &pool->OverlappedKs[nextpos];

		// this one is refurbishing; we will have to continue on.
		if (next->IsReUsableLock) continue;

		// make sure it's not already in-use.
		if (IncUsageLock(next) == 1)
			goto Success;
		// it was..
		DecUsageLock(next);
	}

	// a last ditch effort to get an overlapped.
	// this will never happen with a moderately sized pool.
	SwitchToThread();
	Sleep(0);

	USBWRN("no more overlappedKs (max=%d) but reUseListAvailable=TRUE; retrying..\n",
	       pool->Private.MaxCount);

	goto Retry;

Success:
	OvlK_ReUse((POVERLAPPED_K)next);
	return (POVERLAPPED_K)next;
}

KUSB_EXP BOOL KUSB_API OvlK_Release(
    __in POVERLAPPED_K OverlappedK)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	POVERLAPPED_K_POOL_USER pool;

	ErrorHandle(!overlapped, Error, "OverlappedK");

	pool = (POVERLAPPED_K_POOL_USER)overlapped->Pool;


	LockOnInUseOverlapped(overlapped, FALSE);

	SpinLock_Acquire(&pool->Private.BusyCount, TRUE);

	if (SpinLock_Acquire(&overlapped->IsReUsableLock, FALSE))
	{
		overlapped->ReUseLink.Overlapped = OverlappedK;
		DL_APPEND(pool->Private.ReUseList, &overlapped->ReUseLink);
	}

	SpinLock_Release(&pool->Private.BusyCount);

	// release this functions reference.
	// if ref count is now 1; it will stay this way while in the refurbished list.
	DecUsageLock(overlapped);

	return TRUE;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_DestroyPool(
    __in POVERLAPPED_K_POOL Pool)
{
	POVERLAPPED_K_POOL_USER pool = (POVERLAPPED_K_POOL_USER)Pool;
	ULONG pos;
	ErrorHandle(!Pool, Error, "Pool");

	if (pool == (POVERLAPPED_K_POOL_USER)&DefaultPool)
		IsDefaultPoolInitialized = FALSE;

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

	if (pool != (POVERLAPPED_K_POOL_USER)&DefaultPool)
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

KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreatePool(
    __in USHORT MaxOverlappedCount)
{
	POVERLAPPED_K_POOL_USER pool = NULL;
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


KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreateDefaultPool()
{
	long lockCount;
	while(!IsDefaultPoolInitialized)
	{
		if ((lockCount = IncLock(DefaultPoolInitCount)) == 1)
		{
			// init default pool.
			k_InitPool((POVERLAPPED_K_POOL_USER)&DefaultPool, sizeof(DefaultPool), DEFAULT_POOL_MAX_COUNT);
			IsDefaultPoolInitialized = TRUE;
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
    __in POVERLAPPED_K OverlappedK)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return overlapped->Overlapped.hEvent;
Error:
	return FALSE;
}

KUSB_EXP POVERLAPPED_K_INFO KUSB_API OvlK_GetInfo(
    __in POVERLAPPED_K OverlappedK)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return &overlapped->Private;
Error:
	return NULL;
}

KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetContext(
    __in POVERLAPPED_K OverlappedK)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	ErrorHandle(!overlapped, Error, "OverlappedK");

	return &overlapped->UserContext;
Error:
	return NULL;
}

KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetPoolContext(
    __in_opt POVERLAPPED_K_POOL Pool)
{
	POVERLAPPED_K_POOL_USER pool = (POVERLAPPED_K_POOL_USER)Pool;
	ErrorHandle(!pool, Error, "Pool");

	return &pool->UserContext;
Error:
	return NULL;
}

KUSB_EXP BOOL KUSB_API OvlK_WaitComplete(
    __in POVERLAPPED_K OverlappedK,
    __in_opt DWORD TimeoutMS,
    __in_opt OVERLAPPEDK_WAIT_FLAGS WaitFlags,
    __out PULONG TransferredLength)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	DWORD wait;
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	ErrorHandle(!overlapped, Error, "OverlappedK");

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
			if (WaitFlags & WAIT_FLAGS_ON_FAIL_RELEASE)
				OvlK_Release(OverlappedK);
		}
		else
		{
			if (WaitFlags & WAIT_FLAGS_ON_SUCCESS_RELEASE)
				OvlK_Release(OverlappedK);
		}
		break;
	case WAIT_TIMEOUT:
		errorCode = ERROR_TIMEOUT;
		if (WaitFlags & WAIT_FLAGS_ON_TIMEOUT_CANCEL)
		{
			if (!overlapped->Private.Cancel(OverlappedK))
			{
				errorCode = ERROR_CANCELLED;
				USBERR("failed cancelling OverlappedK.\n");
				SetEvent(overlapped->Overlapped.hEvent);
			}
			else
			{
				if (WaitForSingleObject(overlapped->Overlapped.hEvent, 1) != WAIT_OBJECT_0)
					SetEvent(overlapped->Overlapped.hEvent);
			}
		}
		if (WaitFlags & WAIT_FLAGS_ON_TIMEOUT_RELEASE)
		{
			OvlK_Release(OverlappedK);
		}
		break;
	default:
		errorCode = GetLastError();
		if (WaitFlags & WAIT_FLAGS_ON_FAIL_RELEASE)
		{
			OvlK_Release(OverlappedK);
		}
		break;
	}

	return LusbwError(errorCode);
Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
    __in POVERLAPPED_K OverlappedK)
{
	return WaitForSingleObject(((POVERLAPPED_K_INTERNAL)OverlappedK)->Overlapped.hEvent, 0) == WAIT_OBJECT_0;
}

KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
    __in POVERLAPPED_K OverlappedK)
{
	POVERLAPPED_K_INTERNAL overlapped = (POVERLAPPED_K_INTERNAL)OverlappedK;
	POVERLAPPED_K_POOL_USER pool;

	ErrorHandle(!overlapped, Error, "OverlappedK");

	// make sure this this overlapped is in-use.
	LockOnInUseOverlapped(overlapped, FALSE);

	pool = (POVERLAPPED_K_POOL_USER)overlapped->Pool;

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
