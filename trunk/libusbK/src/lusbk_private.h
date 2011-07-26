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
#include <winioctl.h>
#include <crtdbg.h>

#include "lusbk_version.h"
#include "libusbk.h"
#include "drv_api.h"
#include "lusbk_debug_view_output.h"

//////////////////////////////////////////////////////////////////////////////
// Defines/Macros
//

// The maximum number of simultaneous threads allowed per interface handle
#define MAX_THREADS_PER_INTERFACE 32

#define WM_USER_INIT_HOT_HANDLE			(WM_USER+1)
#define WM_USER_FREE_HOT_HANDLE			(WM_USER+2)

#define GetSetPipePolicy(BackendContextPtr,pipeID)	\
	(BackendContextPtr->PipePolicies[((((pipeID) & 0xF)|(((pipeID)>>3) & 0x10)) & 0x1F)])

#define CreateDeviceFile(DevicePathStrA) \
	CreateFileA(DevicePathStrA, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)

#define IncLock(LockField) InterlockedIncrement(&LockField)
#define DecLock(LockField) InterlockedDecrement(&LockField)

// Sets 'RetErrorCode' and jumps to 'JumpStatement' if 'FailIfTrue' is non-zero.
//
#define FailIf(FailIfTrue, RetErrorCode, JumpStatement) \
	if (FailIfTrue) { ret = RetErrorCode; goto JumpStatement; }

#define ErrorSetAction(FailIfTrue, ErrorCode, ErrorAction, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format USB_LN,ErrorCode,__VA_ARGS__); SetLastError(ErrorCode); ErrorAction; }

#define ErrorSet(FailIfTrue, JumpStatement, ErrorCode, format, ...)	ErrorSetAction(FailIfTrue,ErrorCode,goto JumpStatement,format,__VA_ARGS__)

#define ErrorNoSetAction(FailIfTrue, ErrorAction, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format USB_LN,GetLastError(),__VA_ARGS__); {ErrorAction;} }
#define ErrorNoSet(FailIfTrue, JumpStatement, format, ...) ErrorNoSetAction(FailIfTrue,goto JumpStatement,format,__VA_ARGS__)


#define ErrorHandleAction(FailIfTrue, ParamName, ErrorAction) \
	if (FailIfTrue) { USBERR("Invalid handle/context/memory pointer '%s'."USB_LN,ParamName); SetLastError(ERROR_INVALID_HANDLE); ErrorAction; }

#define ErrorHandle(FailIfTrue, JumpStatement, ParamName) ErrorHandleAction(FailIfTrue,ParamName,goto JumpStatement)

#define ErrorMemoryAction(FailIfTrue, ErrorAction) \
	if (FailIfTrue) { USBERR("Not enough memory."USB_LN); SetLastError(ERROR_NOT_ENOUGH_MEMORY); ErrorAction; }
#define ErrorMemory(FailIfTrue, JumpStatement) ErrorMemoryAction(FailIfTrue,goto JumpStatement)

#define ErrorParamAction(FailIfTrue, ParamName, ErrorAction) \
	if (FailIfTrue) { USBERR("invalid parameter '%s'."USB_LN,ParamName); SetLastError(ERROR_INVALID_PARAMETER); ErrorAction; }

#define ErrorParam(FailIfTrue, JumpStatement, ParamName) ErrorParamAction(FailIfTrue,ParamName,goto JumpStatement)

#define ErrorStore(FailIfTrue, JumpStatement, Store, ErrorCode, format, ...) \
	if (FailIfTrue) { USBERR("ErrorCode=%08Xh "format USB_LN,ErrorCode,__VA_ARGS__); Store = ErrorCode; goto JumpStatement; }

// Checks for null and invalid handle value.
#define IsHandleValid(MemoryPtrOrHandle) (((MemoryPtrOrHandle) && (MemoryPtrOrHandle) != INVALID_HANDLE_VALUE)?TRUE:FALSE)

#define IsStaticHandle(Handle, Pool)	\
		((((PVOID)Handle) >= ((PVOID)&Pool[0])) && (((PVOID)Handle) <= ((PVOID)&Pool[sizeof(Pool)/sizeof(Pool[0])-1])))

#define IsLusbkHandle(PublicInterfaceHandle)	ALLK_VALID_HANDLE(PublicInterfaceHandle,UsbK)

// Validatates the interface handle and cast it to a PKUSB_HANDLE_INTERNAL.
//
#define PublicToPrivateHandle(WinusbInterfaceHandle) \
	(IsLusbkHandle(WinusbInterfaceHandle)?((PKUSB_HANDLE_INTERNAL)(WinusbInterfaceHandle)):(NULL))

// Macros for allocating a libusb_request and setting/getting the data that comes after it.
//
#define AllocRequest(RequiredDataLength) Mem_Alloc(sizeof(libusb_request) + RequiredDataLength)

#define GetRequestDataPtr(LibusbRequestPtr) (&(((PUCHAR)LibusbRequestPtr)[sizeof(libusb_request)]))

#define GetRequestData(LibusbRequestPtr,OutputBuffer,OutputBufferLength)	\
	if ( (LibusbRequestPtr) && (OutputBuffer) && (OutputBufferLength) ) memcpy(OutputBuffer, GetRequestDataPtr(LibusbRequestPtr), (size_t)OutputBufferLength)

#define SetRequestData(LibusbRequestPtr,InputBuffer,InputBufferLength)	\
	if ( (LibusbRequestPtr) && (InputBuffer) && (InputBufferLength) ) memcpy(GetRequestDataPtr(LibusbRequestPtr), InputBuffer, (size_t)InputBufferLength)

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Spin-locks structs/defines/functions
//
#define SPINLOCK_HELD ((long)'KBSU')
typedef struct _SPIN_LOCK_EX
{
	volatile long Lock;
	volatile long ThreadWaiting;
	long RefCount;
	DWORD ThreadID;
} SPIN_LOCK_EX;
typedef SPIN_LOCK_EX* PSPIN_LOCK_EX;

typedef struct _USER_PIPE_POLICY
{
	volatile ULONG timeout;

}* PUSER_PIPE_POLICY, USER_PIPE_POLICY;

typedef BOOL WINAPI KDYN_CancelIoEx(HANDLE, KOVL_HANDLE);
typedef BOOL WINAPI KDYN_PathMatchSpec(__in LPCSTR pszFile, __in LPCSTR pszSpec);

typedef BOOL KUSB_API KOVL_OVERLAPPED_CANCEL_CB (__in KOVL_HANDLE Overlapped);

FORCEINLINE BOOL LusbwError(__in LONG errorCode)
{
	if (errorCode != ERROR_SUCCESS)
	{
		SetLastError(errorCode);
		return FALSE;
	}
	return TRUE;
}

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

FORCEINLINE BOOL SpinLock_Acquire(__in volatile long* lock, __in BOOL wait)
{
	if (wait)
	{
		while (InterlockedExchange(lock, SPINLOCK_HELD) != 0)
			if (!SwitchToThread()) Sleep(0);

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

FORCEINLINE BOOL SpinLock_AcquireEx(__inout PSPIN_LOCK_EX SpinLock, __in BOOL wait)
{
	DWORD threadID;
	long lockVal;

	threadID = GetCurrentThreadId();

	if (SpinLock->ThreadID == threadID)
	{
		// nested spin lock acquire
		SpinLock->RefCount++;
		return TRUE;
	}

	lockVal  = InterlockedExchange(&SpinLock->Lock, SPINLOCK_HELD);
	if (lockVal == 0) goto New_SpinLock_Acquire;

	//
	// ..some other thread is holding the spin lock..
	//

	if (!wait) return FALSE;

	InterlockedIncrement(&SpinLock->ThreadWaiting);
	while (InterlockedExchange(&SpinLock->Lock, SPINLOCK_HELD) != 0)
	{
		if (!SwitchToThread()) Sleep(0);
	}

New_SpinLock_Acquire:
	SpinLock->RefCount++;
	SpinLock->ThreadID = threadID;

	// TODO: get/set thread priority

	return TRUE;

}

FORCEINLINE BOOL SpinLock_ReleaseEx(__inout PSPIN_LOCK_EX SpinLock)
{
	if (!SpinLock->ThreadID) return FALSE;

	if (--SpinLock->RefCount < 1)
	{
		SpinLock->ThreadID = 0;
		SpinLock->RefCount = 0;
		if (SpinLock->ThreadWaiting > 0)
		{
			InterlockedDecrement(&SpinLock->ThreadWaiting);
			InterlockedExchange(&SpinLock->Lock, 0);
			if (!SwitchToThread()) Sleep(0);
		}
		else
		{
			InterlockedExchange(&SpinLock->Lock, 0);
		}
	}

	// TODO: restore thread priority

	return TRUE;

}

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

//////////////////////////////////////////////////////////////////////////////

BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_LUsb0(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in ULONG FunctionID);

#endif
