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
#define WM_USER_NOTIFY_HOT_HANDLE		(WM_USER+3)

#define GetSetPipePolicy(BackendContextPtr,pipeID)	\
	(BackendContextPtr->PipePolicies[((((pipeID) & 0xF)|(((pipeID)>>3) & 0x10)) & 0x1F)])

#define CreateDeviceFile(DevicePathStrA) \
	CreateFileA(DevicePathStrA, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)

#define GUID_STRING_LENGTH 38
#define GUID_FORMAT_STRING "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"

///////////////////////////////////////////////////////////////
// Interlocked inc/dec macros
#define IncLock(mVolatileLong) InterlockedIncrement(&(mVolatileLong))
#define DecLock(mVolatileLong) InterlockedDecrement(&(mVolatileLong))
///////////////////////////////////////////////////////////////

// unversal pragma warning disable/enable macros (GCC, clang, msvc)
#define DIAG_STR(s) #s
#define DIAG_JOINSTR(x,y) DIAG_STR(x ## y)
#ifdef _MSC_VER
#define DIAG_DO_PRAGMA(x) __pragma (#x)
#define DIAG_PRAGMA(compiler,x) DIAG_DO_PRAGMA(warning(x))
#else
#define DIAG_DO_PRAGMA(x) _Pragma (#x)
#define DIAG_PRAGMA(compiler,x) DIAG_DO_PRAGMA(compiler diagnostic x)
#endif
#if defined(__clang__)
# define DISABLE_WARNING(gcc_unused,clang_option,msvc_unused) DIAG_PRAGMA(clang,push) DIAG_PRAGMA(clang,ignored DIAG_JOINSTR(-W,clang_option))
# define ENABLE_WARNING(gcc_unused,clang_option,msvc_unused) DIAG_PRAGMA(clang,pop)
#elif defined(_MSC_VER)
# define DISABLE_WARNING(gcc_unused,clang_unused,msvc_errorcode) DIAG_PRAGMA(msvc,push) DIAG_DO_PRAGMA(warning(disable:##msvc_errorcode))
# define ENABLE_WARNING(gcc_unused,clang_unused,msvc_errorcode) DIAG_PRAGMA(msvc,pop)
#elif defined(__GNUC__)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
# define DISABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,push) DIAG_PRAGMA(GCC,ignored DIAG_JOINSTR(-W,gcc_option))
# define ENABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,pop)
#else
# define DISABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,ignored DIAG_JOINSTR(-W,gcc_option))
# define ENABLE_WARNING(gcc_option,clang_option,msvc_unused) DIAG_PRAGMA(GCC,warning DIAG_JOINSTR(-W,gcc_option))
#endif
#endif

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
	volatile ULONG isoasap;

}* PUSER_PIPE_POLICY, USER_PIPE_POLICY;

//! Opaque UsbK handle, see \ref UsbK_Init.
typedef KLIB_HANDLE WINUSB_ISOCH_BUFFER_HANDLE;
typedef KLIB_HANDLE* PWINUSB_ISOCH_BUFFER_HANDLE;

typedef struct _USBD_ISO_PACKET_DESCRIPTOR {
	ULONG       Offset;
	ULONG       Length;
	ULONG		Status;
} USBD_ISO_PACKET_DESCRIPTOR, *PUSBD_ISO_PACKET_DESCRIPTOR;

typedef BOOL WINAPI KDYN_CancelIoEx(HANDLE, KOVL_HANDLE);
typedef BOOL WINAPI KDYN_PathMatchSpec(__in LPCSTR pszFile, __in LPCSTR pszSpec);

typedef UINT WINAPI KDYN_CM_Get_Device_ID(
    _in   DWORD dnDevInst,
    _out  LPSTR Buffer,
    _in   UINT BufferLen,
    _in   UINT ulFlags
);

typedef UINT WINAPI KDYN_CM_Get_Parent(
    _out  PDWORD pdnDevInst,
    _in   DWORD dnDevInst,
    _in   UINT ulFlags
);

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
                __out_opt PUINT ret);

BOOL Ioctl_SyncWithTimeout(__in HANDLE DeviceHandle,
                           __in INT code,
                           __in libusb_request* request,
                           __inout_opt PVOID out,
                           __in DWORD out_size,
                           __in INT timeout,
                           __in UCHAR PipeID,
                           __out_opt PUINT ret);

BOOL Ioctl_Async(__in HANDLE dev,
                 __in INT code,
                 __in_opt PVOID in,
                 __in DWORD in_size,
                 __inout_opt PVOID out,
                 __in DWORD out_size,
                 __in LPOVERLAPPED overlapped);

//////////////////////////////////////////////////////////////////////////////

BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in LONG FunctionID);
BOOL GetProcAddress_LUsb0(__out KPROC* ProcAddress, __in LONG FunctionID);
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in LONG FunctionID);
BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in LONG FunctionID);


// winusb specific ISO functions that we cannot directly support in the public API because of multi-driver compatibility issues 
typedef BOOL KUSB_API WUSB_GetAdjustedFrameNumber(PULONG CurrentFrameNumber, LARGE_INTEGER TimeStamp);
typedef BOOL KUSB_API WUSB_GetCurrentFrameNumber(KUSB_HANDLE InterfaceHandle, PULONG CurrentFrameNumber, LARGE_INTEGER* TimeStamp);
typedef BOOL KUSB_API WUSB_ReadIsochPipe(WINUSB_ISOCH_BUFFER_HANDLE BufferHandle, ULONG Offset, ULONG Length, PULONG FrameNumber, ULONG NumberOfPackets, PUSBD_ISO_PACKET_DESCRIPTOR IsoPacketDescriptors, LPOVERLAPPED Overlapped);
typedef BOOL KUSB_API WUSB_ReadIsochPipeAsap(WINUSB_ISOCH_BUFFER_HANDLE BufferHandle, ULONG Offset, ULONG Length, BOOL ContinueStream, ULONG NumberOfPackets, PUSBD_ISO_PACKET_DESCRIPTOR IsoPacketDescriptors, LPOVERLAPPED Overlapped);
typedef BOOL KUSB_API WUSB_RegisterIsochBuffer(KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, ULONG BufferLength, PWINUSB_ISOCH_BUFFER_HANDLE IsochBufferHandle);
typedef BOOL KUSB_API WUSB_UnregisterIsochBuffer(WINUSB_ISOCH_BUFFER_HANDLE BufferHandle);
typedef BOOL KUSB_API WUSB_WriteIsochPipe(WINUSB_ISOCH_BUFFER_HANDLE BufferHandle, ULONG Offset, ULONG Length, PULONG FrameNumber, LPOVERLAPPED Overlapped);
typedef BOOL KUSB_API WUSB_WriteIsochPipeAsap(WINUSB_ISOCH_BUFFER_HANDLE BufferHandle, ULONG Offset, ULONG Length, BOOL ContinueStream, LPOVERLAPPED Overlapped);

typedef struct _WINUSB_API
{
	struct
	{
		volatile long Lock;
		BOOL IsInitialized;
		volatile HMODULE DLL;
		BOOL IsIsoSupported;
	} Init;

	BOOL(KUSB_API* Initialize) (HANDLE DeviceHandle, KUSB_HANDLE* InterfaceHandle);
	BOOL(KUSB_API* Free) (KUSB_HANDLE InterfaceHandle);
	BOOL(KUSB_API* GetAssociatedInterface) (KUSB_HANDLE InterfaceHandle, UCHAR AssociatedInterfaceIndex, KUSB_HANDLE* AssociatedInterfaceHandle);
	BOOL(KUSB_API* GetDescriptor) (KUSB_HANDLE InterfaceHandle, UCHAR DescriptorType, UCHAR Index, USHORT LanguageID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred);
	BOOL(KUSB_API* QueryDeviceInformation) (KUSB_HANDLE InterfaceHandle, UINT InformationType, PUINT BufferLength, PVOID Buffer);
	BOOL(KUSB_API* SetCurrentAlternateSetting) (KUSB_HANDLE InterfaceHandle, UCHAR AltSettingNumber);
	BOOL(KUSB_API* GetCurrentAlternateSetting) (KUSB_HANDLE InterfaceHandle, PUCHAR AltSettingNumber);
	BOOL(KUSB_API* SetPipePolicy) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID, UINT PolicyType, UINT ValueLength, PVOID Value);
	BOOL(KUSB_API* GetPipePolicy) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID, UINT PolicyType, PUINT ValueLength, PVOID Value);
	BOOL(KUSB_API* ReadPipe) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL(KUSB_API* WritePipe) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL(KUSB_API* ControlTransfer) (KUSB_HANDLE InterfaceHandle, WINUSB_SETUP_PACKET SetupPacket, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL(KUSB_API* ResetPipe) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL(KUSB_API* AbortPipe) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL(KUSB_API* FlushPipe) (KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL(KUSB_API* SetPowerPolicy) (KUSB_HANDLE InterfaceHandle, UINT PolicyType, UINT ValueLength, PVOID Value);
	BOOL(KUSB_API* GetPowerPolicy) (KUSB_HANDLE InterfaceHandle, UINT PolicyType, PUINT ValueLength, PVOID Value);
	WUSB_GetAdjustedFrameNumber* GetAdjustedFrameNumber;
	WUSB_GetCurrentFrameNumber* GetCurrentFrameNumber;
	WUSB_ReadIsochPipe* ReadIsochPipe;
	WUSB_ReadIsochPipeAsap* ReadIsochPipeAsap;
	WUSB_RegisterIsochBuffer* RegisterIsochBuffer;
	WUSB_UnregisterIsochBuffer* UnregisterIsochBuffer;
	WUSB_WriteIsochPipe* WriteIsochPipe;
	WUSB_WriteIsochPipeAsap* WriteIsochPipeAsap;
	KUSB_GetOverlappedResult* GetOverlappedResult;
	KUSB_QueryPipeEx* QueryPipeEx;
}*PWINUSB_API, WINUSB_API;

extern WINUSB_API WinUsb;

#endif
