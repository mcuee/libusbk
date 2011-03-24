
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

#define GetSetPipePolicy(InterfaceHandleInternal,pipeID)	\
	(&InterfaceHandleInternal->PipePolicies[((((pipeID) & 0xF)|(((pipeID)>>3) & 0x10)) & 0x1F)])

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Structs
//
#include <PSHPACK1.H>

typedef struct _USER_PIPE_POLICY
{
	volatile ULONG timeout;

} USER_PIPE_POLICY, *PUSER_PIPE_POLICY;

// Internal libusbK interface handle
typedef struct _KUSB_INTERFACE_HANDLE_INTERNAL
{
	KUSB_USER_CONTEXT UserContezt;
	PVOID BackendContext;
	HANDLE DeviceHandle;
	INT InterfaceIndex;
	USER_PIPE_POLICY PipePolicies[32];

	struct
	{
		unsigned int Major;
		unsigned int Minor;
		unsigned int Micro;
		unsigned int Nano;
		unsigned int ModValue;
	} Version;
	struct
	{
		volatile LONG ValidCount;
	} inst;

} KUSB_INTERFACE_HANDLE_INTERNAL, *PKUSB_INTERFACE_HANDLE_INTERNAL;

#include <POPPACK.H>

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

BOOL Ioctl_Async(__in HANDLE dev,
                 __in INT code,
                 __in_opt PVOID in,
                 __in DWORD in_size,
                 __inout_opt PVOID out,
                 __in DWORD out_size,
                 __in LPOVERLAPPED overlapped);

BOOL Ioctl_SyncTranfer(__in PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle,
                       __in INT code,
                       __in libusb_request* request,
                       __inout_opt PVOID out,
                       __in DWORD out_size,
                       __in INT timeout,
                       __out_opt PDWORD ret);

VOID InitInterfaceHandle(__in PKUSB_INTERFACE_HANDLE_INTERNAL InterfaceHandle,
                         __in_opt HANDLE DeviceHandle,
                         __in_opt UCHAR InterfaceIndex,
                         __in_opt libusb_request* DriverVersionRequest,
                         __in_opt PKUSB_INTERFACE_HANDLE_INTERNAL PreviousInterfaceHandle);

VOID CheckLibInitialized();

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Macros
//
#define InvalidateHandle(InterfaceHandleInternalPtr) \
	if (InterfaceHandleInternalPtr) InterlockedDecrement(&InterfaceHandleInternalPtr->inst.ValidCount)

// Sets 'RetErrorCode' and jumps to 'JumpStatement' if 'FailIfTrue' is non-zero.
//
#define FailIf(FailIfTrue, RetErrorCode, JumpStatement) \
	if (FailIfTrue) { ret = RetErrorCode; goto JumpStatement; }

// Checks for null and invalid handle value.
#define IsHandleValid(MemoryPtrOrHandle) (((MemoryPtrOrHandle) && (MemoryPtrOrHandle) != INVALID_HANDLE_VALUE)?TRUE:FALSE)

#define IsLusbkHandle(PublicInterfaceHandle)	\
	((((PVOID)PublicInterfaceHandle) >= ((PVOID)&HandleList[0])) && (((PVOID)PublicInterfaceHandle) <= ((PVOID)&HandleList[KUSB_MAX_INTERFACE_HANDLES-1])))

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

PKUSB_INTERFACE_HANDLE_INTERNAL GetNextAvailableHandle();

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

// The semaphore lock structure use for KUSB_INTERFACE_HANDLE_INTERNAL.
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
