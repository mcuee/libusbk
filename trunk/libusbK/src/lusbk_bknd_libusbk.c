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

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

extern ULONG DebugLevel;

#ifndef LIBUSBK0_SYNC_TRANSFER_HELPER_DEFINES__________________________

#define AllocRequest(RequiredDataLength) Mem_Alloc(sizeof(libusb_request) + RequiredDataLength)

#define GetRequestDataPtr(LibusbRequestPtr) (&(((PUCHAR)LibusbRequestPtr)[sizeof(libusb_request)]))

#define GetRequestData(LibusbRequestPtr,OutputBuffer,OutputBufferLength)	\
	if ( (LibusbRequestPtr) && (OutputBuffer) && (OutputBufferLength) ) memcpy(OutputBuffer, GetRequestDataPtr(LibusbRequestPtr), (size_t)OutputBufferLength)

#define SetRequestData(LibusbRequestPtr,InputBuffer,InputBufferLength)	\
	if ( (LibusbRequestPtr) && (InputBuffer) && (InputBufferLength) ) memcpy(GetRequestDataPtr(LibusbRequestPtr), InputBuffer, (size_t)InputBufferLength)

#define SubmitSyncBufferRequest(IoControlCode)					\
	Ioctl_Sync(Dev_Handle(),IoControlCode,&request,sizeof(request),Buffer,*BufferLength, BufferLength);

#define SubmitSyncRequestPtr(IoControlCode,transferredLength)	\
	Ioctl_Sync(Dev_Handle(), IoControlCode, request, sizeof(*request), request, sizeof(*request), &(transferredLength))

#define SubmitSyncRequest(IoControlCode,transferredLength)		\
	Ioctl_Sync(Dev_Handle(), IoControlCode, &request, sizeof(request), &request, sizeof(request), (transferredLength))

#define SubmitSimpleSyncRequestEx(DeviceHandle,IoControlCode)	\
	Ioctl_Sync(DeviceHandle, IoControlCode, &request, sizeof(request), &request, sizeof(request), NULL)
#define SubmitSimpleSyncRequest(IoControlCode)					\
	SubmitSimpleSyncRequestEx(Dev_Handle(),IoControlCode)
#endif

#ifndef HANDLE_CLEANUP_________________________________________________

static void KUSB_API k_Cleanup_DevK(__in PKDEV_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_DevK(handle);
	if (!Str_IsNullOrEmpty(handle->DevicePath))
	{
		Mem_Free(&handle->DevicePath);
		CloseHandle(handle->MasterDeviceHandle);
	}
	if (handle->UsbStack)
	{
		// free the device/interface stack
		UsbStack_Clear(handle->UsbStack);
		Mem_Free(&handle->UsbStack);
	}
	Mem_Free(&handle->ConfigDescriptor);
	Mem_Free(&handle->Backend.Ctx);
}

static void KUSB_API k_Cleanup_UsbK(__in PKUSB_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_UsbK(handle);
	if (handle && handle->Device)
	{
		PoolHandle_Dec_DevK(handle->Device);
	}
}

#endif

#ifndef HANDLE_INIT_CLONE_AND_FREE_FUNCTIONS___________________________

static BOOL k_Init_Version(__in HANDLE DeviceHandle, version_t* Version)
{
	libusb_request request;
	BOOL success = FALSE;

	Mem_Zero(&request, sizeof(request));

	success = SubmitSimpleSyncRequestEx(DeviceHandle, LIBUSB_IOCTL_GET_VERSION);
	ErrorNoSet(!success, Error, "failed get version request.");

	memcpy(Version, &request.version, sizeof(*Version));
	USBMSGN("%s.sys v%u.%u.%u.%u",
	        Version->major > 2 ? "libusbK" : "libusb0",
	        Version->major, Version->minor, Version->micro, Version->nano);

	return success;
Error:
	return success;
}

static BOOL k_Init_Backend(PKUSB_HANDLE_INTERNAL handle)
{
	handle->Device->Backend.CtxK = Mem_Alloc(sizeof(*handle->Device->Backend.CtxK));
	return IsHandleValid(handle->Device->Backend.CtxK);
}

static BOOL k_Init_Config(PKUSB_HANDLE_INTERNAL handle)
{
	libusb_request request;
	UINT transferred = 0;
	USB_CONFIGURATION_DESCRIPTOR configCheck;
	BOOL success;
	UCHAR lastConfigValue = 0;
	UCHAR currentConfigNumber = 0;
	UCHAR configIndex = 0;

	success = k_Init_Version(Dev_Handle(), &handle->Device->Backend.CtxK->Version);
	ErrorNoSet(!success, Error, "->k_Init_Version");

	Mem_Zero(&request, sizeof(request));
	success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_CACHED_CONFIGURATION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     &transferred);

	currentConfigNumber = ((PUCHAR)(&request))[0];
	if (!success || !transferred || !currentConfigNumber)
	{
		currentConfigNumber = 0;
		Mem_Zero(&request, sizeof(request));
		request.configuration.configuration = (unsigned int) - 1;
		success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_SET_CONFIGURATION,
		                     &request, (DWORD)sizeof(request),
		                     &request, (DWORD)sizeof(request),
		                     NULL);

		if (!success)
		{
			USBERRN("failed configuring device. ErrorCode = %08Xh", GetLastError());
			goto Error;
		}
	}

	Mem_Zero(&configCheck, sizeof(configCheck));

	while(success)
	{
		Mem_Zero(&request, sizeof(request));
		request.descriptor.type = USB_DESCRIPTOR_TYPE_CONFIGURATION;
		request.descriptor.index = configIndex++;

		if (configIndex == UCHAR_MAX)
		{
			success = LusbwError(ERROR_NO_MORE_ITEMS);
			goto Error;
		}

		success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_DESCRIPTOR,
		                     &request, sizeof(request),
		                     &configCheck,
		                     sizeof(configCheck),
		                     NULL);

		if (success)
		{
			if (!currentConfigNumber ||
			        configCheck.bConfigurationValue == currentConfigNumber ||
			        configCheck.bConfigurationValue == lastConfigValue)
			{
				break;
			}

			lastConfigValue = configCheck.bConfigurationValue;
		}
	}

	if (success)
	{
		handle->Device->ConfigDescriptor = Mem_Alloc(configCheck.wTotalLength);
		if (!handle->Device->ConfigDescriptor)
			return FALSE;

		success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_DESCRIPTOR,
		                     &request, sizeof(request),
		                     handle->Device->ConfigDescriptor,
		                     configCheck.wTotalLength,
		                     &transferred);

		if (!success)
		{
			Mem_Free(&handle->Device->ConfigDescriptor);
		}
	}

Error:
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_Free(
    _in KUSB_HANDLE InterfaceHandle)
{
	PKUSB_HANDLE_INTERNAL handle;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return TRUE);
	PoolHandle_Dec_UsbK(handle);

	return TRUE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetAssociatedInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AssociatedInterfaceIndex,
    _out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	return UsbStack_GetAssociatedInterface(InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API UsbK_Clone(
    _in KUSB_HANDLE InterfaceHandle,
    _out KUSB_HANDLE* DstInterfaceHandle)
{
	return UsbStack_CloneHandle(InterfaceHandle, DstInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API UsbK_Initialize(
    _in HANDLE DeviceHandle,
    _out KUSB_HANDLE* InterfaceHandle)
{
	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_LIBUSBK, FALSE, DeviceHandle, NULL, NULL, k_Init_Config, k_Init_Backend, k_Cleanup_UsbK, k_Cleanup_DevK);
}

KUSB_EXP BOOL KUSB_API UsbK_Init(
    _out KUSB_HANDLE* InterfaceHandle,
    _in KLST_DEVINFO_HANDLE DevInfo)
{
	return UsbStack_Init(InterfaceHandle, KUSB_DRVID_LIBUSBK, FALSE, NULL, DevInfo, NULL, k_Init_Config, k_Init_Backend, k_Cleanup_UsbK, k_Cleanup_DevK);
}

#endif

#ifndef DEVICE_AND_INTERFACE_FUNCTIONS_________________________________

KUSB_EXP BOOL KUSB_API UsbK_SetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR ConfigurationNumber)
{

	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	if (handle->Device->Backend.CtxK->Version.major > 2)
	{
		if (handle->Device->ConfigDescriptor->bConfigurationValue != ConfigurationNumber)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			goto Error;
		}
		goto Done;
	}

	request.configuration.configuration = ConfigurationNumber;
	success = SubmitSimpleSyncRequest(LIBUSB_IOCTL_SET_CONFIGURATION);
	if (!success)
	{
		USBERRN("failed setting configuration #%d", ConfigurationNumber);
		goto Error;
	}

	// rebuild the interface list.
	success = UsbStack_Rebuild(handle, k_Init_Config);
	ErrorNoSet(!success, Error, "->UsbStack_Rebuild");

Done:
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetConfiguration(
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

KUSB_EXP BOOL KUSB_API UsbK_GetDescriptor(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR DescriptorType,
    _in UCHAR Index,
    _in USHORT LanguageID,
    _out PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.descriptor.type = DescriptorType;
	request.descriptor.index = Index;
	request.descriptor.language_id = LanguageID;

	success = Ioctl_Sync(
	              Dev_Handle(),
	              LIBUSB_IOCTL_GET_DESCRIPTOR,
	              &request, sizeof(request),
	              Buffer, BufferLength,
	              LengthTransferred);
	ErrorNoSet(!success, Error, "Failed getting descriptor.");

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_QueryInterfaceSettings(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{

	return UsbStack_QueryInterfaceSettings(InterfaceHandle, AltSettingNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API UsbK_QueryDeviceInformation(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT InformationType,
    _ref PUINT BufferLength,
    _ref PVOID Buffer)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.query_device.information_type = InformationType;
	success = SubmitSyncBufferRequest(LIBUSB_IOCTL_QUERY_DEVICE_INFORMATION);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_SetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Get_CurSharedInterface(handle, sharedInterface);

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = sharedInterface->ID;
	request.intf.altsetting_number = AltSettingNumber;

	success = SubmitSimpleSyncRequest(LIBUSBK_IOCTL_SET_INTERFACE);
	ErrorNoSet(!success, Error, "Failed setting AltSettingNumber %u", AltSettingNumber);

	Update_SharedInterface_AltSetting(sharedInterface, AltSettingNumber);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR AltSettingNumber)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;

	ErrorParamAction(!AltSettingNumber, "AltSettingNumber", return FALSE);
	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);

	Get_CurSharedInterface(handle, sharedInterface);
	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = sharedInterface->ID;

	success = SubmitSimpleSyncRequest(LIBUSBK_IOCTL_GET_INTERFACE);
	ErrorNoSet(!success, Error, "Failed getting AltSettingNumber");

	*AltSettingNumber = (UCHAR)request.intf.altsetting_number;
	Update_SharedInterface_AltSetting(sharedInterface, request.intf.altsetting_number);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_QueryPipe(
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

KUSB_EXP BOOL KUSB_API UsbK_SetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in UINT PolicyType,
    _in UINT ValueLength,
    _in PVOID Value)
{
	ULONG requestLength = sizeof(libusb_request) + ValueLength;
	libusb_request* request = NULL;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success = FALSE;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	request = AllocRequest(ValueLength);
	ErrorSet(!IsHandleValid(request), Error, ERROR_NOT_ENOUGH_MEMORY, "ValueLength=%u", ValueLength);

	request->pipe_policy.pipe_id = PipeID;
	request->pipe_policy.policy_type = PolicyType;
	SetRequestData(request, Value, ValueLength);

	if (handle->Device->Backend.CtxK->Version.major > 2)
	{
		// this is a libusbK driver; it has full support for policies
		success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_SET_PIPE_POLICY,
		                     request, requestLength,
		                     NULL, 0, NULL);
	}
	else
	{
		if (!(PipeID & 0x0F))
		{
			// libusb-win32 must do a little in the driver and a little in the dll.
			// this pipe policy is for the default pipe; they are handled by the driver.
			SetRequestData(request, Value, ValueLength);
			success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_SET_PIPE_POLICY,
			                     request, requestLength,
			                     NULL, 0, NULL);
		}
		else
		{
			// sync pipe transfer timeouts must be handled in user mode for libusb0.
			// setting the user pipe policy timeout causes the SyncTransfer function
			// to wait and cancel if the timeout expires.
			switch(PolicyType)
			{
			case PIPE_TRANSFER_TIMEOUT:
				if (Value && (ValueLength >= 4))
				{
					InterlockedExchange((PLONG)&GetSetPipePolicy(handle->Device->Backend.CtxK, PipeID).timeout, (LONG) * ((PULONG)Value));
					success = TRUE;
				}
				break;
			default:
				success = LusbwError(ERROR_NOT_SUPPORTED);
				break;
			}
		}
	}

	Mem_Free(&request);
	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	Mem_Free(&request);
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in UINT PolicyType,
    _ref PUINT ValueLength,
    _out PVOID Value)
{

	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success = FALSE;

	ErrorParamAction(!ValueLength || !ValueLength[0], "ValueLength", return FALSE);
	ErrorParamAction(!Value, "Value", return FALSE);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.pipe_policy.pipe_id = PipeID;
	request.pipe_policy.policy_type = PolicyType;

	if (handle->Device->Backend.CtxK->Version.major > 1)
	{
		// this is a libusbK driver; it has full support for policies
		success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_PIPE_POLICY,
		                     &request, sizeof(request),
		                     Value, *ValueLength,
		                     ValueLength);
	}
	else
	{
		if (!(PipeID & 0x0F))
		{
			// libusb-win32 must do a little in the driver and a little in the dll.
			// this pipe policy is for the default pipe; they are handled by the driver.
			success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_PIPE_POLICY,
			                     &request, sizeof(request),
			                     Value, *ValueLength,
			                     ValueLength);
		}
		else
		{
			// sync pipe transfer timeouts must be handled in user mode for libusb0.
			// setting the user pipe policy timeout causes the SyncTransfer function
			// to wait and cancel if the timeout expires.
			switch(PolicyType)
			{
			case PIPE_TRANSFER_TIMEOUT:
				if (*ValueLength >= 4)
				{
					((PULONG)Value)[0] = GetSetPipePolicy(handle->Device->Backend.CtxK, PipeID).timeout;
					*ValueLength = 4;
					success = TRUE;
				}
				break;
			default:
				success = LusbwError(ERROR_NOT_SUPPORTED);
				break;
			}
		}
	}

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_SetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT PolicyType,
    _in UINT ValueLength,
    _in PVOID Value)
{
	ULONG requestLength = sizeof(libusb_request) + ValueLength;
	libusb_request* request = NULL;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	ErrorParamAction(!ValueLength, "ValueLength", return FALSE);
	ErrorParamAction(!Value, "Value", return FALSE);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	request = AllocRequest(ValueLength);
	ErrorSet(!IsHandleValid(request), Error, ERROR_NOT_ENOUGH_MEMORY, "ValueLength=%u", ValueLength);

	request->power_policy.policy_type = PolicyType;
	SetRequestData(request, Value, ValueLength);

	success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_SET_POWER_POLICY,
	                     request, requestLength,
	                     NULL, 0,
	                     NULL);

	Mem_Free(&request);
	PoolHandle_Dec_UsbK(handle);
	return success;

Error:
	Mem_Free(&request);
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UINT PolicyType,
    _ref PUINT ValueLength,
    _out PVOID Value)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	ErrorParamAction(!ValueLength || !ValueLength[0], "ValueLength", return FALSE);
	ErrorParamAction(!Value, "Value", return FALSE);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.power_policy.policy_type = PolicyType;

	success = Ioctl_Sync(Dev_Handle(), LIBUSB_IOCTL_GET_POWER_POLICY,
	                     &request, sizeof(request),
	                     Value, *ValueLength,
	                     ValueLength);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

BOOL k_ClaimOrReleaseInterface(__in KUSB_HANDLE InterfaceHandle,
                               __in INT NumberOrIndex,
                               __in UCHAR IsClaim,
                               __in UCHAR IsIndex)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	INT ioControlCode = IsClaim ? LIBUSBK_IOCTL_CLAIM_INTERFACE : LIBUSBK_IOCTL_RELEASE_INTERFACE;
	PKUSB_INTERFACE_EL interfaceEL;
	BOOL success = FALSE;

	ErrorParamAction(NumberOrIndex < 0, "NumberOrIndex", return FALSE);
	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, interfaceEL, IsIndex, NumberOrIndex);
	ErrorSetAction(!interfaceEL, ERROR_RESOURCE_NOT_FOUND, goto Done, "Interface not found. NumberOrIndex=%u IsClaim=%u IsIndex=%u", NumberOrIndex, IsClaim, IsIndex);

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = (UCHAR)interfaceEL->ID;
	success = SubmitSimpleSyncRequest(ioControlCode);

	if (IsClaim && success)
	{
		Get_SharedInterface(handle, interfaceEL->Index).Claimed = TRUE;
		InterlockedExchange(&handle->Selected_SharedInterface_Index, interfaceEL->Index);
	}
	else
	{
		Get_SharedInterface(handle, interfaceEL->Index).Claimed = FALSE;
	}
	ErrorNoSet(!success, Done, "Failed claiming/releasing interface #%u.", interfaceEL->ID);
Done:
	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ClaimInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	return k_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, TRUE, (UCHAR)IsIndex);
}

KUSB_EXP BOOL KUSB_API UsbK_SelectInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	return UsbStack_SelectInterface(InterfaceHandle, NumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API UsbK_ReleaseInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	return k_ClaimOrReleaseInterface(InterfaceHandle, NumberOrIndex, FALSE, (UCHAR)IsIndex);
}


KUSB_EXP BOOL KUSB_API UsbK_SetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _in UCHAR AltSettingNumber)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;
	PKDEV_SHARED_INTERFACE sharedInterface;
	PKUSB_INTERFACE_EL intfEL;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, IsIndex, NumberOrIndex);
	ErrorSet(!intfEL, Error, ERROR_RESOURCE_NOT_FOUND, "Interface not found. NumberOrIndex=%u IsIndex=%u.", NumberOrIndex, IsIndex);

	sharedInterface = &Get_SharedInterface(handle, intfEL->Index);

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = sharedInterface->ID;
	request.intf.altsetting_number = AltSettingNumber;
	request.intf.intf_use_index = FALSE;
	request.intf.altf_use_index = FALSE;

	success = SubmitSimpleSyncRequest(LIBUSBK_IOCTL_SET_INTERFACE);
	ErrorNoSet(!success, Error, "Failed setting AltSettingNumber %u.", AltSettingNumber);

	Update_SharedInterface_AltSetting(sharedInterface, AltSettingNumber);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _out PUCHAR AltSettingNumber)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success = FALSE;
	PKDEV_SHARED_INTERFACE sharedInterface;
	PKUSB_INTERFACE_EL intfEL;

	ErrorParamAction(!AltSettingNumber, "AltSettingNumber", return FALSE);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, IsIndex, NumberOrIndex);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Interface not found. NumberOrIndex=%u IsIndex=%u AltSettingNumber=%u", NumberOrIndex, IsIndex, AltSettingNumber);

	sharedInterface = &Get_SharedInterface(handle, intfEL->Index);
	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = intfEL->ID;

	success = SubmitSimpleSyncRequest(LIBUSBK_IOCTL_GET_INTERFACE);
	ErrorNoSet(!success, Error, "Failed getting AltSettingNumber.");

	*AltSettingNumber = (UCHAR)request.intf.altsetting_number;
	Update_SharedInterface_AltSetting(sharedInterface, request.intf.altsetting_number);
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetCurrentFrameNumber(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUINT FrameNumber)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = SubmitSimpleSyncRequest(LIBUSBK_IOCTL_GET_CURRENTFRAME_NUMBER);
	if (success)
		*FrameNumber = ((PULONG)&request)[0];

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ResetDevice(
    _in KUSB_HANDLE InterfaceHandle)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	success = SubmitSimpleSyncRequest(LIBUSB_IOCTL_RESET_DEVICE);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ControlTransfer(
    _in KUSB_HANDLE InterfaceHandle,
    _in WINUSB_SETUP_PACKET SetupPacket,
    _refopt PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	libusb_request request;
	INT ioctlCode;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	ioctlCode = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? LIBUSB_IOCTL_CONTROL_READ : LIBUSB_IOCTL_CONTROL_WRITE;

	if (Overlapped)
	{
		Mem_Zero(&request, sizeof(request));
		memcpy(&request.control, &SetupPacket, sizeof(request.control));
		success = Ioctl_Async(Dev_Handle(), ioctlCode, &request, sizeof(request), Buffer, BufferLength, Overlapped);
	}
	else
	{
		UCHAR pipeID = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? 0x80 : 0x00;

		Mem_Zero(&request, sizeof(request));
		memcpy(&request.control, &SetupPacket, sizeof(request.control));
		request.timeout = GetSetPipePolicy(handle->Device->Backend.CtxK, pipeID).timeout;

		success = Ioctl_Sync(Dev_Handle(), ioctlCode, &request, sizeof(request), Buffer, BufferLength, LengthTransferred);
	}

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_GetOverlappedResult(
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

KUSB_EXP BOOL KUSB_API UsbK_ResetPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = SubmitSimpleSyncRequest(LIBUSB_IOCTL_RESET_ENDPOINT);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_AbortPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = SubmitSimpleSyncRequest(LIBUSB_IOCTL_ABORT_ENDPOINT);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_FlushPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	libusb_request request;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = SubmitSimpleSyncRequest(LIBUSB_IOCTL_FLUSH_PIPE);

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	if (Overlapped)
	{
		libusb_request request;
		Mem_Zero(&request, sizeof(request));
		request.endpoint.endpoint = PipeID;
		success = Ioctl_Async(Dev_Handle(), LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		libusb_request request;
		Mem_Zero(&request, sizeof(request));
		request.endpoint.endpoint = PipeID;

		success = Ioctl_SyncWithTimeout(Dev_Handle(),
		                                LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                                &request,
		                                Buffer, BufferLength,
		                                GetSetPipePolicy(handle->Device->Backend.CtxK, PipeID).timeout,
		                                PipeID,
		                                LengthTransferred);
	}

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_WritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in UINT BufferLength,
    _outopt PUINT LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	if (Overlapped)
	{
		libusb_request request;
		Mem_Zero(&request, sizeof(request));
		request.endpoint.endpoint = PipeID;
		success = Ioctl_Async(Dev_Handle(),
		                      LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		libusb_request request;
		Mem_Zero(&request, sizeof(request));
		request.endpoint.endpoint = PipeID;
		success = Ioctl_SyncWithTimeout(Dev_Handle(),
		                                LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                                &request,
		                                Buffer, BufferLength,
		                                GetSetPipePolicy(handle->Device->Backend.CtxK, PipeID).timeout,
		                                PipeID,
		                                LengthTransferred);
	}

	PoolHandle_Dec_UsbK(handle);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_IsoReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in UINT BufferLength,
    _in LPOVERLAPPED Overlapped,
    _refopt PKISO_CONTEXT IsoContext)
{
	libusb_request request;
	ULONG IsoContextSize;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	ErrorParam(!IsHandleValid(Overlapped), Error, "Overlapped");
	if (IsoContext)
	{
		IsoContextSize = sizeof(KISO_CONTEXT) + (IsoContext->NumberOfPackets * sizeof(KISO_PACKET));

		Mem_Zero(&request, sizeof(request));
		request.IsoEx.IsoContext		= IsoContext;
		request.IsoEx.IsoContextSize	= IsoContextSize;
		request.IsoEx.PipeID			= (UCHAR)PipeID;

		success = Ioctl_Async(Dev_Handle(), LIBUSBK_IOCTL_ISOEX_READ,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		Mem_Zero(&request, sizeof(request));
		request.AutoIsoEx.PipeID		= PipeID;

		success = Ioctl_Async(Dev_Handle(), LIBUSBK_IOCTL_AUTOISOEX_READ,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);

	}
	PoolHandle_Dec_UsbK(handle);
	return success;
Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_IsoWritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in UINT BufferLength,
    _in LPOVERLAPPED Overlapped,
    _refopt PKISO_CONTEXT IsoContext)
{
	libusb_request request;
	ULONG IsoContextSize;
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	ErrorParam(!IsHandleValid(Overlapped), Error, "Overlapped");

	if (IsoContext)
	{
		IsoContextSize = sizeof(KISO_CONTEXT) + (IsoContext->NumberOfPackets * sizeof(KISO_PACKET));

		Mem_Zero(&request, sizeof(request));
		request.IsoEx.IsoContext		= IsoContext;
		request.IsoEx.IsoContextSize	= IsoContextSize;
		request.IsoEx.PipeID			= PipeID;

		success = Ioctl_Async(Dev_Handle(), LIBUSBK_IOCTL_ISOEX_WRITE,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		Mem_Zero(&request, sizeof(request));
		request.AutoIsoEx.PipeID		= PipeID;

		success = Ioctl_Async(Dev_Handle(), LIBUSBK_IOCTL_AUTOISOEX_WRITE,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	PoolHandle_Dec_UsbK(handle);
	return success;
Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetProperty(
    _in KUSB_HANDLE InterfaceHandle,
    _in KUSB_PROPERTY PropertyType,
    _ref PUINT PropertySize,
    _out PVOID Value)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success = TRUE;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorParamAction(!PropertySize, "PropertySize", return FALSE);
	ErrorParamAction(!Value, "Value", return FALSE);

	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	switch (PropertyType)
	{
	case KUSB_PROPERTY_DEVICE_FILE_HANDLE:
		if ((*PropertySize) < sizeof(HANDLE))
			success = LusbwError(ERROR_MORE_DATA);
		else
			((HANDLE*)Value)[0] = handle->Device->MasterDeviceHandle;

		*PropertySize = sizeof(HANDLE);
		break;
	}
	PoolHandle_Dec_UsbK(handle);
	return success;
}

#endif
