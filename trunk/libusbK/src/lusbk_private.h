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

#define WM_USER_INIT_HOT_HANDLE			(WM_USER+1)
#define WM_USER_FREE_HOT_HANDLE			(WM_USER+2)

#define GetSetPipePolicy(BackendContextPtr,pipeID)	\
	(BackendContextPtr->PipePolicies[((((pipeID) & 0xF)|(((pipeID)>>3) & 0x10)) & 0x1F)])

#define CreateDeviceFile(DevicePathStrA) \
	CreateFileA(DevicePathStrA, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)

///////////////////////////////////////////////////////////////
// Interlocked inc/dec macros
#define IncLock(mVolatileLong) InterlockedIncrement(&(mVolatileLong))
#define DecLock(mVolatileLong) InterlockedDecrement(&(mVolatileLong))
///////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////
// Spin-lock macros

#define SPINLOCK_HELD ((long)'KBSU')

#define mSpin_Try_Acquire(mVolatileLongRef)			\
	(InterlockedExchange(mVolatileLongRef, SPINLOCK_HELD)==0)

#define mSpin_Acquire_Alertable(mVolatileLongRef)	\
	while (!mSpin_Try_Acquire(mVolatileLongRef)) SleepEx(0,TRUE)

#define mSpin_Acquire(mVolatileLongRef)				\
	while (!mSpin_Try_Acquire(mVolatileLongRef)) if (!SwitchToThread()) Sleep(0)

#define mSpin_Release(mVolatileLongRef)				\
	InterlockedExchange(mVolatileLongRef, 0)
///////////////////////////////////////////////////////////////

#define MsgErrorNoSetAction(FailIfTrue, MsgAction, format, ...) \
	if (FailIfTrue) { USBMSGN("ErrorCode=%08Xh "format,GetLastError(),__VA_ARGS__); {MsgAction;} }

// Sets 'RetErrorCode' and jumps to 'JumpStatement' if 'FailIfTrue' is non-zero.
//
#define FailIf(FailIfTrue, RetErrorCode, JumpStatement) \
	if (FailIfTrue) { ret = RetErrorCode; goto JumpStatement; }

#define ErrorSetAction(FailIfTrue, ErrorCode, ErrorAction, format, ...) \
	if (FailIfTrue) { USBERRN("ErrorCode=%08Xh "format,ErrorCode,__VA_ARGS__); SetLastError(ErrorCode); ErrorAction; }

#define ErrorSet(FailIfTrue, JumpStatement, ErrorCode, format, ...)	ErrorSetAction(FailIfTrue,ErrorCode,goto JumpStatement,format,__VA_ARGS__)

#define ErrorNoSetAction(FailIfTrue, ErrorAction, format, ...) \
	if (FailIfTrue) { USBERRN("ErrorCode=%08Xh "format,GetLastError(),__VA_ARGS__); {ErrorAction;} }
#define ErrorNoSet(FailIfTrue, JumpStatement, format, ...) ErrorNoSetAction(FailIfTrue,goto JumpStatement,format,__VA_ARGS__)


#define ErrorHandleAction(FailIfTrue, ParamName, ErrorAction) \
	if (FailIfTrue) { USBERRN("Invalid handle/context/memory pointer '%s'.",ParamName); SetLastError(ERROR_INVALID_HANDLE); ErrorAction; }

#define ErrorHandle(FailIfTrue, JumpStatement, ParamName) ErrorHandleAction(FailIfTrue,ParamName,goto JumpStatement)

#define ErrorMemoryAction(FailIfTrue, ErrorAction) \
	if (FailIfTrue) { USBERRN("Not enough memory."); SetLastError(ERROR_NOT_ENOUGH_MEMORY); ErrorAction; }
#define ErrorMemory(FailIfTrue, JumpStatement) ErrorMemoryAction(FailIfTrue,goto JumpStatement)

#define ErrorParamAction(FailIfTrue, ParamName, ErrorAction) \
	if (FailIfTrue) { USBERRN("invalid parameter '%s'.",ParamName); SetLastError(ERROR_INVALID_PARAMETER); ErrorAction; }

#define ErrorParam(FailIfTrue, JumpStatement, ParamName) ErrorParamAction(FailIfTrue,ParamName,goto JumpStatement)

#define ErrorStore(FailIfTrue, JumpStatement, Store, ErrorCode, format, ...) \
	if (FailIfTrue) { USBERRN("ErrorCode=%08Xh "format ,ErrorCode,__VA_ARGS__); Store = ErrorCode; goto JumpStatement; }

// Checks for null and invalid handle value.
#define IsHandleValid(MemoryPtrOrHandle) (((MemoryPtrOrHandle) && (MemoryPtrOrHandle) != INVALID_HANDLE_VALUE)?TRUE:FALSE)

//////////////////////////////////////////////////////////////////////////////

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
