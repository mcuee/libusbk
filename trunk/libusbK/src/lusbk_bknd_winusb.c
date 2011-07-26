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
#include "lusbk_stack_collection.h"
#include "lusbk_bknd_unsupported.h"

#define WINUSB_BACKEND_SUPPORT

#ifdef WINUSB_BACKEND_SUPPORT

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

extern ULONG DebugLevel;

#ifndef WINUSB_DLL_DYNAMIC_____________________________________________

typedef struct _WINUSB_API
{
	struct
	{
		volatile long Lock;
		BOOL IsInitialized;
		volatile HMODULE DLL;
	} Init;

	BOOL (KUSB_API* Initialize)				(__in HANDLE DeviceHandle, __out KUSB_HANDLE* InterfaceHandle);
	BOOL (KUSB_API* Free)					(__in KUSB_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface)	(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out KUSB_HANDLE* AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	BOOL (KUSB_API* QueryDeviceInformation)	(__in KUSB_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AltSettingNumber);
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in KUSB_HANDLE InterfaceHandle, __out PUCHAR AltSettingNumber);
	BOOL (KUSB_API* SetPipePolicy)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPipePolicy)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* ReadPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer)		(__in KUSB_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy)			(__in KUSB_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy)			(__in KUSB_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
}* PWINUSB_API, WINUSB_API;

static WINUSB_API WinUsb = {{0, FALSE, NULL}};

static void WUsb_Init_Library()
{

	if (!WinUsb.Init.IsInitialized)
	{
		CheckLibInit();
		SpinLock_Acquire(&WinUsb.Init.Lock, TRUE);
		if ((WinUsb.Init.DLL = LoadLibraryA("winusb.dll")) != NULL)
		{
			WinUsb.AbortPipe = (KUSB_AbortPipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_AbortPipe");
			WinUsb.Initialize = (KUSB_Initialize*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_Initialize");
			WinUsb.Free = (KUSB_Free*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_Free");
			WinUsb.GetAssociatedInterface = (KUSB_GetAssociatedInterface*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_GetAssociatedInterface");
			WinUsb.GetDescriptor = (KUSB_GetDescriptor*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_GetDescriptor");
			WinUsb.QueryDeviceInformation = (KUSB_QueryDeviceInformation*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_QueryDeviceInformation");
			WinUsb.SetCurrentAlternateSetting = (KUSB_SetCurrentAlternateSetting*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_SetCurrentAlternateSetting");
			WinUsb.GetCurrentAlternateSetting = (KUSB_GetCurrentAlternateSetting*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_GetCurrentAlternateSetting");
			WinUsb.SetPipePolicy = (KUSB_SetPipePolicy*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_SetPipePolicy");
			WinUsb.GetPipePolicy = (KUSB_GetPipePolicy*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_GetPipePolicy");
			WinUsb.ReadPipe = (KUSB_ReadPipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_ReadPipe");
			WinUsb.WritePipe = (KUSB_WritePipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_WritePipe");
			WinUsb.ControlTransfer = (KUSB_ControlTransfer*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_ControlTransfer");
			WinUsb.ResetPipe = (KUSB_ResetPipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_ResetPipe");
			WinUsb.AbortPipe = (KUSB_AbortPipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_AbortPipe");
			WinUsb.FlushPipe = (KUSB_FlushPipe*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_FlushPipe");
			WinUsb.SetPowerPolicy = (KUSB_SetPowerPolicy*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_SetPowerPolicy");
			WinUsb.GetPowerPolicy = (KUSB_GetPowerPolicy*)GetProcAddress(WinUsb.Init.DLL, "WinUsb_GetPowerPolicy");
		}
		else
		{
			WinUsb.AbortPipe = Unsupported_AbortPipe;
			WinUsb.Initialize = Unsupported_Initialize;
			WinUsb.Free = Unsupported_Free;
			WinUsb.GetAssociatedInterface = Unsupported_GetAssociatedInterface;
			WinUsb.GetDescriptor = Unsupported_GetDescriptor;
			WinUsb.QueryDeviceInformation = Unsupported_QueryDeviceInformation;
			WinUsb.SetCurrentAlternateSetting = Unsupported_SetCurrentAlternateSetting;
			WinUsb.GetCurrentAlternateSetting = Unsupported_GetCurrentAlternateSetting;
			WinUsb.SetPipePolicy = Unsupported_SetPipePolicy;
			WinUsb.GetPipePolicy = Unsupported_GetPipePolicy;
			WinUsb.ReadPipe = Unsupported_ReadPipe;
			WinUsb.WritePipe = Unsupported_WritePipe;
			WinUsb.ControlTransfer = Unsupported_ControlTransfer;
			WinUsb.ResetPipe = Unsupported_ResetPipe;
			WinUsb.AbortPipe = Unsupported_AbortPipe;
			WinUsb.FlushPipe = Unsupported_FlushPipe;
			WinUsb.SetPowerPolicy = Unsupported_SetPowerPolicy;
			WinUsb.GetPowerPolicy = Unsupported_GetPowerPolicy;
		}
		WinUsb.Init.IsInitialized = TRUE;
		SpinLock_Release(&WinUsb.Init.Lock);
	}
}

#endif

#ifndef OVLK_OVERLAPPED_XFER_DEFINES___________________________________

#define OVLK_CHECK(mOverlapped, mPipeID, mBuffer, mBufferLength, mDeviceHandle, mInterfaceHandle)				\
	if (IS_OVLK(mOverlapped))																					\
	{																											\
		PKOVL_OVERLAPPED_INFO ovlkInfo = KOVL_GET_PRIVATE_INFO(mOverlapped);									\
		ovlkInfo->DataBuffer = mBuffer;																			\
		ovlkInfo->DataBufferSize = mBufferLength;																\
		ovlkInfo->DeviceHandle = mDeviceHandle;																	\
		ovlkInfo->InterfaceHandle = mInterfaceHandle;															\
		ovlkInfo->mPipeID = mPipeID;																			\
		ovlkInfo->Cancel = w_CancelOverlappedK;																	\
	}

#define OVLK_CONTROL_CHECK(mOverlapped, mBuffer, mBufferLength, mDeviceHandle, mInterfaceHandle)				\
	if (IS_OVLK(mOverlapped))																					\
	{																											\
		PKOVL_OVERLAPPED_INFO ovlkInfo = KOVL_GET_PRIVATE_INFO(mOverlapped);									\
		ovlkInfo->DataBuffer = mBuffer;																			\
		ovlkInfo->DataBufferSize = mBufferLength;																\
		ovlkInfo->DeviceHandle = mDeviceHandle;																	\
		ovlkInfo->InterfaceHandle = mInterfaceHandle;															\
		ovlkInfo->Cancel = w_CancelOverlappedK_Control;															\
	}

static BOOL KUSB_API w_CancelOverlappedK(__in KOVL_HANDLE Overlapped)
{
	BOOL success;
	PKOVL_OVERLAPPED_INFO ovInfo = KOVL_GET_PRIVATE_INFO(Overlapped);
	if (AllK.CancelIoEx)
	{
		success = AllK.CancelIoEx(ovInfo->DeviceHandle, Overlapped);
	}
	else
	{
		success = WinUsb.AbortPipe(ovInfo->InterfaceHandle, ovInfo->PipeID);
	}

	return success;
}

static BOOL KUSB_API w_CancelOverlappedK_Control(__in KOVL_HANDLE Overlapped)
{
	BOOL success;
	PKOVL_OVERLAPPED_INFO ovInfo = KOVL_GET_PRIVATE_INFO(Overlapped);
	if (AllK.CancelIoEx)
	{
		success = AllK.CancelIoEx(ovInfo->DeviceHandle, Overlapped);
	}
	else
	{
		success = CancelIo(ovInfo->DeviceHandle);
	}

	return success;
}

#endif

#ifndef HANDLE_CLEANUP_________________________________________________

static void KUSB_API w_Cleanup_DevK(__in PKDEV_HANDLE_INTERNAL SharedDevice)
{
	int pos = 0;
	for (pos = 0; pos < KDEV_SHARED_INTERFACE_COUNT; pos++)
	{
		if (!SharedDevice->SharedInterfaces[pos].InterfaceHandle)
			break;
		WinUsb.Free(SharedDevice->SharedInterfaces[pos].InterfaceHandle);
		SharedDevice->SharedInterfaces[pos].InterfaceHandle = NULL;
	}

	if (!Str_IsNullOrEmpty(SharedDevice->DevicePath))
	{
		Mem_Free(&SharedDevice->DevicePath);
		CloseHandle(SharedDevice->MasterDeviceHandle);
	}
	if (SharedDevice->UsbStack)
	{
		// free the device/interface stack
		UsbStack_Clear(SharedDevice->UsbStack);
		Mem_Free(&SharedDevice->UsbStack);
	}
	Mem_Free(&SharedDevice->ConfigDescriptor);
}

static void KUSB_API w_Cleanup_UsbK(__in PKUSB_HANDLE_INTERNAL InternalHandle)
{
	if (InternalHandle && InternalHandle->Device) PoolHandle_Dec_DevK(InternalHandle->Device);
}

#endif

#ifndef HANDLE_INIT_CLONE_AND_FREE_FUNCTIONS___________________________

static BOOL w_Init_Config(PKUSB_HANDLE_INTERNAL handle)
{
	DWORD transferred = 0;
	USB_CONFIGURATION_DESCRIPTOR configCheck;
	BOOL success;
	UCHAR nextIntefaceIndex = UCHAR_MAX;
	HANDLE nextInterfaceHandle = NULL;

	if (!Intf_Handle())
	{
		success = WinUsb.Initialize(Dev_Handle(), (KUSB_HANDLE*)&Intf_Handle());
		ErrorNoSetAction(!success, return FALSE, "WinUsb.Initialize failed.");
		handle->Device->SharedInterfaces[0].InterfaceHandle = Intf_Handle();
		handle->Device->SharedInterfaces[0].Index = 0;

		while(WinUsb.GetAssociatedInterface(Intf_Handle(), ++nextIntefaceIndex, (KUSB_HANDLE*)&nextInterfaceHandle))
		{
			handle->Device->SharedInterfaces[nextIntefaceIndex + 1].InterfaceHandle = nextInterfaceHandle;
			handle->Device->SharedInterfaces[nextIntefaceIndex + 1].Index = nextIntefaceIndex + 1;
		}
	}

	Mem_Zero(&configCheck, sizeof(configCheck));

	success = WinUsb.GetDescriptor(
	              Intf_Handle(),
	              USB_CONFIGURATION_DESCRIPTOR_TYPE,
	              0,
	              0,
	              (PUCHAR)&configCheck,
	              sizeof(configCheck),
	              &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor1");

	handle->Device->ConfigDescriptor = Mem_Alloc(configCheck.wTotalLength);
	ErrorMemory(!handle->Device->ConfigDescriptor, Error);

	success = WinUsb.GetDescriptor(Intf_Handle(),
	                               USB_CONFIGURATION_DESCRIPTOR_TYPE,
	                               0,
	                               0,
	                               (PUCHAR)handle->Device->ConfigDescriptor,
	                               configCheck.wTotalLength,
	                               &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor2");

Error:
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_Free (__in KUSB_HANDLE InterfaceHandle)
{
	PKUSB_HANDLE_INTERNAL handle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return TRUE);
	PoolHandle_Dec_UsbK(handle);

	return TRUE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetAssociatedInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	return UsbStack_GetAssociatedInterface(InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WUsb_Clone (
    __in KUSB_HANDLE InterfaceHandle,
    __out KUSB_HANDLE* DstInterfaceHandle)
{
	return UsbStack_CloneHandle(InterfaceHandle, DstInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WUsb_Initialize(
    __in HANDLE DeviceHandle,
    __out KUSB_HANDLE* InterfaceHandle)
{
	CheckLibInit();

	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_WINUSB, TRUE, DeviceHandle, NULL, NULL, w_Init_Config, NULL, w_Cleanup_UsbK, w_Cleanup_DevK);
}

KUSB_EXP BOOL KUSB_API WUsb_Open(
    __in KLST_DEVINFO* DevInfo,
    __out KUSB_HANDLE* InterfaceHandle)
{
	CheckLibInit();
	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_WINUSB, TRUE, NULL, DevInfo, NULL, w_Init_Config, NULL, w_Cleanup_UsbK, w_Cleanup_DevK);

}

KUSB_EXP BOOL KUSB_API WUsb_Close(
    __in KUSB_HANDLE InterfaceHandle)
{
	return WUsb_Free(InterfaceHandle);
}

#endif

#ifndef DEVICE_AND_INTERFACE_FUNCTIONS_________________________________

KUSB_EXP BOOL KUSB_API WUsb_SetConfiguration(
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{

	PKUSB_HANDLE_INTERNAL handle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	if (handle->Device->ConfigDescriptor->bConfigurationValue != ConfigurationNumber)
	{
		LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetConfiguration(
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	PKUSB_HANDLE_INTERNAL handle;

	ErrorParamAction(!ConfigurationNumber, "ConfigurationNumber", return FALSE);
	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");
	if (handle->Device->ConfigDescriptor)
		*ConfigurationNumber = handle->Device->ConfigDescriptor->bConfigurationValue;
	else
		*ConfigurationNumber = 0;

	PoolHandle_Dec_UsbK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetDescriptor (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.GetDescriptor(
	              Intf_Handle(),
	              DescriptorType,
	              Index,
	              LanguageID,
	              Buffer,
	              BufferLength,
	              LengthTransferred);

	ErrorNoSet(!success, Error, "Failed getting descriptor.");

	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_QueryInterfaceSettings (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return UsbStack_QueryInterfaceSettings(InterfaceHandle, AltSettingNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API WUsb_QueryDeviceInformation (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.QueryDeviceInformation(
	              Intf_Handle(),
	              InformationType,
	              BufferLength,
	              Buffer);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_QueryPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{

	return UsbStack_QueryPipe(
	           InterfaceHandle,
	           AltSettingNumber,
	           PipeIndex,
	           PipeInformation);
}

KUSB_EXP BOOL KUSB_API WUsb_SetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.SetPipePolicy(Intf_Handle(), PipeID, PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "PipeID=%02Xh PolicyType=%04Xh ValueLength=%u", PipeID, PolicyType, ValueLength);

	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{

	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.GetPipePolicy(Intf_Handle(), PipeID, PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "PipeID=%02Xh PolicyType=%04Xh ValueLength=%p", PipeID, PolicyType, ValueLength);

	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_SetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.SetPowerPolicy(Intf_Handle(), PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "PolicyType=%04Xh ValueLength=%u", PolicyType, ValueLength);

	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.GetPowerPolicy(Intf_Handle(), PolicyType, ValueLength, Value);
	ErrorNoSet(!success, Error, "PolicyType=%04Xh ValueLength=%p", PolicyType, ValueLength);

	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

BOOL w_ClaimOrReleaseInterface(__in KUSB_HANDLE InterfaceHandle,
                               __in INT NumberOrIndex,
                               __in UCHAR IsClaim,
                               __in UCHAR IsIndex)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_INTERFACE_EL interfaceEL;
	BOOL success = FALSE;

	ErrorParamAction(NumberOrIndex < 0, "NumberOrIndex", return FALSE);
	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, interfaceEL, IsIndex, NumberOrIndex);
	ErrorSetAction(!interfaceEL, ERROR_RESOURCE_NOT_FOUND, goto Done, "Interface not found. NumberOrIndex=%u IsClaim=%u IsIndex=%u", NumberOrIndex, IsClaim, IsIndex);

	success = TRUE;

	if (IsClaim && success)
	{
		Get_SharedInterface(handle, interfaceEL->Index).Claimed = TRUE;
		InterlockedExchange(&handle->Selected_SharedInterface_Index, interfaceEL->Index);
	}
	else
	{
		Get_SharedInterface(handle, interfaceEL->Index).Claimed = FALSE;
	}
Done:
	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_ClaimInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{

	return w_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, TRUE, (UCHAR)IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_SelectInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbStack_SelectInterface(InterfaceHandle, NumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_ReleaseInterface(
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	return w_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, FALSE, (UCHAR)IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_SetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Get_CurSharedInterface(handle, sharedInterface);

	success = WinUsb.SetCurrentAlternateSetting(sharedInterface->InterfaceHandle, AltSettingNumber);
	ErrorNoSet(!success, Error, "Failed setting AltSettingNumber %u", AltSettingNumber);

	Update_SharedInterface_AltSetting(sharedInterface, AltSettingNumber);
	UsbStack_RefreshPipeCache(handle);
	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR AltSettingNumber)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;
	UCHAR altNumber = 0;

	ErrorParamAction(!AltSettingNumber, "AltSettingNumber", return FALSE);
	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);

	Get_CurSharedInterface(handle, sharedInterface);

	success = WinUsb.GetCurrentAlternateSetting(sharedInterface->InterfaceHandle, &altNumber);
	ErrorNoSet(!success, Error, "Failed getting AltSettingNumber");

	*AltSettingNumber = altNumber;
	Update_SharedInterface_AltSetting(sharedInterface, altNumber);
	UsbStack_RefreshPipeCache(handle);
	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_SetAltInterface(
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltSettingNumber)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;
	PKUSB_INTERFACE_EL intfEL;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, IsIndex, NumberOrIndex);
	ErrorSet(!intfEL, Error, ERROR_RESOURCE_NOT_FOUND, "Interface not found. NumberOrIndex=%u IsIndex=%u.", NumberOrIndex, IsIndex);

	sharedInterface = &Get_SharedInterface(handle, intfEL->Index);

	success = WinUsb.SetCurrentAlternateSetting(sharedInterface->InterfaceHandle, AltSettingNumber);
	ErrorNoSet(!success, Error, "Failed setting AltSettingNumber %u.", AltSettingNumber);

	Update_SharedInterface_AltSetting(sharedInterface, AltSettingNumber);
	UsbStack_RefreshPipeCache(handle);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_GetAltInterface(
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltSettingNumber)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success = FALSE;
	PKDEV_SHARED_INTERFACE sharedInterface;
	PKUSB_INTERFACE_EL intfEL;
	UCHAR altNumber = 0;

	ErrorParamAction(!AltSettingNumber, "AltSettingNumber", return FALSE);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, IsIndex, NumberOrIndex);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Interface not found. NumberOrIndex=%u IsIndex=%u AltSettingNumber=%u", NumberOrIndex, IsIndex, AltSettingNumber);

	sharedInterface = &Get_SharedInterface(handle, intfEL->Index);

	success = WinUsb.GetCurrentAlternateSetting(sharedInterface->InterfaceHandle, &altNumber);
	ErrorNoSet(!success, Error, "Failed getting AltSettingNumber.");

	*AltSettingNumber = altNumber;
	Update_SharedInterface_AltSetting(sharedInterface, altNumber);
	UsbStack_RefreshPipeCache(handle);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API WUsb_ControlTransfer (
    __in KUSB_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	OVLK_CONTROL_CHECK(Overlapped, Buffer, BufferLength, Dev_Handle(), Intf_Handle());
	success = WinUsb.ControlTransfer(Intf_Handle(), SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetOverlappedResult (
    __in KUSB_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = GetOverlappedResult(Dev_Handle(), lpOverlapped, lpNumberOfBytesTransferred, bWait);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

#endif

#ifndef PIPE_IO_FUNCTIONS______________________________________________

KUSB_EXP BOOL KUSB_API WUsb_ResetPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.ResetPipe(Get_PipeInterfaceHandle(handle, PipeID), PipeID);
	ErrorNoSetAction(!success, USB_LOG_NOP(), "PipeID=%02Xh", PipeID);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_AbortPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.AbortPipe(Get_PipeInterfaceHandle(handle, PipeID), PipeID);
	ErrorNoSetAction(!success, USB_LOG_NOP(), "PipeID=%02Xh", PipeID);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_FlushPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.FlushPipe(Get_PipeInterfaceHandle(handle, PipeID), PipeID);
	ErrorNoSetAction(!success, USB_LOG_NOP(), "PipeID=%02Xh", PipeID);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_ReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	HANDLE intfHandle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	intfHandle = Get_PipeInterfaceHandle(handle, PipeID);

	OVLK_CHECK(Overlapped, PipeID, Buffer, BufferLength, Dev_Handle(), intfHandle);

	success = WinUsb.ReadPipe(intfHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_WritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	HANDLE intfHandle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	intfHandle = Get_PipeInterfaceHandle(handle, PipeID);

	OVLK_CHECK(Overlapped, PipeID, Buffer, BufferLength, Dev_Handle(), intfHandle);
	success = WinUsb.WritePipe(intfHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

#endif

BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	WUsb_Init_Library();

	if (WinUsb.Init.DLL == NULL)
	{
		GetProcAddress_Unsupported(ProcAddress, FunctionID);
		return LusbwError(ERROR_NOT_SUPPORTED);
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
	case KUSB_FNID_Clone:
		*ProcAddress = (KPROC)WUsb_Clone;
		break;
	case KUSB_FNID_SelectInterface:
		*ProcAddress = (KPROC)WUsb_SelectInterface;
		break;
	default:
		GetProcAddress_Unsupported(ProcAddress, FunctionID);
		return LusbwError(ERROR_NOT_SUPPORTED);

	}
	return LusbwError(rtn);
}
#else
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	GetProcAddress_Unsupported(ProcAddress, FunctionID);
	return LusbwError(ERROR_NOT_SUPPORTED);
}
#endif // WINUSB_BACKEND_SUPPORT
