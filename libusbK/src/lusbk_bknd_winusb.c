/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
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

	BOOL (KUSB_API* Initialize) ( HANDLE DeviceHandle, KUSB_HANDLE* InterfaceHandle);
	BOOL (KUSB_API* Free) ( KUSB_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface) ( KUSB_HANDLE InterfaceHandle, UCHAR AssociatedInterfaceIndex, KUSB_HANDLE* AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor) ( KUSB_HANDLE InterfaceHandle, UCHAR DescriptorType, UCHAR Index, USHORT LanguageID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred);
	BOOL (KUSB_API* QueryDeviceInformation) ( KUSB_HANDLE InterfaceHandle, UINT InformationType, PUINT BufferLength, PVOID Buffer);
	BOOL (KUSB_API* SetCurrentAlternateSetting) ( KUSB_HANDLE InterfaceHandle, UCHAR AltSettingNumber);
	BOOL (KUSB_API* GetCurrentAlternateSetting) ( KUSB_HANDLE InterfaceHandle, PUCHAR AltSettingNumber);
	BOOL (KUSB_API* SetPipePolicy) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID, UINT PolicyType, UINT ValueLength, PVOID Value);
	BOOL (KUSB_API* GetPipePolicy) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID, UINT PolicyType, PUINT ValueLength, PVOID Value);
	BOOL (KUSB_API* ReadPipe) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer) ( KUSB_HANDLE InterfaceHandle, WINUSB_SETUP_PACKET SetupPacket, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe) ( KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy) ( KUSB_HANDLE InterfaceHandle, UINT PolicyType, UINT ValueLength, PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy) ( KUSB_HANDLE InterfaceHandle, UINT PolicyType, PUINT ValueLength, PVOID Value);
}* PWINUSB_API, WINUSB_API;

WINUSB_API WinUsb = {{0, FALSE, NULL}};

VOID WUsb_Init_Library()
{

	if (!WinUsb.Init.IsInitialized)
	{
		mSpin_Acquire(&WinUsb.Init.Lock);
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
		mSpin_Release(&WinUsb.Init.Lock);
	}
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
	UINT transferred = 0;
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
	              USB_DESCRIPTOR_TYPE_CONFIGURATION,
	              0,
	              0,
	              (PUCHAR)&configCheck,
	              sizeof(configCheck),
	              &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor1");

	handle->Device->ConfigDescriptor = Mem_Alloc(configCheck.wTotalLength);
	ErrorMemory(!handle->Device->ConfigDescriptor, Error);

	success = WinUsb.GetDescriptor(Intf_Handle(),
	                               USB_DESCRIPTOR_TYPE_CONFIGURATION,
	                               0,
	                               0,
	                               (PUCHAR)handle->Device->ConfigDescriptor,
	                               configCheck.wTotalLength,
	                               &transferred);
	ErrorNoSet(!success, Error, "->WinUsb.GetDescriptor2");

Error:
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetAssociatedInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AssociatedInterfaceIndex,
    _out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	return UsbStack_GetAssociatedInterface(InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WUsb_Initialize(
    _in HANDLE DeviceHandle,
    _out KUSB_HANDLE* InterfaceHandle)
{
	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_WINUSB, TRUE, DeviceHandle, NULL, NULL, w_Init_Config, NULL, w_Cleanup_UsbK, w_Cleanup_DevK);
}

KUSB_EXP BOOL KUSB_API WUsb_Init(
    _out KUSB_HANDLE* InterfaceHandle,
    _in KLST_DEVINFO_HANDLE DevInfo)
{
	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_WINUSB, TRUE, NULL, DevInfo, NULL, w_Init_Config, NULL, w_Cleanup_UsbK, w_Cleanup_DevK);

}

#endif

#ifndef DEVICE_AND_INTERFACE_FUNCTIONS_________________________________

KUSB_EXP BOOL KUSB_API WUsb_SetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR ConfigurationNumber)
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
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR ConfigurationNumber)
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

KUSB_EXP BOOL KUSB_API WUsb_GetDescriptor(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR DescriptorType,
    _in UCHAR Index,
    _in USHORT LanguageID,
    _out PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred)
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

KUSB_EXP BOOL KUSB_API WUsb_QueryDeviceInformation(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT InformationType,
    _ref PUINT BufferLength,
    _ref PVOID Buffer)
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

KUSB_EXP BOOL KUSB_API WUsb_QueryPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _in UCHAR PipeIndex,
    _out PWINUSB_PIPE_INFORMATION PipeInformation)
{

	return UsbStack_QueryPipe(
	           InterfaceHandle,
	           AltSettingNumber,
	           PipeIndex,
	           PipeInformation);
}

KUSB_EXP BOOL KUSB_API WUsb_SetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in UINT PolicyType,
    _in UINT ValueLength,
    _in PVOID Value)
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

KUSB_EXP BOOL KUSB_API WUsb_GetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in UINT PolicyType,
    _ref PUINT ValueLength,
    _out PVOID Value)
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

KUSB_EXP BOOL KUSB_API WUsb_SetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT PolicyType,
    _in UINT ValueLength,
    _in PVOID Value)
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

KUSB_EXP BOOL KUSB_API WUsb_GetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT PolicyType,
    _ref PUINT ValueLength,
    _out PVOID Value)
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

KUSB_EXP BOOL KUSB_API WUsb_ClaimInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{

	return w_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, TRUE, (UCHAR)IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_SelectInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	return UsbStack_SelectInterface(InterfaceHandle, NumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_ReleaseInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	return w_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, FALSE, (UCHAR)IsIndex);
}

KUSB_EXP BOOL KUSB_API WUsb_SetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber)
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

KUSB_EXP BOOL KUSB_API WUsb_GetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR AltSettingNumber)
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
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _in UCHAR AltSettingNumber)
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
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _out PUCHAR AltSettingNumber)
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

KUSB_EXP BOOL KUSB_API WUsb_ControlTransfer(
    _in KUSB_HANDLE InterfaceHandle,
    _in WINUSB_SETUP_PACKET SetupPacket,
    _refopt PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = WinUsb.ControlTransfer(Intf_Handle(), SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_GetOverlappedResult(
    _in KUSB_HANDLE InterfaceHandle,
    _in LPOVERLAPPED Overlapped,
    _out PUINT lpNumberOfBytesTransferred,
    _in BOOL bWait)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = GetOverlappedResult(Dev_Handle(), Overlapped, (LPDWORD)lpNumberOfBytesTransferred, bWait);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

#endif

#ifndef PIPE_IO_FUNCTIONS______________________________________________

KUSB_EXP BOOL KUSB_API WUsb_ResetPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
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

KUSB_EXP BOOL KUSB_API WUsb_AbortPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
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

KUSB_EXP BOOL KUSB_API WUsb_FlushPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
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

KUSB_EXP BOOL KUSB_API WUsb_ReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	HANDLE intfHandle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	intfHandle = Get_PipeInterfaceHandle(handle, PipeID);
	success = WinUsb.ReadPipe(intfHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API WUsb_WritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	HANDLE intfHandle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	intfHandle = Get_PipeInterfaceHandle(handle, PipeID);
	success = WinUsb.WritePipe(intfHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

#endif

BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	WUsb_Init_Library();

	if (WinUsb.Init.DLL == NULL)
		return FALSE;

	switch(FunctionID)
	{
	case KUSB_FNID_Init:
		*ProcAddress = (KPROC)WUsb_Init;
		break;
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)WUsb_Initialize;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)WUsb_GetDescriptor;
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
	default:
		return FALSE;

	}
	return TRUE;
}

#else
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	GetProcAddress_Unsupported(ProcAddress, FunctionID);
	return LusbwError(ERROR_NOT_SUPPORTED);
}
#endif // WINUSB_BACKEND_SUPPORT
