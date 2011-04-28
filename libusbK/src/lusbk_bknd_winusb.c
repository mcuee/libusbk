
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

#include "lusbk_bknd.h"
#include "lusbk_stack_collection.h"

#define WINUSB_BACKEND_SUPPORT

#ifdef WINUSB_BACKEND_SUPPORT

#pragma warning(disable: 4127)

extern ULONG DebugLevel;
extern KUSB_INTERFACE_HANDLE_INTERNAL InternalHandlePool[KUSB_MAX_INTERFACE_HANDLES];

typedef struct _WINUSB_BKND_CONTEXT
{
	KUSB_INTERFACE_STACK UsbStack;
} WINUSB_BKND_CONTEXT, *PWINUSB_BKND_CONTEXT;

typedef struct _WINUSB_API
{
	BOOL (KUSB_API* Initialize)				(__in HANDLE DeviceHandle, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Free)					(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	BOOL (KUSB_API* QueryDeviceInformation)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR SettingNumber);
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR SettingNumber);
	BOOL (KUSB_API* SetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* ReadPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
} WINUSB_API, *PWINUSB_API;

#define W_CTX(BackendContextPtr, InterfaceHandle) \
	GET_BACKEND_CONTEXT((BackendContextPtr), ((PKUSB_INTERFACE_HANDLE_INTERNAL)(InterfaceHandle)), WINUSB_BKND_CONTEXT)

#define W_CTXJ(BackendContextPtr, InterfaceHandle, ErrorJump) \
	GET_BACKEND_CONTEXT_EJUMP((BackendContextPtr), ((PKUSB_INTERFACE_HANDLE_INTERNAL)(InterfaceHandle)), WINUSB_BKND_CONTEXT, ErrorJump)

#define WUSBFN_CTX_PREFIX()							\
	PKUSB_INTERFACE_HANDLE_INTERNAL handle=NULL;		\
	BOOL success=FALSE;									\
	DWORD errorCode=ERROR_SUCCESS;						\
	PWINUSB_BKND_CONTEXT backendContext=NULL;			\
														\
	UNREFERENCED_PARAMETER(success);					\
	UNREFERENCED_PARAMETER(errorCode);					\
														\
	GET_INTERNAL_HANDLE(handle);						\
	W_CTX(backendContext, handle)

#define WUSBFN_CTX_DEVICE_PREFIX()						\
	KUSB_DEVICE_EL deviceEL;							\
	WUSBFN_CTX_PREFIX()

#define WUSBFN_CTX_INTERFACE_PREFIX()					\
	KUSB_INTERFACE_EL interfaceEL;						\
	WUSBFN_CTX_PREFIX()

#define WUSBFN_CTX_PIPE_PREFIX()						\
	WUSBFN_CTX_PREFIX()

#define WUSBFN_CTX_ALL_PREFIX()							\
	KUSB_DEVICE_EL deviceEL;							\
	KUSB_INTERFACE_EL interfaceEL;						\
	WUSBFN_CTX_PREFIX()

#define GetStackInterfaceCache() {success=UsbStack_GetCache(&backendContext->UsbStack, NULL, &(interfaceEL)); }

#define InterfaceHandleByPipeID(PipeID)	 GetPipeInterfaceHandle(backendContext->UsbStack,PipeID)

#define ErrorStackInterfaceCache()	ErrorSet(!success, Error, ERROR_DEVICE_NOT_AVAILABLE, "no interfaces found.")

static volatile long WUsbInitLibLock = 0;
static volatile BOOL IsWUsbInitialized = FALSE;
static volatile HMODULE WinUsb_Dll_Module = NULL;
static WINUSB_API WinUsb = {0};

static VOID WUsb_Init_Library()
{

	CheckLibInitialized();

Retry:
	if (IsWUsbInitialized)
		return;

	if (InterlockedIncrement(&WUsbInitLibLock) > 1)
	{
		InterlockedDecrement(&WUsbInitLibLock);
		Sleep(0);
		goto Retry;
	}
	WinUsb_Dll_Module = LoadLibraryA("winusb.dll");
	if (IsHandleValid(WinUsb_Dll_Module))
	{
		WinUsb.AbortPipe = (KUSB_AbortPipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_AbortPipe");
		WinUsb.Initialize = (KUSB_Initialize*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_Initialize");
		WinUsb.Free = (KUSB_Free*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_Free");
		WinUsb.GetAssociatedInterface = (KUSB_GetAssociatedInterface*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_GetAssociatedInterface");
		WinUsb.GetDescriptor = (KUSB_GetDescriptor*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_GetDescriptor");
		WinUsb.QueryDeviceInformation = (KUSB_QueryDeviceInformation*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_QueryDeviceInformation");
		WinUsb.SetCurrentAlternateSetting = (KUSB_SetCurrentAlternateSetting*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_SetCurrentAlternateSetting");
		WinUsb.GetCurrentAlternateSetting = (KUSB_GetCurrentAlternateSetting*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_GetCurrentAlternateSetting");
		WinUsb.SetPipePolicy = (KUSB_SetPipePolicy*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_SetPipePolicy");
		WinUsb.GetPipePolicy = (KUSB_GetPipePolicy*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_GetPipePolicy");
		WinUsb.ReadPipe = (KUSB_ReadPipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_ReadPipe");
		WinUsb.WritePipe = (KUSB_WritePipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_WritePipe");
		WinUsb.ControlTransfer = (KUSB_ControlTransfer*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_ControlTransfer");
		WinUsb.ResetPipe = (KUSB_ResetPipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_ResetPipe");
		WinUsb.AbortPipe = (KUSB_AbortPipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_AbortPipe");
		WinUsb.FlushPipe = (KUSB_FlushPipe*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_FlushPipe");
		WinUsb.SetPowerPolicy = (KUSB_SetPowerPolicy*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_SetPowerPolicy");
		WinUsb.GetPowerPolicy = (KUSB_GetPowerPolicy*)GetProcAddress(WinUsb_Dll_Module, "WinUsb_GetPowerPolicy");
	}
	else
	{
		InterlockedDecrement(&WUsbInitLibLock);
	}

	IsWUsbInitialized = TRUE;
}

///////////////////////////////////////////////////////////////////////
// private winusb backend functions
///////////////////////////////////////////////////////////////////////
static LONG w_OpenInterfaceCB(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in HANDLE PreviousHandle,
    __in INT InterfaceIndex,
    __out PHANDLE AssociatedHandleRef,
    __in PVOID Context)
{
	BOOL success;

	UNREFERENCED_PARAMETER(UsbStack);
	UNREFERENCED_PARAMETER(Context);

	if (InterfaceIndex == 0)
	{
		success = WinUsb.Initialize(PreviousHandle, AssociatedHandleRef);
		ErrorNoSet(!success, Error, "->WinUsb.Initialize");
	}
	else
	{
		success = WinUsb.GetAssociatedInterface(PreviousHandle, (UCHAR)InterfaceIndex - 1, AssociatedHandleRef);
		ErrorNoSet(!success, Error, "->WinUsb.GetAssociatedInterface: AssociatedIndex=%u", InterfaceIndex - 1);
	}

	return ERROR_SUCCESS;
Error:
	return GetLastError();
}

static LONG w_GetCurrentConfigDescriptorCB(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt LPCSTR DevicePath,
    __in_opt HANDLE DeviceHandle,
    __in_opt HANDLE InterfaceHandle,
    __out PUSB_CONFIGURATION_DESCRIPTOR* ConfigDescriptorRef,
    __in_opt PVOID Context)
{
	DWORD transferred = 0;
	USB_CONFIGURATION_DESCRIPTOR configCheck;
	BOOL success;

	UNREFERENCED_PARAMETER(UsbStack);
	UNREFERENCED_PARAMETER(DevicePath);
	UNREFERENCED_PARAMETER(DeviceHandle);
	UNREFERENCED_PARAMETER(Context);

	Mem_Zero(&configCheck, sizeof(configCheck));

	success = WinUsb.GetDescriptor(
	              InterfaceHandle,
	              USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0,
	              (PUCHAR)&configCheck, sizeof(configCheck), &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor1");

	*ConfigDescriptorRef = Mem_Alloc(configCheck.wTotalLength);
	ErrorMemory(!*ConfigDescriptorRef, Error);

	success = WinUsb.GetDescriptor(InterfaceHandle,
	                               USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0,
	                               (PUCHAR) * ConfigDescriptorRef, configCheck.wTotalLength, &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor2");

Error:
	return success ? ERROR_SUCCESS : GetLastError();
}

static BOOL w_DestroyContext(__inout PKUSB_INTERFACE_HANDLE_INTERNAL InternalHandle)
{
	PWINUSB_BKND_CONTEXT backendContext = NULL;

	// handle uninitialized.
	if (!IsHandleValid(InternalHandle))
		return LusbwError(ERROR_INVALID_HANDLE);

	if (InternalHandle->Instance.UsageCount < 1)
		return LusbwError(ERROR_INVALID_HANDLE);

	backendContext = (PWINUSB_BKND_CONTEXT)InternalHandle->BackendContext;
	if (!IsHandleValid(backendContext))
		goto Done;

	// free the device/interface stack
	UsbStack_Free(&backendContext->UsbStack, backendContext);

Done:
	// destroy the main context semaphore lock.
	DestroyLock(&InternalHandle->Instance.Lock);

	// Release the handle back to the pool.
	DecUsageCount(InternalHandle);

	Mem_Free(&backendContext);

	return TRUE;
}

static LONG w_ClaimReleaseCB (__in PKUSB_INTERFACE_STACK UsbStack,
                              __in PKUSB_INTERFACE_EL InterfaceElement,
                              __in BOOL IsClaim,
                              __in INT InterfaceIndexOrNumber,
                              __in BOOL IsIndex,
                              __in PVOID Context)
{
	LONG errorCode = ERROR_SUCCESS;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(InterfaceIndexOrNumber);

	if (!InterfaceElement)
		return ERROR_NO_MORE_ITEMS;

	// The most recently claimed interface is always at the list head.
	List_Remove(UsbStack->InterfaceList, InterfaceElement);
	if (IsClaim)
	{
		List_AddHead(UsbStack->InterfaceList, InterfaceElement);
	}
	else
	{
		List_AddTail(UsbStack->InterfaceList, InterfaceElement);
	}

	// All device handles and interface handles are cached.
	UsbStack_BuildCache(UsbStack);

	return errorCode;
}

static LONG w_CloseInterface(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in PVOID Context)
{
	BOOL success;
	UNREFERENCED_PARAMETER(UsbStack);
	UNREFERENCED_PARAMETER(Context);

	success = WinUsb.Free(InterfaceElement->ParentDevice->Device->SharedInterfaces[InterfaceElement->Number].InterfaceHandle);
	ErrorNoSet(!success, Error, "->WinUsb.Free");

	return TRUE;
Error:
	return FALSE;
}
static BOOL w_CreateContext(__out PKUSB_INTERFACE_HANDLE_INTERNAL* InternalHandleRef)
{
	BOOL success;
	PWINUSB_BKND_CONTEXT backendContext = NULL;
	PKUSB_INTERFACE_HANDLE_INTERNAL internalHandle = NULL;

	ErrorHandle(!InternalHandleRef, Error, "InternalHandleRef");

	internalHandle = GetInternalPoolHandle();
	ErrorHandle(!internalHandle, Error, "internalHandle");

	memset(internalHandle, 0, sizeof(*internalHandle) - sizeof(internalHandle->Instance));

	backendContext = Mem_Alloc(sizeof(WINUSB_BKND_CONTEXT));
	ErrorMemory(!IsHandleValid(backendContext), Error);
	internalHandle->BackendContext = backendContext;

	success = UsbStack_Init(
	              &backendContext->UsbStack,
	              w_GetCurrentConfigDescriptorCB,
	              w_ClaimReleaseCB,
	              w_OpenInterfaceCB,
	              w_CloseInterface,
	              NULL);

	ErrorNoSet(!success, Error, "->UsbStack_Init");

	success = InitInterfaceLock(&internalHandle->Instance.Lock);
	ErrorNoSet(!success, Error, "->InitInterfaceLock");

	*InternalHandleRef = internalHandle;
	return TRUE;

Error:
	w_DestroyContext(internalHandle);
	*InternalHandleRef = NULL;
	return FALSE;
}

BOOL w_ClaimOrReleaseInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
                               __in INT InterfaceNumberOrIndex,
                               __in UCHAR IsClaim,
                               __in UCHAR IsIndex)
{
	LONG action;
	WUSBFN_CTX_PREFIX();

	if (InterfaceNumberOrIndex < 0)
		return LusbwError(ERROR_NO_MORE_ITEMS);

	AcquireSyncLockWrite(&handle->Instance.Lock);

	GET_BACKEND_CONTEXT(backendContext, handle, WINUSB_BKND_CONTEXT);

	action = UsbStack_ClaimOrRelease(&backendContext->UsbStack,
	                                 IsClaim,
	                                 InterfaceNumberOrIndex,
	                                 IsIndex,
	                                 handle);

	ReleaseSyncLockWrite(&handle->Instance.Lock);

	if (action == ERROR_SUCCESS)
		return TRUE;

	return LusbwError(action);
}

KUSB_EXP BOOL KUSB_API WUsb_Initialize(
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	BOOL success;
	PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	PWINUSB_BKND_CONTEXT backendContext = NULL;

	CheckLibInitialized();

	ErrorHandle(!IsHandleValid(DeviceHandle), Error, "DeviceHandle");
	ErrorHandle(!IsHandleValid(InterfaceHandle), Error, "InterfaceHandle");

	success = w_CreateContext(&interfaceHandle);
	ErrorNoSet(!success, Error, "->w_CreateContext");

	W_CTX(backendContext, interfaceHandle);
	ErrorHandle(!backendContext, Error, "backendContext");

	success = UsbStack_AddDevice(
	              &backendContext->UsbStack,
	              NULL,
	              DeviceHandle,
	              backendContext);

	ErrorNoSet(!success, Error, "->UsbStack_AddDevice");

	*InterfaceHandle = interfaceHandle;
	return TRUE;

Error:
	w_DestroyContext(interfaceHandle);
	*InterfaceHandle = NULL;
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_SetConfiguration(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	USB_STACK_HANDLER_RESULT result;
	WUSBFN_CTX_PREFIX();

	// currently this is only supported by libusb0; so the same is true for K.
	AcquireSyncLockRead(&handle->Instance.Lock);
	result = UsbStackHandler_SetConfiguration(&backendContext->UsbStack, ConfigurationNumber);
	ReleaseSyncLockRead(&handle->Instance.Lock);

	return (BOOL)(result & 1);

}

KUSB_EXP BOOL KUSB_API WUsb_GetConfiguration(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	USB_STACK_HANDLER_RESULT handerResult;
	WUSBFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_GetConfiguration(&backendContext->UsbStack, ConfigurationNumber);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return (BOOL)handerResult & 1;
}

KUSB_EXP BOOL KUSB_API WUsb_Free (__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	PKUSB_INTERFACE_HANDLE_INTERNAL handle;

	GET_INTERNAL_HANDLE(handle);

	AcquireSyncLockWrite(&handle->Instance.Lock);

	w_DestroyContext(handle);

	// Free always returns true;  The Close method does not. This is the primary difference.
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetAssociatedInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	PKUSB_INTERFACE_HANDLE_INTERNAL assocHandle = NULL;
	PWINUSB_BKND_CONTEXT assocContext = NULL;
	PKUSB_INTERFACE_EL assocEL = NULL;
	INT findVirtualIndex;

	WUSBFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	ErrorHandle(!backendContext->UsbStack.InterfaceList, Error, "UsbStack.InterfaceList");

	// this is where we are right now, we must find [this] + 1 + AssociatedInterfaceIndex
	findVirtualIndex = backendContext->UsbStack.InterfaceList->VirtualIndex;
	findVirtualIndex = (findVirtualIndex + 1) + (INT)AssociatedInterfaceIndex;

	List_SearchField(backendContext->UsbStack.InterfaceList, assocEL, VirtualIndex, findVirtualIndex);
	if (!assocEL)
	{
		LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	success = w_CreateContext(&assocHandle);
	ErrorNoSet(!success, Error, "->w_CreateContext");

	W_CTX(assocContext, assocHandle);
	ErrorHandle(!assocContext, Error, "assocContext");

	success = UsbStack_Init(
	              &assocContext->UsbStack,
	              w_GetCurrentConfigDescriptorCB,
	              w_ClaimReleaseCB,
	              w_OpenInterfaceCB,
	              w_CloseInterface,
	              NULL);

	ErrorNoSet(!success, Error, "->UsbStack_Init");

	UsbStack_Clone(&backendContext->UsbStack, &assocContext->UsbStack);
	ErrorNoSet(!success, Error, "->UsbStack_Clone");

	// move this interface to top if stack for the cloned handle.
	List_SearchField(assocContext->UsbStack.InterfaceList, assocEL, VirtualIndex, findVirtualIndex);
	List_Remove(assocContext->UsbStack.InterfaceList, assocEL);
	List_AddHead(assocContext->UsbStack.InterfaceList, assocEL);

	// clone the backend

	// clone the users context
	memcpy(&assocHandle->UserContext, &handle->UserContext, sizeof(assocHandle->UserContext));

	// rebuild the pipe caches
	success = UsbStack_BuildCache(&assocContext->UsbStack);
	ErrorNoSet(!success, Error, "->UsbStack_BuildCache");

	*AssociatedInterfaceHandle = assocHandle;

	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return TRUE;

Error:
	w_DestroyContext(assocHandle);

	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetDescriptor (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	USB_STACK_HANDLER_RESULT handerResult;
	WUSBFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_GetDescriptor(
	                   &backendContext->UsbStack,
	                   DescriptorType,
	                   Index,
	                   LanguageID,
	                   Buffer,
	                   BufferLength,
	                   LengthTransferred);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	success = WinUsb.GetDescriptor(
	              InterfaceHandleByPipeID(0),
	              DescriptorType,
	              Index,
	              LanguageID,
	              Buffer,
	              BufferLength,
	              LengthTransferred);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_QueryInterfaceSettings (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	USB_STACK_HANDLER_RESULT handerResult;

	WUSBFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_QueryInterfaceSettings(
	                   &backendContext->UsbStack,
	                   AlternateSettingNumber,
	                   UsbAltInterfaceDescriptor);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	success = LusbwError(ERROR_NOT_SUPPORTED);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_QueryDeviceInformation (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{

	WUSBFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.QueryDeviceInformation(
	              InterfaceHandleByPipeID(0),
	              InformationType,
	              BufferLength,
	              Buffer);

	ErrorNoSet(!success, Error, "->WinUsb.QueryDeviceInformation");

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_SetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR SettingNumber)
{


	WUSBFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	success = WinUsb.SetCurrentAlternateSetting(InterfaceHandleByPipeID(0), SettingNumber);
	if (success)
	{

		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              interfaceEL.Number, FALSE,
		              SettingNumber, FALSE);
	}


Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;

}

KUSB_EXP BOOL KUSB_API WUsb_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	WUSBFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);


	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	success = WinUsb.GetCurrentAlternateSetting(InterfaceHandleByPipeID(0), SettingNumber);

	if (success)
	{
		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              interfaceEL.Number, FALSE,
		              *SettingNumber, FALSE);

	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_QueryPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	USB_STACK_HANDLER_RESULT handerResult;

	WUSBFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_QueryPipe(
	                   &backendContext->UsbStack,
	                   AlternateSettingNumber,
	                   PipeIndex,
	                   PipeInformation);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	success = LusbwError(ERROR_NOT_SUPPORTED);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_SetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.SetPipePolicy(InterfaceHandleByPipeID(PipeID), PipeID, PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "->WinUsb.SetPipePolicy");

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.GetPipePolicy(InterfaceHandleByPipeID(PipeID), PipeID, PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "->WinUsb.GetPipePolicy");

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_ReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.ReadPipe(InterfaceHandleByPipeID(PipeID), PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_WritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.WritePipe(InterfaceHandleByPipeID(PipeID), PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_ControlTransfer (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.ControlTransfer(InterfaceHandleByPipeID(0), SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_ResetPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.ResetPipe(InterfaceHandleByPipeID(PipeID), PipeID);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_AbortPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.AbortPipe(InterfaceHandleByPipeID(PipeID), PipeID);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_FlushPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.FlushPipe(InterfaceHandleByPipeID(PipeID), PipeID);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_SetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.SetPowerPolicy(InterfaceHandleByPipeID(0), PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "->WinUsb.SetPowerPolicy");

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	WUSBFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	success = WinUsb.GetPowerPolicy(InterfaceHandleByPipeID(0), PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "->WinUsb.GetPowerPolicy");

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetOverlappedResult (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	return UsbK_GetOverlappedResult(InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

KUSB_EXP BOOL KUSB_API WUsb_Open(
    __in PKUSB_DEV_LIST DeviceListItem,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	BOOL success;
	PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	PWINUSB_BKND_CONTEXT backendContext = NULL;

	PKUSB_DEV_LIST nextCompositeEL;

	CheckLibInitialized();

	if(!IsHandleValid(DeviceListItem) || !IsHandleValid(InterfaceHandle))
	{
		success = LusbwError(ERROR_INVALID_HANDLE);
		goto Error;
	}

	success = w_CreateContext(&interfaceHandle);
	ErrorNoSet(!success, Error, "->w_CreateContext");

	W_CTX(backendContext, interfaceHandle);
	ErrorHandle(!backendContext, Error, "backendContext");

	if (DeviceListItem->CompositeList)
	{
		List_ForEach(DeviceListItem->CompositeList, nextCompositeEL)
		{
			success = UsbStack_AddDevice(
			              &backendContext->UsbStack,
			              nextCompositeEL->DevicePath,
			              NULL,
			              backendContext);

			if (!success)
			{
				USBWRN("skipping composite device %s. ErrorCode=%08Xh\n",
				       nextCompositeEL->DeviceDesc, GetLastError());
			}
		}
	}
	else
	{

		success = UsbStack_AddDevice(
		              &backendContext->UsbStack,
		              DeviceListItem->DevicePath,
		              NULL,
		              NULL);
	}

	ErrorNoSet(!backendContext->UsbStack.DeviceCount, Error, "failed opening %s device. ErrorCode=%d",
	           DeviceListItem->DeviceDesc, GetLastError());

	*InterfaceHandle = interfaceHandle;
	return TRUE;

Error:
	w_DestroyContext(interfaceHandle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_Close(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	WUSBFN_CTX_PREFIX();

	// w_DestroyContext will also destroy the lock.
	AcquireSyncLockWrite(&handle->Instance.Lock);

	success = w_DestroyContext(handle);
	if (!success)
	{
		USBERR("->w_DestroyContext\n");
	}

	return success;

}
BOOL KUSB_API WUsb_ClaimInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	return w_ClaimOrReleaseInterface(InterfaceHandle, InterfaceNumberOrIndex, TRUE, (UCHAR)IsIndex);
}

BOOL KUSB_API WUsb_ReleaseInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	return w_ClaimOrReleaseInterface(InterfaceHandle, InterfaceNumberOrIndex, FALSE, (UCHAR)IsIndex);
}

BOOL KUSB_API WUsb_SetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltInterfaceNumber)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	WUSBFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	if (IsIndex)
	{
		List_SearchField(backendContext->UsbStack.InterfaceList, interfaceEL, VirtualIndex, InterfaceNumberOrIndex);
	}
	else
	{
		List_SearchField(backendContext->UsbStack.InterfaceList, interfaceEL, Number, InterfaceNumberOrIndex);
	}

	if (!interfaceEL)
	{
		USBWRN("interface #%u not found.\n", InterfaceNumberOrIndex);
		success = LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	success = WinUsb.SetCurrentAlternateSetting(
	              interfaceEL->ParentDevice->Device->SharedInterfaces[interfaceEL->Number].InterfaceHandle,
	              AltInterfaceNumber);

	if (success)
	{
		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              interfaceEL->Number, FALSE,
		              AltInterfaceNumber, FALSE);

		ErrorNoSet(!success, Error, "->UsbStackHandler_SetAltInterface");

	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

BOOL KUSB_API WUsb_GetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltInterfaceNumber)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	WUSBFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	ErrorParam(!AltInterfaceNumber, Error, "AltInterfaceNumber");

	if (IsIndex)
	{
		List_SearchField(backendContext->UsbStack.InterfaceList, interfaceEL, VirtualIndex, InterfaceNumberOrIndex);
	}
	else
	{
		List_SearchField(backendContext->UsbStack.InterfaceList, interfaceEL, Number, InterfaceNumberOrIndex);
	}

	if (!interfaceEL)
	{
		USBWRN("interface #%u not found.\n", InterfaceNumberOrIndex);
		success = LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	success = WinUsb.GetCurrentAlternateSetting(
	              interfaceEL->ParentDevice->Device->SharedInterfaces[interfaceEL->Number].InterfaceHandle,
	              AltInterfaceNumber);

	if (success)
	{
		success = UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              interfaceEL->Number, FALSE,
		              *AltInterfaceNumber, FALSE);

		ErrorNoSet(!success, Error, "->UsbStackHandler_SetAltInterface");
	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

BOOL KUSB_API WUsb_ResetDevice (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API WUsb_IsoReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(IsoPacketSize);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API WUsb_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(IsoPacketSize);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}


BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	WUsb_Init_Library();

	if (!IsHandleValid(WinUsb_Dll_Module))
	{
		rtn = ERROR_NOT_SUPPORTED;
		*ProcAddress = (KPROC)NULL;
		USBERR("winusb.dll not found.\n");
		return LusbwError(rtn);
	}

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)WUsb_Initialize;
		break;
	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)WUsb_Free;
		break;
	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)WUsb_GetAssociatedInterface;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)WUsb_GetDescriptor;
		break;
	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)WUsb_QueryInterfaceSettings;
		break;
	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)WUsb_QueryDeviceInformation;
		break;
	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)WUsb_SetCurrentAlternateSetting;
		break;
	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)WUsb_GetCurrentAlternateSetting;
		break;
	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)WUsb_QueryPipe;
		break;
	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)WUsb_SetPipePolicy;
		break;
	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)WUsb_GetPipePolicy;
		break;
	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)WUsb_ReadPipe;
		break;
	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)WUsb_WritePipe;
		break;
	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)WUsb_ControlTransfer;
		break;
	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)WUsb_ResetPipe;
		break;
	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)WUsb_AbortPipe;
		break;
	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)WUsb_FlushPipe;
		break;
	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)WUsb_SetPowerPolicy;
		break;
	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)WUsb_GetPowerPolicy;
		break;
	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)WUsb_GetOverlappedResult;
		break;
	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)WUsb_ResetDevice;
		break;
	case KUSB_FNID_Open:
		*ProcAddress = (KPROC)WUsb_Open;
		break;
	case KUSB_FNID_Close:
		*ProcAddress = (KPROC)WUsb_Close;
		break;
	case KUSB_FNID_SetConfiguration:
		*ProcAddress = (KPROC)WUsb_SetConfiguration;
		break;
	case KUSB_FNID_GetConfiguration:
		*ProcAddress = (KPROC)WUsb_GetConfiguration;
		break;
	case KUSB_FNID_ClaimInterface:
		*ProcAddress = (KPROC)WUsb_ClaimInterface;
		break;
	case KUSB_FNID_ReleaseInterface:
		*ProcAddress = (KPROC)WUsb_ReleaseInterface;
		break;
	case KUSB_FNID_SetAltInterface:
		*ProcAddress = (KPROC)WUsb_SetAltInterface;
		break;
	case KUSB_FNID_GetAltInterface:
		*ProcAddress = (KPROC)WUsb_GetAltInterface;
		break;
	case KUSB_FNID_IsoReadPipe:
		*ProcAddress = (KPROC)WUsb_IsoReadPipe;
		break;
	case KUSB_FNID_IsoWritePipe:
		*ProcAddress = (KPROC)WUsb_IsoWritePipe;
		break;


	default:
		rtn = ERROR_NOT_SUPPORTED;
		*ProcAddress = (KPROC)NULL;
		USBERR("Unrecognized function id! FunctionID=%u\n", FunctionID);
		break;

	}
	return LusbwError(rtn);
}
#else
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	UNREFERENCED_PARAMETER(ProcAddress);
	UNREFERENCED_PARAMETER(FunctionID);
	return LusbwError(ERROR_NOT_SUPPORTED);
}
#endif // WINUSB_BACKEND_SUPPORT
