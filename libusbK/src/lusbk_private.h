/*! \file lusbk_private.h
*/

#ifndef __KUSB_USER_PRIVATE_API_H__
#define __KUSB_USER_PRIVATE_API_H__

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <objbase.h>
#include <ctype.h>
#include <windows.h>
#include <winioctl.h>
#include <crtdbg.h>

#include "lusbk_version.h"
#include "lusbk_usb.h"
#include "drv_api.h"
#include "lusbk_debug_view_output.h"

//////////////////////////////////////////////////////////////////////////////
// Defines
//

// The maximum number of open interface handles per proccess.
// Internal interface handles come from a fixed array so the memory is
// always valid as long as the dll is loaded.
#define KUSB_MAX_INTERFACE_HANDLES 128

// The maximum number of simultaneous threads allowed per interface handle
#define MAX_THREADS_PER_INTERFACE 32

#define GetSetPipePolicy(BackendContextPtr,pipeID)	\
	(BackendContextPtr->PipePolicies[((((pipeID) & 0xF)|(((pipeID)>>3) & 0x10)) & 0x1F)])

#define CreateDeviceFile(DevicePathStrA) \
	CreateFileA(DevicePathStrA, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Structs
//

typedef struct _KUSB_SHARED_INTERFACE
{
	volatile HANDLE InterfaceHandle;
	INT Index;
	INT Number;
}* PKUSB_SHARED_INTERFACE, KUSB_SHARED_INTERFACE;

typedef struct _KUSB_SHARED_DEVICE
{
	// This atomic lock must inc to '1' before reading/writing any information.
	volatile long ActionPendingLock;

	volatile long OpenedRefCount;
	volatile long UsageCount;

	volatile HANDLE MasterDeviceHandle;
	volatile HANDLE MasterInterfaceHandle;

	CHAR DevicePath[MAX_PATH];
	PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;

	KUSB_SHARED_INTERFACE SharedInterfaces[32];
}* PKUSB_SHARED_DEVICE, KUSB_SHARED_DEVICE;

// The semaphore lock structure use for KUSB_INTERFACE_HANDLE_INTERNAL.
typedef struct _SYNC_LOCK
{
	HANDLE Handle;
	LONG MaxCount;
}* PSYNC_LOCK, SYNC_LOCK;

typedef struct _USER_PIPE_POLICY
{
	volatile ULONG timeout;

}* PUSER_PIPE_POLICY, USER_PIPE_POLICY;

// Internal libusbK interface handle
typedef struct _KUSB_INTERFACE_HANDLE_INTERNAL
{
	KUSB_USER_CONTEXT UserContext;
	PVOID BackendContext;
	struct
	{
		SYNC_LOCK Lock;
		volatile LONG UsageCount;
	} Instance;

}* PKUSB_INTERFACE_HANDLE_INTERNAL, KUSB_INTERFACE_HANDLE_INTERNAL;

//////////////////////////////////////////////////////////////////////////////
// lusbk_ioctl.c - FUNCTION PROTOTYPES
//
BOOL Ioctl_Sync(__in HANDLE dev,
                __in INT code,
                __in_opt PVOID in,
                __in DWORD in_size,
                __inout_opt PVOID out,
                __in DWORD out_size,
                __out_opt PDWORD ret);

BOOL Ioctl_SyncWithTimeout(__in HANDLE DeviceHandle,
                           __in INT code,
                           __in libusb_request* request,
                           __inout_opt PVOID out,
                           __in DWORD out_size,
                           __in INT timeout,
                           __in UCHAR PipeID,
                           __out_opt PDWORD ret);

BOOL Ioctl_Async(__in HANDLE dev,
                 __in INT code,
                 __in_opt PVOID in,
                 __in DWORD in_size,
                 __inout_opt PVOID out,
                 __in DWORD out_size,
                 __in LPOVERLAPPED overlapped);

VOID CheckLibInitialized();

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Macros
//
#define IncUsageCount(PooledHandle) InterlockedIncrement(&((PooledHandle)->Instance.UsageCount))
#define DecUsageCount(PooledHandle) InterlockedDecrement(&((PooledHandle)->Instance.UsageCount))

#define IncDeviceUsageCount(PooledHandle) InterlockedIncrement(&((PooledHandle)->UsageCount))
#define DecDeviceUsageCount(PooledHandle) InterlockedDecrement(&((PooledHandle)->UsageCount))

// Sets 'RetErrorCode' and jumps to 'JumpStatement' if 'FailIfTrue' is non-zero.
//
#define FailIf(FailIfTrue, RetErrorCode, JumpStatement) \
	if (FailIfTrue) { ret = RetErrorCode; goto JumpStatement; }

#define ErrorSet(FailIfTrue, JumpStatement, ErrorCode, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format"\n",ErrorCode,__VA_ARGS__); SetLastError(ErrorCode); goto JumpStatement; }

#define ErrorNoSet(FailIfTrue, JumpStatement, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format"\n",GetLastError(),__VA_ARGS__); goto JumpStatement; }

#define ErrorHandle(FailIfTrue, JumpStatement, ParamName) \
	if (FailIfTrue) { USBERR("Invalid handle/context/memory pointer '%s'.\n",ParamName); SetLastError(ERROR_INVALID_HANDLE); goto JumpStatement; }

#define ErrorMemory(FailIfTrue, JumpStatement) \
	if (FailIfTrue) { USBERR("Not enough memory.\n"); SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto JumpStatement; }

#define ErrorParam(FailIfTrue, JumpStatement, ParamName) \
	if (FailIfTrue) { USBERR("invalid parameter '%s'.\n",ParamName); SetLastError(ERROR_INVALID_PARAMETER); goto JumpStatement; }

#define ErrorStore(FailIfTrue, JumpStatement, Store, ErrorCode, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format"\n",ErrorCode,__VA_ARGS__); Store = ErrorCode; goto JumpStatement; }

// Checks for null and invalid handle value.
#define IsHandleValid(MemoryPtrOrHandle) (((MemoryPtrOrHandle) && (MemoryPtrOrHandle) != INVALID_HANDLE_VALUE)?TRUE:FALSE)

#define IsStaticHandle(Handle, Pool)	\
		((((PVOID)Handle) >= ((PVOID)&Pool[0])) && (((PVOID)Handle) <= ((PVOID)&Pool[sizeof(Pool)/sizeof(Pool[0])-1])))

#define IsLusbkHandle(PublicInterfaceHandle)	\
	((((PVOID)PublicInterfaceHandle) >= ((PVOID)&InternalHandlePool[0])) && (((PVOID)PublicInterfaceHandle) <= ((PVOID)&InternalHandlePool[KUSB_MAX_INTERFACE_HANDLES-1])))

// Validatates the interface handle and cast it to a PKUSB_INTERFACE_HANDLE_INTERNAL.
//
#define PublicToPrivateHandle(WinusbInterfaceHandle) \
	(IsLusbkHandle(WinusbInterfaceHandle)?((PKUSB_INTERFACE_HANDLE_INTERNAL)(WinusbInterfaceHandle)):(NULL))

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

FORCEINLINE PVOID Mem_Max(__in_opt PVOID memory, __in size_t size)
{
	if (IsHandleValid(memory))
	{
		memset(memory, 0xFF, size);
		return memory;
	}
	return NULL;
}

FORCEINLINE PVOID Mem_Alloc(__in size_t size)
{
	PVOID memory = malloc(size);
	if (!memory) LusbwError(ERROR_NOT_ENOUGH_MEMORY);
	return Mem_Zero(memory, size) ? memory : INVALID_HANDLE_VALUE;
}

FORCEINLINE VOID Mem_Free(__deref_inout_opt PVOID* memoryRef)
{
	if (memoryRef)
	{
		if (IsHandleValid(*memoryRef))
			free(*memoryRef);

		*memoryRef = NULL;
	}
}

FORCEINLINE BOOL Str_IsNullOrEmpty(__in_opt LPCSTR string)
{
	if (string)
	{
		if (strlen(string))
		{
			return FALSE;
		}
	}
	return TRUE;
}

FORCEINLINE INT Str_Cmp(__in_opt LPCSTR str1, __in_opt LPCSTR str2)
{
	if (Str_IsNullOrEmpty(str1) ||
	        Str_IsNullOrEmpty(str2))
		return -1;

	return strcmp(str1, str2);
}

FORCEINLINE LPSTR Str_Dupe(__in_opt LPCSTR string)
{
	if (!Str_IsNullOrEmpty(string))
		return _strdup(string);

	return NULL;
}

//
// Interface context locking functions and macros.
#ifndef UNUSED_LOCK_FUNCTIONS_AND_MACROS

#define InitInterfaceLock(SynclockPtr) Lock_Init(SynclockPtr, MAX_THREADS_PER_INTERFACE)

#define AcquireSyncLockWrite(SyncLock)																			\
{																												\
	if ((SyncLock))																								\
	{																											\
		if (!AcquireWriteLock((SyncLock), 0))																	\
		{																										\
			USBWRN("UsbStack: acquire write lock failed.\n");													\
			SetLastError(ERROR_BUSY);																			\
			return FALSE;																						\
		}																										\
	}																											\
}

#define AcquireSyncLockRead(SyncLock)																			\
{																												\
	if (SyncLock)																								\
	{																											\
		if (!AcquireReadLock(SyncLock, MAX_THREADS_PER_INTERFACE))																\
		{																										\
			USBWRN("UsbStack: acquire read lock failed.\n");													\
			SetLastError(ERROR_BUSY);																			\
			return FALSE;																						\
		}																										\
	}																											\
}

#define ReleaseSyncLockWrite(SyncLock) if (SyncLock) ReleaseWriteLock(SyncLock)
#define ReleaseSyncLockRead(SyncLock) if (SyncLock) ReleaseReadLock(SyncLock)

#define DestroyLock(SynclockPtr) Lock_Destroy(SynclockPtr)

#define AcquireReadLock(SynclockPtr, TimeoutInMs) \
	(WaitForSingleObject((SynclockPtr)->Handle,TimeoutInMs)==WAIT_OBJECT_0)

#define ReleaseReadLock(SynclockPtr) \
	ReleaseSemaphore((SynclockPtr)->Handle,1,NULL)

#define ReleaseWriteLock(SynclockPtr) \
	ReleaseSemaphore((SynclockPtr)->Handle,(SynclockPtr)->MaxCount,NULL)

#define AcquireWriteLock(SynclockPtr, TimeoutInMs) Lock_AcquireWrite(SynclockPtr,TimeoutInMs)

FORCEINLINE BOOL Lock_Init(__in PSYNC_LOCK syncLock, __in ULONG MaximumLockCount)
{
	Mem_Zero(syncLock, sizeof(*syncLock));
	syncLock->MaxCount = MaximumLockCount;
	syncLock->Handle = CreateSemaphore(NULL, syncLock->MaxCount, syncLock->MaxCount, NULL);
	if (!IsHandleValid(syncLock->Handle))
		return FALSE;
	return TRUE;
}

FORCEINLINE BOOL Lock_AcquireWrite(__in PSYNC_LOCK syncLock, __in_opt DWORD timeoutMS)
{
	LONG lockCount = -1;
	while (++lockCount < syncLock->MaxCount)
	{
		if (!AcquireReadLock(syncLock, timeoutMS))
		{
			if (lockCount > 0)
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
		memcpy(&tempLock, syncLock, sizeof(tempLock));
		Mem_Zero(syncLock, sizeof(*syncLock));
		CloseHandle(tempLock.Handle);
	}
	return TRUE;
}

#define SPINLOCK_HELD ((long)'KBSU')
FORCEINLINE BOOL SpinLock_Acquire(__in volatile long* lock, __in BOOL wait)
{
	if (wait)
	{
		while (InterlockedExchange(lock, SPINLOCK_HELD) != 0)
			SwitchToThread();

		return TRUE;
	}
	else
	{
		return InterlockedExchange(lock, SPINLOCK_HELD) == 0;
	}
}

FORCEINLINE VOID SpinLock_Release(__in volatile long* lock)
{
	InterlockedExchange(lock, 0);
}

typedef struct _SPIN_LOCK_EX
{
	volatile long Lock;
	long RefCount;
	DWORD ThreadID;
} SPIN_LOCK_EX;
typedef SPIN_LOCK_EX* PSPIN_LOCK_EX;

FORCEINLINE BOOL SpinLock_AcquireEx(__inout PSPIN_LOCK_EX SpinLock, __in BOOL wait)
{
	DWORD threadID;
	if (!SpinLock) return FALSE;

	threadID = GetCurrentThreadId();
	if (SpinLock->ThreadID == threadID)
	{
		SpinLock->RefCount++;
		return TRUE;
	}

	if (wait)
	{
		while (InterlockedExchange(&SpinLock->Lock, SPINLOCK_HELD) != 0)
			SwitchToThread();

		SpinLock->RefCount++;
		SpinLock->ThreadID = threadID;
		return TRUE;
	}
	else
	{
		if (InterlockedExchange(&SpinLock->Lock, SPINLOCK_HELD) == 0)
		{
			SpinLock->RefCount++;
			SpinLock->ThreadID = threadID;
			return TRUE;
		}
	}
	return FALSE;
}

FORCEINLINE BOOL SpinLock_ReleaseEx(__inout PSPIN_LOCK_EX SpinLock)
{
	if (!SpinLock) return FALSE;

	if (!SpinLock->ThreadID)
		return FALSE;

	if (--SpinLock->RefCount < 1)
	{
		SpinLock->ThreadID = 0;
		SpinLock->RefCount = 0;

		InterlockedExchange(&SpinLock->Lock, 0);
	}

	return TRUE;

}

#endif

#endif
