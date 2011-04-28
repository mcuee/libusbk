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

	struct
	{
		unsigned int IsReUsable: 1;
		unsigned int IsNotEventOwner: 1;
	} Flags;

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

	for (overlappedPos = 0; overlappedPos < MaxOverlappedCount; overlappedPos++)
	{
		Pool->OverlappedKs[overlappedPos].Pool = Pool;
		Pool->OverlappedKs[overlappedPos].ReUseLink.Overlapped = (POVERLAPPED_K)&Pool->OverlappedKs[overlappedPos];
	}
	return TRUE;
}



KUSB_EXP POVERLAPPED_K KUSB_API OvlK_Acquire(
    __in_opt POVERLAPPED_K_POOL Pool)
{
	ULONG count = 0;
	POVERLAPPED_K_INTERNAL next = NULL;
	POVERLAPPED_K_POOL_USER pool = (POVERLAPPED_K_POOL_USER)Pool;
	BOOL reUseListAvailable = FALSE;

	if (!pool)
		pool = (POVERLAPPED_K_POOL_USER)&DefaultPool;
Retry:
	if (pool->Private.ReUseList)
	{
		// first try and get a refurbished overlapped
		if (IncLock(pool->Private.BusyCount) == 1)
		{
			// We can use a refurbished one.
			next = (POVERLAPPED_K_INTERNAL)pool->Private.ReUseList->Overlapped;
			next->Flags.IsReUsable = FALSE;
			List_Remove(pool->Private.ReUseList, pool->Private.ReUseList);
			DecLock(pool->Private.BusyCount);

			// the usage count is already at (1) for Overlapped
			goto Success;
		}
		reUseListAvailable = pool->Private.ReUseList != NULL ? TRUE : FALSE;
		DecLock(pool->Private.BusyCount);
	}

	// No refurbished overlappedKs or the list is busy; we will not wait for it; just get a new one.
	while(count++ < pool->Private.MaxCount)
	{
		// NextPosition rolls around OVERLAPPED_POOLEDHANDLE_MAX
		next = &pool->OverlappedKs[IncLock(pool->Private.NextPosition) & (pool->Private.MaxCount - 1)];

		// this one is refurbishing; we will have to continue on.
		if (next->Flags.IsReUsable) continue;

		// make sure it's not already in-use.
		if (IncUsageLock(next) == 1)
			goto Success;
		// it was..
		DecUsageLock(next);
	}

	if (reUseListAvailable)
	{
		// a last ditch effort to get an overlapped.
		// this will never happen with a moderately sized pool.

		Sleep(0); // this is the only "wait" in the api.

		USBWRN("no more overlappedKs (max=%d) but reUseListAvailable=TRUE; retrying..\n",
		       pool->Private.MaxCount);
		goto Retry;
	}

	// this should never happen; though we will want to now if it does.
	USBERR("no more OverlappedKs! (max=%d)\n", pool->Private.MaxCount);
	LusbwError(ERROR_OUT_OF_STRUCTURES);
	return NULL;

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

	if (IncLock(pool->Private.BusyCount) == 1)
	{
		if (!overlapped->Flags.IsReUsable)
		{
			// we can return to the refurbished queue.
			overlapped->ReUseLink.Overlapped = OverlappedK;
			overlapped->Flags.IsReUsable = TRUE;
			List_AddTail(pool->Private.ReUseList, &overlapped->ReUseLink);

			// release this functions ref (is ref count is now 1; it will stay this way while in the refurbished list.
			DecUsageLock(overlapped);

			// release the BusyCount ref
			DecLock(pool->Private.BusyCount);
			return TRUE;
		}
		else
		{
			// release this functions ref (is ref count is now 1; it will stay this way while in the refurbished list.
			DecUsageLock(overlapped);

			// release the BusyCount ref
			DecLock(pool->Private.BusyCount);
			USBERR("IsReUsable=True\n");
			return LusbwError(ERROR_INVALID_PARAMETER);
		}
	}

	// we didn't get a (1) so we must put BusyCount back to it's previous state.
	DecLock(pool->Private.BusyCount);

	// release this functions ref
	DecUsageLock(overlapped);

	// return this one to the main pool. (no refurbishing because another thread is busy with the list)
	DecUsageLock(overlapped);

	return TRUE;
Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API OvlK_FreePool(
    __in POVERLAPPED_K_POOL Pool)
{
	POVERLAPPED_K_POOL_USER pool = (POVERLAPPED_K_POOL_USER)Pool;
	ULONG pos;
	ErrorHandle(!Pool, Error, "Pool");

	if (pool == (POVERLAPPED_K_POOL_USER)&DefaultPool)
		IsDefaultPoolInitialized = FALSE;

	while(IncLock(pool->Private.BusyCount) > 1)
	{
		DecLock(pool->Private.BusyCount);
		Sleep(0);
	}

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

KUSB_EXP BOOL KUSB_API OvlK_FreeDefaultPool()
{
	return OvlK_FreePool(&DefaultPool);
}

KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreatePool(
    __in USHORT MaxOverlappedCount)
{
	POVERLAPPED_K_POOL_USER pool = NULL;
	ULONG poolSize;
	BOOL success = TRUE;

	if (MaxOverlappedCount != 2 &&
	        MaxOverlappedCount != 4 &&
	        MaxOverlappedCount != 8 &&
	        MaxOverlappedCount != 16 &&
	        MaxOverlappedCount != 32 &&
	        MaxOverlappedCount != 64 &&
	        MaxOverlappedCount != 128 &&
	        MaxOverlappedCount != 256 &&
	        MaxOverlappedCount != 512)
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


KUSB_EXP POVERLAPPED_K KUSB_API OvlK_CreateDefaultPool()
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
	return (POVERLAPPED_K)&DefaultPool;
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
			if (!overlapped->Private.CancelOverlappedCB(OverlappedK))
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
